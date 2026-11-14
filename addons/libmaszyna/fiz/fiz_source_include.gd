@tool
extends RefCounted
class_name FizSourceInclude

## An `include <file> [params...] end` directive the parse followed: where it is written and what
## it passed - its params are the (p1), (p2), ... of the file it brought in. The directive may be
## spread over several lines.

## The file the directive is written in, absolute
var path: String = ""
## Its bytes in that file: from the line it starts on up to the start of the line after its `end`
var begin: int = 0
var end: int = 0
## The file it brought in, absolute
var included_path: String = ""
## The params it passed, in order: (p1) first
var parameters: Array[String] = []
## The directive that brought in the file this one is written in, null in the file read first
var include: FizSourceInclude = null
