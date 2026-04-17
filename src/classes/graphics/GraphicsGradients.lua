local GraphicsGradients = {}

GraphicsGradients.drawLinearGradient = function(self, x1, y1, x2, y2, color1, color2)
	local screen_pos1 = self.camera:layerToScreen({ x = x1, y = y1 })
	local screen_pos2 = self.camera:layerToScreen({ x = x2, y = y2 })
	local sx1, sy1 = screen_pos1.x, screen_pos1.y
	local sx2, sy2 = screen_pos2.x, screen_pos2.y
	local dx = sx2 - sx1
	local dy = sy2 - sy1
	local distance = sqrt(dx * dx + dy * dy)

	for x = sx1, sx2 do
		for y = sy1, sy2 do
			local pixel_dx = x - sx1
			local pixel_dy = y - sy1
			local pixel_dist = sqrt(pixel_dx * pixel_dx + pixel_dy * pixel_dy)
			local t = pixel_dist / distance
			t = max(0, min(1, t))
			local color = flr(color1 + (color2 - color1) * t)
			pset(x, y, color)
		end
	end
end

GraphicsGradients.drawRadialGradient = function(self, cx, cy, inner_radius, outer_radius, inner_color, outer_color)
	local screen_pos = self.camera:layerToScreen({ x = cx, y = cy })
	local scx, scy = screen_pos.x, screen_pos.y
	for x = scx - outer_radius, scx + outer_radius do
		for y = scy - outer_radius, scy + outer_radius do
			local dx = x - scx
			local dy = y - scy
			local distance = sqrt(dx * dx + dy * dy)

			if distance <= outer_radius then
				local t = (distance - inner_radius) / (outer_radius - inner_radius)
				t = max(0, min(1, t))
				local color = flr(inner_color + (outer_color - inner_color) * t)
				pset(x, y, color)
			end
		end
	end
end

GraphicsGradients.drawHorizontalGradient = function(self, x, y, w, h, left_color, right_color)
	local screen_pos = self.camera:layerToScreen({ x = x, y = y })
	local sx, sy = screen_pos.x, screen_pos.y
	for i = 0, w - 1 do
		local t = i / (w - 1)
		local color = flr(left_color + (right_color - left_color) * t)
		line(sx + i, sy, sx + i, sy + h - 1, color)
	end
end

GraphicsGradients.drawVerticalGradient = function(self, x, y, w, h, top_color, bottom_color)
	local screen_pos = self.camera:layerToScreen({ x = x, y = y })
	local sx, sy = screen_pos.x, screen_pos.y
	for i = 0, h - 1 do
		local t = i / (h - 1)
		local color = flr(top_color + (bottom_color - top_color) * t)
		line(sx, sy + i, sx + w - 1, sy + i, color)
	end
end

GraphicsGradients.drawDitheredGradient = function(self, x, y, w, h, color1, color2, pattern)
	local old_fillp = peek(0x5f34)
	pattern = pattern or 0xAA55
	fillp(pattern)

	local screen_pos = self.camera:layerToScreen({ x = x, y = y })
	local sx, sy = screen_pos.x, screen_pos.y
	rectfill(sx, sy, sx + w - 1, sy + h - 1, color1)

	local inverted_pattern = 0xFFFF - pattern
	fillp(inverted_pattern)
	rectfill(sx, sy, sx + w - 1, sy + h - 1, color2)

	fillp(old_fillp)
end

GraphicsGradients.drawPaletteGradient = function(self, x, y, w, h, start_pal, end_pal, steps)
	steps = steps or 16
	local screen_pos = self.camera:layerToScreen({ x = x, y = y })
	local sx, sy = screen_pos.x, screen_pos.y

	local old_pal = {}
	for i = 0, 15 do
		old_pal[i] = peek(0x3000 + i)
	end

	local step_height = h / steps
	for i = 0, steps - 1 do
		local t = i / (steps - 1)
		local interp_color = flr(start_pal + (end_pal - start_pal) * t)
		pal(7, interp_color)
		local rect_y = sy + i * step_height
		rectfill(sx, rect_y, sx + w - 1, rect_y + step_height, 7)
	end

	for i = 0, 15 do
		poke(0x3000 + i, old_pal[i])
	end
	for i = 0, 15 do
		pal(i, i)
	end
end

GraphicsGradients.drawAnimatedGradient = function(self, x, y, w, h, colors, speed)
	speed = speed or 1
	local time_offset = flr(time() * speed * 100) % #colors

	local screen_pos = self.camera:layerToScreen({ x = x, y = y })
	local sx, sy = screen_pos.x, screen_pos.y

	for i = 0, h - 1 do
		local color_index = ((i + time_offset) % #colors) + 1
		line(sx, sy + i, sx + w - 1, sy + i, colors[color_index])
	end
end

GraphicsGradients.drawWaveGradient = function(self, x, y, w, h, color1, color2, frequency, amplitude)
	frequency = frequency or 0.1
	amplitude = amplitude or 0.5

	local screen_pos = self.camera:layerToScreen({ x = x, y = y })
	local sx, sy = screen_pos.x, screen_pos.y

	for i = 0, w - 1 do
		local wave = sin(i * frequency + time()) * amplitude + 0.5
		local color = flr(color1 + (color2 - color1) * wave)
		line(sx + i, sy, sx + i, sy + h - 1, color)
	end
end

GraphicsGradients.setDitherPattern = function(self, pattern)
	fillp(pattern)
end

GraphicsGradients.resetFillPattern = function(self)
	fillp(0x0000)
end

return GraphicsGradients
