local draw = function()
    drawing.clear(color.make(255, 255, 0))
    
    bone = root.bones[keys[index]]

    drawing.text(vector(30), color.black, last_msg)
    drawing.blit(bone.image, bone_offset)
    drawing.circle(bone.image_offset + bone_offset, 3, color.black)
    drawing.circle(bone.image_offset + bone_offset, 2, color.white)
    drawing.circle(bone.image_offset + bone_offset, 1, color.black)
    
    drawing.skeleton(root, pos)
    
    if keys[index] ~= "root" then
        local origin = pos + bone.parent.get_position() + rotate((bone.default.pos + bone.pos), bone.parent.get_rotation())
        local tl = origin - rotate(bone.image_offset, bone.get_rotation())
        local tr = origin + rotate(vector(bone.image.size.x - bone.image_offset.x, -bone.image_offset.y), bone.get_rotation())
        local bl = origin + rotate(vector(-bone.image_offset.x, bone.image.size.y - bone.image_offset.y), bone.get_rotation())
        local br = origin + rotate(bone.image.size - bone.image_offset, bone.get_rotation())
    
        drawing.line(tl, tr, color.make(255, 0, 0))
        drawing.line(tl, bl, color.make(255, 0, 0))
        drawing.line(bl, br, color.make(255, 0, 0))
        drawing.line(tr, br, color.make(255, 0, 0))
    end
    
    drawing.blit(game.resources.cursors, input.mouse.pos - game.cursors.offsets[input.mouse.cursor + 1], false, vector(32 * input.mouse.cursor, 0), vector(32))
    
    local start = vector(400, 600)
    local size = vector(480, 30)
    
    drawing.rect(rect(start, size), color.black)
    drawing.text_center(start + vector(0, size.y + 15), color.black, "0")
    
    if table.getn(kframes) ~= 0 then
        local max = kframes[table.max(kframes)]
    
        for _, v in ipairs(kframes) do
            drawing.line(start + vector(size.x * (v / max), 0), start + vector(size.x * (v / max), size.y), color.black)
            drawing.text_center(start + vector(size.x * (v / max), size.y + 15), color.black, v)
        end
    end
end

engine.events.draw.sub(draw)
