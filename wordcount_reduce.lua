
function reduce( key, value )
    local clist = cjson.decode(value)
    local sum = 0
    for k,v in pairs(clist) do
        sum = sum + v
    end
    push(key,sum)

end
