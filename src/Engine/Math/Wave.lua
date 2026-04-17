local Wave = {}

Wave.fract = function(value)
	return value - flr(value)
end

Wave.triangle01 = function(value)
	local t = Wave.fract(value)
	return 1 - abs(t * 2 - 1)
end

Wave.sine01 = function(value)
	return sin(value) * 0.5 + 0.5
end

Wave.lerp = function(a, b, t)
	return a + (b - a) * t
end

Wave.quantize = function(value, steps)
	steps = max(1, steps or 1)
	return flr(value * steps + 0.5) / steps
end

Wave.cycle01 = function(frame_index, length)
	length = max(1, length or 1)
	return (frame_index or 0) / length
end

Wave.layered = function(x, phase, layers)
	local total = 0
	local weight_total = 0
	for i = 1, #(layers or {}) do
		local layer = layers[i]
		local weight = layer.weight or 1
		local sample = Wave.sine01(x * (layer.frequency or 1) + phase * (layer.speed or 1) + (layer.offset or 0))
		total += sample * weight
		weight_total += weight
	end
	if weight_total <= 0 then
		return 0
	end
	return total / weight_total
end

return Wave
