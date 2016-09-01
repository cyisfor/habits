local path = require 'pl.path'
return function()
    local there = debug.getinfo(2,'S').short_src
    there = path.dirname(there)
    return function (name)
        return path.join(there,name)
    end
end
