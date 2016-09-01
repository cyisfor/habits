pcall(require,'luarocks.loader')
pcall(require,'fancyrequire.relative')
local db = require('db')

print('\x1b]0;checkup\a')

function table.join(delim,t)
	 local result = nil
	 for _,v in ipairs(t) do
			if result == nil then
				 result = v
			else
				 result = result .. delim .. v
			end
	 end
	 return result
end

function interval(i)
	 ok,err = pcall(function()
				 if i == nil then
            return 'never'
				 elseif type(i) == 'string' then
            return i
				 end
				 local result = ''
				 local unit

				 local function addSpace()
            if result == nil then
							 result = ''
            else
							 result = result .. ' '
            end
				 end
				 
				 local function addNamed(one,many)
            return function(things)
							 addSpace()
							 
							 if things == 1 then
									result = result .. things..' '..one
							 elseif things == 0 then
							 else
									result = result .. things..' '..(many or (one..'s'))
							 end
            end
				 end
				 local function addSpaced(space)
            return function(things)
							 if things < 10 then
									things = space.. things
							 end
							 result = result .. things
            end
				 end
				 
				 addMinutes = (function(add)
							 return function(minutes)
									result = result .. ':'
									add(minutes)
							 end
											end)(addSpaced('0'));

				 addHours = (function (add)
							 return function(hours)
									if hours == 0 then return end
									--                addSpace()
									--                add(hours)
									result = result .. hours
							 end
										end)(addSpaced(' '));

				 local function produce(i,multiple,add)
            things,i = math.modf(i*multiple)
            add(things)
            return i,things
				 end
				 i = i / 86400 -- postgresql won't give me julian days >:( 
				 i,days = produce(i,1,addNamed('day'))
				 if i > 0 then
            if days <= 10 then
							 if days > 1 then
									addSpace()
							 end
							 i = produce(i,24,addHours)
							 if days <= 1 then
									i = produce(i,60,addMinutes)
							 else
									result = result .. 'h'
							 end
            end
				 end

				 return result.. ' '
	 end)
	 if not ok then
			print('I is',i)
			for n,v in pairs(i) do
				 print(n,v)
			end
			assert(ok,err)
	 end
	 return err
end

local gi = require('lgi')
local gtk = gi.require('Gtk')
local GObject = gi.require('GObject')
local glib = gi.require('GLib')
local gdk = gi.require('Gdk')
local notify = gi.require('Notify')

notify.init("Catchup")

local b = gtk.Builder.new_from_file("checkup.glade.xml").objects

local items = b.items
local selection = b.selection
assert(selection)
local c = {
   NAME = 1,
   ELAPSED = 2,
   DISABLED = 3,
   DANGER = 4,
   IDENT = 5,
   BACKGROUND = 6
}

local window = b.top
window:stick()
function window:on_destroy()
   gtk.main_quit()
end

math.randomseed(os.time())

local function catch(sub) 
	 return function()
			ok,err = xpcall(sub, function(err)
												 print('*********')
												 print(debug.traceback(err,3))
												 print('*********')
												 return err
			end)
			if not ok then
				 os.exit(3)
			end
			return err
	 end
end

local colorFor = (function()
			return function(ratio)
				 if ratio == nil then return black end
				 local r,g,b
				 b = 0
				 if ratio >= 1 then
            g = 0
            r = 1
				 elseif ratio <= -1 then
            g = 1
            r = 0
				 else
            -- m = (1 - 0) / (-1 - 1) = -1/2
            -- 1 = -1/2*-1+b, b = 1/2
            g = -0.5 * (ratio - 1)
            r = 0.5 * (ratio + 1)
				 end
				 color = gdk.RGBA()
				 color.red = r
				 color.green = g
				 color.blue = b
				 color.alpha = 1
				 return color
			end
								 end)()

local function color(r,g,b,a)
   local res = gdk.RGBA()
   if g == nil then
			g = r
   end
   if b == nil then
			b = r
   end
   if a == nil then
			a = 1.0
   end
   res.red = r
   res.green = g
   res.blue = b
   res.alpha = a
   return res
end

local black = color(0)
local grey = color(0.95)
local white = color(1.0)

local function raiseWindow()
   ok, first = items:get_iter_first()
   if not ok then return end
   local row = items[first]
   local label,habit = row[c.NAME],row[c.IDENT]
   local n = notify.Notification.new('',label,"task-due")
   function n:on_closed(e)
			local reason = n['closed-reason']
			--print('closed notify',reason)
			if reason == 2 then
				 window:present()
				 glib.idle_add(glib.PRIORITY_DEFAULT,catch(function() window:grab_focus() end))
			end
   end
   n:show()
   return true
end

local function update_intervals()
   if creating then glib.source_remove(creating) end
   creating = glib.idle_add(glib.PRIORITY_DEFAULT_IDLE,catch(function()
																	creating = nil								   
																	local i = 0
																	local row = items:get_iter_first()

																	for habit,description,frequency,elapsed in db.pending() do
																		 local thingy
																		 if elapsed ~= nil then
																				thingy = (elapsed - frequency) / frequency
																		 else
																				thingy = nil
																		 end
																		 i = i + 1
																		 if row ~= nil then
																				local function set(column,value)
																					 column = column - 1
																					 -- stoled from lgi/override/Gtk.lua
																					 value = GObject.Value(items:get_column_type(column),value)
																					 items:set_value(row, column, value)
																				end
																				set(c.IDENT,habit)
																				set(c.ELAPSED,interval(elapsed))
																				set(c.DANGER,colorFor(thingy))
																				set(c.DISABLED,false)
																				set(c.NAME,description)
																				if not items:iter_next(row) then
																					 row = nil
																				end
																		 else
																				local background
																				if i % 2 == 0 then
																					 background = white
																				else
																					 background = grey
																				end
																				items:insert(-1, {
																												[c.NAME] = description,
																												[c.ELAPSED] = interval(elapsed),
																												[c.DISABLED] = false,
																												[c.DANGER] = colorFor(thingy),
																												[c.IDENT] = habit,
																												[c.BACKGROUND] = background
																				})
																		 end
																	end		
																	while row do
																		 items:remove(row)
																		 if not items:iter_next(row) then
																				break
																		 end
																	end
																	return false
	 end))
   return true
end
local function complete_row(items, path, iter)
   local habit = items:get_value(iter, c.IDENT-1).value
   db.perform(habit)
end

function b.didit:on_clicked()
   selection:selected_foreach(complete_row)
   update_intervals()
end
function b.disabled:on_toggled(path)
   -- this is SO stupid about Gtk
   local path = gtk.TreePath.new_from_string(path)
   
   local row = items[path]
   row[c.DISABLED] = not row[c.DISABLED]
   db.set_enabled(row[c.IDENT],not row[c.DISABLED])
end

raiseWindow()

;(function()
			local handle,raiser
			local function start()
				 assert(handle == nil)
				 handle = glib.timeout_add_seconds(glib.PRIORITY_DEFAULT,
																					 1,update_intervals)
				 raiser = glib.timeout_add_seconds(glib.PRIORITY_DEFAULT,
																					 600,raiseWindow)
			end
			local function stop()
				 glib.source_remove(handle)
				 handle = nil
				 glib.source_remove(raiser)
				 raiser = nil
			end
			
			local update = b.update
			function update:on_toggled()
				 if update.active then
            start()
				 else
            stop()
				 end
			end
			update.active = true
			update_intervals()
			start()
 end)()

local view = b.view

local css = gtk.CssProvider()
ok, err = css:load_from_data([[
* {
  font-size: 14pt;
  font-weight: bold;
}]])
print(ok,err)
local context = window:get_style_context()
context:add_provider(css,gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)



window:show_all()
gtk.main()
