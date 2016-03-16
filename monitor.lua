local gi = require('lgi')
local gtk = gi.require('Gtk')
local glib = gi.require('GLib')  
local db = require('db')

local alerter = nil

error("Will this even be useful? Just check which ocacsiona")

function alertLater() 
    if alerter == nil then
        alerter = glib.timeout_add_seconds(glib.PRIORITY_DEFAULT,db.waitTime(),alert)
    end
end

function alert() 


gtk.main()

