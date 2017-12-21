
function hash(key,reducer_count) 
    return string.byte(key,1) % reducer_count
end

function combine(key,value)
    local clist = cjson.decode(value)
    local sum = 0
    for k,v in pairs(clist) do
        sum = sum + v
    end
    push(key,sum)
end

function map(key,value)
    print("k '" .. key .. "' v '" .. value .. "'" )
    for word in value:gmatch("%w+") do 
        push(word,1)
    end
end
print("Priming run")
