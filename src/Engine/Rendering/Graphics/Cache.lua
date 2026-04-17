local GraphicsCache = {}

local function add_rendering_methods(target)
	function target:drawHorizontalGradient(x, y, w, h, left_color, right_color)
		for i = 0, w - 1 do
			local t = w > 1 and i / (w - 1) or 0
			local color = flr(left_color + (right_color - left_color) * t)
			self:line(x + i, y, x + i, y + h - 1, color)
		end
	end

	function target:drawVerticalGradient(x, y, w, h, top_color, bottom_color)
		for i = 0, h - 1 do
			local t = h > 1 and i / (h - 1) or 0
			local color = flr(top_color + (bottom_color - top_color) * t)
			self:line(x, y + i, x + w - 1, y + i, color)
		end
	end

	function target:drawDitheredGradient(x, y, w, h, color1, color2, pattern)
		if self.ditheredGradientCommand then
			self:ditheredGradientCommand(x, y, w, h, color1, color2, pattern)
			return
		end
		pattern = pattern or 0xAA55
		self:rectfill(x, y, x + w - 1, y + h - 1, color1)
	end

	function target:drawAnimatedGradient(x, y, w, h, colors, speed, phase)
		speed = speed or 1
		local time_offset = flr((phase or 0) * speed * 100) % #colors
		for i = 0, h - 1 do
			local color_index = ((i + time_offset) % #colors) + 1
			self:line(x, y + i, x + w - 1, y + i, colors[color_index])
		end
	end

	function target:drawWaveGradient(x, y, w, h, color1, color2, frequency, amplitude, time_offset)
		frequency = frequency or 0.1
		amplitude = amplitude or 0.5
		time_offset = time_offset or 0
		for i = 0, w - 1 do
			local wave = sin(i * frequency + time_offset) * amplitude + 0.5
			local color = flr(color1 + (color2 - color1) * wave)
			self:line(x + i, y, x + i, y + h - 1, color)
		end
	end

	function target:drawSpeckles(x, y, w, h, colors, seed, density)
		seed = seed or 1
		density = density or 0.1
		for px = 0, w - 1 do
			for py = 0, h - 1 do
				local noise = abs(sin((px + seed) * 12.9898 + (py + seed) * 78.233))
				if noise - flr(noise) < density then
					local index = ((px + py + seed) % #colors) + 1
					self:pset(x + px, y + py, colors[index])
				end
			end
		end
	end

	return target
end

local function make_command(kind, payload)
	payload.kind = kind
	return payload
end

local function build_recorder()
	local recorder = {
		commands = {},
	}

	function recorder:line(x1, y1, x2, y2, color)
		add(self.commands, make_command("line", {
			x1 = x1,
			y1 = y1,
			x2 = x2,
			y2 = y2,
			color = color,
		}))
	end

	function recorder:rect(x1, y1, x2, y2, color)
		add(self.commands, make_command("rect", {
			x1 = x1,
			y1 = y1,
			x2 = x2,
			y2 = y2,
			color = color,
		}))
	end

	function recorder:rectfill(x1, y1, x2, y2, color)
		add(self.commands, make_command("rectfill", {
			x1 = x1,
			y1 = y1,
			x2 = x2,
			y2 = y2,
			color = color,
		}))
	end

	function recorder:circ(x, y, r, color)
		add(self.commands, make_command("circ", {
			x = x,
			y = y,
			r = r,
			color = color,
		}))
	end

	function recorder:circfill(x, y, r, color)
		add(self.commands, make_command("circfill", {
			x = x,
			y = y,
			r = r,
			color = color,
		}))
	end

	function recorder:print(text, x, y, color)
		add(self.commands, make_command("print", {
			text = text,
			x = x,
			y = y,
			color = color,
		}))
	end

	function recorder:spr(sprite_id, x, y, flip_x, flip_y)
		add(self.commands, make_command("spr", {
			sprite_id = sprite_id,
			x = x,
			y = y,
			flip_x = flip_x,
			flip_y = flip_y,
		}))
	end

	function recorder:pset(x, y, color)
		add(self.commands, make_command("pset", {
			x = x,
			y = y,
			color = color,
		}))
	end

	function recorder:ditheredGradientCommand(x, y, w, h, color1, color2, pattern)
		add(self.commands, make_command("dithered_gradient", {
			x = x,
			y = y,
			w = w,
			h = h,
			color1 = color1,
			color2 = color2,
			pattern = pattern or 0xAA55,
		}))
	end

	return add_rendering_methods(recorder)
end

local function build_immediate_proxy(gfx, base_x, base_y)
	local proxy = {}

	function proxy:line(x1, y1, x2, y2, color)
		gfx:screenLine(base_x + x1, base_y + y1, base_x + x2, base_y + y2, color)
	end

	function proxy:rect(x1, y1, x2, y2, color)
		gfx:screenRect(base_x + x1, base_y + y1, base_x + x2, base_y + y2, color)
	end

	function proxy:rectfill(x1, y1, x2, y2, color)
		gfx:screenRectfill(base_x + x1, base_y + y1, base_x + x2, base_y + y2, color)
	end

	function proxy:circ(x, y, r, color)
		gfx:screenCirc(base_x + x, base_y + y, r, color)
	end

	function proxy:circfill(x, y, r, color)
		gfx:screenCircfill(base_x + x, base_y + y, r, color)
	end

	function proxy:print(text, x, y, color)
		gfx:screenPrint(text, base_x + x, base_y + y, color)
	end

	function proxy:spr(sprite_id, x, y, flip_x, flip_y)
		spr(sprite_id, base_x + x, base_y + y, flip_x, flip_y)
	end

	function proxy:pset(x, y, color)
		pset(base_x + x, base_y + y, color)
	end

	function proxy:ditheredGradientCommand(x, y, w, h, color1, color2, pattern)
		local old_fillp = peek(0x5f34)
		pattern = pattern or 0xAA55
		fillp(pattern)
		gfx:screenRectfill(base_x + x, base_y + y, base_x + x + w - 1, base_y + y + h - 1, color1)
		fillp(0xFFFF - pattern)
		gfx:screenRectfill(base_x + x, base_y + y, base_x + x + w - 1, base_y + y + h - 1, color2)
		fillp(old_fillp)
	end

	return add_rendering_methods(proxy)
end

local function emit_command(target, command, base_x, base_y)
	if command.kind == "line" then
		target:line(base_x + command.x1, base_y + command.y1, base_x + command.x2, base_y + command.y2, command.color)
	elseif command.kind == "rect" then
		target:rect(base_x + command.x1, base_y + command.y1, base_x + command.x2, base_y + command.y2, command.color)
	elseif command.kind == "rectfill" then
		target:rectfill(base_x + command.x1, base_y + command.y1, base_x + command.x2, base_y + command.y2, command.color)
	elseif command.kind == "circ" then
		target:circ(base_x + command.x, base_y + command.y, command.r, command.color)
	elseif command.kind == "circfill" then
		target:circfill(base_x + command.x, base_y + command.y, command.r, command.color)
	elseif command.kind == "print" then
		target:print(command.text, base_x + command.x, base_y + command.y, command.color)
	elseif command.kind == "spr" then
		target:spr(command.sprite_id, base_x + command.x, base_y + command.y, command.flip_x, command.flip_y)
	elseif command.kind == "pset" then
		target:pset(base_x + command.x, base_y + command.y, command.color)
	elseif command.kind == "dithered_gradient" then
		target:ditheredGradientCommand(
			base_x + command.x,
			base_y + command.y,
			command.w,
			command.h,
			command.color1,
			command.color2,
			command.pattern
		)
	end
end

local function emit_command_to_gfx(gfx, command, base_x, base_y)
	if command.kind == "line" then
		gfx:screenLine(base_x + command.x1, base_y + command.y1, base_x + command.x2, base_y + command.y2, command.color)
	elseif command.kind == "rect" then
		gfx:screenRect(base_x + command.x1, base_y + command.y1, base_x + command.x2, base_y + command.y2, command.color)
	elseif command.kind == "rectfill" then
		gfx:screenRectfill(base_x + command.x1, base_y + command.y1, base_x + command.x2, base_y + command.y2, command.color)
	elseif command.kind == "circ" then
		gfx:screenCirc(base_x + command.x, base_y + command.y, command.r, command.color)
	elseif command.kind == "circfill" then
		gfx:screenCircfill(base_x + command.x, base_y + command.y, command.r, command.color)
	elseif command.kind == "print" then
		gfx:screenPrint(command.text, base_x + command.x, base_y + command.y, command.color)
	elseif command.kind == "spr" then
		spr(command.sprite_id, base_x + command.x, base_y + command.y, command.flip_x, command.flip_y)
	elseif command.kind == "pset" then
		pset(base_x + command.x, base_y + command.y, command.color)
	elseif command.kind == "dithered_gradient" then
		local old_fillp = peek(0x5f34)
		local pattern = command.pattern or 0xAA55
		fillp(pattern)
		gfx:screenRectfill(base_x + command.x, base_y + command.y, base_x + command.x + command.w - 1, base_y + command.y + command.h - 1, command.color1)
		fillp(0xFFFF - pattern)
		gfx:screenRectfill(base_x + command.x, base_y + command.y, base_x + command.x + command.w - 1, base_y + command.y + command.h - 1, command.color2)
		fillp(old_fillp)
	end
end

local function replay_commands(gfx, commands, base_x, base_y)
	for i = 1, #commands do
		emit_command_to_gfx(gfx, commands[i], base_x, base_y)
	end
end

local function append_offset_commands(out, commands, offset_x, offset_y)
	offset_x = offset_x or 0
	offset_y = offset_y or 0
	for i = 1, #commands do
		local command = commands[i]
		local shifted = {}
		for key, value in pairs(command) do
			shifted[key] = value
		end
		if shifted.x ~= nil then
			shifted.x += offset_x
		end
		if shifted.y ~= nil then
			shifted.y += offset_y
		end
		if shifted.x1 ~= nil then
			shifted.x1 += offset_x
		end
		if shifted.y1 ~= nil then
			shifted.y1 += offset_y
		end
		if shifted.x2 ~= nil then
			shifted.x2 += offset_x
		end
		if shifted.y2 ~= nil then
			shifted.y2 += offset_y
		end
		add(out, shifted)
	end
end

local function record_cache_counter(gfx, name, amount)
	if gfx.profiler and gfx.profiler.addCounter then
		gfx.profiler:addCounter(name, amount or 1)
	end
end

local function record_cache_observe(gfx, name, value)
	if gfx.profiler and gfx.profiler.observe then
		gfx.profiler:observe(name, value)
	end
end

local function render_to_surface(gfx, surface, builder, clear_color)
	local previous_target = get_draw_target and get_draw_target() or nil
	set_draw_target(surface)
	cls(clear_color or 0)
	builder(build_immediate_proxy(gfx, 0, 0))
	if previous_target ~= nil then
		set_draw_target(previous_target)
	else
		set_draw_target()
	end
end

GraphicsCache.clearRenderCache = function(gfx)
	gfx.render_cache = {}
end

GraphicsCache.setRenderCacheEnabled = function(gfx, enabled)
	gfx.cache_renders = enabled == true
end

GraphicsCache.getCachedRender = function(gfx, key)
	gfx.render_cache = gfx.render_cache or {}
	return gfx.render_cache[key]
end

GraphicsCache.ensureCachedRender = function(gfx, key, builder, options)
	options = options or {}
	if not gfx.cache_renders then
		return nil
	end

	gfx.render_cache = gfx.render_cache or {}
	local entry = gfx.render_cache[key]
	local width = options.w
	local height = options.h
	local should_rebuild = entry == nil
		or entry.w ~= width
		or entry.h ~= height
		or options.rebuild == true

	if should_rebuild then
		if entry == nil then
			record_cache_counter(gfx, "render.cache.misses", 1)
		else
			record_cache_counter(gfx, "render.cache.invalidations", 1)
		end
		local recorder = build_recorder()
		builder(recorder)
		entry = {
			w = width,
			h = height,
			commands = recorder.commands,
		}
		gfx.render_cache[key] = entry
		record_cache_counter(gfx, "render.cache.rebuilds", 1)
		record_cache_observe(gfx, "render.cache.commands_per_entry", #entry.commands)
	else
		record_cache_counter(gfx, "render.cache.hits", 1)
	end

	return entry
end

GraphicsCache.ensureCachedSurface = function(gfx, key, builder, options)
	options = options or {}
	if not gfx.cache_renders then
		return nil
	end

	local width = options.w or 0
	local height = options.h or 0
	if width <= 0 or height <= 0 then
		return nil
	end

	gfx.render_cache = gfx.render_cache or {}
	local entry = gfx.render_cache[key]
	local should_rebuild = entry == nil
		or entry.w ~= width
		or entry.h ~= height
		or entry.surface == nil
		or options.rebuild == true

	if should_rebuild then
		if entry == nil then
			record_cache_counter(gfx, "render.cache.misses", 1)
		else
			record_cache_counter(gfx, "render.cache.invalidations", 1)
		end
		local surface = entry and entry.surface or nil
		if surface == nil or entry.w ~= width or entry.h ~= height then
			surface = userdata("u8", width, height)
		end
		render_to_surface(gfx, surface, builder, options.clear_color or 0)
		entry = {
			w = width,
			h = height,
			surface = surface,
		}
		gfx.render_cache[key] = entry
		record_cache_counter(gfx, "render.cache.rebuilds", 1)
		record_cache_counter(gfx, "render.cache.surface_rebuilds", 1)
		record_cache_observe(gfx, "render.cache.surface_area", width * height)
	else
		record_cache_counter(gfx, "render.cache.hits", 1)
		record_cache_counter(gfx, "render.cache.surface_hits", 1)
	end

	return entry
end

GraphicsCache.buildImmediateTarget = function(gfx, base_x, base_y)
	return build_immediate_proxy(gfx, base_x or 0, base_y or 0)
end

GraphicsCache.recordCommandList = function(gfx, builder)
	local recorder = build_recorder()
	builder(recorder)
	record_cache_counter(gfx, "render.cache.recordings", 1)
	record_cache_observe(gfx, "render.cache.recorded_commands", #recorder.commands)
	return recorder.commands
end

GraphicsCache.replayCommandList = function(gfx, commands, base_x, base_y)
	replay_commands(gfx, commands or {}, base_x or 0, base_y or 0)
	record_cache_counter(gfx, "render.cache.replays", 1)
	record_cache_observe(gfx, "render.cache.replay_commands", #(commands or {}))
end

GraphicsCache.appendOffsetCommands = function(out, commands, offset_x, offset_y)
	append_offset_commands(out, commands or {}, offset_x or 0, offset_y or 0)
end

GraphicsCache.emitCommands = function(target, commands, base_x, base_y)
	for i = 1, #(commands or {}) do
		emit_command(target, commands[i], base_x or 0, base_y or 0)
	end
end

GraphicsCache.drawCachedScreen = function(gfx, key, x, y, builder, options)
	options = options or {}
	if options.cache_mode == "surface" then
		return GraphicsCache.drawCachedSurfaceScreen(gfx, key, x, y, builder, options)
	end
	if not gfx.cache_renders then
		record_cache_counter(gfx, "render.cache.bypass", 1)
		local immediate = build_immediate_proxy(gfx, x, y)
		builder(immediate)
		return nil
	end

	local entry = GraphicsCache.ensureCachedRender(gfx, key, builder, options)
	GraphicsCache.replayCommandList(gfx, entry.commands, x, y)
	return entry
end

GraphicsCache.drawCachedSurfaceScreen = function(gfx, key, x, y, builder, options)
	options = options or {}
	if not gfx.cache_renders then
		record_cache_counter(gfx, "render.cache.bypass", 1)
		builder(build_immediate_proxy(gfx, x, y))
		return nil
	end

	local entry = GraphicsCache.ensureCachedSurface(gfx, key, builder, options)
	if not entry or not entry.surface then
		return nil
	end
	local target = get_draw_target and get_draw_target() or get_display()
	blit(entry.surface, target, 0, 0, x, y, entry.w, entry.h)
	record_cache_counter(gfx, "render.cache.surface_blits", 1)
	record_cache_observe(gfx, "render.cache.surface_blit_area", entry.w * entry.h)
	return entry
end

GraphicsCache.drawCachedLayer = function(gfx, key, layer_x, layer_y, builder, options)
	if not gfx or not gfx.camera then
		return nil
	end
	local sx, sy = gfx.camera:layerToScreenXY(layer_x, layer_y)
	return GraphicsCache.drawCachedScreen(gfx, key, sx, sy, builder, options)
end

GraphicsCache.drawCachedSurfaceLayer = function(gfx, key, layer_x, layer_y, builder, options)
	if not gfx or not gfx.camera then
		return nil
	end
	local sx, sy = gfx.camera:layerToScreenXY(layer_x, layer_y)
	return GraphicsCache.drawCachedSurfaceScreen(gfx, key, sx, sy, builder, options)
end

return GraphicsCache
