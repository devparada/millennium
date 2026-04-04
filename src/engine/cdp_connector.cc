/**
 * ==================================================
 *   _____ _ _ _             _
 *  |     |_| | |___ ___ ___|_|_ _ _____
 *  | | | | | | | -_|   |   | | | |     |
 *  |_|_|_|_|_|_|___|_|_|_|_|_|___|_|_|_|
 *
 * ==================================================
 *
 * Copyright (c) 2026 Project Millennium
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "millennium/cdp_connector.h"
#include "millennium/logger.h"

#include <fmt/format.h>

#ifdef _WIN32
#include <windows.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "millennium/millennium_lifecycle.h"

extern HANDLE g_cdp_pipe_read;
extern HANDLE g_cdp_pipe_write;
extern std::mutex g_cdp_pipe_mutex;
extern std::condition_variable g_cdp_pipe_cv;
extern bool g_cdp_pipes_ready;

/**
 * CDP pipe transport uses the Chrome DevTools Protocol over pipes.
 * Messages are null-terminated JSON strings, one per write/read.
 */

static bool pipe_send(HANDLE hWrite, const std::string& payload)
{
    /** CDP pipe protocol: message followed by null terminator */
    std::string msg = payload;
    msg.push_back('\0');

    const char* data = msg.data();
    DWORD remaining = static_cast<DWORD>(msg.size());

    while (remaining > 0) {
        DWORD written = 0;
        if (!WriteFile(hWrite, data, remaining, &written, nullptr)) {
            LOG_ERROR("CDP pipe write failed (error {})", GetLastError());
            return false;
        }
        data += written;
        remaining -= written;
    }
    return true;
}

static void pipe_read_loop(HANDLE hRead, std::shared_ptr<cdp_client> cdp)
{
    std::string buffer;
    char chunk[4096];

    while (cdp->is_active()) {
        DWORD bytesRead = 0;
        if (!ReadFile(hRead, chunk, sizeof(chunk), &bytesRead, nullptr)) {
            DWORD err = GetLastError();
            if (err == ERROR_BROKEN_PIPE) {
                logger.log("CDP pipe closed by remote end.");
            } else if (err == ERROR_OPERATION_ABORTED) {
                logger.log("CDP pipe read cancelled.");
            } else {
                LOG_ERROR("CDP pipe read failed (error {})", err);
            }
            break;
        }

        if (bytesRead == 0) {
            continue;
        }

        buffer.append(chunk, bytesRead);

        /** extract null-terminated messages */
        size_t pos;
        while ((pos = buffer.find('\0')) != std::string::npos) {
            std::string message = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);

            if (!message.empty()) {
                cdp->handle_message(message);
            }
        }
    }
}

void socket_utils::connect_socket(std::shared_ptr<socket_utils::socket_t> socket_props)
{
    std::string name = socket_props->name;
    auto on_connect_cb = socket_props->on_connect;

    {
        std::unique_lock<std::mutex> lock(g_cdp_pipe_mutex);
        g_cdp_pipe_cv.wait(lock, []
        {
            return g_cdp_pipes_ready;
        });
    }

    if (g_cdp_pipe_read == INVALID_HANDLE_VALUE || g_cdp_pipe_write == INVALID_HANDLE_VALUE) {
        LOG_ERROR("[{}] CDP pipes are not available. Cannot connect.", name);
        return;
    }

    if (!on_connect_cb) {
        LOG_ERROR("[{}] Invalid event handlers. Connection aborted.", name);
        return;
    }

    HANDLE hWrite = g_cdp_pipe_write;
    auto cdp = std::make_shared<cdp_client>([hWrite](const std::string& payload) -> bool
    {
        return pipe_send(hWrite, payload);
    });

    logger.log("[{}] CDP pipe transport connected.", name);

    /** start the read loop on a background thread so messages flow immediately */
    std::thread read_thread([hRead = g_cdp_pipe_read, cdp, name]()
    {
        pipe_read_loop(hRead, cdp);
        logger.log("[{}] CDP pipe read loop exited.", name);
    });

    try {
        on_connect_cb(cdp);
    } catch (const std::exception& e) {
        LOG_ERROR("[{}] Exception in onConnect: {}", name, e.what());
    }

    HANDLE read_thread_handle = read_thread.native_handle();
    std::thread terminate_watcher([cdp, read_thread_handle]()
    {
        millennium_lifecycle::get().terminate.wait();
        CancelSynchronousIo(read_thread_handle);
        cdp->shutdown();
    });

    if (read_thread.joinable()) {
        read_thread.join();
    }

    cdp->shutdown();

    if (terminate_watcher.joinable()) {
        terminate_watcher.join();
    }

    logger.log("Disconnected from [{}] module...", name);
}

#elif __linux__

#include <unistd.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "millennium/millennium_lifecycle.h"

extern int g_cdp_pipe_read_fd;
extern int g_cdp_pipe_write_fd;
extern std::mutex g_cdp_pipe_mutex;
extern std::condition_variable g_cdp_pipe_cv;
extern bool g_cdp_pipes_ready;

static bool pipe_send(int fd, const std::string& payload)
{
    std::string msg = payload;
    msg.push_back('\0');

    const char* data = msg.data();
    ssize_t remaining = static_cast<ssize_t>(msg.size());

    while (remaining > 0) {
        ssize_t written = write(fd, data, static_cast<size_t>(remaining));
        if (written < 0) {
            LOG_ERROR("CDP pipe write failed (errno {})", errno);
            return false;
        }
        data += written;
        remaining -= written;
    }
    return true;
}

static void pipe_read_loop(int data_fd, int cancel_fd, std::shared_ptr<cdp_client> cdp)
{
    std::string buffer;
    char chunk[4096];

    struct pollfd pfds[2] = {
        { data_fd,   POLLIN, 0 },
        { cancel_fd, POLLIN, 0 },
    };

    while (cdp->is_active()) {
        int r = poll(pfds, 2, -1);
        if (r < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("CDP pipe poll failed (errno {})", errno);
            break;
        }

        if (pfds[1].revents & POLLIN) {
            logger.log("CDP pipe read cancelled.");
            break;
        }

        if (pfds[0].revents & (POLLHUP | POLLERR | POLLNVAL)) {
            logger.log("CDP pipe closed or error.");
            break;
        }

        if (!(pfds[0].revents & POLLIN)) continue;

        ssize_t n = read(data_fd, chunk, sizeof(chunk));
        if (n < 0) {
            LOG_ERROR("CDP pipe read failed (errno {})", errno);
            break;
        }
        if (n == 0) {
            logger.log("CDP pipe closed by remote end.");
            break;
        }

        buffer.append(chunk, static_cast<size_t>(n));

        size_t pos;
        while ((pos = buffer.find('\0')) != std::string::npos) {
            std::string message = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            if (!message.empty()) {
                cdp->handle_message(message);
            }
        }
    }
}

void socket_utils::connect_socket(std::shared_ptr<socket_utils::socket_t> socket_props)
{
    std::string name = socket_props->name;
    auto on_connect_cb = socket_props->on_connect;

    {
        std::unique_lock<std::mutex> lock(g_cdp_pipe_mutex);
        g_cdp_pipe_cv.wait(lock, []
        {
            return g_cdp_pipes_ready;
        });
    }

    if (g_cdp_pipe_read_fd < 0 || g_cdp_pipe_write_fd < 0) {
        LOG_ERROR("[{}] CDP pipes are not available. Cannot connect.", name);
        return;
    }

    if (!on_connect_cb) {
        LOG_ERROR("[{}] Invalid event handlers. Connection aborted.", name);
        return;
    }

    int cancel_fd = eventfd(0, EFD_CLOEXEC);
    if (cancel_fd < 0) {
        LOG_ERROR("[{}] Failed to create cancel eventfd (errno {})", name, errno);
        return;
    }

    int write_fd = g_cdp_pipe_write_fd;
    auto cdp = std::make_shared<cdp_client>([write_fd](const std::string& payload) -> bool
    {
        return pipe_send(write_fd, payload);
    });

    logger.log("[{}] CDP pipe transport connected.", name);

    std::thread read_thread([read_fd = g_cdp_pipe_read_fd, cancel_fd, cdp, name]()
    {
        pipe_read_loop(read_fd, cancel_fd, cdp);
        logger.log("[{}] CDP pipe read loop exited.", name);
    });

    try {
        on_connect_cb(cdp);
    } catch (const std::exception& e) {
        LOG_ERROR("[{}] Exception in onConnect: {}", name, e.what());
    }

    std::thread terminate_watcher([cdp, cancel_fd]()
    {
        millennium_lifecycle::get().terminate.wait();
        const uint64_t val = 1;
        (void)write(cancel_fd, &val, sizeof(val));
        cdp->shutdown();
    });

    if (read_thread.joinable()) {
        read_thread.join();
    }

    cdp->shutdown();

    if (terminate_watcher.joinable()) {
        terminate_watcher.join();
    }

    close(cancel_fd);
    logger.log("Disconnected from [{}] module...", name);
}
#else
void socket_utils::connect_socket(std::shared_ptr<socket_utils::socket_t> socket_props)
{
    LOG_ERROR("[{}] CDP pipe transport is not available on this platform.", socket_props->name);
}
#endif
