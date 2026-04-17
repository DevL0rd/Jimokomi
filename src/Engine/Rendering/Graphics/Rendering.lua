local Rendering = {}

local function to_screen(gfx, x, y)
	local screen_pos = gfx.camera:layerToScreen({ x = x, y = y })
	return screen_pos.x, screen_pos.y
end

Rendering.drawHorizontalGradient = function(gfx, x, y, w, h, left_color, right_color)
	local sx, sy = to_screen(gfx, x, y)
	for i = 0, w - 1 do
		local t = w > 1 and i / (w - 1) or 0
		local color = flr(left_color + (right_color - left_color) * t)
		line(sx + i, sy, sx + i, sy + h - 1, color)
	end
end

Rendering.drawVerticalGradient = function(gfx, x, y, w, h, top_color, bottom_color)
	local sx, sy = to_screen(gfx, x, y)
	for i = 0, h - 1 do
		local t = h > 1 and i / (h - 1) or 0
		local color = flr(top_color + (bottom_color - top_color) * t)
		line(sx, sy + i, sx + w - 1, sy + i, color)
	end
end

Rendering.drawDitheredGradient = function(gfx, x, y, w, h, color1, color2, pattern)
	local sx, sy = to_screen(gfx, x, y)
	local old_fillp = peek(0x5f34)
	pattern = pattern or 0xAA55
	fillp(pattern)
	rectfill(sx, sy, sx + w - 1, sy + h - 1, color1)
	fillp(0xFFFF - pattern)
	rectfill(sx, sy, sx + w - 1, sy + h - 1, color2)
	fillp(old_fillp)
end

Rendering.drawAnimatedGradient = function(gfx, x, y, w, h, colors, speed)
	local sx, sy = to_screen(gfx, x, y)
	speed = speed or 1
	local time_offset = flr(time() * speed * 100) % #colors
	for i = 0, h - 1 do
		local color_index = ((i + time_offset) % #colors) + 1
		line(sx, sy + i, sx + w - 1, sy + i, colors[color_index])
	end
end

Rendering.drawWaveGradient = function(gfx, x, y, w, h, color1, color2, frequency, amplitude, time_offset)
	local sx, sy = to_screen(gfx, x, y)
	frequency = frequency or 0.1
	amplitude = amplitude or 0.5
	time_offset = time_offset or time()
	for i = 0, w - 1 do
		local wave = sin(i * frequency + time_offset) * amplitude + 0.5
		local color = flr(color1 + (color2 - color1) * wave)
		line(sx + i, sy, sx + i, sy + h - 1, color)
	end
end

Rendering.drawSpeckles = function(gfx, x, y, w, h, colors, seed, density)
	local sx, sy = to_screen(gfx, x, y)
	seed = seed or 1
	density = density or 0.1
	for px = 0, w - 1 do
		for py = 0, h - 1 do
			local noise = abs(sin((px + seed) * 12.9898 + (py + seed) * 78.233))
			if noise - flr(noise) < density then
				local index = ((px + py + seed) % #colors) + 1
				pset(sx + px, sy + py, colors[index])
			end
		end
	end
end

return Rendering
