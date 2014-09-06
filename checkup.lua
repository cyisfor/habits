local db = require('db')

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

        function addNamed(one,many)
            return function(things)
                if result == nil then 
                    result = ''
                else
                    result = result..' '
                end
                if things == 1 then
                    result = result .. things..' '..one
                elseif things == 0 then
                else
                    result = result .. things..' '..(many or (one..'s'))
                end
            end
        end

        function addSpaced(space)
            return function(things)
                if things < 10 then
                    things = space.. things
                end
                result = result .. things
            end
        end
        addSeconds = addSpaced('0')
        function addMinutes(things)
            addSeconds(things)
            result = result .. ':'
        end
        function addHours(things)        
            if things == 0 then return end
            addSpaced(' ')(things) 
            result = result .. ':'
        end
        function produce(i,multiple,add)
            things,i = math.modf(i*multiple)
            add(things)
            return i
        end
        i = i / 86400 -- postgresql won give me julian days >:( 
        i = produce(i,1,addNamed('day'))
        if i > 0 then
            i = produce(i,24,addHours)
            if i > 0 then
                i = produce(i,60,addMinutes)
                if i > 0 then
                    produce(i,60,addSeconds)
                end
            end
        end

        return result
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

local window = gtk.Window {
    title = 'Checkup on Friends',
    default_width = 400,
    default_height = 400,
    on_destroy = function()
        gtk.main_quit()
        db.close()
    end
}

window:stick()

local vbox = gtk.VBox()
local grid = gtk.Grid()
window:add(vbox)
vbox:pack_start(grid,true,true,1)

local intervals = (function ()
    local store = {}
    return {
        set = function(key,value) 
            store[key.id] = {key,value}
        end,
        get = function(key)
            local entry = store[key.id]
            if not entry then return end
            entry[1] = key
            return entry[2]
        end,
        check = function(key)
            local entry = store[key.id]
            if not entry then return false end
            local oldkey = entry[1]
            if oldkey.dirty then return false end
            store[key.id] = {key,entry[2]}
            return true
        end,
        clear = function()
            store = {}
        end,
        pairs = function()
            local iter,init,id = pairs(store)
            return function()
                id,value = iter(init,id)
                if id == nil then return nil end
                return unpack(value)
            end
        end
    }
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

local creating = false
local function createGrid()
    if creating then glib.source_remove(creating) end
    intervals.clear()
    grid:forall(gtk.Widget.destroy)
    print('size',grid:get_size_request())
    creating = glib.idle_add(glib.PRIORITY_DEFAULT_IDLE,catch(function()
        local i = 0
        for habit in db.pending() do
            grid:insert_row(i)
            grid:attach(gtk.Label{label=habit.description},0,i,1,1);
            (function (habit,i)
                local intervalLabel = gtk.Label{
                    label=interval(habit.elapsed),
                }
                if habit.elapsed ~= nil then
                    intervalLabel:override_color(gtk.StateFlags.NORMAL,colorFor((habit.elapsed - habit.frequency) / habit.frequency))
                else
                    intervalLabel:override_color(gtk.StateFlags.NORMAL,black)
                end
                intervals.set(habit,intervalLabel)
                -- never using contacted in a callback
                -- no need to save in the environment
                grid:attach(intervalLabel,1,i,1,1)
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

local function raiseWindow()
    window:present()
    glib.idle_add(glib.PRIORITY_DEFAULT,catch(function() window:grab_focus() end))
    return true
end


(function()
    local handle
    local function start()
        assert(handle == nil)
        handle = glib.timeout_add_seconds(glib.PRIORITY_DEFAULT,1,updateIntervals)
        glib.timeout_add_seconds(glib.PRIORITY_DEFAULT,1800,raiseWindow)
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
