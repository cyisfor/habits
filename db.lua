require('luarocks.loader')
package.cpath = 'luapsql/?.so;'..package.cpath
local pq = require('psql')
local db = pq.connect "port=5433 dbname=habits"

local function checkset(res)
    local stat = res:status()
    assert(stat == 'PGRES_COMMAND_OK' or stat == 'PGRES_TUPLES_OK',db:error())
    return res
end


local M = {}

local print = function(...)
    local first = true
    for i,v in ipairs({...}) do
        if first then
            first = false
        else
            io.write(' ')
        end
        io.write(tostring(v))
    end
    io.write("\n");
end

function M.transaction(handler)
    checkset(db:exec('BEGIN'))
    ok,err = pcall(handler)
    if ok then
        checkset(db:exec('COMMIT'))
    else
        checkset(db:exec('ROLLBACK'))
    end
    assert(ok,err)
end

M.transaction(function()
    local inp = nil
    ok,err = pcall(function()
        inp = io.open("base.sql")
        db:execmany(inp:read('*a'))
    end)
    if inp ~= nil then
        inp:close()
    end
    assert(ok,err)
end)

prepctr = 0
local function prep(stmts,handler)
    local preps = {}
    for n,stmt in pairs(stmts) do
        preps[n] = assert(db:prepare(stmt,'prep'..prepctr),db:error())
        prepctr = prepctr + 1
    end
    return handler(preps)
end

M.perform = prep({
    action = 'SELECT perform($1::int)',
},function(p)
    return function(ident)
        checkset(p.action:exec(ident))
    end
end)

local function makeHabit(res,tuple,nextone)
    habit = {}
    for n,v in pairs(res:fields()) do
        habit[n] = tuple[n]
    end
    habit.nextone = nextone
    function habit.perform()
        M.transaction(function()
            M.perform(habit.id)
        end)
    end
    return habit
end

M.waitTime = prep({
    -- how long until the next action
    -- how often - time since last
    find = "SELECT EXTRACT(EPOCH FROM howOften - (now() - performed)) AS whenn FROM habits WHERE howOften < now() - performed ORDER BY whenn DESC"
},function(p)
    return function()
        local res = p.find:exec()
        if #res == 0 then
            return 0
        end
        return res[1].whenn
    end
end)


M.pending = prep({
    find = "SELECT id,description,EXTRACT(EPOCH FROM howOften),EXTRACT(EPOCH FROM now() - performed) AS elapsed,performed IS NOT NULL AS hasperformed FROM habits WHERE performed IS NULL OR (howOften < now() - performed) ORDER BY performed"
},function(p)
    return function()
        local res = p.find:exec()
        local habits = {}
        for index,habit in res:rows() do
            habits[#habits+1] = makeHabit(res,habit,#habits+2)
        end 
        return function(nothing,last)
            if last then
                return habits[last.nextone]
            else
                return habits[1]
            end
        end
    end
end)

M.close = function()
    db:finish()
    print('finished')
end
    
return M
