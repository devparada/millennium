cmake_minimum_required(VERSION 3.21)

# The parent CMakeLists sets -m32 globally (also cached) for the 32-bit
# Millennium library. lua_host is an independent native-64-bit process,
# so strip -m32 from this directory's flags before any targets are created.
if(UNIX AND NOT APPLE)
    string(REPLACE "-m32" "" CMAKE_C_FLAGS       "${CMAKE_C_FLAGS}")
    string(REPLACE "-m32" "" CMAKE_CXX_FLAGS     "${CMAKE_CXX_FLAGS}")
    string(REPLACE "-m32" "" CMAKE_ASM_FLAGS     "${CMAKE_ASM_FLAGS}")
    string(REPLACE "-m32" "" CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
endif()

set(LUA_HOST_SOURCES
    main.cc
    rpc.cc
    crash_handler.cc
    ${MILLENNIUM_BASE}/src/shared/crash_report.cc
    ${MILLENNIUM_BASE}/src/shared/crash_handler_core.cc
    lua_millennium_rpc.cc
    lua_logger_rpc.cc
    lua_http.cc
    lua_utils.cc
    lua_filesystem.cc
    lua_regex.cc
    lua_datetime.cc
)

add_executable(${MILLENNIUM_RTB_NAME} ${LUA_HOST_SOURCES})

target_compile_definitions(${MILLENNIUM_RTB_NAME} PRIVATE
    MILLENNIUM_RTB_NAME="${MILLENNIUM_RTB_NAME}"
)

target_include_directories(${MILLENNIUM_RTB_NAME} PRIVATE
    ${MILLENNIUM_BASE}/src
    ${MILLENNIUM_BASE}/src/include
)

if(UNIX AND NOT APPLE)
    # The lua sandbox is an independent process (IPC over unix sockets), so it
    # does not need to match the 32-bit Millennium library.  Building it as
    # native 64-bit avoids the need for 32-bit system libraries (openssl, etc.).
    # HTTP is handled by the parent process over RPC, so no curl needed here.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LUAJIT REQUIRED IMPORTED_TARGET luajit)

    target_link_libraries(${MILLENNIUM_RTB_NAME}
        PkgConfig::LUAJIT
        nlohmann_json
        fmt::fmt
    )

    target_include_directories(${MILLENNIUM_RTB_NAME} PRIVATE
        ${LUAJIT_INCLUDE_DIRS}
    )
    # cjson sources are added by the parent CMakeLists.txt after deps are fetched
else()
    target_include_directories(${MILLENNIUM_RTB_NAME} PRIVATE
        ${luajit_BINARY_DIR}
        ${luajit_SOURCE_DIR}/src
    )

    target_link_libraries(${MILLENNIUM_RTB_NAME}
        luajit::lib
        cjson
        nlohmann_json
        fmt::fmt
    )
endif()

# Always ship with debug info so crash dumps are useful
if(MSVC)
    target_compile_options(${MILLENNIUM_RTB_NAME} PRIVATE /Z7)
    target_link_options(${MILLENNIUM_RTB_NAME} PRIVATE /DEBUG)
    target_link_libraries(${MILLENNIUM_RTB_NAME} dbghelp ws2_32)
else()
    target_compile_options(${MILLENNIUM_RTB_NAME} PRIVATE -g)
    target_link_options(${MILLENNIUM_RTB_NAME} PRIVATE -rdynamic)
endif()

set_target_properties(${MILLENNIUM_RTB_NAME} PROPERTIES
    OUTPUT_NAME "${MILLENNIUM_RTB_NAME}"
)

if(NOT GITHUB_ACTION_BUILD)
    include(${MILLENNIUM_BASE}/scripts/cmake/find_steam_path.cmake)
    find_steam_path(${MILLENNIUM_RTB_NAME})
endif()
