// maszyna-python-host: CPython 2.7 for PythonScreenServer, in a process of its own (the protocol:
// PythonHostProtocol.hpp). Whatever the runtime does - Py_FatalError() ends with abort(), an
// extension module of a broken installation crashes - ends this process, never the game.
//
// Usage: maszyna-python-host <game_dir> <home> <library>, <home> and <library> relative to
// <game_dir> or absolute.
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

/// The part of the CPython 2.7 C API the screens need, resolved from the library at run
/// time. PyObject stays opaque: references are counted through Py_IncRef/Py_DecRef, which
/// 2.7 exports as functions, so no Python header is needed to build the host.
struct PyObject;
// names mirror the Python C API symbols resolved below
// NOLINTBEGIN(readability-identifier-naming)
using Py_ssize_t = intptr_t;

struct PythonApi {
        int *Py_IgnoreEnvironmentFlag = nullptr;
        int *Py_DontWriteBytecodeFlag = nullptr;
        int *Py_NoUserSiteDirectory = nullptr;
        void (*Py_SetPythonHome)(char *) = nullptr;
        void (*Py_InitializeEx)(int) = nullptr;
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
        PyObject *(*PyInt_FromLong)(long) = nullptr;
        long (*PyInt_AsLong)(PyObject *) = nullptr;
        PyObject *(*PyFloat_FromDouble)(double) = nullptr;
        PyObject *(*PyString_FromString)(const char *) = nullptr;
        char *(*PyString_AsString)(PyObject *) = nullptr;
        int (*PyString_AsStringAndSize)(PyObject *, char **, Py_ssize_t *) = nullptr;
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
            HMODULE library = LoadLibraryW(wide_path.c_str());
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
            bind(Py_IgnoreEnvironmentFlag, "Py_IgnoreEnvironmentFlag");
            bind(Py_DontWriteBytecodeFlag, "Py_DontWriteBytecodeFlag");
            bind(Py_NoUserSiteDirectory, "Py_NoUserSiteDirectory");
            bind(Py_SetPythonHome, "Py_SetPythonHome");
            bind(Py_InitializeEx, "Py_InitializeEx");
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
            bind(PyInt_FromLong, "PyInt_FromLong");
            bind(PyInt_AsLong, "PyInt_AsLong");
            bind(PyFloat_FromDouble, "PyFloat_FromDouble");
            bind(PyString_FromString, "PyString_FromString");
            bind(PyString_AsString, "PyString_AsString");
            bind(PyString_AsStringAndSize, "PyString_AsStringAndSize");
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

        /// Runs a script file in __main__, the way the original runs every script
        /// (python_taskqueue::run_file()) - the class it defines lands there
        bool run_file(PyObject *p_main, const std::string &p_path) const {
            PyObject *path = PyString_FromString(p_path.c_str());
            PyObject_SetAttrString(p_main, "_maszyna_script_path", path);
            Py_DecRef(path);
            return PyRun_SimpleStringFlags("execfile(_maszyna_script_path)", nullptr) == 0;
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
    const char *value = text != nullptr ? python.PyString_AsString(text) : nullptr;
    const bool sent = value == nullptr || *value == '\0' || send_log(value);
    if (text != nullptr) {
        python.Py_DecRef(text);
    }
    PyObject *result =
            python.PyObject_CallMethod(captured, const_cast<char *>("truncate"), const_cast<char *>("(i)"), 0);
    if (result != nullptr) {
        python.Py_DecRef(result);
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
            return python.PyInt_FromLong(static_cast<long>(p_reader.get<int64_t>()));
        case Value::FLOAT:
            return python.PyFloat_FromDouble(p_reader.get<double>());
        case Value::STRING:
            return python.PyString_FromString(p_reader.get_string().c_str());
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

/// One instance of each script's class, as python_taskqueue::fetch_renderer() keeps them;
/// a failure is kept as well, so a broken script is not re-run on every update
static std::unordered_map<std::string, PyObject *> renderers;
static PyObject *main_module = nullptr;

/// The scripts open "./fonts/..." and "./textures/..." and import "from scripts", all
/// relative to the game directory, which the original is always started in. Entering
/// another one forgets the scripts' instances and the modules imported from the last one.
static void enter_game_dir(const std::string &p_game_dir) {
    for (const auto &renderer: renderers) {
        if (renderer.second != nullptr) {
            python.Py_DecRef(renderer.second);
        }
    }
    renderers.clear();
    PyObject *game_dir = python.PyString_FromString(p_game_dir.c_str());
    python.PyObject_SetAttrString(main_module, "_maszyna_game_dir", game_dir);
    python.Py_DecRef(game_dir);
    python.PyRun_SimpleStringFlags(
            "import os, sys\n"
            "_maszyna_left_dir = globals().get('_maszyna_entered_dir')\n"
            "if _maszyna_left_dir:\n"
            "    if _maszyna_left_dir in sys.path:\n"
            "        sys.path.remove(_maszyna_left_dir)\n"
            "    for _maszyna_name, _maszyna_module in list(sys.modules.items()):\n"
            "        _maszyna_file = getattr(_maszyna_module, '__file__', None)\n"
            "        if _maszyna_file and os.path.abspath(_maszyna_file).startswith(_maszyna_left_dir):\n"
            "            del sys.modules[_maszyna_name]\n"
            "_maszyna_entered_dir = _maszyna_game_dir\n"
            "os.chdir(_maszyna_game_dir)\n"
            "sys.path.insert(0, _maszyna_game_dir)\n",
            nullptr);
    // the base class of nearly every screen; a script that does not derive from it
    // still runs when it is missing
    python.run_file(main_module, p_game_dir + "/python/local/abstractscreenrenderer.py");
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

    // python_taskqueue::fetch_renderer()
    auto cached = renderers.find(script_path);
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
        cached = renderers.emplace(script_path, renderer).first;
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
                python.PyString_AsStringAndSize(output, &buffer, &size) == 0) {
                width = static_cast<int>(python.PyInt_AsLong(width_object));
                height = static_cast<int>(python.PyInt_AsLong(height_object));
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

        PyObject *command_list = python.PyObject_CallMethod(renderer, const_cast<char *>("getCommands"), nullptr);
        if (command_list != nullptr) {
            const Py_ssize_t command_count = python.PySequence_Size(command_list);
            for (Py_ssize_t i = 0; i < command_count; i++) {
                PyObject *command = python.PySequence_GetItem(command_list, i);
                const char *text = command != nullptr ? python.PyString_AsString(command) : nullptr;
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
    // the program, the game directory, the home, the library
    constexpr size_t ARGUMENT_COUNT = 4;
    if (arguments.size() != ARGUMENT_COUNT) {
        print_to_stderr("usage: maszyna-python-host <game_dir> <home> <library>");
        return python_host::EXIT_FAILED;
    }
    const std::string &game_dir = arguments[1];
    std::string home = arguments[2];
    const std::string &library = arguments[3];

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
    // PC/getpathp.c looks for Lib\os.py under the home
    const char *landmark = "/Lib/os.py";
#else
    protocol_out = dup(STDOUT_FILENO);
    dup2(STDERR_FILENO, STDOUT_FILENO);
    const bool entered = chdir(game_dir.c_str()) == 0;
    // Modules/getpath.c looks for lib/python2.7/os.py under the home
    const char *landmark = "/lib/python2.7/os.py";
#endif
    if (!entered) {
        print_to_stderr("cannot enter the game directory " + game_dir);
        return python_host::EXIT_FAILED;
    }
    for (const std::string &required: {library, std::string(home).append(landmark)}) {
        if (!file_exists(required)) {
            print_to_stderr(std::string(required).append(" is missing in ").append(game_dir));
            return python_host::EXIT_FAILED;
        }
    }
    if (!python.load(library)) {
        return python_host::EXIT_FAILED;
    }
    send_log("library loaded: " + library + ", home " + home);

    // the player's own Python installation must not leak in, and nothing is written into the
    // game directory (the scripts' .pyc would land next to them)
    *python.Py_IgnoreEnvironmentFlag = 1;
    *python.Py_DontWriteBytecodeFlag = 1;
    *python.Py_NoUserSiteDirectory = 1;
    // Py_SetPythonHome keeps the pointer: `home` lives as long as the interpreter
    python.Py_SetPythonHome(home.data());
    python.Py_InitializeEx(0);
    send_log("interpreter started");
    main_module = python.PyImport_AddModule("__main__");
    // sys.stderr is captured and sent to the game's log, sys.stdout dropped (PyInt.cpp:265
    // keeps the first in a StringIO too). The home may be relative to the game directory, which a
    // script may leave: the search path is made absolute while it still resolves.
    // The data is made on Windows, whose file names ignore letter case, and the scripts name files
    // as they please ("WS_gotowosc.png" for ws_gotowosc.png): a path that is not there as written
    // is looked for letter case aside, one directory at a time - for open(), which PIL opens images
    // with, and os.path.isfile(), which the scripts look for them with
    python.PyRun_SimpleStringFlags(
            "import os, sys, cStringIO, __builtin__\n"
            "class _MaszynaDiscard(object):\n"
            "    def write(self, text):\n"
            "        pass\n"
            "    def flush(self):\n"
            "        pass\n"
            "sys.stdout = _MaszynaDiscard()\n"
            "sys.stderr = cStringIO.StringIO()\n"
            "sys.path = [os.path.abspath(path) for path in sys.path]\n"
            "_maszyna_exists = os.path.exists\n"
            "def _maszyna_find_path(path):\n"
            "    if not isinstance(path, basestring) or _maszyna_exists(path):\n"
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
            "_maszyna_open = __builtin__.open\n"
            "__builtin__.open = lambda name, *args, **kwargs: _maszyna_open(_maszyna_find_path(name), *args, "
            "**kwargs)\n"
            "_maszyna_isfile = os.path.isfile\n"
            "os.path.isfile = lambda path: _maszyna_isfile(_maszyna_find_path(path))\n",
            nullptr);
    enter_game_dir(game_dir);
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
            case Request::ENTER_GAME_DIR: {
                const std::string entering = reader.get_string();
                enter_game_dir(entering);
                if (!send_captured_errors() || !MessageWriter().send(Reply::DONE)) {
                    return EXIT_DONE;
                }
                break;
            }
            case Request::RENDER:
                if (!render(reader)) {
                    return python_host::EXIT_FAILED;
                }
                break;
            default:
                print_to_stderr("an unknown request");
                return python_host::EXIT_FAILED;
        }
    }
}
