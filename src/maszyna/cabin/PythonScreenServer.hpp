#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/semaphore.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {
    /// Cab screens drawn by the original engine's Python 2 scripts (`pyscreen:` of an MMD).
    ///
    /// A script is a class named after its file, built with the directory it lives in; each
    /// update hands it a dictionary of the train state, `render()` returns an RGBA buffer and
    /// `getCommands()` the commands the screen sends back to the train (PyInt.cpp:23-224).
    ///
    /// CPython 2.7 runs in a process of its own, maszyna-python-host (python_host/main.cpp), which
    /// loads the game directory's runtime (`maszyna/python/home`) the way the original does. Nothing
    /// the runtime does can end the game: when the host ends - a fatal Python error, a crash in an
    /// extension module, a runtime that is missing - what it printed and its exit code go to the
    /// log, `python_runtime_failed` is emitted once and the screens stay blank. One worker thread
    /// talks to the host (PythonHostProtocol.hpp), a request at a time. Screens of the same script
    /// share one instance of its class, as in the original (python_taskqueue::fetch_renderer()).
    class PythonScreenServer : public Object {
            GDCLASS(PythonScreenServer, Object)

        public:
            /// Every script is asked for RGBA (manul_set_format("RGBA"), PyInt.cpp:468)
            static constexpr int BYTES_PER_PIXEL = 4;
            /// Samples per axis the average colour of a drawn frame is taken from
            static constexpr int AVERAGE_COLOR_SAMPLES = 16;
            /// A frame was drawn onto the screen's texture (screen: RID)
            static const char *screen_rendered_signal;
            /// The Python host has ended and the screens stay blank (message: why, for the player)
            static const char *python_runtime_failed_signal;
            /// Command-line switch: no Python runtime is loaded and the screens stay blank
            static constexpr const char *ARG_NO_PYTHON = "--no-python";

            static PythonScreenServer *get_instance() {
                return Object::cast_to<PythonScreenServer>(
                        Engine::get_singleton()->get_singleton("PythonScreenServer"));
            }

        private:
            struct Screen {
                    String script_path;
                    Ref<ImageTexture> texture;
                    Callable commands_received;
                    /// Of the last drawn frame, what the screen throws around it
                    Color average_color;
            };

            struct Request {
                    RID screen;
                    String script_path;
                    Dictionary state;
            };

            // main thread only
            HashMap<RID, Screen> screens;
            const bool disabled = OS::get_singleton()->get_cmdline_args().has(ARG_NO_PYTHON);

            // shared with the worker, under the mutex
            Mutex mutex;
            Ref<Semaphore> semaphore;
            Ref<Thread> worker;
            List<Request> requests;
            /// The game directory the worker moves the interpreter into, empty when it stays
            String entering_game_dir;
            bool exiting = false;
            /// The host has ended; no more requests are taken
            bool host_failed = false;

            void _on_data_reload_requested();
            void _announce_failure(const String &p_message);
            void _worker_loop(const String &p_host, const PackedStringArray &p_arguments);
            void _publish(
                    const RID &p_screen, int p_width, int p_height, const PackedByteArray &p_pixels,
                    const PackedStringArray &p_commands);

        protected:
            static void _bind_methods();

        public:
            PythonScreenServer();
            /// Joins the worker, whose pipe closes with it - the host then ends on its own. The worker
            /// never calls into a script of the project, so this is late enough (unlike the scenery
            /// workers in FINDINGS.md, 2026-09-24)
            ~PythonScreenServer() override;

            /// `p_script_path` is the script's absolute path without `.py`; `p_commands_received`
            /// is called with the commands the script returns (PackedStringArray), on the main thread
            RID screen_create(const String &p_script_path, const Callable &p_commands_received);
            /// The average colour of the last frame the script drew, black before the first
            Color screen_get_average_color(const RID &p_screen) const;
            /// The screen's texture - a blank one until the script has drawn the first frame
            Ref<Texture2D> screen_get_texture(const RID &p_screen) const;
            /// Queues a render with this state; a render still waiting for the same screen is
            /// replaced, not repeated (python_taskqueue::insert())
            void screen_request_render(const RID &p_screen, const Dictionary &p_state);
            void screen_free(const RID &p_screen);
    };
} // namespace godot
