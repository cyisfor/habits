local db = require('db')

local gi = require('lgi')
local gtk = gi.require('Gtk')
local glib = gi.require('GLib')

w = gtk.Window{
    on_destroy=gtk.main_quit,
    title="Edit Habits"
}

local defaults = {
   importance= '1',
   enabled= 'TRUE',
}

local nope = {
   performed= 1
}

function buildGrid(table)
    local grid = gtk.Grid()
    local entries = {}
    local idlabel = gtk.Label{label='???'}
    grid:attach(gtk.Label{label="id"},0,0,1,1)
    grid:attach(idlabel,1,0,1,1)
    entries['id'] = idlabel
    
    for i,name in ipairs(db.fields(table)) do
       print('yay',name)
       if name ~= 'id' and nope[name] == nil then
            grid:insert_row(i)
            grid:attach(gtk.Label{label=name},0,i,1,1)
            local entry = gtk.Entry()
            grid:attach(entry,1,i,1,1)
            entries[name] = entry
            if defaults[name] then
               entry:set_text(defaults[name])
               print('default',defaults[name])
            end
        end
    end
    print('yay',grid)
    return grid, entries
end

local vbox = gtk.VBox()
w:add(vbox)
local hbox = gtk.HBox()
vbox:pack_start(hbox,true,false,1)

local grid = nil
local entries = nil

local which = gtk.Entry{
    text = 'habits',
}
function remakeGrid()
    local table = which:get_text()
    local newgrid
    newgrid, entries = buildGrid(table)
	assert(entries ~= nil)
    if grid then grid:destroy() end
    grid = newgrid
    vbox:pack_start(grid,true,true,1)
    grid:show_all()
end

hbox:pack_start(which,true,true,1)
hbox:pack_start(gtk.Button{
    label = "Switch",
    on_clicked = remakeGrid
},true,false,1)

local finder = gtk.Entry()

function find()
    local search = finder:get_text()
    search = '%'..search..'%'
    local res = db.find(search)
    if #res == 0 then return end
    res = res[1]

    for name,entry in pairs(entries) do
        local value = res[name]
        if value then
            entry:set_text(tostring(value))
        end
    end
end

hbox:pack_start(finder,true,true,1)
hbox:pack_start(gtk.Button{
    label = "Find",
    on_clicked = find
},true,false,1)

hbox:pack_start(gtk.Button{
    label = "Set",
    on_clicked = function()
        local sets = {}
        for n,e in pairs(entries) do
           local v = e:get_text()
           if v ~= '' then 
              sets[n] = v
           end
        end
        
        db.set(sets)
    end
},true,false,1)

w:show_all()
gtk.main()
