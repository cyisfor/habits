local ffi = require("ffi")
ffi.cdef[[
int sleep(int);
]]
local ok,sleep = pcall(function() return ffi.C.sleep end)
if not ok then
    sleep = function(n) 
        os.execute('sleep '..n)
    end
end
return sleep
