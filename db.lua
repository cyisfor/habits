require('luarocks.loader')
local resource = require('resource')()
local safeopen = require('sanity.open')
local pq = require('psql')
local db = pq.connect "port=5433 dbname=habits"
local l = require 'lpeg'

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

local pats = {
    transaction =l.P{'BEGIN' * (l.V(1) + (1 - l.P("END")))^0 * "END"},
    longstring = l.P"$$" * (1 - l.P"$$")^0 * l.P"$$",
    basic = (1 - l.P';')
}
pats.any = pats.transaction + pats.longstring + pats.basic

pats.statement = l.C(pats.any^1)

statements = l.Ct(pats.statement * (l.P';' * pats.statement)^0) * l.P';'^0

M.transaction(function()
    local inp = nil
    ok,err = pcall(function()
        safeopen(resource("base.sql"),function(inp)
            for i,statement in ipairs(statements:match(inp:read('*a'))) do
                statement = statement:gsub("^%s+","")
                statement = statement:gsub("%s+$","")
                if #statement > 0 then
                    db:exec(statement)
                end
            end
        end)
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
    local habit = {}
    for n,v in pairs(res:fields()) do
        habit[n] = tuple[n]
    end
    habit.nextone = nextone
    habit.perform = function()
        M.perform(habit.id)
        habit.hasperformed = true
        habit.dirty = true
    end
    return habit
end

--[[
M.waitTime = prep({
    -- how long until the next action
    -- how often - time since last
    find = "SELECT EXTRACT(EPOCH FROM howOften - (now() - performed)) AS whenn FROM habits WHERE whenn > 0 ORDER BY whenn DESC LIMIT 1"
},function(p)
    return function()
        local res = p.find:exec()
        if #res == 0 then
            return 0
        end
        return res[1].whenn
    end
end)
]]--


M.pending = prep({
    find = [[WITH result AS (
    SELECT id,description,EXTRACT(EPOCH FROM howOften) AS frequency,EXTRACT(EPOCH FROM now() - performed) AS elapsed,performed IS NOT NULL AS hasperformed,performed FROM habits) 
    SELECT id,description,frequency,elapsed,hasperformed FROM results
    WHERE ( NOT hasperformed ) OR (frequency / 2 < now() - performed) ORDER BY elapsed / frequency
]]
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

M.fields = prep({
    getclass = "SELECT oid FROM pg_class WHERE relname = $1::name AND relkind = $2",
    get = [[
SELECT attname
FROM   pg_attribute
WHERE  attrelid = $1
AND    attnum > 0
AND    NOT attisdropped
ORDER  BY attnum]]
},function(p)
    return function(table)
        print('searching for public.'..table)
        local res = p.getclass:exec(table,'r')        
        if #res == 0 then return {} end
        local class = res[1].oid
        print('class',class)
        local res = checkset(p.get:exec(class))
        print('got res',table,tostring(#res))
        local ret = {}
        for index,row in res:rows() do
            ret[#ret+1] = row.attname
        end
        return ret
    end
end)

M.set = function(sets)
    local id = nil
    local query = nil
    local args = {}
    for n,v in pairs(sets) do
        if n == 'id' then
            print('idee')
            args[#args+1] = tonumber(v)
            id = #args
        else
            args[#args+1] = v
            if query then
                query = query .. ' AND '..n..'='..tostring(#args)
            else
                query = n..'='..tostring(#args)
            end
        end
    end
    query = 'UPDATE habits SET '..query..' WHERE id = $'..tostring(id)
    return prep({set = query},function(p)
        return p:exec(unpack(args))
    end)
end

M.find = prep({
    find = 'SELECT * FROM habits WHERE description LIKE $1'
},function(p)
    return function(search)
        local res = checkset(p.find:exec(search))
        print('search for',search,'got',#res)
        return res
    end
end)

return M
