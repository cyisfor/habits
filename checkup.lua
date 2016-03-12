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

local b = gtk.Builder.new_from_file("checkup.glade.xml")

local window = b:get_object('top')
window:stick()

local items = b:get_object('items')
local view = b:get_object('view')

math.randomseed(os.time())
local intervals = (function ()
    local labels = {}
    local habits = {}
    local scale = 1000
    local a = scale * scale
    return setmetatable({
        set = function(habit,order,value) 
            labels[habit.id] = {habit,value}
            habits[order] = habit
        end,
        get = function(habit)
            if habit == nil then
                local order = math.random(0,scale)
                order = 1 - order * (scale * 2 - order) / a
                -- bias it towards 0
                assert(order >= 0)
                assert(order < 1)
                habit = habits[math.floor(order*#habits)+1] -- 1-based addressing grumble
            end
            local entry = labels[habit.id]
            if not entry then return end
            entry[1] = habit
            return entry[2],habit
        end,
        check = function(habit)
            local entry = labels[habit.id]
            if not entry then return false end
            local oldhabit = entry[1]
            if oldhabit.dirty then return false end
            labels[habit.id] = {habit,entry[2]}
            return true
        end,
        clear = function()
            labels = {}
        end},        
    {
        __call = function()
            local iter,init,order = ipairs(habits)
            return function()
                order,habit = iter(init,order)
                if order == nil then return nil end
                return unpack(labels[habit.id])
            end
        end
    })
end)()

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

local elapsed = b:get_object('elapsed')
elapsed:set_cell_data_func(
   -- why does set_cell_data_func also set the renderer??
   b:get_object('elapsed_renderer'),
   function(col,renderer,model,iter,data)
	  renderer.set_property("foreground-rgba",
							colorFor(model.get(1)))
   end,nil,nil)

local function raiseWindow()
    local label,habit = intervals.get()
    local n = notify.Notification.new('',habit.description,"task-due")
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

local creating = false
local function createGrid()
    if creating then glib.source_remove(creating) end
    intervals.clear()
	store.clear()
    creating = glib.idle_add(glib.PRIORITY_DEFAULT_IDLE,catch(function()
        local i = 0
        for habit in db.pending() do
		   iter = store:append()
		   store:set(iter,
					 0,habit.description,
					 1,interval(habit.elapsed),
					 2,colorFor(habit.elapsed))

		   -- oh god, on clicked in a... in a... uh... liststore...??
		   -- never using contacted in a callback
		   -- no need to save in the environment
                local contact = gtk.Button{label='Did it'}
                function contact:on_clicked()
                    ok,err = pcall(function() 
                        db.transaction(function()
                            habit.perform()
                        end)
                        intervalLabel:set_text('just now')                        
                        createGrid()
                    end)
                    if not ok then print(err,'err') end
                    assert(ok,err)
                end
                grid:attach(contact,2,i,1,1)
                local disable = gtk.Button{label='Disable'}
                function disable:on_clicked()
                    db.transaction(function()
                        habit.enabled(false)
                    end)
                    glib.idle_add(glib.PRIORITY_DEFAULT_IDLE,createGrid)
                end 
                grid:attach(disable,3,i,1,1)
            end)(habit,i)
            i = i + 1
        end
        grid:show_all()
        creating = false
        raiseWindow()
        return false
    end))
end
createGrid()

local function updateIntervals()
    for habit in db.pending() do
        if not intervals.check(habit) then
            createGrid()
            return true
        end
        label = intervals.get(habit)
        label:set_text(interval(habit.elapsed))
        if habit.elapsed ~= nil then
            label:override_color(gtk.StateFlags.NORMAL,colorFor((habit.elapsed - habit.frequency) / habit.frequency))
        else
            label:override_color(gtk.StateFlags.NORMAL,black)
        end
    end
    return true
end


(function()
    local handle
    local function start()
        assert(handle == nil)
        handle = glib.timeout_add_seconds(glib.PRIORITY_DEFAULT,1,updateIntervals)
        glib.timeout_add_seconds(glib.PRIORITY_DEFAULT,600,raiseWindow)
    end
    local function stop()
        glib.source_remove(handle)
        handle = nil
    end
    
    local update = gtk.CheckButton{label="Update Interval"}
    vbox:pack_start(update,false,true,1)
    function update:on_toggled()
        if update.active then
            start()
        else
            stop()
        end
    end
    update.active = true
end)()

window:show_all()
gtk.main()
