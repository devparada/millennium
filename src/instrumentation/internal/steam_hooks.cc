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

#include <atomic>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "millennium/logger.h"
#include "millennium/steam_hooks.h"
#include "millennium/cmdline_parse.h"
#include "millennium/millennium_lifecycle.h"

std::string STEAM_DEVELOPER_TOOLS_PORT = "8080";

uint_least16_t bind_random_port()
{
    asio::io_context io_context;
    asio::ip::tcp::acceptor acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
    return acceptor.local_endpoint().port();
}

const char* get_devtools_port(const bool is_developer_mode)
{
    const char* port = is_developer_mode ? DEFAULT_DEVTOOLS_PORT : STEAM_DEVELOPER_TOOLS_PORT.c_str();
    return port;
}

struct command
{
    std::string exec;
    std::vector<std::string> params;

    command(const char* full_cmd)
    {
        parse(full_cmd);
    }

    std::string_view executable() const
    {
        auto slash = exec.rfind('/');
        auto backslash = exec.rfind('\\');
        size_t sep = std::string::npos;

        if (slash != std::string::npos && backslash != std::string::npos)
            sep = std::max(slash, backslash);
        else if (slash != std::string::npos)
            sep = slash;
        else if (backslash != std::string::npos)
            sep = backslash;

        return sep != std::string::npos ? std::string_view(exec).substr(sep + 1) : std::string_view(exec);
    }

    void ensure_param(std::string_view key, const char* value)
    {
        std::string entry = value ? std::string(key) + "=" + value : std::string(key);

        for (auto& p : params) {
            if (p == key || (p.size() > key.size() && p.compare(0, key.size(), key) == 0 && p[key.size()] == '=')) {
                p = entry;
                return;
            }
        }

        params.insert(params.begin(), std::move(entry));
    }

    std::string build() const
    {
        std::string result;
        result += quote_token(exec);
        for (const auto& p : params) {
            result += ' ';
            result += quote_token(p);
        }
        return result;
    }

  private:
    static bool is_quote_char(char c)
    {
#ifdef _WIN32
        return c == '"';
#else
        return c == '"' || c == '\'';
#endif
    }

    static std::string quote_token(const std::string& s)
    {
#ifdef _WIN32
        if (s.find_first_of(" =") != std::string::npos) return '"' + s + '"';
        return s;
#else
        if (s.find_first_of(" \t\"'") == std::string::npos) return s;
        std::string out = "\"";
        for (char c : s) {
            if (c == '"' || c == '\\') out += '\\';
            out += c;
        }
        return out + '"';
#endif
    }

    void parse(const char* full_cmd)
    {
        std::string token;
        bool in_quotes = false;
        char quote_char = 0;
        bool first = true;

        for (const char* p = full_cmd; *p; ++p) {
            if (is_quote_char(*p) && (!in_quotes || *p == quote_char)) {
                quote_char = in_quotes ? 0 : *p;
                in_quotes = !in_quotes;
                continue;
            }
            if (*p == ' ' && !in_quotes) {
                if (!token.empty()) flush(token, first);
                continue;
            }
            token += *p;
        }
        if (!token.empty()) flush(token, first);
    }

    void flush(std::string& token, bool& first)
    {
        if (first) {
            exec = std::move(token);
            first = false;
        } else {
            params.push_back(std::move(token));
        }
        token.clear();
    }
};

const char* Plat_HookedCreateSimpleProcess(const char* cmd)
{
    /** only wait on the first call. */
    if (!millennium_lifecycle::get().backends_loaded.flag.load()) {
        millennium_lifecycle::get().backends_loaded.wait();
    }

    command cmd_line(cmd);

    const char* target_executable =
#ifdef _WIN32
        "steamwebhelper.exe";
#elif __linux__
        "steamwebhelper.sh";
#elif __APPLE__
        "Steam Helper";
#endif

    if (cmd_line.executable() != target_executable) {
        return cmd;
    }

    bool is_developer_mode = CommandLineArguments::has_argument("-dev");

    /** block any browser web requests when running in normal mode */
    cmd_line.ensure_param("--remote-allow-origins", is_developer_mode ? "*" : "");
    /** always ensure the debugger is only local, and can't be accessed over lan */
    cmd_line.ensure_param("--remote-debugging-address", "127.0.0.1");
    /** enable the CEF remote debugger */
    cmd_line.ensure_param("--remote-debugging-port", get_devtools_port(is_developer_mode));

    static thread_local std::string result;
    result = cmd_line.build();
    return result.c_str();
}

#ifdef _WIN32
#define SNARE_STATIC
#define SNARE_IMPLEMENTATION
#include <libsnare.h>

#include "millennium/argp_win32.h"
#include "millennium/logger.h"
#include "millennium/backend_mgr.h"
#include "millennium/plat_msg.h"

#define LDR_DLL_NOTIFICATION_REASON_LOADED 1
#define LDR_DLL_NOTIFICATION_REASON_UNLOADED 2

LdrRegisterDllNotification_t LdrRegisterDllNotification = nullptr;
LdrUnregisterDllNotification_t LdrUnregisterDllNotification = nullptr;
PVOID g_notification_cookie = nullptr;

static snare_inline_t g_create_hook = nullptr;
static snare_inline_t g_rdcw_hook = nullptr;

HMODULE steam_tier0_module;

INT hooked_create_simple_process(const char* a1, char a2, const char* lp_multi_byte_str)
{
    snare_inline_remove(g_create_hook);
    auto orig = reinterpret_cast<INT(__cdecl*)(const char*, char, const char*)>(snare_inline_get_src(g_create_hook));
    INT result = orig(Plat_HookedCreateSimpleProcess(a1), a2, lp_multi_byte_str);
    snare_inline_install(g_create_hook);
    return result;
}

/**
 * tier0_s.dll is a *cross platform* library bundled with Steam that helps it
 * manage low-level system interactions and provides various utility functions.
 *
 * It houses various functions, and we are interested in hooking its functions Steam
 * uses to spawn the Steam web helper.
 */
VOID handle_tier0_dll(PVOID module_base_address)
{
    steam_tier0_module = static_cast<HMODULE>(module_base_address);
    logger.log("Setting up hooks for tier0_s.dll");

    FARPROC proc = GetProcAddress(steam_tier0_module, "CreateSimpleProcess");
    if (proc != nullptr) {
        g_create_hook = snare_inline_new(reinterpret_cast<void*>(proc), reinterpret_cast<void*>(&hooked_create_simple_process));
        if (!g_create_hook || snare_inline_install(g_create_hook) < 0) {
            platform::messagebox::show("Millennium", "Failed to create hook for CreateSimpleProcess", platform::messagebox::error);
            return;
        }
    }
}

/**
 * Millennium uses dll_notification_callback to hook when steamclient.dll unloaded
 * This signifies the Steam Client is unloading.
 * The reason it is done this way is to prevent windows loader lock from deadlocking or destroying parts of Millennium.
 * When we are running inside the callback, we are inside the loader lock (so windows can't continue loading/unloading dlls)
 */
VOID handle_steam_unload()
{
    /**
     * notify the millennium frontend to disconnect
     * once the frontend disconnects, it will shutdown the rest of Millennium
     */
    millennium_lifecycle::get().disconnect_frontend.store(true);
    // auto& backendManager = BackendManager::GetInstance();

    // /** No need to wait if all backends aren't python */
    // if (!backendManager.HasAnyPythonBackends() && !backendManager.HasAnyLuaBackends()) {
    //     Logger.Warn("No backends detected, skipping shutdown wait...");
    //     return;
    // }

    // std::unique_lock<std::mutex> lk(mtx_hasSteamUnloaded);
    // Logger.Warn("Waiting for Millennium to be unloaded...");

    // /** wait for Millennium to be unloaded */
    // cv_hasSteamUnloaded.wait(lk, [&backendManager]()
    // {
    //     /** wait for all backends to stop so we can safely free the loader lock */
    //     if (backendManager.HasAllPythonBackendsStopped() && backendManager.HasAllLuaBackendsStopped()) {
    //         Logger.Warn("All backends have stopped, proceeding with termination...");

    //         std::unique_lock<std::mutex> lk2(mtx_hasAllPythonPluginsShutdown);
    //         cv_hasAllPythonPluginsShutdown.notify_all();
    //         return true;
    //     }
    //     return false;
    // });

    logger.warn("Terminate condition variable signaled, exiting...");
}

VOID handle_steam_load()
{
    logger.log("[dll_notification_callback] Notified that Steam UI has loaded, notifying main thread...");
    millennium_lifecycle::get().steam_ui_loaded.notify();
}

VOID CALLBACK dll_notification_callback(ULONG notification_reason, PLDR_DLL_NOTIFICATION_DATA notification_data, [[maybe_unused]] PVOID context)
{
    std::wstring_view base_dll_name(notification_data->BaseDllName->Buffer, notification_data->BaseDllName->Length / sizeof(WCHAR));

    /** hook Steam load */
    if (notification_reason == LDR_DLL_NOTIFICATION_REASON_LOADED && base_dll_name == L"steamui.dll") {
        handle_steam_load();
        return;
    }

    /** hook Steam unload */
    if (notification_reason == LDR_DLL_NOTIFICATION_REASON_UNLOADED && base_dll_name == L"steamclient64.dll") {
        logger.log("[dll_notification_callback] Notified that steamclient64.dll has unloaded, handling Steam unload...");
        handle_steam_unload();
        return;
    }

    /** hook steam cross platform api (used to hook create proc) */
    if (notification_reason == LDR_DLL_NOTIFICATION_REASON_LOADED && base_dll_name == L"tier0_s64.dll") {
        handle_tier0_dll(notification_data->DllBase);
        return;
    }
}

void handle_already_loaded(LPCWSTR dll_name)
{
    HMODULE module = GetModuleHandleW(dll_name);

    if (!module) {
        return;
    }

    LDR_DLL_NOTIFICATION_DATA notif_data = {};
    UNICODE_STRING unicode_dll_name = {};
    unicode_dll_name.Buffer = const_cast<PWSTR>(dll_name);
    unicode_dll_name.Length = static_cast<USHORT>(wcslen(dll_name) * sizeof(WCHAR));
    notif_data.BaseDllName = &unicode_dll_name;
    notif_data.DllBase = module;
    dll_notification_callback(LDR_DLL_NOTIFICATION_REASON_LOADED, &notif_data, nullptr);
}

/**
 * Hook and disable ReadDirectoryChangesW to prevent the Steam client from monitoring changes
 * to the steamui dir, which is incredibly fucking annoying during development.
 */
BOOL WINAPI hooked_read_directory_changes_w(void*, void*, void*, void*, void*, LPDWORD bytes_ret, void*, void*)
{
    logger.log("[Steam] Blocked attempt to ReadDirectoryChangesW...");

    if (bytes_ret) *bytes_ret = 0; // no changes
    return TRUE;                   // indicate success
}

bool initialize_steam_hooks()
{
    const auto start_time = std::chrono::system_clock::now();

#ifdef MILLENNIUM_64BIT

    STEAM_DEVELOPER_TOOLS_PORT = std::to_string(bind_random_port());

    HMODULE h_tier0 = GetModuleHandleW(L"tier0_s.dll");
    if (h_tier0) {
        handle_tier0_dll(h_tier0);
        return true;
    }

    HMODULE ntdll_module = GetModuleHandleA("ntdll.dll");
    if (!ntdll_module) {
        platform::messagebox::show("Millennium", "Failed to get handle for ntdll.dll", platform::messagebox::error);
        return false;
    }

    // Check if modules are already loaded and handle them
    handle_already_loaded(L"steamui.dll");
    handle_already_loaded(L"steamclient64.dll");
    handle_already_loaded(L"tier0_s64.dll");

    LdrRegisterDllNotification = reinterpret_cast<LdrRegisterDllNotification_t>((void*)GetProcAddress(ntdll_module, "LdrRegisterDllNotification"));
    LdrUnregisterDllNotification = reinterpret_cast<LdrUnregisterDllNotification_t>((void*)GetProcAddress(ntdll_module, "LdrUnregisterDllNotification"));

    if (!LdrRegisterDllNotification || !LdrUnregisterDllNotification) {
        platform::messagebox::show("Millennium", "Failed to get address for LdrRegisterDllNotification or LdrUnregisterDllNotification", platform::messagebox::error);
        return false;
    }

    auto dll_reg_status = NT_SUCCESS(LdrRegisterDllNotification(0, dll_notification_callback, nullptr, &g_notification_cookie));

    logger.log("[SH_Hook] Waiting for Steam UI to load...");

    /** wait for steamui.dll to load (which signifies Steam is actually starting and not updating/verifying files) */
    millennium_lifecycle::get().steam_ui_loaded.wait();
#elif MILLENNIUM_32BIT
    bool dll_reg_status = true;
#endif
    const auto end_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time).count();
    logger.log("[SH_Hook] Steam UI loaded in {} ms, continuing Millennium startup...", end_time);

    /** only hook if developer mode is enabled */
    if (CommandLineArguments::has_argument("-dev")) {
        g_rdcw_hook = snare_inline_new(reinterpret_cast<void*>(&ReadDirectoryChangesW), reinterpret_cast<void*>(&hooked_read_directory_changes_w));
        if (g_rdcw_hook) snare_inline_install(g_rdcw_hook);
    }

    return dll_reg_status;
}

void uninitialize_steam_hooks()
{
    if (g_rdcw_hook) {
        snare_inline_remove(g_rdcw_hook);
        snare_inline_free(g_rdcw_hook);
        g_rdcw_hook = nullptr;
    }
    if (g_create_hook) {
        snare_inline_remove(g_create_hook);
        snare_inline_free(g_create_hook);
        g_create_hook = nullptr;
    }
}
#elif __linux__
#include "millennium/logger.h"

#include <dlfcn.h>
#include <string>
#include <fmt/format.h>
#define SNARE_STATIC
#define SNARE_IMPLEMENTATION
#include <libsnare.h>
#include <stdlib.h>

static snare_inline create_hook;

/**
 * It seems a2, a3 might be a stack allocated struct pointer, but we don't really need them
 * Even if we mistyped them, it doesn't change the actual underlying data being sent in memory.
 * I assume it has something to with working directory &| flags
 */
extern "C" int hooked_create_simple_process(const char* cmd, unsigned int a2, const char* a3)
{
    /** temporarily remove the hook to prevent recursive hook calls */
    snare_inline::scoped_remove remove(&create_hook);
    cmd = Plat_HookedCreateSimpleProcess(cmd);
    /** call the original */
    return reinterpret_cast<int (*)(const char* cmd, unsigned int flags, const char* cwd)>(create_hook.get_src())(cmd, a2, a3);
}

static void* get_module_handle(const char* libneedle, const char* symbol)
{
    void* handle = dlopen(libneedle, RTLD_NOW | RTLD_NOLOAD);
    if (handle) {
        void* s = dlsym(handle, symbol);
        dlclose(handle);
        if (s) return s;
    }

    void* s2 = dlsym(RTLD_DEFAULT, symbol);
    if (s2) return s2;

    return nullptr;
}

bool initialize_steam_hooks()
{
    const char* libneedle = "libtier0_s.so";
    const char* symbol = "CreateSimpleProcess";

    STEAM_DEVELOPER_TOOLS_PORT = std::to_string(bind_random_port());

    void* target = get_module_handle(libneedle, symbol);
    if (!target) {
        LOG_ERROR("Failed to locate symbol '{}'", symbol);
        return false;
    }

    logger.log("Located {} at address {}", symbol, target);
    const bool success = create_hook.install(target, (void*)hooked_create_simple_process);
    logger.log("Hook install success?: {}", success);
    return true;
}
#elif defined(__APPLE__)
#include "millennium/logger.h"

#include <filesystem>
#include <unistd.h>

bool initialize_steam_hooks()
{
    const char* configured_port = std::getenv("MILLENNIUM_DEBUG_PORT");
    if (configured_port && configured_port[0] != '\0') {
        STEAM_DEVELOPER_TOOLS_PORT = configured_port;
    } else {
        STEAM_DEVELOPER_TOOLS_PORT = DEFAULT_DEVTOOLS_PORT;
    }

    std::filesystem::path helper_hook_path;
    std::filesystem::path child_hook_path;

    const char* configured_hook_path = std::getenv("MILLENNIUM_HOOK_HELPER_PATH");
    if (configured_hook_path && configured_hook_path[0] != '\0') {
        helper_hook_path = configured_hook_path;
    } else {
        const char* runtime_path = std::getenv("MILLENNIUM_RUNTIME_PATH");
        if (runtime_path && runtime_path[0] != '\0') {
            const std::filesystem::path runtime_directory = std::filesystem::path(runtime_path).parent_path();
            helper_hook_path = runtime_directory / "libmillennium_hhx64.dylib";
            child_hook_path = runtime_directory / "libmillennium_child_hook.dylib";
        }
    }

    const char* configured_child_hook_path = std::getenv("MILLENNIUM_CHILD_HOOK_PATH");
    if (configured_child_hook_path && configured_child_hook_path[0] != '\0') {
        child_hook_path = configured_child_hook_path;
    }

    if (helper_hook_path.empty()) {
        const char* steam_path = std::getenv("MILLENNIUM__STEAM_PATH");
        if (steam_path && steam_path[0] != '\0') {
            const std::filesystem::path steam_directory = steam_path;
            helper_hook_path = steam_directory / "libmillennium_hhx64.dylib";
            if (child_hook_path.empty()) {
                child_hook_path = steam_directory / "libmillennium_child_hook.dylib";
            }
        }
    }

    if (helper_hook_path.empty()) {
        logger.warn("macOS bootstrap-based helper injection could not resolve libmillennium_hhx64.dylib.");
        return false;
    }

    if (access(helper_hook_path.c_str(), R_OK) != 0) {
        logger.warn("macOS bootstrap-based helper injection is not available because '{}' is missing.", helper_hook_path.string());
        return false;
    }

    if (child_hook_path.empty()) {
        logger.warn("macOS bootstrap-based helper injection could not resolve libmillennium_child_hook.dylib.");
        return false;
    }

    if (access(child_hook_path.c_str(), R_OK) != 0) {
        logger.warn("macOS bootstrap-based helper injection is not available because '{}' is missing.", child_hook_path.string());
        return false;
    }

    logger.log("Using bootstrap-based Steam Helper injection on macOS with debugger port {}", STEAM_DEVELOPER_TOOLS_PORT);
    return true;
}
#endif

void platform::wait_for_backend_load()
{
    logger.log("__backend_load was called!");
    millennium_lifecycle::get().backends_loaded.notify();
}

bool platform::initialize_steam_hooks()
{
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    return ::initialize_steam_hooks();
#else
    return false;
#endif
}
