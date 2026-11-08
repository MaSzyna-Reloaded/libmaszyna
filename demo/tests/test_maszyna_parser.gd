extends MaszynaGutTest

## MaszynaParser - the original's parser reads the keywords lower-cased
## (simulationstateserializer.cpp:125-128); the tokens themselves keep their case

var _read:Array[String] = []


func _read_name(p_parser:MaszynaParser) -> Array:
    _read.append(p_parser.next_token())
    return []


func test_a_keyword_with_capitals_calls_its_handler() -> void:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize("Node Tor_A node tor_b NODE Tor_C".to_utf8_buffer(), [])
    parser.register_handler("node", _read_name)
    parser.parse()

    assert_eq(_read, ["Tor_A", "tor_b", "Tor_C"], "every spelling is the keyword, the token keeps its case")
