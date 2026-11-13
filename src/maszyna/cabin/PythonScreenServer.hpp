#pragma once
#include "utils/RIDHash.hpp"
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
    /// Cab screens drawn by the original engine's Python scripts (`pyscreen:` of an MMD), written
    /// for Python 2 or 3.
    ///
    /// A script is a class named after its file, built with the directory it lives in; each
    /// update hands it a dictionary of the train state, `render()` returns an RGBA buffer and
    /// `getCommands()` the commands the screen sends back to the train (PyInt.cpp:23-224).
    ///
    /// CPython runs in processes of their own, maszyna-python-host (python_host/main.cpp): one host
    /// for a Python 3 runtime (3.14 or later) and one for a 2.7 runtime, side by side. A runtime is
    /// the game's own beside the host (python3.NN/, python2.7/ - make python-runtime-linux and
    /// python-runtime-windows), else the game directory's (`maszyna/python/home` for 2.7). A script
    /// runs on 3.x when it compiles there with the modules of the game directory it imports and
    /// loads (the 3.x host checks it before its first render), on 2.7 otherwise, and on 2.7 when no
    /// 3.x runtime is found. A script that fails on 3.x while it is made or draws - written for 2.7
    /// and the Pillow of its time - runs on 2.7 from then on. Each host
    /// has the base classes of its runtime first in its search path (`cabin/python/<runtime>/`, the
    /// original's `python/local/`, copied out to `user://python/`), not the game directory's. A host
    /// serves one game directory: a data reload ends both, and the next render finds the runtimes
    /// of the game directory set by then.
    ///
    /// Nothing a runtime does can end the game: when a host ends - a fatal Python error, a crash in
    /// an extension module, a runtime that is missing - what it printed and its exit code go to the
    /// log, `python_runtime_failed` is emitted once and its screens stay blank (a 3.x host's
    /// scripts then run on 2.7). One worker thread a host talks to it (PythonHostProtocol.hpp), a
    /// request at a time. Each screen has an instance of its script's class of its own, made on its
    /// first render and released by screen_free(); the original shares one a script for as long as
    /// it runs (python_taskqueue::fetch_renderer(), MASZYNA_ORIGINAL_QUIRKS.md).
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
            /// The base classes of the scripts, a directory a runtime ("3", "2.7")
            static constexpr const char *BASE_SCRIPTS_PATH = "res://addons/libmaszyna/cabin/python";
            /// Where the base classes are copied out for the hosts, which read only files on disk
            static constexpr const char *BASE_SCRIPTS_COPY_PATH = "user://python";

            /// Registers its project settings with their defaults and hints as the library starts
            /// (MaszynaPlugin::register_classes(), ReloadedProjectSetting)
            static void project_settings_register();

            static PythonScreenServer *get_instance() {
                return Object::cast_to<PythonScreenServer>(
                        Engine::get_singleton()->get_singleton("PythonScreenServer"));
            }

        private:
            /// CPython 2.7 prefix the screens run with (lib/libpython2.7.so.1.0 on Linux); empty is
            /// python2.7 in the game directory on Linux and the original's python64 on Windows. The
            /// 3.x runtime the scripts that compile there run on is found in the game directory itself
            static constexpr const char *PYTHON_HOME_SETTING = "maszyna/python/home";

            /// The runtime a host runs, the one a script is checked on first
            enum Runtime {
                RUNTIME_PYTHON_3,
                RUNTIME_PYTHON_2,
                RUNTIME_COUNT,
            };

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
                    /// The screen is freed: its script's instance goes, nothing is drawn
                    bool freed = false;
            };

            // main thread only
            RIDMap<Screen> screens;
            const bool disabled = OS::get_singleton()->get_cmdline_args().has(ARG_NO_PYTHON);

            struct Host {
                    /// Started by the first render (3.x) or the first script that needs it (2.7), ended
                    /// when the data is reloaded
                    Ref<Thread> worker;
                    Ref<Semaphore> semaphore;
                    // under the mutex
                    List<Request> requests;
                    /// The runtime is missing or its host has ended; no more requests are taken
                    bool unavailable = false;
            };

            // shared with the workers, under the mutex
            Mutex mutex;
            Host hosts[RUNTIME_COUNT];
            /// The runtime each script checked runs on, by its path
            HashMap<String, Runtime> script_runtimes;
            /// The workers end (_stop_hosts())
            bool stopping = false;

            void _start_host(Runtime p_runtime);
            void _stop_hosts();
            void _on_data_reload_requested();
            void _announce_failure(const String &p_message);
            /// A screen whose script did not compile on 3.x, or failed there while it drew,
            /// rendered again - on 2.7 now
            void _render_after_check(const RID &p_screen, const Dictionary &p_state);
            /// `p_runtime` a Runtime, bound as an int: a callable binds no unregistered enum
            void _worker_loop(int p_runtime, const String &p_host, const PackedStringArray &p_arguments);
            void _publish(
                    const RID &p_screen, int p_width, int p_height, const PackedByteArray &p_pixels,
                    const PackedStringArray &p_commands);

        protected:
            static void _bind_methods();

        public:
            PythonScreenServer();
            /// Joins the workers, whose pipes close with them - the hosts then end on their own. A worker
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
            /// The screen's texture goes, and in the host the instance of its script
            void screen_free(const RID &p_screen);
    };
} // namespace godot
