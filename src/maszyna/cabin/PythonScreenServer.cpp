#include "PythonScreenServer.hpp"
#include "game_data/GameDataServer.hpp"
#include "maszyna/MaszynaGameDirectory.hpp"
#include "maszyna/cabin/python_host/PythonHostProtocol.hpp"
#include "utils/ReloadedProjectSetting.hpp"
#include <cstdint>
#include <cstring>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *PythonScreenServer::screen_rendered_signal = "screen_rendered";
    const char *PythonScreenServer::python_runtime_failed_signal = "python_runtime_failed";
    namespace {
        using python_host::Reply;
        using python_host::Value;

        /// Bytes of the host's stderr read at a time, once it has ended
        constexpr int64_t ERRORS_CHUNK_SIZE = 4096;
        /// By PythonScreenServer::Runtime: its directory of the base classes (BASE_SCRIPTS_PATH)
        /// and its name in the log
        constexpr const char *BASE_SCRIPTS_DIRS[] = {"3", "2.7"};
        constexpr const char *RUNTIME_NAMES[] = {"Python 3", "Python 2.7"};

        /// One request, its fields written in order after the room left for its size and type
        /// (PythonHostProtocol.hpp)
        struct RequestWriter {
                PackedByteArray data;

                explicit RequestWriter(const python_host::Request p_type) {
                    data.resize(sizeof(uint32_t));
                    data.push_back(static_cast<uint8_t>(p_type));
                }

                template<typename T>
                void put(const T p_value) {
                    const int64_t offset = data.size();
                    data.resize(offset + static_cast<int64_t>(sizeof(T)));
                    memcpy(&data[offset], &p_value, sizeof(T));
                }

                void put_string(const String &p_text) {
                    const CharString utf8 = p_text.utf8();
                    put(static_cast<uint32_t>(utf8.length()));
                    const int64_t offset = data.size();
                    data.resize(offset + utf8.length());
                    if (utf8.length() > 0) {
                        memcpy(&data[offset], utf8.get_data(), utf8.length());
                    }
                }

                /// A value of the state with the types the original passes (dictionary_source:
                /// floats, integers, bools, strings and lists of 2D points); false, and nothing
                /// written, for any other type
                bool put_value(const Variant &p_value) {
                    switch (p_value.get_type()) {
                        case Variant::BOOL:
                            put(static_cast<uint8_t>(Value::BOOL));
                            put(static_cast<uint8_t>(static_cast<bool>(p_value) ? 1 : 0));
                            return true;
                        case Variant::INT:
                            put(static_cast<uint8_t>(Value::INT));
                            put(static_cast<int64_t>(p_value));
                            return true;
                        case Variant::FLOAT:
                            put(static_cast<uint8_t>(Value::FLOAT));
                            put(static_cast<double>(p_value));
                            return true;
                        case Variant::STRING:
                        case Variant::STRING_NAME:
                            put(static_cast<uint8_t>(Value::STRING));
                            put_string(p_value);
                            return true;
                        case Variant::VECTOR2: {
                            const Vector2 point = p_value;
                            put(static_cast<uint8_t>(Value::POINT));
                            put(static_cast<double>(point.x));
                            put(static_cast<double>(point.y));
                            return true;
                        }
                        case Variant::ARRAY: {
                            const Array values = p_value;
                            put(static_cast<uint8_t>(Value::ARRAY));
                            const int64_t count_offset = data.size();
                            uint32_t count = 0;
                            put(count);
                            for (int64_t i = 0; i < values.size(); i++) {
                                count += put_value(values[i]) ? 1 : 0;
                            }
                            memcpy(&data[count_offset], &count, sizeof(count));
                            return true;
                        }
                        default:
                            return false;
                    }
                }

                /// The state's keys with a value of a type put_value() takes; the rest is left out
                void put_state(const Dictionary &p_state) {
                    const int64_t count_offset = data.size();
                    uint32_t count = 0;
                    put(count);
                    const Array keys = p_state.keys();
                    for (int64_t i = 0; i < keys.size(); i++) {
                        const int64_t key_offset = data.size();
                        put_string(keys[i]);
                        if (put_value(p_state[keys[i]])) {
                            count++;
                        } else {
                            data.resize(key_offset);
                        }
                    }
                    memcpy(&data[count_offset], &count, sizeof(count));
                }

                bool send(const Ref<FileAccess> &p_pipe) {
                    const auto size = static_cast<uint32_t>(data.size() - static_cast<int64_t>(sizeof(uint32_t)));
                    memcpy(&data[0], &size, sizeof(size));
                    return p_pipe->store_buffer(data);
                }
        };

        /// The fields of one reply, read in order after its type; `valid` turns false on the
        /// first read past its end
        struct ReplyReader {
                const PackedByteArray &data;
                int64_t offset = 0;
                bool valid = true;

                template<typename T>
                T get() {
                    T value{};
                    if (offset + static_cast<int64_t>(sizeof(T)) > data.size()) {
                        valid = false;
                        return value;
                    }
                    memcpy(&value, &data[offset], sizeof(T));
                    offset += sizeof(T);
                    return value;
                }

                PackedByteArray get_bytes() {
                    const auto size = static_cast<int64_t>(get<uint32_t>());
                    if (!valid || offset + size > data.size()) {
                        valid = false;
                        return {};
                    }
                    const PackedByteArray bytes = data.slice(offset, offset + size);
                    offset += size;
                    return bytes;
                }

                String get_string() {
                    return get_bytes().get_string_from_utf8();
                }
        };

    } // namespace

    /// Exactly `p_size` bytes of a pipe, fewer when it closes first
    static PackedByteArray read_exact(const Ref<FileAccess> &p_pipe, const int64_t p_size) {
        PackedByteArray bytes;
        while (bytes.size() < p_size) {
            const PackedByteArray part = p_pipe->get_buffer(p_size - bytes.size());
            if (part.is_empty()) {
                break;
            }
            bytes.append_array(part);
        }
        return bytes;
    }

    void PythonScreenServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("screen_create", "script_path", "commands_received"), &PythonScreenServer::screen_create);
        ClassDB::bind_method(D_METHOD("screen_get_texture", "screen"), &PythonScreenServer::screen_get_texture);
        ClassDB::bind_method(
                D_METHOD("screen_get_average_color", "screen"), &PythonScreenServer::screen_get_average_color);
        ADD_SIGNAL(MethodInfo(screen_rendered_signal, PropertyInfo(Variant::RID, "screen")));
        ADD_SIGNAL(MethodInfo(python_runtime_failed_signal, PropertyInfo(Variant::STRING, "message")));
        ClassDB::bind_method(
                D_METHOD("screen_request_render", "screen", "state"), &PythonScreenServer::screen_request_render);
        ClassDB::bind_method(D_METHOD("screen_free", "screen"), &PythonScreenServer::screen_free);
    }

    PythonScreenServer::PythonScreenServer() {
        if (disabled) {
            UtilityFunctions::print("[PythonScreen] disabled by ", ARG_NO_PYTHON, " - the screens stay blank");
        }
        if (GameDataServer *game_data = GameDataServer::get_instance(); game_data != nullptr) {
            game_data->connect(
                    GameDataServer::data_reload_requested_signal,
                    callable_mp(this, &PythonScreenServer::_on_data_reload_requested));
        }
    }

    /// The runtimes and the scripts are the game directory's: the hosts end, and the next render
    /// finds the runtimes of the game directory set by then, starts their hosts in it and checks
    /// its scripts again
    void PythonScreenServer::_on_data_reload_requested() {
        _stop_hosts();
    }

    PythonScreenServer::~PythonScreenServer() {
        _stop_hosts();
    }

    /// Each worker ends, its pipe closes with it and its host ends on its own; the hosts are as
    /// before the first render then
    void PythonScreenServer::_stop_hosts() {
        {
            MutexLock lock(mutex);
            stopping = true;
        }
        for (Host &host: hosts) {
            if (host.worker.is_valid()) {
                host.semaphore->post();
                host.worker->wait_to_finish();
            }
        }
        MutexLock lock(mutex);
        for (Host &host: hosts) {
            host = Host();
        }
        script_runtimes.clear();
        stopping = false;
    }

    /* A host is a process of its own with a CPython in it, which most sessions - no cab with a
     * Python screen - never need: the 3.x one is started by the first render, the 2.7 one by the
     * first script that needs it, in the game directory set by then, not the one at the engine's
     * start. Started once a game directory: its worker stays until the data is reloaded, a host
     * that failed included, so no render starts it again. Its runtime is the game's own, beside the
     * host (python2.7/, python3.NN/, each its own home), else the game directory's; without a 3.x
     * runtime none starts, and every script runs on 2.7 */
    void PythonScreenServer::_start_host(const Runtime p_runtime) {
        const MaszynaGameDirectory *game_directory = MaszynaGameDirectory::get_instance();
        ERR_FAIL_NULL(game_directory);
        OS *os = OS::get_singleton();
        const String game_dir = game_directory->get_game_dir();
        // an exported game has the host next to its executable (the [dependencies] of
        // libmaszyna.gdextension), the project next to this library
        String library_path;
        gdextension_interface::get_library_path(gdextension_interface::library, library_path._native_ptr());
        const String host_dir = os->has_feature("template")
                                        ? os->get_executable_path().get_base_dir()
                                        : ProjectSettings::get_singleton()->globalize_path(library_path.get_base_dir());

        String home;
        String library;
        // the game's own runtime, as make python-runtime-linux and python-runtime-windows make it:
        // python3.NN/ or python2.7/, with libpython3.NN.so.1.0 in its lib/, or python3NN.dll in it
        for (const String &dir: DirAccess::get_directories_at(host_dir)) {
            if (p_runtime == RUNTIME_PYTHON_3 ? !dir.begins_with("python3.") : dir != "python2.7") {
                continue;
            }
#ifdef _WIN32
            const String candidate = host_dir.path_join(dir).path_join(dir.replace(".", "") + ".dll");
#else
            const String candidate = host_dir.path_join(dir).path_join("lib/lib" + dir + ".so.1.0");
#endif
            if (FileAccess::file_exists(candidate)) {
                home = host_dir.path_join(dir);
                library = candidate;
            }
        }
        // else the game directory's: the home is relative to it, which the host starts in, as the
        // original gives it (PyInt.cpp:233); the library is a path, which Windows does not search for
        if (library.is_empty() && p_runtime == RUNTIME_PYTHON_3) {
#ifdef _WIN32
            // the original's 3.x runtime: python3NN.dll beside eu07.exe, its home python64 as 2.7's
            // (PyInt.cpp, python_taskqueue::init()); python3.dll is only the stable ABI's forwarder
            for (const String &file: DirAccess::get_files_at(game_dir)) {
                const String name = file.to_lower();
                if (name.begins_with("python3") && name.get_extension() == "dll" && name != "python3.dll") {
                    home = "python64";
                    library = game_dir.path_join(file);
                }
            }
#else
            // a runtime of make python-runtime-linux put there: python3.NN/lib/libpython3.NN.so.1.0
            for (const String &dir: DirAccess::get_directories_at(game_dir)) {
                const String candidate = dir.path_join("lib/lib" + dir + ".so.1.0");
                if (dir.begins_with("python3.") && FileAccess::file_exists(game_dir.path_join(candidate))) {
                    home = dir;
                    library = candidate;
                }
            }
#endif
            if (library.is_empty()) {
                UtilityFunctions::print(
                        "[PythonScreen] no Python 3 runtime beside ", host_dir, " nor in ", game_dir,
                        " - the scripts run on Python 2.7");
                MutexLock lock(mutex);
                hosts[p_runtime].unavailable = true;
                return;
            }
        } else if (library.is_empty()) {
            home = ProjectSettings::get_singleton()->get_setting(PYTHON_HOME_SETTING);
#ifdef _WIN32
            // the 64-bit Windows runtime ships in the game directory
            home = home.is_empty() ? String("python64") : home;
            library = game_dir.path_join("python27.dll");
#else
            // the original's linuxpython64 (PyInt.cpp:238) is only a virtualenv over the system's
            // libpython, without PIL - a runtime of make python-runtime-linux has a directory of its own
            home = home.is_empty() ? String("python2.7") : home;
            library = home.path_join("lib/libpython2.7.so.1.0");
#endif
        }

        // the base classes go out of the project (or the exported pack) to files the host can read
        const String base_source = String(BASE_SCRIPTS_PATH).path_join(BASE_SCRIPTS_DIRS[p_runtime]);
        const String base_dir = ProjectSettings::get_singleton()->globalize_path(
                String(BASE_SCRIPTS_COPY_PATH).path_join(BASE_SCRIPTS_DIRS[p_runtime]));
        DirAccess::make_dir_recursive_absolute(base_dir);
        for (const String &file: DirAccess::get_files_at(base_source)) {
            if (file.get_extension() == "py") {
                const Ref<FileAccess> copy = FileAccess::open(base_dir.path_join(file), FileAccess::WRITE);
                ERR_CONTINUE_MSG(copy.is_null(), "[PythonScreen] cannot write " + base_dir.path_join(file));
                copy->store_buffer(FileAccess::get_file_as_bytes(base_source.path_join(file)));
            }
        }

        const String host_path = host_dir.path_join(MASZYNA_PYTHON_HOST_FILE);
        UtilityFunctions::print(
                "[PythonScreen] starting the ", RUNTIME_NAMES[p_runtime], " host: ", host_path, ", game directory ",
                game_dir, ", home ", home, ", library ", library, ", base classes ", base_dir);
        Host &host = hosts[p_runtime];
        host.semaphore.instantiate();
        host.worker.instantiate();
        host.worker->start(callable_mp(this, &PythonScreenServer::_worker_loop)
                                   .bind(static_cast<int>(p_runtime), host_path,
                                         PackedStringArray({game_dir, home, library, base_dir})));
    }

    RID PythonScreenServer::screen_create(const String &p_script_path, const Callable &p_commands_received) {
        Ref<Image> blank = Image::create_empty(1, 1, false, Image::FORMAT_RGBA8);
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        screens.insert(rid, Screen{p_script_path, ImageTexture::create_from_image(blank), p_commands_received});
        return rid;
    }

    Ref<Texture2D> PythonScreenServer::screen_get_texture(const RID &p_screen) const {
        const Screen *screen = screens.getptr(p_screen);
        ERR_FAIL_NULL_V(screen, Ref<Texture2D>());
        return screen->texture;
    }

    Color PythonScreenServer::screen_get_average_color(const RID &p_screen) const {
        const Screen *screen = screens.getptr(p_screen);
        ERR_FAIL_NULL_V(screen, Color());
        return screen->average_color;
    }

    /// A script not checked yet goes to the 3.x host, which checks it first; one that does not
    /// compile or load there, or fails there while it draws, and every script while 3.x is
    /// unavailable, goes to 2.7
    void PythonScreenServer::screen_request_render(const RID &p_screen, const Dictionary &p_state) {
        const Screen *screen = screens.getptr(p_screen);
        ERR_FAIL_NULL(screen);
        if (disabled) {
            return; // no interpreter to draw it
        }
        // a host is written only by its worker once it is started, so before that no lock is needed
        if (hosts[RUNTIME_PYTHON_3].worker.is_null() && !hosts[RUNTIME_PYTHON_3].unavailable) {
            _start_host(RUNTIME_PYTHON_3);
        }
        Runtime runtime = RUNTIME_PYTHON_2;
        {
            MutexLock lock(mutex);
            if (!hosts[RUNTIME_PYTHON_3].unavailable) {
                const Runtime *checked = script_runtimes.getptr(screen->script_path);
                runtime = checked != nullptr ? *checked : RUNTIME_PYTHON_3;
            }
        }
        if (runtime == RUNTIME_PYTHON_2 && hosts[RUNTIME_PYTHON_2].worker.is_null() &&
            !hosts[RUNTIME_PYTHON_2].unavailable) {
            _start_host(RUNTIME_PYTHON_2);
        }
        Host &host = hosts[runtime];
        {
            MutexLock lock(mutex);
            if (host.unavailable) {
                return; // reported once, when the host ended
            }
            bool replaced = false;
            for (Request &request: host.requests) {
                if (request.screen == p_screen) {
                    request.state = p_state.duplicate();
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                host.requests.push_back(Request{p_screen, screen->script_path, p_state.duplicate()});
            }
        }
        host.semaphore->post();
    }

    void PythonScreenServer::_render_after_check(const RID &p_screen, const Dictionary &p_state) {
        if (screens.has(p_screen)) {
            screen_request_render(p_screen, p_state); // freed while it was checked otherwise
        }
    }

    /// The instance of its script is released in whichever host has one - a screen of a 3.x host
    /// that ended draws on 2.7 since
    void PythonScreenServer::screen_free(const RID &p_screen) {
        screens.erase(p_screen);
        MutexLock lock(mutex);
        for (Host &host: hosts) {
            for (List<Request>::Element *element = host.requests.front(); element != nullptr;
                 element = element->next()) {
                if (element->get().screen == p_screen) {
                    element->erase();
                    break;
                }
            }
            if (host.worker.is_valid() && !host.unavailable) {
                host.requests.push_back(Request{p_screen, String(), Dictionary(), true});
                host.semaphore->post();
            }
        }
    }

    void PythonScreenServer::_publish(
            const RID &p_screen, const int p_width, const int p_height, const PackedByteArray &p_pixels,
            const PackedStringArray &p_commands) {
        Screen *screen = screens.getptr(p_screen);
        if (screen == nullptr) {
            return; // freed while it was being drawn
        }
        if (!p_pixels.is_empty()) {
            const Ref<Image> image = Image::create_from_data(p_width, p_height, false, Image::FORMAT_RGBA8, p_pixels);
            if (screen->texture->get_width() == p_width && screen->texture->get_height() == p_height) {
                screen->texture->update(image);
            } else {
                screen->texture->set_image(image);
            }
            // a grid of samples is enough for the colour a screen throws around it
            Color sum;
            for (int row = 0; row < AVERAGE_COLOR_SAMPLES; row++) {
                for (int column = 0; column < AVERAGE_COLOR_SAMPLES; column++) {
                    const int x = ((column * 2) + 1) * p_width / (AVERAGE_COLOR_SAMPLES * 2);
                    const int y = ((row * 2) + 1) * p_height / (AVERAGE_COLOR_SAMPLES * 2);
                    const int64_t pixel = ((static_cast<int64_t>(y) * p_width) + x) * BYTES_PER_PIXEL;
                    sum += Color::from_rgba8(
                            p_pixels[pixel], p_pixels[pixel + 1], p_pixels[pixel + 2], p_pixels[pixel + 3]);
                }
            }
            screen->average_color = sum / static_cast<float>(AVERAGE_COLOR_SAMPLES * AVERAGE_COLOR_SAMPLES);
            emit_signal(screen_rendered_signal, p_screen);
        }
        if (!p_commands.is_empty()) {
            screen->commands_received.call(p_commands);
        }
    }

    void PythonScreenServer::_announce_failure(const String &p_message) {
        emit_signal(python_runtime_failed_signal, p_message);
    }

    void
    PythonScreenServer::_worker_loop(const int p_runtime, const String &p_host, const PackedStringArray &p_arguments) {
        const auto runtime = static_cast<Runtime>(p_runtime);
        OS *os = OS::get_singleton();
        Host &host = hosts[runtime];
        const Dictionary process = os->execute_with_pipe(p_host, p_arguments);
        const Ref<FileAccess> pipe = process.get("stdio", Variant());
        const Ref<FileAccess> errors = process.get("stderr", Variant());
        const int64_t pid = process.get("pid", -1);

        // Reads the host's replies to one request up to its last: the lines it logs on the way,
        // then DONE, the FRAME of a render, which is published, FAILED, kept in `render_failed`,
        // or CHECKED, kept in `compiles`.
        // False when the host has ended - or speaks out of turn, and is ended so that its stderr
        // can be read to the end
        bool compiles = false;
        bool render_failed = false;
        const auto receive = [&]() -> bool {
            while (true) {
                const PackedByteArray size_field = read_exact(pipe, sizeof(uint32_t));
                if (size_field.size() < static_cast<int64_t>(sizeof(uint32_t))) {
                    return false;
                }
                uint32_t size = 0;
                memcpy(&size, size_field.ptr(), sizeof(size));
                const PackedByteArray message = read_exact(pipe, size);
                if (size == 0 || message.size() < size) {
                    return false;
                }
                ReplyReader reader{message};
                switch (static_cast<Reply>(reader.get<uint8_t>())) {
                    case Reply::LOG:
                        UtilityFunctions::print("[PythonScreen] ", reader.get_string().strip_edges());
                        break;
                    case Reply::DONE:
                        return true;
                    case Reply::CHECKED:
                        compiles = reader.get<uint8_t>() != 0;
                        if (!reader.valid) {
                            os->kill(static_cast<int32_t>(pid));
                            return false;
                        }
                        return true;
                    case Reply::FAILED:
                        reader.get<uint64_t>();
                        if (!reader.valid) {
                            os->kill(static_cast<int32_t>(pid));
                            return false;
                        }
                        render_failed = true;
                        return true;
                    case Reply::FRAME: {
                        const RID screen =
                                UtilityFunctions::rid_from_int64(static_cast<int64_t>(reader.get<uint64_t>()));
                        const auto width = reader.get<int32_t>();
                        const auto height = reader.get<int32_t>();
                        const PackedByteArray pixels = reader.get_bytes();
                        PackedStringArray commands;
                        const auto count = reader.get<uint32_t>();
                        for (uint32_t i = 0; i < count && reader.valid; i++) {
                            commands.push_back(reader.get_string());
                        }
                        if (!reader.valid) {
                            os->kill(static_cast<int32_t>(pid));
                            return false;
                        }
                        if (!pixels.is_empty() || !commands.is_empty()) {
                            callable_mp(this, &PythonScreenServer::_publish)
                                    .call_deferred(screen, width, height, pixels, commands);
                        }
                        return true;
                    }
                    default:
                        os->kill(static_cast<int32_t>(pid));
                        return false;
                }
            }
        };

        // The host has ended: what it printed on its way out (a Python fatal error, the runtime
        // file it did not find) and its exit code go to the log, and the game hears of it once.
        // Its screens stay blank - a 3.x host's draw on 2.7 from their next render; the host is
        // not restarted.
        const auto fail = [&]() {
            PackedByteArray output;
            while (errors.is_valid()) {
                const PackedByteArray part = errors->get_buffer(ERRORS_CHUNK_SIZE);
                if (part.is_empty()) {
                    break;
                }
                output.append_array(part);
            }
            // -1 while the process is still on its way out; on Windows a crash is an NTSTATUS
            // (0xC0000005 an access violation) and abort() is 3
            const int64_t exit_code = pid >= 0 ? os->get_process_exit_code(static_cast<int32_t>(pid)) : -1;
            const String message =
                    pipe.is_null() ? "cannot start " + p_host
                                   : vformat("the %s host ended with exit code %d (0x%x): %s", RUNTIME_NAMES[runtime],
                                             exit_code, static_cast<uint32_t>(exit_code),
                                             output.get_string_from_utf8().strip_edges());
            UtilityFunctions::push_error("[PythonScreen] ", message, " - Python screens stay blank");
            {
                MutexLock lock(mutex);
                host.unavailable = true;
                host.requests.clear();
            }
            callable_mp(this, &PythonScreenServer::_announce_failure).call_deferred(message);
        };

        // the host's first reply says the interpreter has started in the game directory
        bool running = pipe.is_valid() && receive();
        if (!running) {
            fail();
        }
        while (true) {
            host.semaphore->wait();
            while (true) {
                Request request;
                bool checked = true;
                Runtime script_runtime = runtime;
                {
                    MutexLock lock(mutex);
                    if (stopping) {
                        return; // the pipe closes with it, and the host ends on its own
                    }
                    if (host.requests.is_empty()) {
                        break;
                    }
                    request = host.requests.front()->get();
                    host.requests.pop_front();
                    // a script reaches the 3.x host before it is checked
                    const Runtime *known = script_runtimes.getptr(request.script_path);
                    checked = request.freed || runtime == RUNTIME_PYTHON_2 || known != nullptr;
                    script_runtime = known != nullptr ? *known : script_runtime;
                }
                if (!running) {
                    continue; // no host; the failure has been reported once already
                }
                if (request.freed) {
                    RequestWriter message(python_host::Request::FREE);
                    message.put(static_cast<uint64_t>(request.screen.get_id()));
                    running = message.send(pipe) && receive();
                } else if (!checked) {
                    RequestWriter message(python_host::Request::CHECK);
                    message.put_string(request.script_path);
                    running = message.send(pipe) && receive();
                    if (running) {
                        {
                            MutexLock lock(mutex);
                            script_runtimes.insert(request.script_path, compiles ? RUNTIME_PYTHON_3 : RUNTIME_PYTHON_2);
                        }
                        if (compiles) {
                            // drawn on this host, as checked now
                            MutexLock lock(mutex);
                            host.requests.push_front(request);
                        } else {
                            UtilityFunctions::print(
                                    "[PythonScreen] ", request.script_path,
                                    " does not run on Python 3 - it runs on Python 2.7");
                            callable_mp(this, &PythonScreenServer::_render_after_check)
                                    .call_deferred(request.screen, request.state);
                        }
                    }
                } else if (script_runtime == RUNTIME_PYTHON_2 && runtime == RUNTIME_PYTHON_3) {
                    // checked by another screen of the same script since it was queued here
                    callable_mp(this, &PythonScreenServer::_render_after_check)
                            .call_deferred(request.screen, request.state);
                } else {
                    RequestWriter message(python_host::Request::RENDER);
                    message.put(static_cast<uint64_t>(request.screen.get_id()));
                    message.put_string(request.script_path);
                    message.put_state(request.state);
                    render_failed = false;
                    running = message.send(pipe) && receive();
                    if (running && render_failed) {
                        // a script of the game data written for 2.7 that passed the check, but not
                        // what it draws (#395): on 2.7 from now on, this frame included, and its
                        // instance here goes
                        {
                            MutexLock lock(mutex);
                            script_runtimes.insert(request.script_path, RUNTIME_PYTHON_2);
                        }
                        UtilityFunctions::print(
                                "[PythonScreen] ", request.script_path,
                                " fails on Python 3 while it draws - it runs on Python 2.7");
                        RequestWriter free(python_host::Request::FREE);
                        free.put(static_cast<uint64_t>(request.screen.get_id()));
                        running = free.send(pipe) && receive();
                        callable_mp(this, &PythonScreenServer::_render_after_check)
                                .call_deferred(request.screen, request.state);
                    }
                }
                if (!running) {
                    fail();
                }
            }
        }
    }

    void PythonScreenServer::project_settings_register() {
        ReloadedProjectSetting::add(PYTHON_HOME_SETTING, String(), Variant::STRING, PROPERTY_HINT_GLOBAL_DIR);
    }
} // namespace godot
