require "classes/class"
local Shapes = require "classes/lib/HardonCollider/shapes"

newclass("Polygon", 
    function(first, ...)
        local points
        
        if type(first) == "table" then
            points = first
        else
            points = { first, ... }
        end
    
        return {
            points = points,
            shape = nil
        }
    end
)

function Polygon:addPoint(x, y)
    table.insert(self.points, x)
    table.insert(self.points, y)
    self:invalidate()
end

function Polygon:removePoint()
    table.remove(self.points, #self.points)
    table.remove(self.points, #self.points)
    self:invalidate()
end

function Polygon:size()
    return #self.points / 2
end

function Polygon:invalidate()
    self.shape = nil
end

function Polygon:rebuildCollision()
    if self.shape or #self.points < 6 then return end
    
    self.shape = Shapes.newPolygonShape(unpack(self.points))
end

function Polygon:hitTest(x, y)
    self:rebuildCollision()
    
    if self.shape then
        return self.shape:contains(x, y)
    end
    
    return false
end

function Polygon:getBox()
    self:rebuildCollision()

    if self.shape then
        return self.shape:getBBox()
    end
    
    return 0, 0, 0, 0
end