require('luarocks.loader')
local resource = require('resource')()
local safeopen = require('sanity.open')
local pq = require('psql')
local db = pq.connect "port=5433 dbname=habits host=/run"
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
       print('prepare',n,stmt)
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

M.set_enabled = prep({
    update = 'UPDATE habits SET enabled = $2 WHERE id = $1'
},function(p)
    return function(ident,enabled)
		print('enabeld',enabled,ident)
        return p.update:exec(ident,enabled)
    end
end)

M.pending = prep({
    find = [[WITH result AS (
    SELECT id,description,
EXTRACT(EPOCH FROM howOften) AS frequency,
EXTRACT(EPOCH FROM now() - performed) AS elapsed,
performed IS NOT NULL AS hasperformed,
howOften,
performed FROM habits WHERE enabled = TRUE)
    SELECT id,description,frequency,elapsed FROM result
    WHERE ( NOT hasperformed ) OR (howOften / 1.5 < now() - performed) ORDER BY elapsed / frequency
DESC NULLS LAST ]]
},function(p)
    return function()
        local res = p.find:exec()
		local f,s,index = res:rows()
		return function()
		   nindex,row = f(s,index)
		   if row == nil then return nil end
		   index = nindex
		   return row.id, row.description, row.frequency, row.elapsed
		end
    end
end)

M.close = function()
    db:finish()
    print('finished')
end

M.fields = prep({
    getclass = "SELECT oid FROM pg_catalog.pg_class WHERE relname = $1::text AND relkind = $2::\"char\"",
    get = [[
SELECT attname::text
FROM   pg_attribute
WHERE  attrelid = $1
AND    attnum > 0
AND    NOT attisdropped
ORDER  BY attnum]]
},function(p)
    return function(able)
       print('searching for public.('..able..')')
       local res = checkset(p.getclass:exec(able,'r'))
        print('uh',res,p.getclass)
        if #res == 0 then
           error("No table found by the relname "..able)
           return {}
        end
        local class = res[1].oid
        print('class',class)
        local res = checkset(p.get:exec(class))
        print('got res',able,tostring(#res))
        local ret = {}
        for index,row in res:rows() do
            ret[#ret+1] = row.attname
        end
        return ret
    end
end)

M.set = function(sets)
   local id = sets.id
   sets.id = nil
   local args = {}
   if id then
      id = tonumber(id)
      args[1] = id
      -- don't add to fields though, because not checking AND id = $...
   end
   local query = nil
   local fields = {}
   for n,v in pairs(sets) do
      fields[#fields+1] = n
      args[#args+1] = v
   end

   if id == nil then
      query = 'INSERT INTO habits ('
      for i,n in ipairs(fields) do
         if i > 1 then
            query = query .. ', '
         end
         query = query .. n
      end
      query = query .. ') VALUES ('
      for i,n in ipairs(fields) do
         if i > 1 then
            query = query .. ', '
         end
         query = query .. '$' .. i
      end
      query = query .. ') RETURNING id'
   else
      for i,n in ipairs(fields) do
         -- be sure to use i+1 since 1 = id
         if query then
            query = query .. ' AND '..n..' = $'..(i+1)
         else
            query = n..' = $'..(i+1)
         end
      end

      query = 'UPDATE habits SET '..query..' WHERE id = $1'
   end
   for i,v in ipairs(args) do
      print(i,v)
   end
   print(#args)
   print('meep')
   return prep({set = query},function(p)
         return checkset(p.set:exec(unpack(args)))
   end)
end

M.find = prep({
    find = 'SELECT *,howoften::text as howoften FROM habits WHERE description LIKE $1'
},function(p)
    return function(search)
        local res = checkset(p.find:exec(search))
        print('search for',search,'got',#res)
        return res
    end
end)

return M
