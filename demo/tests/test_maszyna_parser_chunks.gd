extends MaszynaGutTest

## MaszynaParser.parse_chunk() - SceneryInstancer parses the top-level .scn in chunks to report
## loading progress; the result must match a single parse().

const SOURCE:String = "node a 1 2 end\nnode b 3 4 end\nskip x\nnode c 5 6 end\n"


func _make_parser() -> MaszynaParser:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize(SOURCE.to_utf8_buffer())
    parser.register_handler("node", func(p:MaszynaParser) -> Array: return [p.get_tokens_until("end")])
    return parser


func test_chunks_match_whole_parse() -> void:
    var expected:Array = _make_parser().parse()
    assert_eq(expected.size(), 3)

    var parser:MaszynaParser = _make_parser()
    var chunked:Array = []
    var positions:Array[int] = []
    while not parser.eof_reached():
        chunked.append_array(parser.parse_chunk(4))
        positions.append(parser.get_position())
    assert_eq(chunked, expected)
    assert_gt(positions.size(), 1, "small chunks should take more than one step")
    assert_eq(positions[-1], parser.get_length())
    assert_eq(parser.get_length(), SOURCE.to_utf8_buffer().size())


## cParser::findQuotes() (parser.cpp:479): a quoted text is glued to its token without the quotes -
## spaces and "//" inside kept, a backslash taking the next character as it is
func test_a_quoted_text_is_glued_to_its_token_without_quotes() -> void:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize('parameters: tex="./dynamic/a b//c" next "say \\"hi\\"" end'.to_utf8_buffer())

    assert_eq(parser.get_tokens_until("end"), ["parameters:", "tex=./dynamic/a b//c", "next", 'say "hi"', "end"])
