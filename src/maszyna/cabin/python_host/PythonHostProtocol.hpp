#pragma once
#include <cstdint>

/// The pipe protocol between PythonScreenServer and maszyna-python-host, the process CPython (2.7 or
/// 3.x, one host each) runs in so that nothing the runtime does - a fatal error, a crash in an extension module - can
/// end the game. The server writes requests to the host's stdin and reads replies from its stdout;
/// the host's stderr is read only once the host has ended, for what it printed on its way out.
///
/// A message is its size (uint32, the bytes after it), its type (uint8) and the fields of the type.
/// Both ends run on one machine, so numbers are in its own byte order: integers fixed-size, a
/// double as itself, a string as its size (uint32) and its UTF-8 bytes.
namespace python_host {
    /// Server -> host. Every request is answered by DONE, FRAME, FAILED or CHECKED, LOG lines may
    /// come before it.
    enum class Request : uint8_t {
        /// uint64 screen, string script_path, uint32 count, count x (string key, a Value)
        RENDER = 2,
        /// uint64 screen: the screen is gone, and so is its script's instance
        FREE = 3,
        /// string script_path: does the script compile, with the modules of the game directory it
        /// imports, and load
        CHECK = 4,
    };

    /// Host -> server
    enum class Reply : uint8_t {
        /// string text - a line for the game's log
        LOG = 1,
        /// the host has started in its game directory (its first reply) or a screen has been freed
        DONE = 2,
        /// uint64 screen, int32 width, int32 height, uint32 size, size bytes of RGBA pixels (none
        /// when the script drew nothing), uint32 count, count x string command
        FRAME = 3,
        /// uint8 1 when the script of a CHECK compiles and loads, 0 when it does not
        CHECKED = 4,
        /// uint64 screen: on 3.x the script of a RENDER could not be made or raised while drawing
        /// (what it printed came as LOG); there is no frame
        FAILED = 5,
    };

    /// A value of the state: its type (uint8), then the value
    enum class Value : uint8_t {
        /// uint8 0 or 1
        BOOL = 1,
        /// int64
        INT = 2,
        /// double
        FLOAT = 3,
        /// string
        STRING = 4,
        /// double x, double y
        POINT = 5,
        /// uint32 count, count x a Value
        ARRAY = 6,
    };

    /// The host's exit code when it stops by itself - the runtime is missing or does not load, a
    /// request is malformed; the reason is on its stderr
    constexpr int EXIT_FAILED = 2;
} // namespace python_host
