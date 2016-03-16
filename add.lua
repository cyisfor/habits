pcall(require,'luarocks.loader')
pcall(require,'fancyrequire.relative')

local gi = require('lgi')
local gtk = gi.require('Gtk')

local window = gtk.Window {
    title = 'Add Habit',
    default_width = 400,
    default_height = 400,
    on_destroy = function()
        gtk.main_quit()
        db.close()
    end
}
local vbox = gtk.VBox()
window:add(vbox)

function entry(name,default)
   local box = gtk.HBox()
   vbox:pack_start(hbox,true,true,1)
   local lbl = gtk.Label{label=name}
   local ent = gtk.Entry()
   if default then
      ent.set_text(default)
   end
   box.pack_start(lbl,true,true,1)
   box.pack_start(ent,true,true,1)
   return ent
end
local fields = {
   importance = entry('Importance',1),
   description = entry('Description'),
   howoften = entry('How Often','1 hour'),
   category = entry('Category')
}

local db = require('db')

