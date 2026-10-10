-- writes to the memory cell the test made, through the maszyna modules
local util = require("LIB.UTIL")
local output = maszyna.memory.find("lua_test_output")
maszyna.memory.write(output, util.greeting(), 1, 2)
