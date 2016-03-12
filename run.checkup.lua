#!/usr/bin/env luajit 

require('luarocks.loader')
here = debug.getinfo(1,"S").source
assert(here:sub(1,1)=='@')
here = here:sub(2)
local hsals = here:reverse():find('/')
if hsals == nil then
   here = "."
else
   local slash = #here - here:reverse():find('/')+1
   here = here:sub(0,slash-1)
end
package.path = package.path .. ';'..here..'/?.lua'
package.cpath = package.cpath .. ';'..here..'/?.so'
print(here)
require('checkup')
