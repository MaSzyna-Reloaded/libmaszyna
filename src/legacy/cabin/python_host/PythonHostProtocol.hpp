#pragma once
#include <cstdint>

/// The pipe protocol between PythonScreenServer and maszyna-python-host, the process CPython 2.7
/// runs in so that nothing the runtime does - a fatal error, a crash in an extension module - can
/// end the game. The server writes requests to the host's stdin and reads replies from its stdout;
/// the host's stderr is read only once the host has ended, for what it printed on its way out.
///
/// A message is its size (uint32, the bytes after it), its type (uint8) and the fields of the type.
/// Both ends run on one machine, so numbers are in its own byte order: integers fixed-size, a
/// double as itself, a string as its size (uint32) and its UTF-8 bytes.
namespace python_host {
    /// Server -> host. Every request is answered by DONE or FRAME, LOG lines may come before it.
    enum class Request : uint8_t {
        /// string game_dir: the scripts are now this directory's
        ENTER_GAME_DIR = 1,
        /// uint64 screen, string script_path, uint32 count, count x (string key, a Value)
        RENDER = 2,
    };

    /// Host -> server
    enum class Reply : uint8_t {
        /// string text - a line for the game's log
        LOG = 1,
        /// the host has started (its first reply), or the game directory has been entered
        DONE = 2,
        /// uint64 screen, int32 width, int32 height, uint32 size, size bytes of RGBA pixels (none
        /// when the script drew nothing), uint32 count, count x string command
        FRAME = 3,
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
