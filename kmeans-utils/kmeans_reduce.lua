--[[
    Params:
        @param key - center of the given list in JSON format
        @param values - list of points in JSON format assigned to this center
    
    Return:
        NewCenter = {x = x, y = y} in JSON format
]]

function reduce(key, value)
    -- print( key .. " <=> " .. value )
    -- parse string JSON values
    -- key is the center point, value is the list of points
    local center, points = cjson.decode(key), cjson.decode(value)

    local newCenter = {}

    if #points == 0 then
        newCenter = center
    else
        newCenter = baryCenter(points, center.id)
    end    

    push( "-", cjson.encode(newCenter) )
end

function baryCenter(points,cid)
    local x, y = 0, 0

    for _, p in pairs(points) do
        x = x + p.x
        y = y + p.y
    end

    x = x / #points
    y = y / #points

    return {x = x, y = y, id = cid} 
end

