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
                addSpace()
                add(hours)
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
local glib = gi.require('GLib')
local gdk = gi.require('Gdk')
local notify = gi.require('Notify')

notify.init("Catchup")

local b = gtk.Builder.new_from_file("checkup.glade.xml").objects

local items = b.items
local selection = b.selection
local c = {
   NAME = 1,
   ELAPSED = 2,
   DISABLED = 3,
   DANGER = 4,
   IDENT = 5
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

local black = gdk.RGBA()
black.red = 0
black.green= 0
black.blue = 0
black.alpha = 1

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

local num_items = 0

local function update_intervals()
   print('um')
   if creating then glib.source_remove(creating) end
   creating = glib.idle_add(glib.PRIORITY_DEFAULT_IDLE,catch(function()
        creating = nil								   
        local i = 0
        for habit,description,frequency,elapsed in db.pending() do
		   --print('BEEP',habit,description,frequency,elapsed)
		   local thingy
		   if elapsed then
			  thingy = (elapsed - frequency) / frequency
		   else
			  thingy = nil
		   end
		   local thingy		   
		   i = i + 1
		   print('i',i)
		   if num_items >= i then
			  local path = gtk.TreePath.new_from_string(i)
			  local iter = items:get_iter(path)
			  assert iter
			  local olditem = items[iter]
			  if olditem[c.IDENT] == habit then
				 olditem[c.ELAPSED] = interval(elapsed)
				 olditem[c.DANGER] = colorFor(thingy)
				 olditem[c.DISABLED] = false
				 olditem[c.NAME] = description
			  else
				 olditem[c.NAME] = description
				 olditem[c.ELAPSED] = interval(elapsed)
				 olditem[c.DISABLED] = false
				 olditem[c.DANGER] = colorFor(thingy)
				 olditem[c.IDENT] = habit
			  end
		   else
			  items:insert(-1, {
							  [c.NAME] = description,
							  [c.ELAPSED] = interval(elapsed),
							  [c.DISABLED] = false,
							  [c.DANGER] = colorFor(thingy),
							  [c.IDENT] = habit
			  })
			  num_items = num_items + 1
		   end
		end
		for gone = i+1,num_items do
		   items:remove(gone)
		end
        return false
	end))
   print('ummm')
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

window:show_all()
gtk.main()
