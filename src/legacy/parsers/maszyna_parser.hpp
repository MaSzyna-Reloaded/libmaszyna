#pragma once

#include <atomic>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <string>
#include <vector>

namespace godot {
    class MaszynaParser : public RefCounted {
            GDCLASS(MaszynaParser, RefCounted);

        private:
            PackedByteArray buffer;
            const uint8_t *data = nullptr; // buffer bytes, read directly in the tokenizer hot loop
            Dictionary handlers;
            int cursor = 0;
            int length = 0;
            bool interrupted = false;
            /* Set while everything is being torn down. A parse runs on a loading-queue worker and
             * the teardown joins that worker, so it has to be able to stop between tokens - a big
             * .scn otherwise held the window open until the whole file was through, and on
             * Windows there is no shell to interrupt it from (see `FINDINGS.md`, 2026-09-24). */
            static std::atomic<bool> cancelled;
            /* The first byte past ASCII; the stop tables cover ASCII only (the original's
             * parser.cpp:27 takes all 256 bytes) */
            static constexpr uint8_t ASCII_END = 0x80;
            TypedArray<Dictionary> meta;
            Array default_stop_chars;
            bool default_stop_table[ASCII_END] = {};
            Dictionary parameters;
            static void _make_stop_table(const Array &p_stops, bool (&p_r_table)[ASCII_END]);
            static String _to_token(const std::string &p_raw);
            String _read_token(const bool (&p_stop_table)[ASCII_END]);

        protected:
            static void _bind_methods();

        public:
            /// Stops every parse in flight at its next token; cleared once the workers are joined.
            static void set_cancelled(bool p_cancelled);
            /// So the GDScript half of a loading task can stop on the same signal.
            static bool is_cancelled();
            MaszynaParser();
            void initialize(const PackedByteArray &p_buffer, const Array &p_default_stop_chars);
            // void _create_instance(const PackedByteArray &buffer);
            int get8();
            String get_line();
            bool eof_reached() const;
            void register_handler(const String &p_token, const Callable &p_callback);
            void unregister_handler(const String &p_token);
            void set_parameters(const Dictionary &p_parameters);
            bool as_bool(const String &p_token);
            Vector3 as_vector3(const Array &p_tokens);
            Array get_tokens(int p_num, const Array &p_stops = Array());
            String next_token(const Array &p_stops = Array());
            Vector3 next_vector3(const Array &p_stops = Array());
            Array get_tokens_until(const String &p_token, const Array &p_stops = Array());
            Array parse();
            Array parse_chunk(int p_bytes);
            void interrupt();
            int get_position() const;
            int get_length() const;
            Dictionary get_parsed_metadata();
            void push_metadata();
            Dictionary pop_metadata();
            void clear_metadata();
    };
} // namespace godot
