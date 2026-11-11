// maszyna-python-host: CPython 2.7 or 3.14+ for PythonScreenServer, in a process of its own (the
// protocol: PythonHostProtocol.hpp). Whatever the runtime does - Py_FatalError() ends with abort(),
// an extension module of a broken installation crashes - ends this process, never the game. The
// server runs one host a runtime, side by side.
//
// Usage: maszyna-python-host <game_dir> <home> <library> <base_dir>, <home> and <library> relative
// to <game_dir> or absolute; <base_dir> holds the base classes of the scripts for this runtime.
#include "PythonHostProtocol.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <windows.h>

// after windows.h, which it needs
#include <shellapi.h>
#else
#include <cerrno>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using python_host::Reply;
using python_host::Request;
using python_host::Value;

/// Every script is asked for RGBA (manul_set_format("RGBA"), PyInt.cpp:468)
constexpr int BYTES_PER_PIXEL = 4;
/// Exit code of a normal end: the server closed the pipe
constexpr int EXIT_DONE = 0;

/// A line for the server, which reads the host's stderr once the host has ended
static void print_to_stderr(const std::string &p_text) {
    fputs((p_text + "\n").c_str(), stderr);
}

/// The part of the CPython C API the screens need, resolved from the library at run time - of 2.7
/// or of 3.14 and later, told apart by PyInitConfig_Create (PEP 741, 3.14). PyObject stays opaque:
/// references are counted through Py_IncRef/Py_DecRef, which both export as functions, and 3.x is
/// configured through the PyInitConfig functions, not a structure, so no Python header is needed
/// to build the host.
struct PyObject;
struct PyInitConfig;
// names mirror the Python C API symbols resolved below
// NOLINTBEGIN(readability-identifier-naming)
using Py_ssize_t = intptr_t;

struct PythonApi {
        /// The library is CPython 3.x, not 2.7
        bool python3 = false;

        // 2.7 only
        int *Py_IgnoreEnvironmentFlag = nullptr;
        int *Py_DontWriteBytecodeFlag = nullptr;
        int *Py_NoUserSiteDirectory = nullptr;
        void (*Py_SetPythonHome)(char *) = nullptr;
        void (*Py_InitializeEx)(int) = nullptr;

        // 3.14 and later only
        PyInitConfig *(*PyInitConfig_Create)() = nullptr;
        void (*PyInitConfig_Free)(PyInitConfig *) = nullptr;
        int (*PyInitConfig_SetInt)(PyInitConfig *, const char *, int64_t) = nullptr;
        int (*PyInitConfig_SetStr)(PyInitConfig *, const char *, const char *) = nullptr;
        int (*PyInitConfig_GetError)(PyInitConfig *, const char **) = nullptr;
        int (*Py_InitializeFromInitConfig)(PyInitConfig *) = nullptr;

        // both, under their names in either version: an int, a string in UTF-8 and the bytes of a
        // frame are PyInt, PyString and PyString on 2.7, PyLong, PyUnicode and PyBytes on 3.x
        PyObject *(*int_from_long)(long) = nullptr;
        long (*int_as_long)(PyObject *) = nullptr;
        PyObject *(*string_from_utf8)(const char *) = nullptr;
        const char *(*string_as_utf8)(PyObject *) = nullptr;
        int (*bytes_as_buffer)(PyObject *, char **, Py_ssize_t *) = nullptr;

        int (*PyRun_SimpleStringFlags)(const char *, void *) = nullptr;
        PyObject *(*PyImport_AddModule)(const char *) = nullptr;
        PyObject *(*PyObject_GetAttrString)(PyObject *, const char *) = nullptr;
        int (*PyObject_SetAttrString)(PyObject *, const char *, PyObject *) = nullptr;
        PyObject *(*PyObject_CallFunction)(PyObject *, char *, ...) = nullptr;
        PyObject *(*PyObject_CallMethod)(PyObject *, char *, char *, ...) = nullptr;
        PyObject *(*PyDict_New)() = nullptr;
        PyObject *(*PyList_New)(Py_ssize_t) = nullptr;
        int (*PyList_Append)(PyObject *, PyObject *) = nullptr;
        PyObject *(*Py_BuildValue)(const char *, ...) = nullptr;
        int (*PyDict_SetItemString)(PyObject *, const char *, PyObject *) = nullptr;
        PyObject *(*PyBool_FromLong)(long) = nullptr;
        PyObject *(*PyFloat_FromDouble)(double) = nullptr;
        Py_ssize_t (*PySequence_Size)(PyObject *) = nullptr;
        PyObject *(*PySequence_GetItem)(PyObject *, Py_ssize_t) = nullptr;
        PyObject *(*PyErr_Occurred)() = nullptr;
        void (*PyErr_Print)() = nullptr;
        void (*Py_DecRef)(PyObject *) = nullptr;
        // NOLINTEND(readability-identifier-naming)

        /// Loads the library and resolves every symbol; false with the reason on stderr
        bool load(const std::string &p_library_path) {
#ifdef _WIN32
            const std::wstring wide_path = to_wide(p_library_path);
            // the libraries it loads are looked for beside it first: the C runtime of the game's own
            // runtime (vcruntime140.dll, 2.7's private msvcr90.dll) is in its directory, not the host's
            HMODULE library = LoadLibraryExW(wide_path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
            auto resolve = [library](const char *p_name) -> void * {
                return reinterpret_cast<void *>(GetProcAddress(library, p_name));
            };
            if (library == nullptr) {
                print_to_stderr(
                        "cannot load " + p_library_path + " (Windows error " + std::to_string(GetLastError()) + ")");
                return false;
            }
#else
            // RTLD_GLOBAL: the extension modules a script imports resolve the interpreter's
            // symbols from the global namespace
            void *library = dlopen(p_library_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
            auto resolve = [library](const char *p_name) -> void * { return dlsym(library, p_name); };
            if (library == nullptr) {
                print_to_stderr("cannot load " + p_library_path + ": " + dlerror());
                return false;
            }
#endif
            bool resolved = true;
            auto bind = [&resolve, &resolved](auto &p_function, const char *p_name) {
                p_function = reinterpret_cast<std::remove_reference_t<decltype(p_function)>>(resolve(p_name));
                if (p_function == nullptr) {
                    print_to_stderr(std::string(p_name) + " missing from the library");
                    resolved = false;
                }
            };
            python3 = resolve("PyInitConfig_Create") != nullptr;
            if (python3) {
                bind(PyInitConfig_Create, "PyInitConfig_Create");
                bind(PyInitConfig_Free, "PyInitConfig_Free");
                bind(PyInitConfig_SetInt, "PyInitConfig_SetInt");
                bind(PyInitConfig_SetStr, "PyInitConfig_SetStr");
                bind(PyInitConfig_GetError, "PyInitConfig_GetError");
                bind(Py_InitializeFromInitConfig, "Py_InitializeFromInitConfig");
                bind(int_from_long, "PyLong_FromLong");
                bind(int_as_long, "PyLong_AsLong");
                bind(string_from_utf8, "PyUnicode_FromString");
                bind(string_as_utf8, "PyUnicode_AsUTF8");
                bind(bytes_as_buffer, "PyBytes_AsStringAndSize");
            } else if (resolve("PyInt_FromLong") != nullptr) {
                bind(Py_IgnoreEnvironmentFlag, "Py_IgnoreEnvironmentFlag");
                bind(Py_DontWriteBytecodeFlag, "Py_DontWriteBytecodeFlag");
                bind(Py_NoUserSiteDirectory, "Py_NoUserSiteDirectory");
                bind(Py_SetPythonHome, "Py_SetPythonHome");
                bind(Py_InitializeEx, "Py_InitializeEx");
                bind(int_from_long, "PyInt_FromLong");
                bind(int_as_long, "PyInt_AsLong");
                bind(string_from_utf8, "PyString_FromString");
                bind(string_as_utf8, "PyString_AsString");
                bind(bytes_as_buffer, "PyString_AsStringAndSize");
            } else {
                print_to_stderr(p_library_path + " is neither CPython 2.7 nor 3.14 or later (no PyInitConfig_Create)");
                return false;
            }
            bind(PyRun_SimpleStringFlags, "PyRun_SimpleStringFlags");
            bind(PyImport_AddModule, "PyImport_AddModule");
            bind(PyObject_GetAttrString, "PyObject_GetAttrString");
            bind(PyObject_SetAttrString, "PyObject_SetAttrString");
            bind(PyObject_CallFunction, "PyObject_CallFunction");
            bind(PyObject_CallMethod, "PyObject_CallMethod");
            bind(PyDict_New, "PyDict_New");
            bind(PyList_New, "PyList_New");
            bind(PyList_Append, "PyList_Append");
            bind(Py_BuildValue, "Py_BuildValue");
            bind(PyDict_SetItemString, "PyDict_SetItemString");
            bind(PyBool_FromLong, "PyBool_FromLong");
            bind(PyFloat_FromDouble, "PyFloat_FromDouble");
            bind(PySequence_Size, "PySequence_Size");
            bind(PySequence_GetItem, "PySequence_GetItem");
            bind(PyErr_Occurred, "PyErr_Occurred");
            bind(PyErr_Print, "PyErr_Print");
            bind(Py_DecRef, "Py_DecRef");
            return resolved;
        }

        /// Prints the pending exception, if there is one, into the captured sys.stderr
        void print_error() const {
            if (PyErr_Occurred() != nullptr) {
                PyErr_Print();
            }
        }

        /// Sets a string attribute of a module
        void set_string(PyObject *p_module, const char *p_name, const std::string &p_value) const {
            PyObject *value = string_from_utf8(p_value.c_str());
            PyObject_SetAttrString(p_module, p_name, value);
            Py_DecRef(value);
        }

        /// Runs a script file in __main__, the way the original runs every script
        /// (python_taskqueue::run_file()) - the class it defines lands there. execfile() is 2.x
        /// only; exec() of the compiled file is the same on both
        bool run_file(PyObject *p_main, const std::string &p_path) const {
            set_string(p_main, "_maszyna_script_path", p_path);
            return PyRun_SimpleStringFlags(
                           "exec(compile(open(_maszyna_script_path, 'rb').read(), _maszyna_script_path, 'exec'))",
                           nullptr) == 0;
        }

#ifdef _WIN32
        static std::wstring to_wide(const std::string &p_utf8) {
            const int size = MultiByteToWideChar(CP_UTF8, 0, p_utf8.c_str(), -1, nullptr, 0);
            std::wstring wide(size, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, p_utf8.c_str(), -1, wide.data(), size);
            return wide;
        }
#endif
};

static PythonApi python;

// The protocol's own ends of the pipes. stdout is taken over for the protocol before the
// runtime is loaded, and the process' stdout is pointed at stderr: whatever else writes to it
// (a C-level print of the runtime) cannot break into a message.
#ifdef _WIN32
static HANDLE protocol_in = nullptr;
static HANDLE protocol_out = nullptr;

static bool read_exact(std::vector<uint8_t> &p_data) {
    size_t done = 0;
    while (done < p_data.size()) {
        DWORD read = 0;
        if (!ReadFile(protocol_in, &p_data[done], static_cast<DWORD>(p_data.size() - done), &read, nullptr) ||
            read == 0) {
            return false;
        }
        done += read;
    }
    return true;
}

static bool write_all(const std::vector<uint8_t> &p_data) {
    size_t done = 0;
    while (done < p_data.size()) {
        DWORD written = 0;
        if (!WriteFile(protocol_out, &p_data[done], static_cast<DWORD>(p_data.size() - done), &written, nullptr)) {
            return false;
        }
        done += written;
    }
    return true;
}
#else
static int protocol_in = STDIN_FILENO;
static int protocol_out = -1;

static bool read_exact(std::vector<uint8_t> &p_data) {
    size_t done = 0;
    while (done < p_data.size()) {
        const ssize_t read = ::read(protocol_in, &p_data[done], p_data.size() - done);
        if (read < 0 && errno == EINTR) {
            continue;
        }
        if (read <= 0) {
            return false;
        }
        done += static_cast<size_t>(read);
    }
    return true;
}

static bool write_all(const std::vector<uint8_t> &p_data) {
    size_t done = 0;
    while (done < p_data.size()) {
        const ssize_t written = ::write(protocol_out, &p_data[done], p_data.size() - done);
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            return false;
        }
        done += static_cast<size_t>(written);
    }
    return true;
}
#endif

/// The fields of one message, read in order; `valid` turns false on the first read past its end
struct MessageReader {
        const std::vector<uint8_t> &data;
        size_t offset = 0;
        bool valid = true;

        template<typename T>
        T get() {
            T value{};
            if (offset + sizeof(T) > data.size()) {
                valid = false;
                return value;
            }
            memcpy(&value, &data[offset], sizeof(T));
            offset += sizeof(T);
            return value;
        }

        std::string get_string() {
            const auto size = get<uint32_t>();
            if (!valid || offset + size > data.size()) {
                valid = false;
                return {};
            }
            std::string text(size, '\0');
            if (size > 0) {
                memcpy(text.data(), &data[offset], size);
            }
            offset += size;
            return text;
        }
};

/// One message, its fields written in order after the room left for its size and type
struct MessageWriter {
        std::vector<uint8_t> data = std::vector<uint8_t>(sizeof(uint32_t) + sizeof(uint8_t));

        template<typename T>
        void put(const T p_value) {
            const size_t offset = data.size();
            data.resize(offset + sizeof(T));
            memcpy(&data[offset], &p_value, sizeof(T));
        }

        void put_bytes(const char *p_bytes, const size_t p_size) {
            put(static_cast<uint32_t>(p_size));
            const size_t offset = data.size();
            data.resize(offset + p_size);
            if (p_size > 0) {
                memcpy(&data[offset], p_bytes, p_size);
            }
        }

        void put_string(const std::string &p_text) {
            put_bytes(p_text.data(), p_text.size());
        }

        bool send(const Reply p_type) {
            const auto size = static_cast<uint32_t>(data.size() - sizeof(uint32_t));
            memcpy(data.data(), &size, sizeof(size));
            data[sizeof(uint32_t)] = static_cast<uint8_t>(p_type);
            return write_all(data);
        }
};

static bool send_log(const std::string &p_text) {
    MessageWriter message;
    message.put_string(p_text);
    return message.send(Reply::LOG);
}

/// What the scripts wrote to sys.stderr since the last time - their tracebacks above all -
/// goes to the game's log (the original keeps it in a StringIO too, PyInt.cpp:265)
static bool send_captured_errors() {
    PyObject *sys = python.PyImport_AddModule("sys");
    PyObject *captured = python.PyObject_GetAttrString(sys, "stderr");
    PyObject *text = python.PyObject_CallMethod(captured, const_cast<char *>("getvalue"), nullptr);
    const char *value = text != nullptr ? python.string_as_utf8(text) : nullptr;
    const bool sent = value == nullptr || *value == '\0' || send_log(value);
    if (text != nullptr) {
        python.Py_DecRef(text);
    }
    // io.StringIO (3.x) keeps its position past a truncate and would pad the next text with zeros
    // up to it; the original seeks back too (python_taskqueue::error())
    for (const char *method: {"truncate", "seek"}) {
        PyObject *result =
                python.PyObject_CallMethod(captured, const_cast<char *>(method), const_cast<char *>("(i)"), 0);
        if (result != nullptr) {
            python.Py_DecRef(result);
        }
    }
    python.Py_DecRef(captured);
    return sent;
}

/// A value of the state as a Python object, with the types the original passes
/// (dictionary_source: floats, integers, bools, strings and lists of 2D points)
static PyObject *read_value(MessageReader &p_reader) {
    switch (static_cast<Value>(p_reader.get<uint8_t>())) {
        case Value::BOOL:
            return python.PyBool_FromLong(p_reader.get<uint8_t>());
        case Value::INT:
            return python.int_from_long(static_cast<long>(p_reader.get<int64_t>()));
        case Value::FLOAT:
            return python.PyFloat_FromDouble(p_reader.get<double>());
        case Value::STRING:
            return python.string_from_utf8(p_reader.get_string().c_str());
        case Value::POINT: {
            const auto x = p_reader.get<double>();
            const auto y = p_reader.get<double>();
            return python.Py_BuildValue("(dd)", x, y);
        }
        case Value::ARRAY: {
            const auto count = p_reader.get<uint32_t>();
            PyObject *list = python.PyList_New(0);
            for (uint32_t i = 0; i < count && p_reader.valid; i++) {
                PyObject *item = read_value(p_reader);
                if (item != nullptr) {
                    python.PyList_Append(list, item);
                    python.Py_DecRef(item);
                }
            }
            return list;
        }
        default:
            p_reader.valid = false;
            return nullptr;
    }
}

static bool file_exists(const std::string &p_path) {
#ifdef _WIN32
    return GetFileAttributesW(PythonApi::to_wide(p_path).c_str()) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat status{};
    return stat(p_path.c_str(), &status) == 0;
#endif
}

/// One instance of the script's class a screen, made on its first render and released when the
/// screen is freed - not one a script, as python_taskqueue::fetch_renderer() keeps them
/// (MASZYNA_ORIGINAL_QUIRKS.md); a failure is kept as well, so a broken script is not re-run on
/// every update
static std::unordered_map<uint64_t, PyObject *> renderers;
static PyObject *main_module = nullptr;
/// The base classes of the scripts for this runtime (PythonScreenServer's own, not the game
/// directory's python/local/), first in sys.path
static std::string base_dir;

/// Answers whether a script compiles on this runtime, with the modules of the game directory it
/// imports, and loads there (_maszyna_check()); PythonScreenServer asks the 3.x host, and a script
/// that does not runs on 2.7. False when the request is malformed or the answer cannot be sent
static bool check(MessageReader &p_reader) {
    const std::string script_path = p_reader.get_string();
    if (!p_reader.valid) {
        print_to_stderr("a malformed check request");
        return false;
    }
    PyObject *checker = python.PyObject_GetAttrString(main_module, "_maszyna_check");
    PyObject *result =
            checker != nullptr
                    ? python.PyObject_CallFunction(checker, const_cast<char *>("(s)"), (script_path + ".py").c_str())
                    : nullptr;
    const bool compiles = result != nullptr && python.int_as_long(result) != 0;
    if (result != nullptr) {
        python.Py_DecRef(result);
    }
    if (checker != nullptr) {
        python.Py_DecRef(checker);
    }
    python.print_error();
    MessageWriter checked;
    checked.put(static_cast<uint8_t>(compiles ? 1 : 0));
    return send_captured_errors() && checked.send(Reply::CHECKED);
}

/// Draws one frame of a screen and answers with it; false when the request is malformed or
/// the answer cannot be sent
static bool render(MessageReader &p_reader) {
    const auto screen = p_reader.get<uint64_t>();
    const std::string script_path = p_reader.get_string();
    const auto count = p_reader.get<uint32_t>();
    PyObject *state = python.PyDict_New();
    for (uint32_t i = 0; i < count && p_reader.valid; i++) {
        const std::string key = p_reader.get_string();
        PyObject *item = read_value(p_reader);
        if (item != nullptr) {
            python.PyDict_SetItemString(state, key.c_str(), item);
            python.Py_DecRef(item);
        }
    }
    if (!p_reader.valid) {
        python.Py_DecRef(state);
        print_to_stderr("a malformed render request");
        return false;
    }

    // python_taskqueue::fetch_renderer(), by the screen
    auto cached = renderers.find(screen);
    if (cached == renderers.end()) {
        PyObject *renderer = nullptr;
        if (python.run_file(main_module, script_path + ".py")) {
            const size_t slash = script_path.rfind('/');
            const std::string class_name = script_path.substr(slash + 1);
            PyObject *renderer_class = python.PyObject_GetAttrString(main_module, class_name.c_str());
            if (renderer_class != nullptr) {
                renderer = python.PyObject_CallFunction(
                        renderer_class, const_cast<char *>("(s)"), (script_path.substr(0, slash) + "/").c_str());
                python.Py_DecRef(renderer_class);
            }
            if (renderer != nullptr) {
                PyObject *result = python.PyObject_CallMethod(
                        renderer, const_cast<char *>("manul_set_format"), const_cast<char *>("(s)"), "RGBA");
                if (result != nullptr) {
                    python.Py_DecRef(result);
                }
            }
            python.print_error();
        }
        cached = renderers.emplace(screen, renderer).first;
    }
    PyObject *renderer = cached->second;

    int width = 0;
    int height = 0;
    const char *pixels = nullptr;
    size_t pixels_size = 0;
    PyObject *output = nullptr;
    std::vector<std::string> commands;
    if (renderer != nullptr) {
        output = python.PyObject_CallMethod(renderer, const_cast<char *>("render"), const_cast<char *>("(O)"), state);
        if (output != nullptr) {
            PyObject *width_object = python.PyObject_CallMethod(renderer, const_cast<char *>("get_width"), nullptr);
            PyObject *height_object = python.PyObject_CallMethod(renderer, const_cast<char *>("get_height"), nullptr);
            char *buffer = nullptr;
            Py_ssize_t size = 0;
            if (width_object != nullptr && height_object != nullptr &&
                python.bytes_as_buffer(output, &buffer, &size) == 0) {
                width = static_cast<int>(python.int_as_long(width_object));
                height = static_cast<int>(python.int_as_long(height_object));
                const int64_t expected = static_cast<int64_t>(width) * height * BYTES_PER_PIXEL;
                if (width > 0 && height > 0 && size >= expected) {
                    pixels = buffer;
                    pixels_size = static_cast<size_t>(expected);
                } else {
                    send_log(
                            script_path + " returned " + std::to_string(size) + " bytes for " + std::to_string(width) +
                            "x" + std::to_string(height));
                }
            }
            if (width_object != nullptr) {
                python.Py_DecRef(width_object);
            }
            if (height_object != nullptr) {
                python.Py_DecRef(height_object);
            }
        }
        python.print_error();
    }
    // a script of the game data written for 2.7 and the Pillow of its time may fail on 3.x only
    // while it is made or draws (a rectangle with its corners swapped): no frame, and the server
    // runs it on 2.7 from then on
    if (output == nullptr && python.python3) {
        python.Py_DecRef(state);
        MessageWriter failed;
        failed.put(screen);
        return send_captured_errors() && failed.send(Reply::FAILED);
    }
    if (renderer != nullptr) {
        PyObject *command_list = python.PyObject_CallMethod(renderer, const_cast<char *>("getCommands"), nullptr);
        if (command_list != nullptr) {
            const Py_ssize_t command_count = python.PySequence_Size(command_list);
            for (Py_ssize_t i = 0; i < command_count; i++) {
                PyObject *command = python.PySequence_GetItem(command_list, i);
                const char *text = command != nullptr ? python.string_as_utf8(command) : nullptr;
                if (text != nullptr) {
                    commands.emplace_back(text);
                }
                if (command != nullptr) {
                    python.Py_DecRef(command);
                }
            }
            python.Py_DecRef(command_list);
        }
        python.print_error();
    }
    python.Py_DecRef(state);

    MessageWriter frame;
    frame.put(screen);
    frame.put(static_cast<int32_t>(width));
    frame.put(static_cast<int32_t>(height));
    frame.put_bytes(pixels, pixels_size);
    frame.put(static_cast<uint32_t>(commands.size()));
    for (const std::string &command: commands) {
        frame.put_string(command);
    }
    // the pixels are the output's own buffer, so it is released only once they are sent
    const bool sent = send_captured_errors() && frame.send(Reply::FRAME);
    if (output != nullptr) {
        python.Py_DecRef(output);
    }
    return sent;
}

/// Releases the instance of a freed screen's script; false when the request is malformed
static bool free_screen(MessageReader &p_reader) {
    const auto screen = p_reader.get<uint64_t>();
    if (!p_reader.valid) {
        print_to_stderr("a malformed free request");
        return false;
    }
    const auto renderer = renderers.find(screen);
    if (renderer != renderers.end()) {
        if (renderer->second != nullptr) {
            python.Py_DecRef(renderer->second);
        }
        renderers.erase(renderer);
    }
    return true;
}

/// The program's arguments in UTF-8, whatever the platform passes them in
static std::vector<std::string> get_arguments(const int p_argc, char **p_argv) {
#ifdef _WIN32
    (void)p_argc;
    (void)p_argv;
    int count = 0;
    LPWSTR *wide = CommandLineToArgvW(GetCommandLineW(), &count);
    std::vector<std::string> arguments;
    for (int i = 0; i < count; i++) {
        const int size = WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, nullptr, 0, nullptr, nullptr);
        std::string argument(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, argument.data(), size, nullptr, nullptr);
        argument.resize(size - 1);
        arguments.push_back(argument);
    }
    LocalFree(static_cast<HLOCAL>(wide));
    return arguments;
#else
    return {p_argv, std::next(p_argv, p_argc)};
#endif
}

int main(int p_argc, char **p_argv) {
    const std::vector<std::string> arguments = get_arguments(p_argc, p_argv);
    // the program, the game directory, the home, the library, the base classes' directory
    constexpr size_t ARGUMENT_COUNT = 5;
    if (arguments.size() != ARGUMENT_COUNT) {
        print_to_stderr("usage: maszyna-python-host <game_dir> <home> <library> <base_dir>");
        return python_host::EXIT_FAILED;
    }
    const std::string &game_dir = arguments[1];
    std::string home = arguments[2];
    const std::string &library = arguments[3];
    base_dir = arguments[4];

    // the runtime is started where the original starts it: in the game directory, which the
    // relative home and library are resolved against, and on Windows with the game directory
    // searched for the DLLs the runtime and its extension modules load, as next to eu07.exe
#ifdef _WIN32
    protocol_in = GetStdHandle(STD_INPUT_HANDLE);
    protocol_out = GetStdHandle(STD_OUTPUT_HANDLE);
    SetStdHandle(STD_OUTPUT_HANDLE, GetStdHandle(STD_ERROR_HANDLE));
    const std::wstring wide_game_dir = PythonApi::to_wide(game_dir);
    const bool entered = SetCurrentDirectoryW(wide_game_dir.c_str()) != 0;
    SetDllDirectoryW(wide_game_dir.c_str());
    // 2.7's PC/getpathp.c looks for Lib\os.py under the home
    const char *landmark = "/Lib/os.py";
#else
    protocol_out = dup(STDOUT_FILENO);
    dup2(STDERR_FILENO, STDOUT_FILENO);
    const bool entered = chdir(game_dir.c_str()) == 0;
    // 2.7's Modules/getpath.c looks for lib/python2.7/os.py under the home
    const char *landmark = "/lib/python2.7/os.py";
#endif
    if (!entered) {
        print_to_stderr("cannot enter the game directory " + game_dir);
        return python_host::EXIT_FAILED;
    }
    if (!file_exists(library)) {
        print_to_stderr(library + " is missing in " + game_dir);
        return python_host::EXIT_FAILED;
    }
    if (!python.load(library)) {
        return python_host::EXIT_FAILED;
    }
    // 3.x reports a home it cannot start from itself (PyInitConfig_GetError()); 2.7 ends in
    // Py_FatalError()
    if (!python.python3 && !file_exists(std::string(home).append(landmark))) {
        print_to_stderr(std::string(home).append(landmark).append(" is missing in ").append(game_dir));
        return python_host::EXIT_FAILED;
    }
    send_log("library loaded: " + library + ", home " + home + (python.python3 ? ", Python 3" : ", Python 2"));

    // the player's own Python installation must not leak in, and nothing is written into the
    // game directory (the scripts' .pyc would land next to them)
    if (python.python3) {
        // an isolated configuration: no environment, no user site directory; the signal handlers
        // stay the process' own, as the original's (PyInt.cpp, install_signal_handlers = 0)
        PyInitConfig *config = python.PyInitConfig_Create();
        const bool configured = config != nullptr && python.PyInitConfig_SetStr(config, "home", home.c_str()) == 0 &&
                                python.PyInitConfig_SetInt(config, "write_bytecode", 0) == 0 &&
                                python.PyInitConfig_SetInt(config, "install_signal_handlers", 0) == 0;
        if (!configured || python.Py_InitializeFromInitConfig(config) != 0) {
            const char *error = nullptr;
            if (config != nullptr && python.PyInitConfig_GetError(config, &error) == 1) {
                print_to_stderr(std::string("cannot start the interpreter: ") + error);
            } else {
                print_to_stderr("cannot start the interpreter");
            }
            if (config != nullptr) {
                python.PyInitConfig_Free(config);
            }
            return python_host::EXIT_FAILED;
        }
        python.PyInitConfig_Free(config);
    } else {
        *python.Py_IgnoreEnvironmentFlag = 1;
        *python.Py_DontWriteBytecodeFlag = 1;
        *python.Py_NoUserSiteDirectory = 1;
        // Py_SetPythonHome keeps the pointer: `home` lives as long as the interpreter
        python.Py_SetPythonHome(home.data());
        python.Py_InitializeEx(0);
    }
    send_log("interpreter started");
    main_module = python.PyImport_AddModule("__main__");
    python.set_string(main_module, "_maszyna_base_dir", base_dir);
    // The same code on 2.7 and 3.x: what moved (cStringIO, __builtin__, basestring) is taken
    // from where the runtime has it.
    // sys.stderr is captured and sent to the game's log, sys.stdout dropped (PyInt.cpp:265
    // keeps the first in a StringIO too). The home may be relative to the game directory, which a
    // script may leave: the search path is made absolute while it still resolves. The base
    // classes' directory goes first, the game directory after it.
    // The data is made on Windows, whose file names ignore letter case, and the scripts name files
    // as they please ("WS_gotowosc.png" for ws_gotowosc.png): a path that is not there as written
    // is looked for letter case aside, one directory at a time - for open(), which PIL opens images
    // with, and os.path.isfile(), which the scripts look for them with.
    // _maszyna_check() compiles a script and the modules of the game directory it imports
    // (`from scripts import rozklad_tabelka`), then loads the script - its imports and its class -
    // into a copy of __main__, as check() asks: `from sys import maxint` compiles on 3.x and fails
    // only there
    python.PyRun_SimpleStringFlags(
            "import ast, os, sys\n"
            "try:\n"
            "    from cStringIO import StringIO as _MaszynaStringIO\n"
            "except ImportError:\n"
            "    from io import StringIO as _MaszynaStringIO\n"
            "try:\n"
            "    import __builtin__ as _maszyna_builtins\n"
            "except ImportError:\n"
            "    import builtins as _maszyna_builtins\n"
            "try:\n"
            "    _maszyna_string_types = basestring\n"
            "except NameError:\n"
            "    _maszyna_string_types = str\n"
            "class _MaszynaDiscard(object):\n"
            "    def write(self, text):\n"
            "        pass\n"
            "    def flush(self):\n"
            "        pass\n"
            "sys.stdout = _MaszynaDiscard()\n"
            "sys.stderr = _MaszynaStringIO()\n"
            "sys.path = [os.path.abspath(path) for path in sys.path]\n"
            "sys.path.insert(0, os.path.abspath(_maszyna_base_dir))\n"
            "_maszyna_exists = os.path.exists\n"
            "def _maszyna_find_path(path):\n"
            "    if not isinstance(path, _maszyna_string_types) or _maszyna_exists(path):\n"
            "        return path\n"
            "    parts = os.path.normpath(path).split(os.sep)\n"
            "    current = '.' if parts[0] else os.sep\n"
            "    for part in parts:\n"
            "        if not part:\n"
            "            continue\n"
            "        candidate = os.path.join(current, part)\n"
            "        if not _maszyna_exists(candidate):\n"
            "            try:\n"
            "                names = [name for name in os.listdir(current) if name.lower() == part.lower()]\n"
            "            except OSError:\n"
            "                return path\n"
            "            if not names:\n"
            "                return path\n"
            "            candidate = os.path.join(current, names[0])\n"
            "        current = candidate\n"
            "    return current\n"
            "_maszyna_open = _maszyna_builtins.open\n"
            "_maszyna_builtins.open = lambda name, *args, **kwargs: _maszyna_open(_maszyna_find_path(name), *args, "
            "**kwargs)\n"
            "_maszyna_isfile = os.path.isfile\n"
            "os.path.isfile = lambda path: _maszyna_isfile(_maszyna_find_path(path))\n"
            "def _maszyna_check(path):\n"
            "    pending = [path]\n"
            "    checked = set()\n"
            "    while pending:\n"
            "        current = pending.pop()\n"
            "        if current in checked:\n"
            "            continue\n"
            "        checked.add(current)\n"
            "        try:\n"
            "            with open(current, 'rb') as source:\n"
            "                tree = compile(source.read(), current, 'exec', ast.PyCF_ONLY_AST)\n"
            "            compile(tree, current, 'exec')\n"
            "        except (SyntaxError, ValueError) as error:\n"
            "            sys.stderr.write('%s does not compile: %s\\n' % (current, error))\n"
            "            return False\n"
            "        for node in ast.walk(tree):\n"
            "            if isinstance(node, ast.Import):\n"
            "                names = [alias.name for alias in node.names]\n"
            "            elif isinstance(node, ast.ImportFrom) and node.module and not node.level:\n"
            "                names = [node.module] + [node.module + '.' + alias.name for alias in node.names]\n"
            "            else:\n"
            "                continue\n"
            "            for name in names:\n"
            "                module = os.path.join(_maszyna_game_dir, *name.split('.'))\n"
            "                for candidate in (module + '.py', os.path.join(module, '__init__.py')):\n"
            "                    if os.path.isfile(candidate):\n"
            "                        pending.append(_maszyna_find_path(candidate))\n"
            "    try:\n"
            "        with open(path, 'rb') as source:\n"
            "            exec(compile(source.read(), path, 'exec'), dict(globals()))\n"
            "    except Exception as error:\n"
            "        sys.stderr.write('%s does not load: %s: %s\\n' % (path, type(error).__name__, error))\n"
            "        return False\n"
            "    return True\n",
            nullptr);
    // The scripts open "./fonts/..." and "./textures/..." and import "from scripts", all relative to
    // the game directory, which the original is always started in; a host serves one game
    // directory for its whole life
    python.set_string(main_module, "_maszyna_game_dir", game_dir);
    python.PyRun_SimpleStringFlags("sys.path.insert(1, os.path.abspath(_maszyna_game_dir))\n", nullptr);
    // the base class of nearly every screen, as the original runs python/local/'s at its start
    // (python_taskqueue::init())
    python.run_file(main_module, base_dir + "/abstractscreenrenderer.py");
    if (!send_captured_errors() || !MessageWriter().send(Reply::DONE)) {
        return EXIT_DONE;
    }

    std::vector<uint8_t> size_field(sizeof(uint32_t));
    std::vector<uint8_t> data;
    while (true) {
        if (!read_exact(size_field)) {
            return EXIT_DONE; // the server has closed the pipe
        }
        uint32_t size = 0;
        memcpy(&size, size_field.data(), sizeof(size));
        data.resize(size);
        if (size == 0 || !read_exact(data)) {
            return EXIT_DONE;
        }
        MessageReader reader{data, 0, true};
        switch (static_cast<Request>(reader.get<uint8_t>())) {
            case Request::RENDER:
                if (!render(reader)) {
                    return python_host::EXIT_FAILED;
                }
                break;
            case Request::CHECK:
                if (!check(reader)) {
                    return python_host::EXIT_FAILED;
                }
                break;
            case Request::FREE:
                if (!free_screen(reader)) {
                    return python_host::EXIT_FAILED;
                }
                if (!send_captured_errors() || !MessageWriter().send(Reply::DONE)) {
                    return EXIT_DONE;
                }
                break;
            default:
                print_to_stderr("an unknown request");
                return python_host::EXIT_FAILED;
        }
    }
}
