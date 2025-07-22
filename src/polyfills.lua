function random_float(min_val, max_val)
    return min_val + rnd(max_val - min_val)
end

function random_int(min_val, max_val)
    return flr(min_val + rnd(max_val - min_val + 1))
end

function round(num, decimal_places)
    if decimal_places == nil then
        return flr(num + 0.5)
    else
        local factor = 10 ^ decimal_places
        return flr(num * factor + 0.5) / factor
    end
end
