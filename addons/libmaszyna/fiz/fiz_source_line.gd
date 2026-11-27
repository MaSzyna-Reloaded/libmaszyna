@tool
extends RefCounted
class_name FizSourceLine

## Where a line the parse read stands in the files: a section's header line (`Wheels: D=1.25 ...`)
## or a row of its table (a MotorParamTable: line, a brake position of Cntrl.). The bytes are the
## file's own, before an include's (pN) are substituted - the line as it is written there.

## The file, absolute
var path: String = ""
## The byte the line starts at in the file; it runs up to its \r or \n
var begin: int = 0
## The section the line belongs to (FizVehicleBuilder.create_sections()'s prefix: "Wheels:"), or
## the table the row is of ("BPT" for Cntrl.'s brake positions)
var section: String = ""
## A row of the section's table, not its header
var row: bool = false
## The directive that brought the file in, null in the file read first
var include: FizSourceInclude = null
