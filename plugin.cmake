# libmaszyna - MaSzyna's vehicles and sceneries, a plugin of Rails Reloaded compiled into the core's
# library (LIBRELOADED_PLUGINS in the core's CMakeLists.txt, which includes this file)
SET(MASZYNA_PLUGIN_DIR "${CMAKE_CURRENT_LIST_DIR}")

FILE(GLOB_RECURSE MASZYNA_PLUGIN_SOURCES CONFIGURE_DEPENDS
        "${MASZYNA_PLUGIN_DIR}/src/*.cpp" "${MASZYNA_PLUGIN_DIR}/src/*.hpp")
# maszyna-python-host is a program of its own (below), not a part of the library
LIST(FILTER MASZYNA_PLUGIN_SOURCES EXCLUDE REGEX "src/maszyna/cabin/python_host/main\\.cpp$")
# the memory cells and the original's eu07.events, opened in every scenario script's Lua state
IF (NOT LIBRELOADED_LUA)
    LIST(FILTER MASZYNA_PLUGIN_SOURCES EXCLUDE REGEX "src/maszyna/scenario/MaszynaLua(Events|Memory)Module")
ENDIF ()
TARGET_SOURCES(${LIBNAME} PRIVATE ${MASZYNA_PLUGIN_SOURCES})
TARGET_INCLUDE_DIRECTORIES(${LIBNAME} PRIVATE "${MASZYNA_PLUGIN_DIR}/src")

FILE(GLOB_RECURSE MASZYNA_VENDORED_FILES "${MASZYNA_PLUGIN_DIR}/src/maszyna/maszyna-mover/*.cpp")
IF (CMAKE_SYSTEM_NAME STREQUAL "Android")
    # Disable warnings for maszyna directory explicitly for Android
    SET_SOURCE_FILES_PROPERTIES(${MASZYNA_VENDORED_FILES} PROPERTIES COMPILE_OPTIONS "-w")
ENDIF ()
IF (NOT MSVC)
    # The vendored engine is written against its own stdafx.h, which includes <stdlib.h>: that is
    # what makes an unqualified abs(double) resolve to std::abs. Without it C's int abs() wins and
    # every |x| < 1 truncates to 0 - the brake handles' control reservoir froze (FINDINGS 2026-09-24)
    SET_PROPERTY(SOURCE ${MASZYNA_VENDORED_FILES} APPEND PROPERTY COMPILE_OPTIONS "-include" "stdlib.h")
ENDIF ()

# PythonScreenServer runs CPython in maszyna-python-host, a program next to the library (and next
# to an exported game's executable, [dependencies] of libreloaded.gdextension): whatever the runtime
# does ends that process, never the game. It has no Godot in it; it loads CPython at run time.
# A platform without it (Android, iOS) names none, and the screens stay blank with the error said.
IF (CMAKE_SYSTEM_NAME MATCHES "^(Windows|Linux|Darwin)$")
    ADD_EXECUTABLE(maszyna-python-host "${MASZYNA_PLUGIN_DIR}/src/maszyna/cabin/python_host/main.cpp")
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
        # no MinGW runtime DLLs beside the game, as for the library
        TARGET_LINK_OPTIONS(maszyna-python-host PRIVATE -static-libgcc -static-libstdc++ -static)
    ENDIF ()
    # For multi-config generators (like Visual Studio): no config subfolder, as for the library
    GET_PROPERTY(MASZYNA_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    IF (MASZYNA_MULTI_CONFIG)
        FOREACH (CONFIG ${CMAKE_CONFIGURATION_TYPES})
            STRING(TOUPPER ${CONFIG} CONFIG)
            SET_TARGET_PROPERTIES(maszyna-python-host PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY_${CONFIG} "${GODOT_PROJECT_BINARY_DIR}")
        ENDFOREACH ()
    ENDIF ()
    ADD_DEPENDENCIES(${LIBNAME} maszyna-python-host)
    TARGET_COMPILE_DEFINITIONS(${LIBNAME} PRIVATE "MASZYNA_PYTHON_HOST_FILE=\"$<TARGET_FILE_NAME:maszyna-python-host>\"")
ELSE ()
    TARGET_COMPILE_DEFINITIONS(${LIBNAME} PRIVATE "MASZYNA_PYTHON_HOST_FILE=\"\"")
ENDIF ()
# beside the exported executable (an empty target)
STRING(APPEND RELOADED_PLUGIN_DEPENDENCIES
        "; maszyna-python-host, the process PythonScreenServer runs CPython in\n"
        "windows.x86_64 = { \"res://bin/libreloaded/windows/maszyna-python-host.64.exe\": \"\" }\n"
        "linux.x86_64 = { \"res://bin/libreloaded/linux/maszyna-python-host.64\": \"\" }\n"
        "linux.arm64 = { \"res://bin/libreloaded/linux/maszyna-python-host.arm64\": \"\" }\n"
        "linux.rv64 = { \"res://bin/libreloaded/linux/maszyna-python-host.rv64\": \"\" }\n")

FILE(GLOB_RECURSE MASZYNA_PLUGIN_DOC_XML LIST_DIRECTORIES NO CONFIGURE_DEPENDS "${MASZYNA_PLUGIN_DIR}/doc_classes/*.xml")
LIST(APPEND DOC_XML ${MASZYNA_PLUGIN_DOC_XML})

reloaded_link_addon("${MASZYNA_PLUGIN_DIR}/addons/libmaszyna" libmaszyna)
reloaded_plugin(MaszynaPlugin "maszyna/MaszynaPlugin.hpp")
