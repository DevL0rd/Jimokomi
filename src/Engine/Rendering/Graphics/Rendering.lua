local Rendering = {}

local function to_screen(gfx, x, y)
	return gfx.camera:layerToScreenXY(x, y)
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

Rendering.drawSparkline = function(gfx, values, x, y, w, h, color, max_value)
	if not values or #values == 0 or w <= 1 or h <= 1 then
		return
	end
	local limit = max_value or 0
	if limit <= 0 then
		for i = 1, #values do
			if values[i] > limit then
				limit = values[i]
			end
		end
	end
	if limit <= 0 then
		limit = 1
	end
	local prev_x = nil
	local prev_y = nil
	for i = 1, #values do
		local t = #values > 1 and (i - 1) / (#values - 1) or 0
		local px = x + flr(t * (w - 1))
		local py = y + h - 1 - flr((values[i] / limit) * (h - 1))
		if prev_x ~= nil then
			gfx:screenLine(prev_x, prev_y, px, py, color)
		end
		prev_x = px
		prev_y = py
	end
end

Rendering.drawBarChart = function(gfx, entries, x, y, w, bar_h, colors, max_value)
	if not entries or #entries == 0 then
		return
	end
	local limit = max_value or 0
	if limit <= 0 then
		for i = 1, #entries do
			local entry_value = entries[i].value or 0
			if entry_value > limit then
				limit = entry_value
			end
		end
	end
	if limit <= 0 then
		limit = 1
	end
	for i = 1, #entries do
		local entry = entries[i]
		local row_y = y + (i - 1) * (bar_h + 2)
		local label_color = colors and colors.label or 7
		local bar_color = entry.color or (colors and colors.bar) or 11
		local frame_color = colors and colors.frame or 5
		local text = entry.label or "?"
		gfx:screenPrint(text, x, row_y, label_color)
		local label_w = #text * 4 + 6
		local bar_x = x + label_w
		local bar_w = max(8, w - label_w - 26)
		local fill_w = flr((min(entry.value or 0, limit) / limit) * bar_w)
		gfx:screenRect(bar_x, row_y, bar_x + bar_w, row_y + bar_h, frame_color)
		if fill_w > 0 then
			gfx:screenRectfill(bar_x + 1, row_y + 1, bar_x + fill_w - 1, row_y + bar_h - 1, bar_color)
		end
		local value_text = entry.value_text or tostr(flr((entry.value or 0) * 100) / 100)
		gfx:screenPrint(value_text, bar_x + bar_w + 4, row_y, label_color)
	end
end

Rendering.drawPerfPanel = function(gfx, profiler, x, y, w)
	if not profiler then
		return y
	end
	local entries = profiler.last_report_entries or {}
	local history_by_scope = profiler.history_by_scope or {}
	if #entries == 0 then
		gfx:screenPrint("perf: collecting...", x, y, 6)
		return y + 6
	end

	gfx:screenPrint("perf", x, y, 10)
	local top_entries = {}
	local top_count = min(4, #entries)
	local max_total = 0
	for i = 1, top_count do
		local entry = entries[i]
		local panel_value = entry.total_ms > 0 and entry.total_ms or entry.hz or entry.calls or 0
		add(top_entries, {
			label = entry.name,
			value = panel_value,
			value_text = entry.total_ms > 0 and (tostr(flr(entry.total_ms * 100) / 100) .. "ms") or (tostr(flr((entry.hz or 0) * 10) / 10) .. "hz"),
			color = i == 1 and 8 or i == 2 and 9 or i == 3 and 11 or 10,
		})
		if panel_value > max_total then
			max_total = panel_value
		end
	end
	Rendering.drawBarChart(gfx, top_entries, x, y + 7, w, 4, {
		label = 7,
		bar = 11,
		frame = 5,
	}, max_total)

	local chart_y = y + 7 + top_count * 6 + 4
	for i = 1, top_count do
		local entry = entries[i]
		local history = history_by_scope[entry.name]
		local color = top_entries[i].color
		local row_y = chart_y + (i - 1) * 10
		gfx:screenPrint(entry.name, x, row_y, color)
		gfx:screenRect(x + 40, row_y, x + w - 1, row_y + 7, 5)
		if history and #history > 0 then
			Rendering.drawSparkline(gfx, history, x + 41, row_y + 1, w - 42, 6, color, max_total)
		end
	end

	return chart_y + top_count * 10
end

return Rendering
