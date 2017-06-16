--[[
    Params:
        @param key - list of centers in JSON format
        @param value - list of points in JSON format assigned to this mapper
]]

function hash(key,reducer_count) 
    local center = cjson.decode(key)
    return center.id % reducer_count
end

function map(key, value)
    -- parse string JSON values
    -- print( key .. " " .. value )
    local centers, points = cjson.decode(key), cjson.decode(value)

    for _, point in pairs(points) do
        local min, centerToAssign = math.huge, -1
        -- find the closest center to assign the point
        for _, center in pairs(centers) do
            local distance = calculateEuclidianDistance(center, point)

            if distance < min then
                min = distance
                centerToAssign = center
            end
        end

        push( cjson.encode(centerToAssign), cjson.encode(point) )
    end
end

function calculateEuclidianDistance(point, otherPoint)
    local x = (point.x - otherPoint.x) * (point.x - otherPoint.x)
    local y = (point.y - otherPoint.y) * (point.y - otherPoint.y)

    return math.sqrt(x + y)
end

