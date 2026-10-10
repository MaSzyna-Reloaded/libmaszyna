#include "maszyna_parser.hpp"

namespace godot {
    namespace {
        /// Unicode code points of the bytes 0x80-0xFF in cp1250, the encoding of the original's data
        /// files (U+FFFD for undefined bytes) - the table of Windows1250.gd
        constexpr char32_t CP1250_HIGH[] = {
                0x20AC, 0xFFFD, 0x201A, 0xFFFD, 0x201E, 0x2026, 0x2020, 0x2021, 0xFFFD, 0x2030, 0x0160, 0x2039, 0x015A,
                0x0164, 0x017D, 0x0179, 0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0xFFFD, 0x2122,
                0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A, 0x00A0, 0x02C7, 0x02D8, 0x0141, 0x00A4, 0x0104, 0x00A6,
                0x00A7, 0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B, 0x00B0, 0x00B1, 0x02DB, 0x0142,
                0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C, 0x0154,
                0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7, 0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD,
                0x00CE, 0x010E, 0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7, 0x0158, 0x016E, 0x00DA,
                0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF, 0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
                0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F, 0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4,
                0x0151, 0x00F6, 0x00F7, 0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9,
        };
    } // namespace

    std::atomic<bool> MaszynaParser::cancelled{false};

    void MaszynaParser::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaParser", D_METHOD("set_cancelled", "cancelled"), &MaszynaParser::set_cancelled);
        ClassDB::bind_static_method("MaszynaParser", D_METHOD("is_cancelled"), &MaszynaParser::is_cancelled);
        ClassDB::bind_method(
                D_METHOD("initialize", "buffer", "default_stop"), &MaszynaParser::initialize, DEFVAL(Array()));
        ClassDB::bind_method(D_METHOD("get8"), &MaszynaParser::get8);
        ClassDB::bind_method(D_METHOD("get_line"), &MaszynaParser::get_line);
        ClassDB::bind_method(D_METHOD("eof_reached"), &MaszynaParser::eof_reached);
        ClassDB::bind_method(D_METHOD("register_handler", "token", "callback"), &MaszynaParser::register_handler);
        ClassDB::bind_method(D_METHOD("unregister_handler", "token"), &MaszynaParser::unregister_handler);
        ClassDB::bind_method(D_METHOD("as_bool", "token"), &MaszynaParser::as_bool);
        ClassDB::bind_method(D_METHOD("as_vector3", "tokens"), &MaszynaParser::as_vector3);
        ClassDB::bind_method(D_METHOD("get_tokens", "num", "stops"), &MaszynaParser::get_tokens, DEFVAL(Array()));
        ClassDB::bind_method(D_METHOD("next_token", "stops"), &MaszynaParser::next_token, DEFVAL(Array()));
        ClassDB::bind_method(D_METHOD("next_vector3", "stops"), &MaszynaParser::next_vector3, DEFVAL(Array()));
        ClassDB::bind_method(
                D_METHOD("get_tokens_until", "token", "stops"), &MaszynaParser::get_tokens_until, DEFVAL(Array()));
        ClassDB::bind_method(D_METHOD("parse"), &MaszynaParser::parse);
        ClassDB::bind_method(D_METHOD("parse_chunk", "bytes"), &MaszynaParser::parse_chunk);
        ClassDB::bind_method(D_METHOD("interrupt"), &MaszynaParser::interrupt);
        ClassDB::bind_method(D_METHOD("get_position"), &MaszynaParser::get_position);
        ClassDB::bind_method(D_METHOD("get_length"), &MaszynaParser::get_length);
        ClassDB::bind_method(D_METHOD("get_parsed_metadata"), &MaszynaParser::get_parsed_metadata);
        ClassDB::bind_method(D_METHOD("push_metadata"), &MaszynaParser::push_metadata);
        ClassDB::bind_method(D_METHOD("pop_metadata"), &MaszynaParser::pop_metadata);
        ClassDB::bind_method(D_METHOD("clear_metadata"), &MaszynaParser::clear_metadata);
        ClassDB::bind_method(D_METHOD("set_parameters"), &MaszynaParser::set_parameters);
    }

    MaszynaParser::MaszynaParser() {
        default_stop_chars = Array::make(" ", "\t", "\n", "\r", ";");
        _make_stop_table(default_stop_chars, default_stop_table);
        meta.push_back(Dictionary());
    }

    /// Single ASCII stop characters; others never matched a single byte in the tokenizer anyway
    void MaszynaParser::_make_stop_table(const Array &p_stops, bool (&p_r_table)[ASCII_END]) {
        for (bool &entry: p_r_table) {
            entry = false;
        }
        for (int i = 0; i < p_stops.size(); i++) {
            const String stop = p_stops[i];
            if (stop.length() == 1 && stop[0] < ASCII_END) {
                p_r_table[stop[0]] = true;
            }
        }
    }

    /// A byte past ASCII is a cp1250 letter, as the original's files are written - a signed char
    /// taken as it was lost it (U+FFFD), and with it the station of every W4 named in Polish;
    /// pure ASCII tokens are converted at once.
    String MaszynaParser::_to_token(const std::string &p_raw) {
        for (const char c: p_raw) {
            if (static_cast<uint8_t>(c) >= ASCII_END) {
                String token;
                for (const char raw_c: p_raw) {
                    const auto byte = static_cast<uint8_t>(raw_c);
                    token += byte < ASCII_END ? static_cast<char32_t>(byte) : CP1250_HIGH[byte - ASCII_END];
                }
                return token;
            }
        }
        return {p_raw.c_str()};
    }

    void MaszynaParser::set_parameters(const Dictionary &p_parameters) {
        parameters = p_parameters;
    }

    void MaszynaParser::initialize(const PackedByteArray &p_buffer, const Array &p_default_stop_chars) {
        this->buffer = p_buffer;
        data = buffer.ptr();
        length = static_cast<int>(p_buffer.size());
        cursor = 0;
        if (!p_default_stop_chars.is_empty()) {
            default_stop_chars = p_default_stop_chars;
            _make_stop_table(default_stop_chars, default_stop_table);
        }
    }

    int MaszynaParser::get8() {
        if (cursor < length) {
            // the raw buffer pointer keeps the per-byte hot loop off the PackedByteArray accessor call
            return data[cursor++]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
        return -1;
    }

    String MaszynaParser::get_line() {
        PackedByteArray subbuf;
        while (!eof_reached()) {
            const int c = get8();
            if (c == -1 || c == '\n' || c == '\r') {
                break;
            }
            subbuf.append(static_cast<uint8_t>(c));
        }
        return subbuf.get_string_from_utf8();
    }

    bool MaszynaParser::eof_reached() const {
        return cursor >= length;
    }

    void MaszynaParser::register_handler(const String &p_token, const Callable &p_callback) {
        handlers[p_token] = p_callback;
    }

    void MaszynaParser::unregister_handler(const String &p_token) {
        handlers.erase(p_token);
    }

    bool MaszynaParser::as_bool(const String &p_token) {
        const String lower = p_token.to_lower();
        return lower == "yes" || lower == "on" || lower == "1" || lower == "true" || lower == "vis";
    }

    Vector3 MaszynaParser::as_vector3(const Array &p_tokens) {
        if (p_tokens.size() < 3) {
            return Vector3();
        }
        return Vector3(p_tokens[0], p_tokens[1], p_tokens[2]);
    }

    /// Reads one token (empty at a comment or repeated stop characters), parameters substituted
    String MaszynaParser::_read_token(const bool (&p_stop_table)[ASCII_END]) {
        std::string raw;
        bool maybe_comment = false;
        bool maybe_endcomment = false;

        while (!eof_reached()) {
            int c_int = get8();
            if (c_int == -1) {
                break;
            }
            char c = static_cast<char>(c_int);
            bool skip = false;

            if (c == '/') {
                if (maybe_comment) {
                    maybe_comment = false;
                    // Line comment detected
                    while (!eof_reached()) {
                        int next_c = get8();
                        if (next_c == -1 || next_c == '\n' || next_c == '\r') {
                            break;
                        }
                    }
                    skip = true;
                } else {
                    maybe_comment = true;
                    continue;
                }
            } else if (c == '*') {
                if (maybe_comment) {
                    maybe_comment = false;
                    maybe_endcomment = false;
                    // Block comment detected
                    while (!eof_reached()) {
                        int next_c = get8();
                        if (next_c == -1) {
                            break;
                        }
                        char bc = static_cast<char>(next_c);
                        if (bc == '*') {
                            maybe_endcomment = true;
                        } else if (bc == '/' && maybe_endcomment) {
                            break;
                        } else {
                            maybe_endcomment = false;
                        }
                    }
                    break;
                }
            }

            if (!skip && maybe_comment) {
                maybe_comment = false;
                raw += '/';
            }

            if (skip || (static_cast<uint8_t>(c) < ASCII_END && p_stop_table[static_cast<uint8_t>(c)])) {
                break;
            }

            // a quoted text is glued to the token without its quotes - no stops and no comments
            // inside, a backslash takes the next character as it is (cParser::findQuotes(),
            // readQuotes(), parser.cpp:425-490)
            if (c == '"') {
                bool escaped = false;
                while (!eof_reached()) {
                    const int quoted = get8();
                    if (quoted == -1 || (!escaped && quoted == '"')) {
                        break;
                    }
                    if (!escaped && quoted == '\\') {
                        escaped = true;
                        continue;
                    }
                    escaped = false;
                    raw += static_cast<char>(quoted);
                }
                continue;
            }

            raw += c;
        }

        String token = _to_token(raw).strip_edges();
        // a parameter reference is always "(name)"
        if (!parameters.is_empty() && token.contains("(")) {
            Array keys = parameters.keys();
            for (const auto &key: keys) {
                String param = key;
                String value = parameters[param];
                token = token.replace("(" + param + ")", value);
            }
        }
        return token;
    }

    Array MaszynaParser::get_tokens(const int p_num, const Array &p_stops) {
        bool stop_table[ASCII_END] = {};
        if (!p_stops.is_empty()) {
            _make_stop_table(p_stops, stop_table);
        }
        const bool (&table)[ASCII_END] = p_stops.is_empty() ? default_stop_table : stop_table;

        Array tokens;
        while (tokens.size() < p_num && !eof_reached()) {
            if (String token = _read_token(table); !token.is_empty()) {
                tokens.append(token);
            }
        }
        return tokens;
    }

    String MaszynaParser::next_token(const Array &p_stops) {
        bool stop_table[ASCII_END] = {};
        if (!p_stops.is_empty()) {
            _make_stop_table(p_stops, stop_table);
        }
        const bool (&table)[ASCII_END] = p_stops.is_empty() ? default_stop_table : stop_table;

        while (!eof_reached()) {
            if (String token = _read_token(table); !token.is_empty()) {
                return token;
            }
        }
        return "";
    }

    Vector3 MaszynaParser::next_vector3(const Array &p_stops) {
        const Array tokens = get_tokens(3, p_stops);
        return as_vector3(tokens);
    }

    Array MaszynaParser::get_tokens_until(const String &p_token, const Array &p_stops) {
        Array tokens;
        while (!eof_reached()) {
            if (String upcoming_token = next_token(p_stops); !upcoming_token.is_empty()) {
                tokens.append(upcoming_token);
                if (upcoming_token == p_token) {
                    break;
                }
            }
        }

        return tokens;
    }


    void MaszynaParser::set_cancelled(const bool p_cancelled) {
        cancelled = p_cancelled;
    }

    bool MaszynaParser::is_cancelled() {
        return cancelled;
    }

    Array MaszynaParser::parse() {
        return parse_chunk(length);
    }

    // Stops at the first token boundary after p_bytes; a handler may read past it.
    Array MaszynaParser::parse_chunk(const int p_bytes) {
        Array result;
        const int end = cursor + p_bytes;
        interrupted = false;
        while (!eof_reached() && cursor < end && !interrupted && !cancelled) {
            if (String token = next_token(); handlers.has(token)) {
                if (Callable callback = handlers[token]; callback.is_valid()) {
                    Variant parsed_v = callback.call(this);
                    if (parsed_v.get_type() == Variant::ARRAY) {
                        Array parsed = parsed_v;
                        if (parsed.size() > 0) {
                            result.append_array(parsed);
                        }
                    }
                }
            }
        }

        return result;
    }

    void MaszynaParser::interrupt() {
        interrupted = true;
    }

    int MaszynaParser::get_position() const {
        return cursor;
    }

    int MaszynaParser::get_length() const {
        return length;
    }

    Dictionary MaszynaParser::get_parsed_metadata() {
        if (meta.is_empty()) {
            return Dictionary();
        }
        return meta[0];
    }

    void MaszynaParser::push_metadata() {
        meta.push_front(Dictionary());
    }

    Dictionary MaszynaParser::pop_metadata() {
        if (meta.is_empty()) {
            return Dictionary();
        }
        Dictionary top = meta[0];
        meta.pop_front();
        return top;
    }

    void MaszynaParser::clear_metadata() {
        meta.clear();
        meta.push_back(Dictionary());
    }
} // namespace godot
