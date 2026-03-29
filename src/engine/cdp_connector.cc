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
        g_cdp_pipe_cv.wait(lock, [] { return g_cdp_pipes_ready; });
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
    auto cdp = std::make_shared<cdp_client>([hWrite](const std::string& payload) -> bool {
        return pipe_send(hWrite, payload);
    });

    logger.log("[{}] CDP pipe transport connected.", name);

    /** start the read loop on a background thread so messages flow immediately */
    std::thread read_thread([hRead = g_cdp_pipe_read, cdp, name]() {
        pipe_read_loop(hRead, cdp);
        logger.log("[{}] CDP pipe read loop exited.", name);
    });

    try {
        on_connect_cb(cdp);
    } catch (const std::exception& e) {
        LOG_ERROR("[{}] Exception in onConnect: {}", name, e.what());
    }

    /** block until the read loop finishes (pipe closed or shutdown) */
    if (read_thread.joinable()) {
        read_thread.join();
    }

    logger.log("[{}] Shutting down CDP client...", name);
    cdp->shutdown();
    logger.log("Disconnected from [{}] module...", name);
}

#else

void socket_utils::connect_socket(std::shared_ptr<socket_utils::socket_t> socket_props)
{
    LOG_ERROR("[{}] CDP pipe transport is only available on Windows.", socket_props->name);
}

#endif
