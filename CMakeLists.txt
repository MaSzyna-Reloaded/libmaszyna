CMAKE_MINIMUM_REQUIRED(VERSION 3.17)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_EXTENSIONS ON)
SET(CMAKE_COLOR_DIAGNOSTICS ON)
# scripts/style-check binds the same double-precision API as the build, so clang-tidy sees the
# same types; it is dumped by the configure below and never versioned
set(GODOTCPP_PRECISION "double" CACHE STRING "" FORCE)
set(GODOTCPP_CUSTOM_API_FILE "${CMAKE_SOURCE_DIR}/extension_api.json" CACHE FILEPATH "" FORCE)
# godot-cpp ships the single-precision API only, so the double one is dumped once from a
# double-precision Godot and not versioned; delete it to dump it again after a GODOT_VERSION bump
IF (NOT EXISTS "${GODOTCPP_CUSTOM_API_FILE}")
    FIND_PROGRAM(
        GODOT_EXECUTABLE NAMES godot-double godot REQUIRED
        DOC "A double-precision Godot, used to dump extension_api.json"
    )
    MESSAGE(STATUS "Dumping extension_api.json with ${GODOT_EXECUTABLE}")
    # --dump-extension-api writes the file into the working directory
    EXECUTE_PROCESS(
        COMMAND "${GODOT_EXECUTABLE}" --headless --dump-extension-api
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE GODOT_DUMP_RESULT
        OUTPUT_QUIET
    )
    IF (EXISTS "${GODOTCPP_CUSTOM_API_FILE}")
        # "precision" sits in the "header" object at the top of the dump
        FILE(READ "${GODOTCPP_CUSTOM_API_FILE}" GODOT_API_HEADER LIMIT 1024)
        STRING(FIND "${GODOT_API_HEADER}" "\"precision\": \"double\"" GODOT_API_DOUBLE)
    ENDIF ()
    IF (NOT GODOT_DUMP_RESULT EQUAL 0 OR NOT GODOT_API_DOUBLE GREATER -1)
        FILE(REMOVE "${GODOTCPP_CUSTOM_API_FILE}")
        MESSAGE(FATAL_ERROR
            "${GODOT_EXECUTABLE} did not dump a double-precision extension API; "
            "pass -DGODOT_EXECUTABLE=<path to a double-precision Godot>")
    ENDIF ()
ENDIF ()
OPTION(LIBMASZYNA_DEBUG "If the build should contain custom library logger" OFF)
# Do not hard‑force a generator; allow users to select one suitable for cross‑compiling.
# Silence unused variable warning when specified from toolchain
IF (CMAKE_C_COMPILER)
ENDIF ()

# Allow selecting platform via -DGODOTCPP_PLATFORM=android|windows|linux|macos|ios|web
IF (DEFINED GODOTCPP_PLATFORM AND NOT CMAKE_SYSTEM_NAME)
    STRING(TOLOWER "${GODOTCPP_PLATFORM}" _req_platform)
    IF (_req_platform STREQUAL "android")
        # Prefer the official Android NDK toolchain if not explicitly set
        # Allow manual override via ANDROID_NDK_TOOLCHAIN (full path) or ANDROID_NDK_ROOT (NDK dir)
        IF (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
            # 1) Explicit toolchain file provided by user
            IF (DEFINED ANDROID_NDK_TOOLCHAIN)
                SET(CMAKE_TOOLCHAIN_FILE "${ANDROID_NDK_TOOLCHAIN}" CACHE FILEPATH "Android NDK toolchain (manual)" FORCE)
                # 2) NDK root directory provided by user
            ELSEIF (DEFINED ANDROID_NDK_ROOT)
                SET(CMAKE_TOOLCHAIN_FILE "${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" CACHE FILEPATH "Android NDK toolchain (from ANDROID_NDK_ROOT)" FORCE)
                # 3) Environment variables commonly used
            ELSEIF (DEFINED ENV{ANDROID_NDK})
                SET(CMAKE_TOOLCHAIN_FILE "$ENV{ANDROID_NDK}/build/cmake/android.toolchain.cmake" CACHE FILEPATH "Android NDK toolchain (from ANDROID_NDK)" FORCE)
            ELSEIF (DEFINED ENV{ANDROID_NDK_HOME})
                SET(CMAKE_TOOLCHAIN_FILE "$ENV{ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" CACHE FILEPATH "Android NDK toolchain (from ANDROID_NDK_HOME)" FORCE)
            ENDIF ()
        ENDIF ()

        # Tell CMake we are targeting Android when using the NDK toolchain
        SET(CMAKE_SYSTEM_NAME Android CACHE STRING "" FORCE)

        # Provide reasonable defaults if not supplied by user
        IF (NOT DEFINED ANDROID_ABI)
            SET(ANDROID_ABI arm64-v8a CACHE STRING "Android ABI")
        ENDIF ()
        IF (NOT DEFINED ANDROID_PLATFORM)
            SET(ANDROID_PLATFORM 34 CACHE STRING "Android API level")
        ENDIF ()

        IF (DEFINED CMAKE_TOOLCHAIN_FILE)
            MESSAGE(STATUS "Configuring cross‑compile for Android (ABI=${ANDROID_ABI}, API=${ANDROID_PLATFORM}) using toolchain: ${CMAKE_TOOLCHAIN_FILE}")
        ELSE ()
            MESSAGE(STATUS "Configuring cross‑compile for Android (ABI=${ANDROID_ABI}, API=${ANDROID_PLATFORM}).")
        ENDIF ()
    ELSEIF (_req_platform STREQUAL "windows")
        # Windows cross‑compile via MinGW (recommended from non‑Windows hosts)
        IF (NOT DEFINED CMAKE_TOOLCHAIN_FILE AND NOT CMAKE_HOST_WIN32)
            # Best effort: if cross compilers are present, set them. This must be before project().
            FIND_PROGRAM(MINGW_C "x86_64-w64-mingw32-gcc")
            FIND_PROGRAM(MINGW_CXX "x86_64-w64-mingw32-g++")
            IF (MINGW_C AND MINGW_CXX)
                SET(CMAKE_SYSTEM_NAME Windows CACHE STRING "" FORCE)
                SET(CMAKE_C_COMPILER ${MINGW_C} CACHE FILEPATH "" FORCE)
                SET(CMAKE_CXX_COMPILER ${MINGW_CXX} CACHE FILEPATH "" FORCE)
                MESSAGE(STATUS "Configuring cross‑compile for Windows using MinGW (x86_64).")
            ELSE ()
                # Fallback: still set system name; user must provide a toolchain or compilers.
                SET(CMAKE_SYSTEM_NAME Windows CACHE STRING "" FORCE)
                MESSAGE(WARNING "GODOTCPP_PLATFORM=windows set but no MinGW cross compiler auto‑detected. Provide a toolchain file or set CMAKE_C_COMPILER/CMAKE_CXX_COMPILER.")
            ENDIF ()
        ELSEIF (NOT CMAKE_HOST_WIN32)
            SET(CMAKE_SYSTEM_NAME Windows CACHE STRING "" FORCE)
            MESSAGE(STATUS "Configuring cross‑compile for Windows using specified toolchain: ${CMAKE_TOOLCHAIN_FILE}")
        ENDIF ()
    ENDIF ()
ENDIF ()

SET(LIBNAME "libmaszyna" CACHE STRING "The name of the library")
SET(GODOT_PROJECT_DIR "demo" CACHE STRING "The directory of a Godot project folder - relative to this one, or absolute (a game using libmaszyna as a submodule builds into its own project)")
GET_FILENAME_COMPONENT(GODOT_PROJECT_PATH "${GODOT_PROJECT_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
SET(IS_DEBUG_BUILD)

FIND_PACKAGE(Python3 3.4 REQUIRED)
ADD_SUBDIRECTORY(godot-cpp SYSTEM)

# Add godot-cpp's module path and include the exported functions.
# This is made available for documentation generation
SET(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${godot-cpp_SOURCE_DIR}/cmake")
INCLUDE(GodotCPPModule)

# The godot-cpp target has some of useful properties attached that can be retrieved like so.
GET_TARGET_PROPERTY(GODOTCPP_SUFFIX godot::cpp GODOTCPP_SUFFIX)
GET_TARGET_PROPERTY(GODOTCPP_PLATFORM godot::cpp GODOTCPP_PLATFORM)

PROJECT(Maszyna-API-Wrapper
        VERSION 1.0
        DESCRIPTION "MaSzyna API wrapper for Godot Engine written in C++ as GDExtension"
        HOMEPAGE_URL "https://github.com/MaSzyna-Reloaded/MaSzyna-API-wrapper"
        LANGUAGES CXX
)

IF (MSVC)
    # /utf-8: Set source and execution character sets to UTF-8
    # /bigobj: Increase the number of addressable sections in an .obj file
    ADD_COMPILE_OPTIONS(/utf-8 /bigobj)
ENDIF ()

FILE(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp" "src/*.hpp")
# maszyna-python-host is a program of its own (below), not a part of the library
LIST(FILTER SRC_FILES EXCLUDE REGEX "src/legacy/cabin/python_host/main\\.cpp$")

# Scenario scripts (ScenarioScriptServer) run on Lua, the vendor/lua submodule. Without it the
# server stays, and refuses every script as the original's build without Lua does
OPTION(LIBMASZYNA_LUA "Run scenario scripts in Lua (vendor/lua)" ON)
IF (NOT LIBMASZYNA_LUA)
    LIST(FILTER SRC_FILES EXCLUDE REGEX "src/scripting/lua/|src/legacy/scenario/MaszynaLegacyLuaEventsModule")
ENDIF ()

IF (CMAKE_SYSTEM_NAME STREQUAL "Android")
    # Disable warnings for maszyna directory explicitly for Android
    FILE(GLOB_RECURSE MASZYNA_SRC_FILES "src/legacy/maszyna-mover/*.cpp")
    IF(MASZYNA_SRC_FILES)
        SET_SOURCE_FILES_PROPERTIES(${MASZYNA_SRC_FILES} PROPERTIES COMPILE_OPTIONS "-w")
    ENDIF()
ENDIF ()

IF (NOT MSVC)
    # The vendored engine is written against its own stdafx.h, which includes <stdlib.h>: that is
    # what makes an unqualified abs(double) resolve to std::abs. Without it C's int abs() wins and
    # every |x| < 1 truncates to 0 - the brake handles' control reservoir froze (FINDINGS 2026-09-24)
    FILE(GLOB_RECURSE MASZYNA_VENDORED_FILES "src/legacy/maszyna-mover/*.cpp")
    SET_PROPERTY(SOURCE ${MASZYNA_VENDORED_FILES} APPEND PROPERTY COMPILE_OPTIONS "-include" "stdlib.h")
ENDIF ()

ADD_LIBRARY(${LIBNAME} SHARED ${SRC_FILES})
TARGET_INCLUDE_DIRECTORIES(${LIBNAME} PRIVATE src)
TARGET_SOURCES(${LIBNAME} PRIVATE)

FILE(GLOB_RECURSE DOC_XML LIST_DIRECTORIES NO CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/doc_classes/*.xml")

IF (DOC_XML)
    IF (GODOTCPP_TARGET MATCHES "editor|template_debug")
        TARGET_DOC_SOURCES(${LIBNAME} "${DOC_XML}") #Must be in order for Godot to properly read all the documentation
    ENDIF ()
ENDIF ()

TARGET_LINK_LIBRARIES(${LIBNAME} godot-cpp)

IF (LIBMASZYNA_LUA)
    # The core and the libraries a scenario script may use - not the interpreters, the test
    # harness, the amalgamation, nor io/os/debug/package, which a script has no business with
    SET(LUA_SRC_FILES)
    FOREACH (LUA_MODULE lapi lauxlib lbaselib lcode lcorolib lctype ldebug ldo ldump lfunc lgc llex
             lmathlib lmem lobject lopcodes lparser lstate lstring lstrlib ltable ltablib ltm lundump
             lutf8lib lvm lzio)
        LIST(APPEND LUA_SRC_FILES "vendor/lua/${LUA_MODULE}.c")
    ENDFOREACH ()
    # Compiled as C++: a script error then unwinds as an exception (LUAI_THROW, ldo.c) instead of a
    # longjmp over the String and Variant locals of the wrapper's own functions
    SET_SOURCE_FILES_PROPERTIES(${LUA_SRC_FILES} PROPERTIES LANGUAGE CXX)
    ADD_LIBRARY(maszyna_lua STATIC ${LUA_SRC_FILES})
    SET_TARGET_PROPERTIES(maszyna_lua PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_VISIBILITY_PRESET hidden)
    TARGET_COMPILE_OPTIONS(maszyna_lua PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>)
    TARGET_INCLUDE_DIRECTORIES(${LIBNAME} PRIVATE vendor/lua)
    TARGET_COMPILE_DEFINITIONS(${LIBNAME} PRIVATE LIBMASZYNA_LUA)
    TARGET_LINK_LIBRARIES(${LIBNAME} maszyna_lua)
ENDIF ()

# The build number is not stamped here. A stamp written while the library links is overwritten by
# the next build of any other configuration, and the export then packs whichever one happened to
# be on disk - which is how a release shipped a library and a project two hours apart. The
# Makefile's `build-number` target writes it when asked, so it describes a release and so the
# Linux and the Windows package, built by separate invocations, carry the same one.

# A MinGW-built DLL otherwise needs libstdc++-6.dll, libgcc_s_seh-1.dll and libwinpthread-1.dll
# sitting next to the game, and Godot only ships the executable and this library - the extension
# would simply fail to load on a clean Windows. Godot's own MinGW builds do the same
# (use_static_cpp).
IF (MINGW)
    TARGET_LINK_OPTIONS(${LIBNAME} PRIVATE -static-libgcc -static-libstdc++ -static)
ENDIF ()

SET_TARGET_PROPERTIES(${LIBNAME}
        PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        RUNTIME_OUTPUT_DIRECTORY "$<1:${GODOT_PROJECT_PATH}/bin/libmaszyna/${GODOTCPP_PLATFORM}>"
        PREFIX ""
        OUTPUT_NAME "${LIBNAME}"
)

# Determine architecture suffix for output filename
SET(ARCH_SUFFIX "")
IF (APPLE)
    SET(ARCH_SUFFIX "")
ELSE ()
    # For Android, prefer ANDROID_ABI when available
    IF (DEFINED ANDROID_ABI)
        STRING(TOLOWER "${ANDROID_ABI}" _proc_lc)
        IF (_proc_lc MATCHES "^arm64\-v8a$")
            SET(_proc_lc "arm64")
        ELSEIF (_proc_lc MATCHES "^armeabi\-v7a$")
            SET(_proc_lc "arm32")
        ELSEIF (_proc_lc STREQUAL "x86_64")
            SET(_proc_lc "x86_64")
        ELSEIF (_proc_lc STREQUAL "x86")
            SET(_proc_lc "x86")
        ENDIF ()
    ELSE ()
        STRING(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _proc_lc)
    ENDIF ()
    IF (_proc_lc MATCHES "^(x86_64|amd64)$")
        SET(ARCH_SUFFIX ".64")
    ELSEIF (_proc_lc MATCHES "^(aarch64|arm64)$")
        SET(ARCH_SUFFIX ".arm64")
    ELSEIF (_proc_lc MATCHES "^(riscv64|rv64)$")
        SET(ARCH_SUFFIX ".rv64")
    ELSEIF (_proc_lc MATCHES "^arm(32)?$")
        SET(ARCH_SUFFIX ".arm32")
    ELSEIF (_proc_lc MATCHES "^x86$")
        SET(ARCH_SUFFIX ".32")
    ELSE ()
        # Fallback: use pointer size to detect generic 64-bit x86 when processor string is unknown
        IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
            SET(ARCH_SUFFIX ".64")
        ENDIF ()
    ENDIF ()
ENDIF ()

# Compose final output name using GODOTCPP_TARGET for debug/release and architecture suffix
# GODOTCPP_TARGET values: template_debug | template_release
SET(DEBUG_SUFFIX "")
IF (GODOTCPP_TARGET STREQUAL "template_debug")
    SET(DEBUG_SUFFIX ".debug")
ENDIF ()

IF (LIBMASZYNA_DEBUG)
    TARGET_COMPILE_DEFINITIONS(${LIBNAME} PRIVATE LIBMASZYNA_DEBUG)
ENDIF ()

IF (GODOTCPP_TARGET STREQUAL "template_debug")
    GET_TARGET_PROPERTY(_libmaszyna_link_options ${LIBNAME} LINK_OPTIONS)
    IF (_libmaszyna_link_options)
        LIST(REMOVE_ITEM _libmaszyna_link_options "-s")
        SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES LINK_OPTIONS "${_libmaszyna_link_options}")
    ENDIF ()
ENDIF ()

SET(_OUTPUT_NAME "${LIBNAME}${DEBUG_SUFFIX}${ARCH_SUFFIX}")
SET(GODOT_PROJECT_BINARY_DIR "${GODOT_PROJECT_PATH}/bin/libmaszyna/${GODOTCPP_PLATFORM}")

SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES OUTPUT_NAME "${_OUTPUT_NAME}" LIBRARY_OUTPUT_DIRECTORY "${GODOT_PROJECT_BINARY_DIR}")

# PythonScreenServer runs CPython in maszyna-python-host, a program next to the library (and next
# to an exported game's executable, [dependencies] of libmaszyna.gdextension): whatever the runtime
# does ends that process, never the game. It has no Godot in it; it loads CPython at run time.
# A platform without it (Android, iOS) names none, and the screens stay blank with the error said.
IF (CMAKE_SYSTEM_NAME MATCHES "^(Windows|Linux|Darwin)$")
    ADD_EXECUTABLE(maszyna-python-host src/legacy/cabin/python_host/main.cpp)
    SET_TARGET_PROPERTIES(maszyna-python-host PROPERTIES
            OUTPUT_NAME "maszyna-python-host${ARCH_SUFFIX}"
            RUNTIME_OUTPUT_DIRECTORY "${GODOT_PROJECT_BINARY_DIR}"
    )
    TARGET_LINK_LIBRARIES(maszyna-python-host ${CMAKE_DL_LIBS})
    IF (WIN32)
        # CommandLineToArgvW - the arguments in UTF-16, whatever the code page
        TARGET_LINK_LIBRARIES(maszyna-python-host shell32)
    ENDIF ()
    IF (MINGW)
        # no MinGW runtime DLLs beside the game, as for the library below
        TARGET_LINK_OPTIONS(maszyna-python-host PRIVATE -static-libgcc -static-libstdc++ -static)
    ENDIF ()
    ADD_DEPENDENCIES(${LIBNAME} maszyna-python-host)
    TARGET_COMPILE_DEFINITIONS(${LIBNAME} PRIVATE "MASZYNA_PYTHON_HOST_FILE=\"$<TARGET_FILE_NAME:maszyna-python-host>\"")
ELSE ()
    TARGET_COMPILE_DEFINITIONS(${LIBNAME} PRIVATE "MASZYNA_PYTHON_HOST_FILE=\"\"")
ENDIF ()

# For multi-config generators (like Visual Studio), ensure output directory doesn't have a config subfolder
GET_PROPERTY(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
IF (IS_MULTI_CONFIG)
    FOREACH (CONFIG ${CMAKE_CONFIGURATION_TYPES})
        STRING(TOUPPER ${CONFIG} CONFIG)
        SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY_${CONFIG} "${GODOT_PROJECT_BINARY_DIR}"
                RUNTIME_OUTPUT_DIRECTORY_${CONFIG} "${GODOT_PROJECT_BINARY_DIR}"
                ARCHIVE_OUTPUT_DIRECTORY_${CONFIG} "${GODOT_PROJECT_BINARY_DIR}"
        )
        IF (TARGET maszyna-python-host)
            SET_TARGET_PROPERTIES(maszyna-python-host PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${CONFIG} "${GODOT_PROJECT_BINARY_DIR}")
        ENDIF ()
    ENDFOREACH ()
ENDIF ()

SET(ADDONS_DEMO_DIR "${GODOT_PROJECT_PATH}/addons")
ADD_CUSTOM_COMMAND(TARGET ${LIBNAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ADDONS_DEMO_DIR}"
)

SET(SRC_ADDON_LIBMASZYNA "${PROJECT_SOURCE_DIR}/addons/libmaszyna")
SET(DST_ADDON_LIBMASZYNA "${ADDONS_DEMO_DIR}/libmaszyna")
MESSAGE(NOTICE "Copying/linking addons and submodules to ${ADDONS_DEMO_DIR}...")
IF (UNIX)
    # On UNIX: prefer symlinks; if destination already a symlink, do nothing.
    IF (NOT IS_SYMLINK "${DST_ADDON_LIBMASZYNA}")
        MESSAGE(NOTICE "No symlinks detected, linking...")
        ADD_CUSTOM_COMMAND(TARGET ${LIBNAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf "${DST_ADDON_LIBMASZYNA}"
                COMMAND ${CMAKE_COMMAND} -E create_symlink "${SRC_ADDON_LIBMASZYNA}" "${DST_ADDON_LIBMASZYNA}"
        )
    ENDIF ()
else ()
    # Non-UNIX: copy directory; if destination is a symlink, omit copying.
    IF (NOT IS_SYMLINK "${DST_ADDON_LIBMASZYNA}")
        ADD_CUSTOM_COMMAND(TARGET ${LIBNAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf "${DST_ADDON_LIBMASZYNA}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_ADDON_LIBMASZYNA}" "${DST_ADDON_LIBMASZYNA}"
        )
    ENDIF ()
ENDIF ()

SET(SRC_ADDON_GUT "${PROJECT_SOURCE_DIR}/vendor/gut/addons/gut")
SET(DST_ADDON_GUT "${ADDONS_DEMO_DIR}/gut")
IF (UNIX)
    # On UNIX: prefer symlinks; if destination already a symlink, do nothing.
    IF (NOT IS_SYMLINK "${DST_ADDON_GUT}")
        MESSAGE(NOTICE "No symlinks detected, linking...")
        ADD_CUSTOM_COMMAND(TARGET ${LIBNAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf "${DST_ADDON_GUT}"
                COMMAND ${CMAKE_COMMAND} -E create_symlink "${SRC_ADDON_GUT}" "${DST_ADDON_GUT}"
        )
    ENDIF ()
else ()
    # Non-UNIX: copy directory; if destination is a symlink, omit copying.
    IF (NOT IS_SYMLINK "${DST_ADDON_GUT}")
        ADD_CUSTOM_COMMAND(TARGET ${LIBNAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf "${DST_ADDON_GUT}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_ADDON_GUT}" "${DST_ADDON_GUT}"
        )
    ENDIF ()
ENDIF ()

SET(SRC_ADDON_GND_SFX "${PROJECT_SOURCE_DIR}/vendor/gnd-sfx/addons/gnd_sfx")
SET(DST_ADDON_GND_SFX "${ADDONS_DEMO_DIR}/gnd_sfx")
IF (UNIX)
    # On UNIX: prefer symlinks; if destination already a symlink, do nothing.
    IF (NOT IS_SYMLINK "${DST_ADDON_GND_SFX}")
        MESSAGE(NOTICE "No symlinks detected, linking...")
        ADD_CUSTOM_COMMAND(TARGET ${LIBNAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${DST_ADDON_GND_SFX}"
            COMMAND ${CMAKE_COMMAND} -E create_symlink "${SRC_ADDON_GND_SFX}" "${DST_ADDON_GND_SFX}"
        )
    ENDIF ()
else ()
    # Non-UNIX: copy directory; if destination is a symlink, omit copying.
    IF (NOT IS_SYMLINK "${DST_ADDON_GND_SFX}")
        ADD_CUSTOM_COMMAND(TARGET ${LIBNAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf "${DST_ADDON_GND_SFX}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_ADDON_GND_SFX}" "${DST_ADDON_GND_SFX}"
        )
    ENDIF ()
ENDIF ()
