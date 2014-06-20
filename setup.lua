local db = require('db')

local gi = require('lgi')
local gtk = gi.require('Gtk')
local glib = gi.require('GLib')

w = gtk.Window{
    on_destroy=gtk.main_quit,
    title="Edit Habits"
}

function buildGrid(table)
    local grid = gtk.Grid()
    local entries = {}
    for i,name in ipairs(db.fields(table)) do
        grid:insert_row(i)
        grid:attach(gtk.Label{label=name},0,i,1,1)
        local entry = gtk.Entry()
        grid:attach(entry,1,i,1,1)
        entries[name] = entry
    end
    return grid, entries
end

local vbox = gtk.VBox()
w:add(vbox)
local hbox = gtk.HBox()
vbox:pack_start(hbox,true,false,1)

local grid = nil
local entries = nil

local which = gtk.Entry()
function remakeGrid()
    local table = which:get_text()
    local newgrid = buildGrid(table)
    if grid then grid:destroy() end
    grid = newgrid
    vbox:pack_start(grid,true,true,1)
    grid.show_all()
end

function fillGrid()
    local query = ''
    local args = {}
    for name,entry in ipairs(entries) do
        local t = entry.get_text()
        if #t > 0 then
            args[#args+1] = t
            if query then
                query = query .. ' AND '
            end
            query = query .. name..'=$'..tostring(#args)
        end
    end
    print(query)
    local res = db:exec...
end

hbox:pack_start(which,true,true,1)
hbox:pack_start(gtk.Button{
    label = "Switch",
    on_clicked = remakeGrid
},true,false,1)

w:show_all()
gtk.main()
