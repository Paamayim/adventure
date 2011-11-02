events.game = {
    tick = event.create()
}

events.game.tick.sub(function(state)
    local elapsed = 1 / framerate
    tasks.update(elapsed * 1000)
    clock.tick()
    
    conversation.pump_words(elapsed)

    for key, actor in pairs(room.scene) do
        if actor.aplay then
            animation.play(actor.aplay, elapsed)
        end

        if state == "game" then
            update_actor(actor, elapsed)
        end
    end

    table.sort(room.scene, zorder_sort)
end)

function get_size(actor)
    if actor.aplay then
        return actor.aplay.set.width, actor.aplay.set.height
    elseif actor.sprite then
        return get_image_size(actor.sprite)
    else
        return 0, 0
    end
end

function make_walkspot(actor)
    if type(actor) == "string" then
        actor = table.find(room.scene, function(key, val)
            return val.id == actor
        end)
    end
    
    if not actor then return 0, 0 end
    
    if actor.walkspot then
        return actor.walkspot.x, actor.walkspot.y
    end

    local x = actor.pos.x
    local y = actor.pos.y
    local sx, sy = get_size(actor)
    local ox, oy = get_origin(actor)
    local flip = 1
    
    if actor.flipped then flip = -1 end
    
    x = x - ox + sx
    y = y - oy + sy
    
    
    for dist = sx / 2, sx * 5, sx / 2 do
        for degree = 0, math.pi, math.pi / 12 do
            local ax = math.cos(degree) * dist * flip
            local ay = math.sin(degree) * dist
            
            if is_walkable(x + ax, y + ay) then
                return x + ax, y + ay
            end
        end
    end
    
    return x, y
end

function get_origin(actor)
    if not actor then return 0, 0 end

    if type(actor) == "string" then
        actor = table.find(room.scene, function(key, val)
            return val.id == actor
        end)
    end
    
    if actor.aplay then
        return actor.aplay.set.xorigin, actor.aplay.set.yorigin
    end
    
    if actor.height then
        return 0, actor.height
    end
    
    return 0, 0
end

function update_actor(actor, elapsed)
    local name = actor.id

    if actor.goal and type(actor.goal) ~= "boolean" then
        if actor.aplay then
            animation.switch(actor.aplay, "walk")
        end
        
        if type(actor.goal) == "table" then
            actor.goal = actor.goal
        
            local speed = actor.speed * elapsed
            local dif = actor.goal - actor.pos

            if dif.len() > speed then
                local dir = dif.normal()

                if dir.x < 0 then
                    actor.flipped = true
                else
                    actor.flipped = false
                end

                actor.pos.x = actor.pos.x + dir.x * speed
                actor.pos.y = actor.pos.y + dir.y * speed
            else
                actor.pos = actor.goal
                actor.goal = nil

                if actor.goals and actor.goals[1] then
                    actor.goal = actor.goals[1]
                    table.remove(actor.goals, 1)
                end
                
                if actor.aplay then
                    animation.switch(actor.aplay, "stand")
                end

                if not actor.goal then
                    actor.events.goal()

                    if actor == player then
                        signal_goal()
                    end
                end
            end
        elseif type(actor.goal) == "function" then
            if actor.aplay then
                animation.switch(actor.aplay, "stand")
            end
        
            tasks.begin({ actor.goal, function() 
                if actor.goals and actor.goals[1] then
                    actor.goal = actor.goals[1]
                    table.remove(actor.goals, 1)
                end
            end })
            actor.goal = true
        end
    end
    
    if actor == player then
        local xoffset = math.clamp(player.pos.x - screen_width / 2, 0, room_width - screen_width)
        local yoffset = math.clamp(player.pos.y - screen_height / 2, 0, room_height - screen_height)
        
        set_viewport(xoffset, yoffset)
    end
end

function zorder_sort(a, b)
    if not a or not b then
        return a
    end
    
    ah = a.height
    bh = b.height
    
    ay = a.baseline
    by = b.baseline
    
    if not ay then
        ay = a.pos.y
    end
    
    if not by then
        by = b.pos.y
    end

    if not ah then ah = 0 end
    if not bh then bh = 0 end
    
    return ay + ah < by + bh
end

-- this should be implemented in C
function distance(from, to)
    return 1
end

function do_pathfinding(from, to)
    local function add_path(old, new, cost)
        return { cost = cost, location = new, previous = old }
    end

    local closed = { }
    local open = { { 0, add_path(nil, from, 0) } }

    while open do
        local continue = true
        repeat
            local path = pqueue.dequeue(open)

            if not path then return nil end

            if table.contains(closed, path.location) then continue = true; break end
            if to == path.location then return path end
            table.insert(closed, path.location)

            for key, val in ipairs(get_neighbors(path.location)) do
                local dist = distance(path.location, get_waypoint(val))
                pqueue.enqueue(open, -(path.cost + dist), add_path(path, val, path.cost + dist))
            end
        until true
        if not continue then break end
    end
end

function walk(actor, to, y)
    if y then 
        to = vec(to, y)
    else
        to = vec(to.x, to.y)-- get a new vector
    end

    actor.goal = get_waypoint(get_closest_waypoint(actor.pos))
    actor.goals = unwrap_path(do_pathfinding(get_closest_waypoint(actor.pos), get_closest_waypoint(to)))

    if actor.goals then
        table.insert(actor.goals, to)
    end
end

function unwrap_path(path)
    if not path then return nil end

    if path.previous then
        local wind = unwrap_path(path.previous)
        table.insert(wind, get_waypoint(path.location));
        return wind
    else
        return { get_waypoint(path.location) }
    end
end

function give_item(actor, item)
    actor.inventory[item] = items[item]
end

function remove_item(actor, item)
    actor.inventory[item] = nil
end
