require "classes/actor"
require "classes/game"
require "classes/costume"
require "classes/sheet"

local sheet = Sheet.new("foreground")

sheet:setClickAcceptor(Sheet.prop_acceptor)
sheet:setHoverAcceptor(Sheet.prop_acceptor)
sheet:install()

--------------------------------------------------

local function makeProp()
    local prop = MOAIProp2D.new()
    sheet:insertProp(prop)
    return prop
end

local function destroyProp(prop)
    sheet:removeProp(prop)
end

game.export("makeProp", makeProp)
game.export("destroyProp", destroyProp)

--------------------------------------------------

function sheet:onClick(prop, x, y, down)
    if prop then
        game.interactWith(x, y, down)
    end
end

function sheet:onHover(prop, x, y)
    if prop.actor then
        game.setCurrentObject(prop.actor)
    else
        game.setCurrentObject(nil)
    end
    
    game.setCursor(5)

    return true
end
