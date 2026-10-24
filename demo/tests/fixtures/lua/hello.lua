-- writes to the memory cell the test made, through the maszyna modules
local util = require("LIB.UTIL")
local output = reloaded.memory.find("lua_test_output")
reloaded.memory.write(output, util.greeting(), 1, 2)
