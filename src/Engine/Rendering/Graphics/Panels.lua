local Panels = {}

local function record_counter(gfx, name, amount)
	if gfx.profiler and gfx.profiler.addCounter then
		gfx.profiler:addCounter(name, amount or 1)
	end
end

local function get_style_value(style, key, default)
	if style and style[key] ~= nil then
		return style[key]
	end
	return default
end

local function get_cache_key(w, h, style)
	local key_parts = {
		tostr(get_style_value(style, "key", "panel")),
		tostr(w),
		tostr(h),
		tostr(get_style_value(style, "shadow_offset", 0)),
		tostr(get_style_value(style, "shadow_color", 1)),
		tostr(get_style_value(style, "bg_top", 1)),
		tostr(get_style_value(style, "bg_bottom", 1)),
		tostr(get_style_value(style, "stripe_color", -1)),
		tostr(get_style_value(style, "stripe_step", 3)),
		tostr(get_style_value(style, "border_color", 6)),
		tostr(get_style_value(style, "inner_border_color", 5)),
		tostr(get_style_value(style, "highlight_color", 7)),
		tostr(get_style_value(style, "speckle_color", 6)),
		tostr(get_style_value(style, "speckle_density", 0)),
		tostr(get_style_value(style, "seed", 1)),
	}
	return table.concat(key_parts, ":")
end

local function get_noise(seed, x, y)
	local n = abs(sin((x + seed * 17) * 12.9898 + (y + seed * 31) * 78.233))
	return n - flr(n)
end

Panels.clearPanelCache = function(gfx)
	gfx.panel_cache = {}
end

Panels.getCachedPanel = function(gfx, w, h, style)
	gfx.panel_cache = gfx.panel_cache or {}
	style = style or {}
	local key = get_cache_key(w, h, style)
	if gfx.panel_cache[key] then
		record_counter(gfx, "render.panel_cache.hits", 1)
		return gfx.panel_cache[key]
	end
	record_counter(gfx, "render.panel_cache.misses", 1)

	local panel = {
		key = key,
		w = w,
		h = h,
		shadow_offset = get_style_value(style, "shadow_offset", 1),
		shadow_color = get_style_value(style, "shadow_color", 1),
		border_color = get_style_value(style, "border_color", 6),
		inner_border_color = get_style_value(style, "inner_border_color", 5),
		highlight_color = get_style_value(style, "highlight_color", 7),
		fill_rows = {},
		speckles = {},
	}

	local top_color = get_style_value(style, "bg_top", 1)
	local bottom_color = get_style_value(style, "bg_bottom", top_color)
	local stripe_color = get_style_value(style, "stripe_color", nil)
	local stripe_step = max(1, get_style_value(style, "stripe_step", 3))
	for row = 0, h do
		local t = h > 0 and row / h or 0
		local color = flr(top_color + (bottom_color - top_color) * t)
		if stripe_color and row > 1 and row < h - 1 and row % stripe_step == 0 then
			color = stripe_color
		end
		panel.fill_rows[row + 1] = color
	end

	local speckle_density = get_style_value(style, "speckle_density", 0)
	if speckle_density > 0 then
		local seed = get_style_value(style, "seed", 1)
		local speckle_color = get_style_value(style, "speckle_color", panel.highlight_color)
		for py = 2, h - 2 do
			for px = 2, w - 2 do
				if get_noise(seed, px, py) < speckle_density then
					add(panel.speckles, {
						x = px,
						y = py,
						color = speckle_color,
					})
				end
			end
		end
	end

	gfx.panel_cache[key] = panel
	record_counter(gfx, "render.panel_cache.rebuilds", 1)
	return panel
end

Panels.appendPanelCommands = function(gfx, target, panel)
	if not panel then
		return
	end
	if panel.shadow_offset and panel.shadow_offset > 0 then
		target:rectfill(
			panel.shadow_offset,
			panel.shadow_offset,
			panel.w + panel.shadow_offset,
			panel.h + panel.shadow_offset,
			panel.shadow_color
		)
	end

	for row = 0, panel.h do
		target:line(0, row, panel.w, row, panel.fill_rows[row + 1])
	end

	target:rect(0, 0, panel.w, panel.h, panel.border_color)
	target:rect(1, 1, panel.w - 1, panel.h - 1, panel.inner_border_color)
	target:line(1, 1, panel.w - 1, 1, panel.highlight_color)
	target:line(1, 2, panel.w - 1, 2, panel.inner_border_color)

	for i = 1, #panel.speckles do
		local speckle = panel.speckles[i]
		target:pset(speckle.x, speckle.y, speckle.color)
	end
end

Panels.drawPanel = function(gfx, panel, x, y)
	if not panel then
		return
	end
	gfx:drawCachedScreen(panel.key, x, y, function(target)
		Panels.appendPanelCommands(gfx, target, panel)
	end, {
		w = panel.w,
		h = panel.h,
		cache_mode = "surface",
	})
end

Panels.newRetainedPanel = function(gfx, config)
	config = config or {}
	local style = config.style or {}
	local node = gfx:newRetainedLeaf({
		key = config.key or "retained.panel",
		cache_mode = config.cache_mode or "retained",
		x = config.x or 0,
		y = config.y or 0,
		w = config.w or 0,
		h = config.h or 0,
	})

	node.panel_style = style

	function node:setPanelStyle(next_style)
		self.panel_style = next_style or {}
		self:markDirty()
		return self
	end

	node:setBuilder(function(target, panel_node)
		local panel = gfx:getCachedPanel(panel_node.w, panel_node.h, panel_node.panel_style)
		if panel then
			gfx:appendPanelCommands(target, panel)
		end
	end)

	return node
end

return Panels
