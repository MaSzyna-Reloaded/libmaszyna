#include "PythonScreenServer.hpp"
#include "game_data/GameDataServer.hpp"
#include "legacy/cabin/python_host/PythonHostProtocol.hpp"
#include "utils/UserSettings.hpp"
#include <cstdint>
#include <cstring>
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
        semaphore.instantiate();
        if (disabled) {
            UtilityFunctions::print("[PythonScreen] disabled by ", ARG_NO_PYTHON, " - the screens stay blank");
        }
        if (GameDataServer *game_data = GameDataServer::get_instance(); game_data != nullptr) {
            game_data->connect(
                    GameDataServer::data_reload_requested_signal,
                    callable_mp(this, &PythonScreenServer::_on_data_reload_requested));
        }
    }

    /// The scripts are the game directory's: the host moves the interpreter into the new one.
    /// The interpreter itself stays - CPython 2.7 with its extension modules (PIL) is not started
    /// twice in one process - and so does the runtime it was loaded from.
    void PythonScreenServer::_on_data_reload_requested() {
        const UserSettings *user_settings = UserSettings::get_instance();
        ERR_FAIL_NULL(user_settings);
        if (worker.is_null()) {
            return; // the host starts in whatever game directory is set when a screen needs it
        }
        {
            MutexLock lock(mutex);
            entering_game_dir = user_settings->get_maszyna_game_dir();
        }
        semaphore->post();
    }

    PythonScreenServer::~PythonScreenServer() {
        if (worker.is_null()) {
            return;
        }
        {
            MutexLock lock(mutex);
            exiting = true;
        }
        semaphore->post();
        worker->wait_to_finish();
    }

    RID PythonScreenServer::screen_create(const String &p_script_path, const Callable &p_commands_received) {
        if (worker.is_null() && !disabled) {
            const UserSettings *user_settings = UserSettings::get_instance();
            ERR_FAIL_NULL_V(user_settings, RID());
            OS *os = OS::get_singleton();
            const String game_dir = user_settings->get_maszyna_game_dir();
            String home = ProjectSettings::get_singleton()->get_setting("maszyna/python/home", "");
            // the home is relative to the game directory, which the host starts in, as the original
            // gives it (PyInt.cpp:233); the library is a path, which Windows does not search for
#ifdef _WIN32
            // the 64-bit Windows runtime ships in the game directory
            home = home.is_empty() ? String("python64") : home;
            const String library = game_dir.path_join("python27.dll");
#else
            // the original's linuxpython64 (PyInt.cpp:238) is only a virtualenv over the system's
            // libpython, without PIL - the wrapper's own runtime has a directory of its own
            home = home.is_empty() ? String("python2.7") : home;
            const String library = home.path_join("lib/libpython2.7.so.1.0");
#endif
            // an exported game has the host next to its executable (the [dependencies] of
            // libmaszyna.gdextension), the project next to this library
            String library_path;
            gdextension_interface::get_library_path(gdextension_interface::library, library_path._native_ptr());
            const String host_dir =
                    os->has_feature("template")
                            ? os->get_executable_path().get_base_dir()
                            : ProjectSettings::get_singleton()->globalize_path(library_path.get_base_dir());
            const String host = host_dir.path_join(MASZYNA_PYTHON_HOST_FILE);
            UtilityFunctions::print(
                    "[PythonScreen] starting the host: ", host, ", game directory ", game_dir, ", home ", home,
                    ", library ", library);
            worker.instantiate();
            worker->start(callable_mp(this, &PythonScreenServer::_worker_loop)
                                  .bind(host, PackedStringArray({game_dir, home, library})));
        }
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

    void PythonScreenServer::screen_request_render(const RID &p_screen, const Dictionary &p_state) {
        const Screen *screen = screens.getptr(p_screen);
        ERR_FAIL_NULL(screen);
        if (disabled) {
            return; // no interpreter to draw it
        }
        {
            MutexLock lock(mutex);
            if (host_failed) {
                return; // reported once, when the host ended
            }
            bool replaced = false;
            for (Request &request: requests) {
                if (request.screen == p_screen) {
                    request.state = p_state.duplicate();
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                requests.push_back(Request{p_screen, screen->script_path, p_state.duplicate()});
            }
        }
        semaphore->post();
    }

    void PythonScreenServer::screen_free(const RID &p_screen) {
        screens.erase(p_screen);
        MutexLock lock(mutex);
        for (List<Request>::Element *element = requests.front(); element != nullptr; element = element->next()) {
            if (element->get().screen == p_screen) {
                element->erase();
                break;
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

    void PythonScreenServer::_worker_loop(const String &p_host, const PackedStringArray &p_arguments) {
        OS *os = OS::get_singleton();
        const Dictionary process = os->execute_with_pipe(p_host, p_arguments);
        const Ref<FileAccess> pipe = process.get("stdio", Variant());
        const Ref<FileAccess> errors = process.get("stderr", Variant());
        const int64_t pid = process.get("pid", -1);

        // Reads the host's replies to one request up to its last: the lines it logs on the way,
        // then DONE or the FRAME of a render, which is published. False when the host has ended -
        // or speaks out of turn, and is ended so that its stderr can be read to the end
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
        // The screens stay blank; the host is not restarted.
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
            const String message = pipe.is_null() ? "cannot start " + p_host
                                                  : vformat("the Python host ended with exit code %d (0x%x): %s",
                                                            exit_code, static_cast<uint32_t>(exit_code),
                                                            output.get_string_from_utf8().strip_edges());
            UtilityFunctions::push_error("[PythonScreen] ", message, " - Python screens stay blank");
            {
                MutexLock lock(mutex);
                host_failed = true;
                requests.clear();
            }
            callable_mp(this, &PythonScreenServer::_announce_failure).call_deferred(message);
        };

        // the host's first reply says the interpreter has started in the game directory
        bool running = pipe.is_valid() && receive();
        if (!running) {
            fail();
        }
        while (true) {
            semaphore->wait();
            while (true) {
                Request request;
                String entering;
                {
                    MutexLock lock(mutex);
                    if (exiting) {
                        return; // the pipe closes with it, and the host ends on its own
                    }
                    entering = entering_game_dir;
                    entering_game_dir = String();
                    if (entering.is_empty()) {
                        if (requests.is_empty()) {
                            break;
                        }
                        request = requests.front()->get();
                        requests.pop_front();
                    }
                }
                if (!running) {
                    continue; // no host; the failure has been reported once already
                }
                if (!entering.is_empty()) {
                    RequestWriter message(python_host::Request::ENTER_GAME_DIR);
                    message.put_string(entering);
                    running = message.send(pipe) && receive();
                } else {
                    RequestWriter message(python_host::Request::RENDER);
                    message.put(static_cast<uint64_t>(request.screen.get_id()));
                    message.put_string(request.script_path);
                    message.put_state(request.state);
                    running = message.send(pipe) && receive();
                }
                if (!running) {
                    fail();
                }
            }
        }
    }
} // namespace godot
