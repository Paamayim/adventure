-- this file is procedurally generated by utils/build_costume.py
-- so you probably shouldn't edit it by hand :)

local cost = nil
local anim = nil
cost = Costume.new()
cost.poses = {
	idle = { },
	walk = { },
	nil
}
anim = Animation.new(load.image("game/costumes/santino/idle.png"), 24, 1, 24)
anim.events[0] = "Start"
anim.loops = true
cost.poses.idle[5] = anim
anim = Animation.new(load.image("game/costumes/santino/walk8.png"), 24, 1, 24)
anim.events[0] = "Start"
anim.events[24] = "End"
anim.events[10] = "Return"
anim.loops = true
cost.poses.walk[8] = anim
anim = Animation.new(load.image("game/costumes/santino/walk2.png"), 24, 1, 24)
anim.events[0] = "Start"
anim.events[24] = "End"
anim.events[10] = "Return"
anim.loops = true
cost.poses.walk[2] = anim
anim = Animation.new(load.image("game/costumes/santino/walk4.png"), 24, 1, 24)
anim.events[0] = "Start"
anim.events[24] = "End"
anim.events[10] = "Return"
anim.loops = true
cost.poses.walk[4] = anim
anim = Animation.new(load.image("game/costumes/santino/walk6.png"), 24, 1, 24)
anim.events[0] = "Start"
anim.events[24] = "End"
anim.events[10] = "Return"
anim.loops = true
cost.poses.walk[6] = anim
costumes.santino = cost

