cmake_minimum_required(VERSION 3.21)

if(WIN32)
    include(${MILLENNIUM_BASE}/scripts/cmake/millennium_version.cmake)

    set(VERSION_COMPANY      "Steam Homebrew")
    set(VERSION_COPYRIGHT    "Steam Homebrew 2025")
    set(VERSION_TRADEMARK1   "All Rights Reserved")
    set(VERSION_DESCRIPTION  "Lua plugin runtime for Millennium")
    set(VERSION_FILENAME     "${MILLENNIUM_RTB_NAME}.exe")
    set(VERSION_PRODUCT      "Millennium Lua Runtime")
    set(VERSION_FILETYPE     "VFT_APP")

    set(VERSION_FILE "${CMAKE_BINARY_DIR}/${MILLENNIUM_RTB_NAME}.rc")
    configure_file(${MILLENNIUM_BASE}/scripts/version.rc.in ${VERSION_FILE} @ONLY)
endif()

set(LUA_HOST_SOURCES
    main.cc
    rpc.cc
    crash_handler.cc
    ${MILLENNIUM_BASE}/src/shared/crash_report.cc
    ${MILLENNIUM_BASE}/src/shared/crash_handler_core.cc
    lua_millennium_rpc.cc
    lua_config_rpc.cc
    lua_logger_rpc.cc
    lua_http.cc
    lua_utils.cc
    lua_filesystem.cc
    lua_regex.cc
    lua_datetime.cc
)

if(WIN32)
    list(APPEND LUA_HOST_SOURCES ${VERSION_FILE})
endif()

add_executable(${MILLENNIUM_RTB_NAME} ${LUA_HOST_SOURCES})

target_compile_definitions(${MILLENNIUM_RTB_NAME} PRIVATE
    MILLENNIUM_RTB_NAME="${MILLENNIUM_RTB_NAME}"
)

target_include_directories(${MILLENNIUM_RTB_NAME} PRIVATE
    ${MILLENNIUM_BASE}/src
    ${MILLENNIUM_BASE}/src/include
)

target_link_libraries(${MILLENNIUM_RTB_NAME}
    luajit::lib
    nlohmann_json
    fmt::fmt
)

if(NOT UNIX OR APPLE)
    target_link_libraries(${MILLENNIUM_RTB_NAME} cjson)
endif()

# Always ship with debug info so crash dumps are useful
if(MSVC)
    target_compile_options(${MILLENNIUM_RTB_NAME} PRIVATE /Z7)
    target_link_options(${MILLENNIUM_RTB_NAME} PRIVATE /DEBUG /ENTRY:mainCRTStartup
        "/MANIFESTDEPENDENCY:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'"
    )
    target_link_libraries(${MILLENNIUM_RTB_NAME} dbghelp ws2_32)
else()
    target_compile_options(${MILLENNIUM_RTB_NAME} PRIVATE -g)
    target_link_options(${MILLENNIUM_RTB_NAME} PRIVATE -rdynamic)
endif()

set_target_properties(${MILLENNIUM_RTB_NAME} PROPERTIES
    OUTPUT_NAME "${MILLENNIUM_RTB_NAME}"
)

if(WIN32)
    set_target_properties(${MILLENNIUM_RTB_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(NOT GITHUB_ACTION_BUILD)
    include(${MILLENNIUM_BASE}/scripts/cmake/find_steam_path.cmake)
    find_steam_path(${MILLENNIUM_RTB_NAME})
endif()
