local GraphicsCache = {}

local CACHE_MODE_AUTO = 1
local CACHE_MODE_OFF = 2
local CACHE_MODE_ON = 3
local CACHE_MODE_RETAINED = 4
local CACHE_MODE_PANEL = 5

GraphicsCache.MODE_AUTO = CACHE_MODE_AUTO
GraphicsCache.MODE_OFF = CACHE_MODE_OFF
GraphicsCache.MODE_ON = CACHE_MODE_ON
GraphicsCache.MODE_RETAINED = CACHE_MODE_RETAINED
GraphicsCache.MODE_PANEL = CACHE_MODE_PANEL

local function get_cache_mode_label(mode)
	if mode == CACHE_MODE_AUTO then
		return "auto"
	end
	if mode == CACHE_MODE_OFF then
		return "off"
	end
	if mode == CACHE_MODE_RETAINED then
		return "retained"
	end
	if mode == CACHE_MODE_PANEL then
		return "panel"
	end
	return "on"
end

GraphicsCache.getCacheModeLabel = get_cache_mode_label

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

	function recorder:map(tile_x, tile_y, dest_x, dest_y, tiles_w, tiles_h)
		add(self.commands, make_command("map", {
			tile_x = tile_x,
			tile_y = tile_y,
			dest_x = dest_x,
			dest_y = dest_y,
			tiles_w = tiles_w,
			tiles_h = tiles_h,
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

	function proxy:map(tile_x, tile_y, dest_x, dest_y, tiles_w, tiles_h)
		map(tile_x, tile_y, base_x + dest_x, base_y + dest_y, tiles_w, tiles_h)
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
	elseif command.kind == "map" then
		target:map(
			command.tile_x,
			command.tile_y,
			base_x + command.dest_x,
			base_y + command.dest_y,
			command.tiles_w,
			command.tiles_h
		)
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
	elseif command.kind == "map" then
		map(
			command.tile_x,
			command.tile_y,
			base_x + command.dest_x,
			base_y + command.dest_y,
			command.tiles_w,
			command.tiles_h
		)
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
		local kind = command.kind
		local shifted = nil
		if kind == "line" then
			shifted = {
				kind = kind,
				x1 = command.x1 + offset_x,
				y1 = command.y1 + offset_y,
				x2 = command.x2 + offset_x,
				y2 = command.y2 + offset_y,
				color = command.color,
			}
		elseif kind == "rect" or kind == "rectfill" then
			shifted = {
				kind = kind,
				x1 = command.x1 + offset_x,
				y1 = command.y1 + offset_y,
				x2 = command.x2 + offset_x,
				y2 = command.y2 + offset_y,
				color = command.color,
			}
		elseif kind == "circ" or kind == "circfill" then
			shifted = {
				kind = kind,
				x = command.x + offset_x,
				y = command.y + offset_y,
				r = command.r,
				color = command.color,
			}
		elseif kind == "print" then
			shifted = {
				kind = kind,
				text = command.text,
				x = command.x + offset_x,
				y = command.y + offset_y,
				color = command.color,
			}
		elseif kind == "spr" then
			shifted = {
				kind = kind,
				sprite_id = command.sprite_id,
				x = command.x + offset_x,
				y = command.y + offset_y,
				flip_x = command.flip_x,
				flip_y = command.flip_y,
			}
		elseif kind == "map" then
			shifted = {
				kind = kind,
				tile_x = command.tile_x,
				tile_y = command.tile_y,
				dest_x = command.dest_x + offset_x,
				dest_y = command.dest_y + offset_y,
				tiles_w = command.tiles_w,
				tiles_h = command.tiles_h,
			}
		elseif kind == "pset" then
			shifted = {
				kind = kind,
				x = command.x + offset_x,
				y = command.y + offset_y,
				color = command.color,
			}
		elseif kind == "dithered_gradient" then
			shifted = {
				kind = kind,
				x = command.x + offset_x,
				y = command.y + offset_y,
				w = command.w,
				h = command.h,
				color1 = command.color1,
				color2 = command.color2,
				pattern = command.pattern,
			}
		else
			shifted = {}
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

local AUTO_CACHE_MIN_SAMPLES = 8
local AUTO_CACHE_AVERAGE_KEEP = 0.75
local AUTO_CACHE_KEEP_CACHE_MARGIN = 1.05
local AUTO_CACHE_LIVE_ADVANTAGE_MARGIN = 0.9
local AUTO_CACHE_ACTIVE_STALE_MS = 500
local AUTO_CACHE_PROFILE_STALE_MS = 1500
local AUTO_CACHE_PRUNE_INTERVAL_MS = 250
local RENDER_CACHE_DEFAULT_ENTRY_TTL_MS = 15000
local RENDER_CACHE_DEFAULT_MAX_ENTRIES = 512
local RENDER_CACHE_DEFAULT_PRUNE_INTERVAL_MS = 1000

local function measure_cache_cost(fn)
	local started_at = time()
	local started_cpu = stat and stat(1) or nil
	local result = fn()
	local elapsed_ms = max(0, (time() - started_at) * 1000)
	local elapsed_cpu = 0
	local current_cpu = stat and stat(1) or nil
	if current_cpu ~= nil and started_cpu ~= nil then
		elapsed_cpu = max(0, current_cpu - started_cpu)
	end
	return result, elapsed_cpu, elapsed_ms
end

local function get_render_cache_entry_ttl_ms(gfx)
	return max(0, flr((gfx and gfx.render_cache_entry_ttl_ms) or RENDER_CACHE_DEFAULT_ENTRY_TTL_MS))
end

local function get_render_cache_max_entries(gfx)
	return max(0, flr((gfx and gfx.render_cache_max_entries) or RENDER_CACHE_DEFAULT_MAX_ENTRIES))
end

local function get_render_cache_prune_interval_ms(gfx)
	return max(1, flr((gfx and gfx.render_cache_prune_interval_ms) or RENDER_CACHE_DEFAULT_PRUNE_INTERVAL_MS))
end

local function touch_render_cache_entry(entry)
	if not entry then
		return
	end
	local now = time()
	entry.last_used_at = now
	entry.created_at = entry.created_at or now
end

local function remove_render_cache_entry(gfx, cache_key, reason)
	if not gfx or not gfx.render_cache or cache_key == nil then
		return false
	end
	local entry = gfx.render_cache[cache_key]
	if entry == nil then
		return false
	end
	gfx.render_cache[cache_key] = nil
	record_cache_counter(gfx, "render.cache.pruned", 1)
	if reason == "stale" then
		record_cache_counter(gfx, "render.cache.pruned_stale", 1)
	elseif reason == "overflow" then
		record_cache_counter(gfx, "render.cache.pruned_overflow", 1)
	elseif reason == "decision_off" then
		record_cache_counter(gfx, "render.cache.pruned_decision_off", 1)
	end
	if entry.commands ~= nil then
		record_cache_counter(gfx, "render.cache.command_entries_pruned", 1)
	end
	if entry.surface ~= nil then
		record_cache_counter(gfx, "render.cache.surface_entries_pruned", 1)
	end
	return true
end

local function prune_shared_surface_stamp_entries(gfx)
	local shared_entries = gfx and gfx.shared_surface_stamp_entries or nil
	local rotated_entries = gfx and gfx.shared_rotated_surface_stamp_entries or nil
	local render_cache = gfx and gfx.render_cache or nil
	if shared_entries then
		for cache_key, entry in pairs(shared_entries) do
			if entry == false then
				if render_cache == nil or render_cache[cache_key] == nil then
					shared_entries[cache_key] = nil
				end
			elseif render_cache == nil or render_cache[cache_key] ~= entry then
				shared_entries[cache_key] = nil
			end
		end
	end
	if rotated_entries then
		for cache_key, entry in pairs(rotated_entries) do
			if entry == false then
				if render_cache == nil or render_cache[cache_key] == nil then
					rotated_entries[cache_key] = nil
				end
			elseif render_cache == nil or render_cache[cache_key] ~= entry then
				rotated_entries[cache_key] = nil
			end
		end
	end
end

local function prune_render_cache(gfx, force)
	if not gfx then
		return
	end
	gfx.render_cache = gfx.render_cache or {}
	local now = time()
	local prune_interval_ms = get_render_cache_prune_interval_ms(gfx)
	local next_prune_at = gfx.render_cache_next_prune_at or 0
	if force ~= true and now < next_prune_at then
		return
	end
	gfx.render_cache_next_prune_at = now + (prune_interval_ms / 1000)
	record_cache_counter(gfx, "render.cache.prune_passes", 1)

	local ttl_ms = get_render_cache_entry_ttl_ms(gfx)
	local max_entries = get_render_cache_max_entries(gfx)
	local entry_count = 0
	local stale_keys = {}

	for cache_key, entry in pairs(gfx.render_cache) do
		local should_remove = false
		if entry == nil then
			should_remove = true
		else
			local entry_ttl_ms = max(0, flr(entry.entry_ttl_ms or ttl_ms))
			local last_used_at = entry.last_used_at or entry.created_at or now
			if entry_ttl_ms > 0 and max(0, (now - last_used_at) * 1000) >= entry_ttl_ms then
				should_remove = true
			end
		end
		if should_remove then
			add(stale_keys, cache_key)
		else
			entry_count += 1
		end
	end
	for i = 1, #stale_keys do
		remove_render_cache_entry(gfx, stale_keys[i], "stale")
	end

	if max_entries <= 0 then
		local overflow_keys = {}
		for cache_key in pairs(gfx.render_cache) do
			add(overflow_keys, cache_key)
		end
		for i = 1, #overflow_keys do
			remove_render_cache_entry(gfx, overflow_keys[i], "overflow")
		end
		return
	end

	while entry_count > max_entries do
		local oldest_key = nil
		local oldest_time = nil
		for cache_key, entry in pairs(gfx.render_cache) do
			local last_used_at = entry and (entry.last_used_at or entry.created_at) or nil
			if oldest_key == nil or (last_used_at or 0) < (oldest_time or 0) then
				oldest_key = cache_key
				oldest_time = last_used_at or 0
			end
		end
		if oldest_key == nil then
			break
		end
		if remove_render_cache_entry(gfx, oldest_key, "overflow") then
			entry_count -= 1
		else
			break
		end
	end
	prune_shared_surface_stamp_entries(gfx)
end

local function normalize_cache_mode(mode, fallback_mode)
	if mode == nil or mode == true or mode == "default" then
		return fallback_mode
	end
	if mode == false or mode == "off" or mode == "immediate" then
		return CACHE_MODE_OFF
	end
	if mode == "auto" then
		return CACHE_MODE_AUTO
	end
	if mode == "retained" then
		return CACHE_MODE_RETAINED
	end
	if mode == "panel" then
		return CACHE_MODE_PANEL
	end
	if mode == "enabled" or mode == "on" or mode == "surface" or mode == "cache" then
		return CACHE_MODE_ON
	end
	if mode == CACHE_MODE_AUTO or mode == CACHE_MODE_OFF or mode == CACHE_MODE_ON or
		mode == CACHE_MODE_RETAINED or mode == CACHE_MODE_PANEL then
		return mode
	end
	return fallback_mode
end

local function get_cache_tag_mode(gfx, tag)
	local cache_tag_modes = gfx and gfx.cache_tag_modes or nil
	if not cache_tag_modes or not tag or tag == "" then
		return nil
	end
	return cache_tag_modes[tag]
end

local function normalize_global_cache_mode(mode, fallback_mode)
	if mode == nil or mode == true or mode == "default" then
		return fallback_mode or CACHE_MODE_ON
	end
	if mode == false then
		return CACHE_MODE_OFF
	end
	return normalize_cache_mode(mode, fallback_mode or CACHE_MODE_ON)
end

local function get_global_cache_mode(gfx)
	local fallback_mode = gfx and gfx.cache_renders == true and CACHE_MODE_ON or CACHE_MODE_OFF
	return normalize_global_cache_mode(gfx and gfx.render_cache_mode or nil, fallback_mode)
end

local function resolve_requested_cache_mode(gfx, options, fallback_mode)
	options = options or {}
	local default_mode = normalize_cache_mode(options.cache_mode, fallback_mode or CACHE_MODE_ON)
	local resolved_mode = normalize_cache_mode(options.cache_override_mode, default_mode)
	if resolved_mode == default_mode then
		local tag_mode = normalize_cache_mode(get_cache_tag_mode(gfx, options.cache_tag), default_mode)
		if tag_mode ~= nil then
			resolved_mode = tag_mode
		end
	end
	if resolved_mode ~= CACHE_MODE_OFF and resolved_mode ~= CACHE_MODE_AUTO then
		resolved_mode = CACHE_MODE_ON
	end
	return normalize_cache_mode(resolved_mode, default_mode)
end

local function resolve_cache_mode(gfx, options, fallback_mode)
	local requested_mode = resolve_requested_cache_mode(gfx, options, fallback_mode)
	if requested_mode == CACHE_MODE_OFF then
		return CACHE_MODE_OFF
	end
	if options and options.force_cache == true then
		return requested_mode
	end
	if get_global_cache_mode(gfx) == CACHE_MODE_OFF then
		return CACHE_MODE_OFF
	end
	return requested_mode
end

local function get_cache_profile_key_value(options)
	if options and options.cache_profile_key ~= nil then
		return tostr(options.cache_profile_key)
	end
	return nil
end

local function get_cache_profile_signature_value(options)
	if options and options.cache_profile_signature ~= nil then
		return tostr(options.cache_profile_signature)
	end
	return nil
end

local function build_shared_cache_key_from_options(options)
	local profile_key = get_cache_profile_key_value(options)
	if profile_key == nil then
		return nil
	end
	local signature = get_cache_profile_signature_value(options)
	if signature == nil or signature == "" then
		return profile_key
	end
	return profile_key .. ":" .. signature
end

local function resolve_cache_key(key, options)
	if type(key) == "function" then
		return key()
	end
	if key ~= nil then
		return key
	end
	if options and options.cache_key_builder then
		return options.cache_key_builder()
	end
	local shared_key = build_shared_cache_key_from_options(options)
	if shared_key ~= nil then
		return shared_key
	end
	return "anonymous"
end

local function starts_with(value, prefix)
	if value == nil or prefix == nil then
		return false
	end
	local text = tostr(value)
	return text:sub(1, #prefix) == prefix
end

local function get_dimension_bucket(value)
	local size = max(0, flr(value or 0))
	if size <= 0 then
		return "0"
	end
	if size <= 8 then
		return "xs"
	end
	if size <= 16 then
		return "s"
	end
	if size <= 32 then
		return "m"
	end
	if size <= 64 then
		return "l"
	end
	if size <= 128 then
		return "xl"
	end
	return "xxl"
end

local function get_size_bucket(options)
	options = options or {}
	return table.concat({
		get_dimension_bucket(options.w or 0),
		get_dimension_bucket(options.h or 0),
	}, "x")
end

local function infer_auto_profile_bucket(options)
	options = options or {}
	if options.cache_auto_profile_key ~= nil then
		return tostr(options.cache_auto_profile_key)
	end

	local tag = tostr(options.cache_tag or "")
	if tag == "debug.tile_labels" or tag == "debug.labels" then
		return "primitive.text"
	end
	if tag == "debug.summary" then
		return "primitive.text_block"
	end
	if tag == "layer.map" then
		return "primitive.map"
	end
	if tag == "debug.grid" then
		return "primitive.grid"
	end
	if tag == "debug.guides" then
		return "primitive.path"
	end
	if tag == "debug.raycast" or tag == "debug.collision" or tag == "debug.velocity" then
		return "primitive.vector"
	end
	if starts_with(tag, "debug.shapes.") or starts_with(tag, "shape.") or tag == "debug.collider.shape"
		or tag == "debug.collider.overlay" then
		return "primitive.shape"
	end
	if starts_with(tag, "node.sprite") or starts_with(tag, "particle.") then
		return "primitive.sprite"
	end
	if starts_with(tag, "panel.") or tag == "debug.summary.panel" then
		return "panel"
	end
	if starts_with(tag, "retained.") then
		return "retained"
	end
	if starts_with(tag, "procedural_sprite.") or tag == "background.skybox" then
		return "procedural.dynamic"
	end
	return "surface.generic"
end

local function draw_immediate_builder(gfx, x, y, builder)
	local immediate = build_immediate_proxy(gfx, x or 0, y or 0)
	builder(immediate)
end

local function estimate_text_size(text)
	local safe_text = text and tostr(text) or ""
	return max(1, #safe_text * 4 + 2), 6
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

local function get_snapped_rotation_bucket(angle, bucket_count)
	bucket_count = max(1, flr(bucket_count or 1))
	if bucket_count <= 1 then
		return 0, 0
	end
	local tau = 6.283185307179586
	local normalized = ((angle or 0) % tau) / tau
	local bucket = flr(normalized * bucket_count + 0.5) % bucket_count
	return bucket, (bucket / bucket_count) * tau
end

local function get_rotated_surface_size(width, height, angle)
	local cos_a = cos(angle or 0)
	local sin_a = sin(angle or 0)
	local rotated_w = max(1, ceil(abs(width * cos_a) + abs(height * sin_a)))
	local rotated_h = max(1, ceil(abs(width * sin_a) + abs(height * cos_a)))
	return rotated_w, rotated_h
end

local function rotate_surface_nearest(dest, src, angle)
	local src_w = src:width()
	local src_h = src:height()
	local dest_w = dest:width()
	local dest_h = dest:height()
	local cos_a = cos(angle or 0)
	local sin_a = sin(angle or 0)
	local src_cx = (src_w - 1) * 0.5
	local src_cy = (src_h - 1) * 0.5
	local dest_cx = (dest_w - 1) * 0.5
	local dest_cy = (dest_h - 1) * 0.5
	for dy = 0, dest_h - 1 do
		local rel_y = dy - dest_cy
		for dx = 0, dest_w - 1 do
			local rel_x = dx - dest_cx
			local src_rel_x = rel_x * cos_a + rel_y * sin_a
			local src_rel_y = -rel_x * sin_a + rel_y * cos_a
			local sx = flr(src_rel_x + src_cx + 0.5)
			local sy = flr(src_rel_y + src_cy + 0.5)
			if sx >= 0 and sx < src_w and sy >= 0 and sy < src_h then
				dest:set(dx, dy, src:get(sx, sy, 1))
			else
				dest:set(dx, dy, 0)
			end
		end
	end
end

local function get_auto_profile_key(bucket)
	return tostr(bucket or "surface.generic")
end

local function get_auto_profile_signature(options, bucket)
	options = options or {}
	if options.cache_auto_profile_signature ~= nil then
		return tostr(options.cache_auto_profile_signature)
	end
	if bucket == "panel" or bucket == "retained" or bucket == "procedural.dynamic"
		or bucket == "surface.generic" then
		if options.cache_profile_signature ~= nil then
			return tostr(options.cache_profile_signature)
		end
	end
	return get_size_bucket(options)
end

local function get_auto_profile_remaining_samples(profile)
	if not profile or profile.decision ~= nil then
		return 0
	end
	return
		max(0, AUTO_CACHE_MIN_SAMPLES - (profile.immediate_samples or 0)) +
		max(0, AUTO_CACHE_MIN_SAMPLES - (profile.cached_samples or 0))
end

local function touch_auto_profile(profile)
	if not profile then
		return
	end
	profile.last_seen_at = time()
end

local clear_active_auto_profile

local function prune_auto_profiles(gfx)
	if not gfx then
		return
	end
	local now = time()
	local next_prune_at = gfx.auto_cache_next_prune_at or 0
	if now < next_prune_at then
		return
	end
	gfx.auto_cache_next_prune_at = now + (AUTO_CACHE_PRUNE_INTERVAL_MS / 1000)
	local active_key = gfx.auto_cache_active_profile_key
	local profiles = gfx.auto_cache_profiles or {}
	for profile_key, profile in pairs(profiles) do
		local last_seen_at = profile and profile.last_seen_at or nil
		local is_stale = profile
			and profile.decision == nil
			and last_seen_at ~= nil
			and max(0, (now - last_seen_at) * 1000) >= AUTO_CACHE_PROFILE_STALE_MS
		if is_stale then
			profiles[profile_key] = nil
			record_cache_counter(gfx, "render.cache.auto.pending_pruned", 1)
			if active_key == profile_key then
				clear_active_auto_profile(gfx, profile_key)
			end
		end
	end
end

local function get_auto_queue_snapshot(gfx)
	prune_auto_profiles(gfx)
	local snapshot = {
		total = 0,
		completed = 0,
		pending = 0,
		waiting = 0,
		active = 0,
		pending_samples = 0,
		active_samples = 0,
		active_key = nil,
		active_bucket = nil,
	}
	if not gfx then
		return snapshot
	end
	local active_key = gfx.auto_cache_active_profile_key
	for profile_key, profile in pairs(gfx.auto_cache_profiles or {}) do
		if profile then
			snapshot.total += 1
			if profile.decision ~= nil then
				snapshot.completed += 1
			else
				local remaining_samples = get_auto_profile_remaining_samples(profile)
				snapshot.pending += 1
				snapshot.pending_samples += remaining_samples
				if active_key ~= nil and profile_key == active_key then
					snapshot.active += 1
					snapshot.active_samples += remaining_samples
					snapshot.active_key = profile.display_key or profile.key or "anonymous"
					snapshot.active_bucket = profile.bucket or profile.display_key or "anonymous"
				end
			end
		end
	end
	snapshot.waiting = max(0, snapshot.pending - snapshot.active)
	return snapshot
end

local function get_auto_profile(gfx, options)
	if not gfx then
		return nil
	end
	prune_auto_profiles(gfx)
	gfx.auto_cache_profiles = gfx.auto_cache_profiles or {}
	local bucket = infer_auto_profile_bucket(options)
	local signature = get_auto_profile_signature(options, bucket)
	local profile_key = table.concat({
		"draw",
		get_auto_profile_key(bucket),
		"sig:" .. signature,
	}, ":")
	local profile = gfx.auto_cache_profiles[profile_key]
	if not profile then
		profile = {
			key = profile_key,
			display_key = bucket,
			cache_tag = options.cache_tag or "-",
			family = "draw",
			bucket = bucket,
			signature = signature,
			immediate_cpu = nil,
			immediate_ms = nil,
			cached_cpu = nil,
			cached_ms = nil,
			immediate_samples = 0,
			cached_samples = 0,
			decision = nil,
			decision_noted = false,
			decision_serial = 0,
			cpu_delta = 0,
			ms_delta = 0,
			last_warmup_sample = nil,
		}
		gfx.auto_cache_profiles[profile_key] = profile
	end
	touch_auto_profile(profile)
	return profile
end

local function should_use_live_fast_path(gfx, options)
	if not gfx then
		return false
	end
	local resolved_mode = resolve_cache_mode(gfx, options, CACHE_MODE_ON)
	if resolved_mode == CACHE_MODE_OFF then
		return true
	end
	if get_global_cache_mode(gfx) ~= CACHE_MODE_AUTO then
		return false
	end
	local profile = get_auto_profile(gfx, options)
	return profile ~= nil and profile.decision == "off"
end

local function set_auto_decision(profile, decision)
	if not profile then
		return decision
	end
	if profile.decision ~= decision then
		profile.decision = decision
		profile.decision_noted = false
	end
	return profile.decision
end

local function choose_auto_decision(profile)
	if not profile then
		return "surface"
	end
	if (profile.immediate_samples or 0) < AUTO_CACHE_MIN_SAMPLES
		or (profile.cached_samples or 0) < AUTO_CACHE_MIN_SAMPLES then
		return nil
	end
	if profile.immediate_cpu == nil or profile.cached_cpu == nil then
		return nil
	end
	local immediate_score = profile.immediate_cpu * 1000 + (profile.immediate_ms or 0) * 0.01
	local cached_score = profile.cached_cpu * 1000 + (profile.cached_ms or 0) * 0.01
	if cached_score <= immediate_score * AUTO_CACHE_KEEP_CACHE_MARGIN then
		return set_auto_decision(profile, "surface")
	end
	if immediate_score <= cached_score * AUTO_CACHE_LIVE_ADVANTAGE_MARGIN then
		return set_auto_decision(profile, "off")
	end
	return set_auto_decision(profile, "surface")
end

local function update_auto_average(profile, sample_kind, cpu_cost, ms_cost)
	if not profile then
		return
	end
	local count_key = sample_kind .. "_samples"
	local cpu_key = sample_kind .. "_cpu"
	local ms_key = sample_kind .. "_ms"
	local sample_count = profile[count_key] or 0
	if sample_count <= 0 or profile[cpu_key] == nil then
		profile[cpu_key] = cpu_cost or 0
		profile[ms_key] = ms_cost or 0
	else
		profile[cpu_key] = (profile[cpu_key] or 0) * AUTO_CACHE_AVERAGE_KEEP + (cpu_cost or 0) * (1 - AUTO_CACHE_AVERAGE_KEEP)
		profile[ms_key] = (profile[ms_key] or 0) * AUTO_CACHE_AVERAGE_KEEP + (ms_cost or 0) * (1 - AUTO_CACHE_AVERAGE_KEEP)
	end
	profile[count_key] = sample_count + 1
end

local function get_auto_sample_mode(profile)
	if not profile then
		return "surface"
	end
	if profile.decision ~= nil then
		if profile.decision == "off" then
			return "immediate"
		end
		return "surface"
	end
	local immediate_samples = profile.immediate_samples or 0
	local cached_samples = profile.cached_samples or 0
	if immediate_samples < cached_samples then
		profile.last_warmup_sample = "immediate"
		return "immediate"
	end
	if cached_samples < immediate_samples then
		profile.last_warmup_sample = "surface"
		return "surface"
	end
	if profile.last_warmup_sample == "immediate" then
		profile.last_warmup_sample = "surface"
		return "surface"
	end
	profile.last_warmup_sample = "immediate"
	return "immediate"
end

local function insert_recent_auto_entry(entries, candidate, limit)
	limit = limit or 5
	local insert_at = #entries + 1
	for i = 1, #entries do
		if (candidate.serial or 0) > (entries[i].serial or 0) then
			insert_at = i
			break
		end
	end
	add(entries, candidate, insert_at)
	while #entries > limit do
		deli(entries, #entries)
	end
end

clear_active_auto_profile = function(gfx, profile_key)
	if not gfx then
		return
	end
	if profile_key == nil or gfx.auto_cache_active_profile_key == profile_key then
		gfx.auto_cache_active_profile_key = nil
		gfx.auto_cache_active_profile_seen_at = nil
	end
end

local function expire_stale_active_auto_profile(gfx)
	if not gfx or gfx.auto_cache_active_profile_key == nil then
		return
	end
	local seen_at = gfx.auto_cache_active_profile_seen_at
	if seen_at == nil then
		clear_active_auto_profile(gfx)
		return
	end
	local elapsed_ms = max(0, (time() - seen_at) * 1000)
	if elapsed_ms >= AUTO_CACHE_ACTIVE_STALE_MS then
		clear_active_auto_profile(gfx)
		record_cache_counter(gfx, "render.cache.auto.stale_releases", 1)
	end
end

local function claim_auto_profile_sampling(gfx, profile)
	if not gfx or not profile then
		return true
	end
	touch_auto_profile(profile)
	expire_stale_active_auto_profile(gfx)
	local active_key = gfx.auto_cache_active_profile_key
	if active_key == nil or active_key == profile.key then
		gfx.auto_cache_active_profile_key = profile.key
		gfx.auto_cache_active_profile_seen_at = time()
		return true
	end
	record_cache_counter(gfx, "render.cache.auto.deferred", 1)
	return false
end

local function note_auto_decision(gfx, profile)
	if not gfx or not profile or profile.decision == nil or profile.decision_noted == true then
		return profile and profile.decision or nil
	end
	touch_auto_profile(profile)
	profile.decision_noted = true
	gfx.auto_cache_decision_serial = (gfx.auto_cache_decision_serial or 0) + 1
	profile.decision_serial = gfx.auto_cache_decision_serial
	profile.cpu_delta = (profile.immediate_cpu or 0) - (profile.cached_cpu or 0)
	profile.ms_delta = (profile.immediate_ms or 0) - (profile.cached_ms or 0)
	record_cache_counter(gfx, "render.cache.auto.decisions", 1)
	record_cache_counter(
		gfx,
		profile.decision == "off" and "render.cache.auto.choose_immediate" or "render.cache.auto.choose_cached",
		1
	)
	record_cache_observe(gfx, "render.cache.auto.delta_cpu_signed", profile.cpu_delta)
	record_cache_observe(gfx, "render.cache.auto.delta_ms_signed", profile.ms_delta)
	record_cache_observe(gfx, "render.cache.auto.delta_cpu_abs", abs(profile.cpu_delta))
	record_cache_observe(gfx, "render.cache.auto.delta_ms_abs", abs(profile.ms_delta))
	gfx.auto_cache_recent_decisions = gfx.auto_cache_recent_decisions or {}
	insert_recent_auto_entry(gfx.auto_cache_recent_decisions, {
		serial = profile.decision_serial or 0,
		choice = profile.decision == "off" and "live" or "cache",
		mode = profile.decision == "off" and "live" or profile.decision,
		tag = profile.cache_tag or "-",
		key = profile.display_key or "anonymous",
		cpu_delta = profile.cpu_delta or 0,
		ms_delta = profile.ms_delta or 0,
		immediate_samples = profile.immediate_samples or 0,
		cached_samples = profile.cached_samples or 0,
		immediate_cpu = profile.immediate_cpu or 0,
		cached_cpu = profile.cached_cpu or 0,
		immediate_ms = profile.immediate_ms or 0,
		cached_ms = profile.cached_ms or 0,
		family = profile.family or "render",
	}, 8)
	clear_active_auto_profile(gfx, profile.key)
	return profile.decision
end

local function record_auto_sample(gfx, profile, sample_kind, cpu_cost, ms_cost)
	if not gfx or not profile then
		return
	end
	touch_auto_profile(profile)
	if sample_kind == "immediate" then
		record_cache_counter(gfx, "render.cache.auto.sample_immediate", 1)
		record_cache_observe(gfx, "render.cache.auto.immediate_cpu", cpu_cost or 0)
	else
		record_cache_counter(gfx, "render.cache.auto.sample_cached", 1)
		record_cache_observe(gfx, "render.cache.auto.cached_cpu", cpu_cost or 0)
	end
	update_auto_average(profile, sample_kind, cpu_cost or 0, ms_cost or 0)
	choose_auto_decision(profile)
	if profile.decision == nil then
		gfx.auto_cache_active_profile_key = profile.key
		gfx.auto_cache_active_profile_seen_at = time()
	end
	note_auto_decision(gfx, profile)
end

local function build_immediate_entry(gfx, builder, options)
	local recorder = build_recorder()
	builder(recorder)
	record_cache_counter(gfx, "render.cache.bypass", 1)
	record_cache_counter(gfx, "render.cache.rebuilds", 1)
	record_cache_observe(gfx, "render.cache.commands_per_entry", #recorder.commands)
	return {
		w = options and options.w or nil,
		h = options and options.h or nil,
		commands = recorder.commands,
	}
end

local function draw_cached_surface_mode(gfx, key, x, y, builder, options)
	local cache_options = {}
	for opt_key, opt_value in pairs(options or {}) do
		cache_options[opt_key] = opt_value
	end
	cache_options.force_cache = true
	local entry = GraphicsCache.ensureCachedSurface(gfx, key, builder, cache_options)
	if not entry or not entry.surface then
		return nil
	end
	spr(entry.surface, x, y)
	record_cache_counter(gfx, "render.cache.surface_blits", 1)
	record_cache_observe(gfx, "render.cache.surface_blit_area", entry.w * entry.h)
	return entry
end

local function draw_shared_surface_stamp_mode(gfx, key, x, y, builder, options)
	options = options or {}
	local cache_key = resolve_cache_key(key, options)
	gfx.shared_surface_stamp_entries = gfx.shared_surface_stamp_entries or {}
	local entry = gfx.shared_surface_stamp_entries[cache_key]
	if entry == nil or (gfx.render_cache and gfx.render_cache[cache_key] ~= entry) then
		local cache_options = {}
		for opt_key, opt_value in pairs(options) do
			cache_options[opt_key] = opt_value
		end
		cache_options.force_cache = true
		entry = GraphicsCache.ensureCachedSurface(gfx, cache_key, builder, cache_options)
		gfx.shared_surface_stamp_entries[cache_key] = entry or false
	end
	if entry == false or entry == nil or entry.surface == nil then
		record_cache_counter(gfx, "render.cache.bypass", 1)
		draw_immediate_builder(gfx, x, y, builder)
		return nil
	end
	touch_render_cache_entry(entry)
	spr(entry.surface, x, y)
	record_cache_counter(gfx, "render.cache.surface_blits", 1)
	record_cache_counter(gfx, "render.cache.shared_stamp_blits", 1)
	record_cache_observe(gfx, "render.cache.surface_blit_area", entry.w * entry.h)
	return entry
end

local function ensure_rotated_surface_entry(gfx, base_key, builder, options)
	options = options or {}
	local source_w = max(1, flr(options.w or 0))
	local source_h = max(1, flr(options.h or 0))
	local bucket_count = max(1, flr(options.rotation_bucket_count or 1))
	local bucket_index, snapped_angle = get_snapped_rotation_bucket(options.rotation_angle or 0, bucket_count)
	local resolved_base_key = resolve_cache_key(base_key, options)
	gfx.shared_surface_stamp_entries = gfx.shared_surface_stamp_entries or {}
	local base_entry = gfx.shared_surface_stamp_entries[resolved_base_key]
	if base_entry == nil or (gfx.render_cache and gfx.render_cache[resolved_base_key] ~= base_entry) then
		base_entry = GraphicsCache.ensureCachedSurface(gfx, resolved_base_key, builder, options)
		gfx.shared_surface_stamp_entries[resolved_base_key] = base_entry or false
	end
	if not base_entry or not base_entry.surface then
		return nil
	end
	if bucket_count <= 1 or bucket_index == 0 then
		return {
			entry = base_entry,
			draw_offset_x = 0,
			draw_offset_y = 0,
			rotation_bucket_index = 0,
			rotation_angle = 0,
		}
	end
	local rotated_w, rotated_h = get_rotated_surface_size(source_w, source_h, snapped_angle)
	local rotated_key = table.concat({
		resolved_base_key,
		"rot",
		tostr(bucket_count),
		tostr(bucket_index),
		tostr(rotated_w),
		tostr(rotated_h),
	}, ":")
	gfx.shared_rotated_surface_stamp_entries = gfx.shared_rotated_surface_stamp_entries or {}
	local rotated_entry = gfx.shared_rotated_surface_stamp_entries[rotated_key]
	if rotated_entry == nil or (gfx.render_cache and gfx.render_cache[rotated_key] ~= rotated_entry) then
		local rotated_entry_ttl_ms = max(
			0,
			flr(options.rotation_entry_ttl_ms or options.entry_ttl_ms or get_render_cache_entry_ttl_ms(gfx))
		)
		rotated_entry = GraphicsCache.ensureCachedSurface(gfx, rotated_key, function(_)
			return
		end, {
			w = rotated_w,
			h = rotated_h,
			force_cache = true,
			cache_tag = options.cache_tag,
			cache_profile_key = options.cache_profile_key and (options.cache_profile_key .. ":rotation") or nil,
			cache_profile_signature = table.concat({
				tostr(options.cache_profile_signature or ""),
				tostr(bucket_count),
				tostr(bucket_index),
				tostr(rotated_w),
				tostr(rotated_h),
			}, ":"),
			entry_ttl_ms = rotated_entry_ttl_ms,
			rebuild = options.rebuild == true,
		})
		gfx.shared_rotated_surface_stamp_entries[rotated_key] = rotated_entry or false
	end
	if not rotated_entry or not rotated_entry.surface then
		return {
			entry = base_entry,
			draw_offset_x = 0,
			draw_offset_y = 0,
			rotation_bucket_index = 0,
			rotation_angle = 0,
		}
	end
	local source_created_at = base_entry.created_at or 0
	local should_rebuild = options.rebuild == true
		or rotated_entry._rotation_source_key ~= resolved_base_key
		or rotated_entry._rotation_source_created_at ~= source_created_at
		or rotated_entry._rotation_bucket_index ~= bucket_index
	if should_rebuild then
		rotate_surface_nearest(rotated_entry.surface, base_entry.surface, snapped_angle)
		rotated_entry._rotation_source_key = resolved_base_key
		rotated_entry._rotation_source_created_at = source_created_at
		rotated_entry._rotation_bucket_index = bucket_index
		rotated_entry._rotation_bucket_count = bucket_count
	end
	return {
		entry = rotated_entry,
		draw_offset_x = flr(source_w * 0.5 - rotated_w * 0.5),
		draw_offset_y = flr(source_h * 0.5 - rotated_h * 0.5),
		rotation_bucket_index = bucket_index,
		rotation_angle = snapped_angle,
	}
end

GraphicsCache.clearRenderCache = function(gfx)
	gfx.render_cache = {}
	gfx.shared_surface_stamp_entries = {}
	gfx.shared_rotated_surface_stamp_entries = {}
	gfx.render_cache_next_prune_at = nil
	gfx.auto_cache_profiles = {}
	gfx.auto_cache_decision_serial = 0
	gfx.auto_cache_recent_decisions = {}
	gfx.auto_cache_active_profile_key = nil
	gfx.auto_cache_active_profile_seen_at = nil
	gfx.auto_cache_next_prune_at = nil
end

GraphicsCache.setRenderCacheEnabled = function(gfx, enabled)
	if enabled == true then
		gfx:setRenderCacheMode(CACHE_MODE_ON)
	else
		gfx:setRenderCacheMode(CACHE_MODE_OFF)
	end
end

GraphicsCache.setRenderCacheMode = function(gfx, mode)
	local resolved_mode = normalize_global_cache_mode(mode, CACHE_MODE_ON)
	gfx.render_cache_mode = resolved_mode
	gfx.cache_renders = resolved_mode ~= CACHE_MODE_OFF
end

GraphicsCache.resolveCacheMode = function(gfx, options, fallback_mode)
	return resolve_cache_mode(gfx, options, fallback_mode)
end

GraphicsCache.setCacheTagMode = function(gfx, tag, mode)
	gfx.cache_tag_modes = gfx.cache_tag_modes or {}
	gfx.cache_tag_modes[tag] = mode
end

GraphicsCache.clearCacheTagMode = function(gfx, tag)
	if gfx.cache_tag_modes then
		gfx.cache_tag_modes[tag] = nil
	end
end

GraphicsCache.getCachedRender = function(gfx, key)
	gfx.render_cache = gfx.render_cache or {}
	local entry = gfx.render_cache[resolve_cache_key(key)]
	touch_render_cache_entry(entry)
	return entry
end

GraphicsCache.getRenderCacheSnapshot = function(gfx)
	local snapshot = {
		entries = 0,
		command_entries = 0,
		surface_entries = 0,
		total_commands = 0,
		surface_area = 0,
	}
	for _, entry in pairs(gfx and gfx.render_cache or {}) do
		if entry then
			snapshot.entries += 1
			if entry.commands ~= nil then
				snapshot.command_entries += 1
				snapshot.total_commands += #(entry.commands or {})
			end
			if entry.surface ~= nil then
				snapshot.surface_entries += 1
				snapshot.surface_area += max(0, (entry.w or 0) * (entry.h or 0))
			end
		end
	end
	return snapshot
end

GraphicsCache.getAutoCacheDecisionSnapshot = function(gfx, limit)
	local queue = get_auto_queue_snapshot(gfx)
	local snapshot = {
		known = queue.completed or 0,
		cached = 0,
		live = 0,
		total = queue.total or 0,
		completed = queue.completed or 0,
		pending = queue.pending or 0,
		waiting = queue.waiting or 0,
		active = queue.active or 0,
		pending_samples = queue.pending_samples or 0,
		active_samples = queue.active_samples or 0,
		active_key = queue.active_key,
		active_bucket = queue.active_bucket,
		recent = {},
	}
	for _, profile in pairs(gfx.auto_cache_profiles or {}) do
		if profile and profile.decision ~= nil then
			if profile.decision == "off" then
				snapshot.live += 1
			else
				snapshot.cached += 1
			end
		end
	end
	for i = 1, #(gfx.auto_cache_recent_decisions or {}) do
		insert_recent_auto_entry(snapshot.recent, gfx.auto_cache_recent_decisions[i], limit or 5)
	end
	return snapshot
end

GraphicsCache.shouldUseLiveFastPath = function(gfx, options, desired_mode, family)
	return should_use_live_fast_path(gfx, options or {})
end

GraphicsCache.ensureCachedRender = function(gfx, key, builder, options)
	options = options or {}
	record_cache_counter(gfx, "render.cache.command_disabled", 1)
	return build_immediate_entry(gfx, builder, options)
end

GraphicsCache.ensureCachedSurface = function(gfx, key, builder, options)
	options = options or {}
	local cache_key = resolve_cache_key(key, options)
	if resolve_cache_mode(gfx, options, CACHE_MODE_ON) == CACHE_MODE_OFF then
		record_cache_counter(gfx, "render.cache.bypass", 1)
		return nil
	end

	local width = options.w or 0
	local height = options.h or 0
	if width <= 0 or height <= 0 then
		return nil
	end

	gfx.render_cache = gfx.render_cache or {}
	prune_render_cache(gfx)
	local entry = gfx.render_cache[cache_key]
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
			created_at = time(),
			entry_ttl_ms = options.entry_ttl_ms,
		}
		gfx.render_cache[cache_key] = entry
		record_cache_counter(gfx, "render.cache.rebuilds", 1)
		record_cache_counter(gfx, "render.cache.surface_rebuilds", 1)
		record_cache_observe(gfx, "render.cache.surface_area", width * height)
	else
		record_cache_counter(gfx, "render.cache.hits", 1)
		record_cache_counter(gfx, "render.cache.surface_hits", 1)
	end
	touch_render_cache_entry(entry)

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

GraphicsCache.appendCachedCommands = function(gfx, out, key, builder, options)
	options = options or {}
	local entry = GraphicsCache.ensureCachedRender(gfx, key, builder, options)
	if not entry or not entry.commands then
		return nil
	end
	append_offset_commands(out, entry.commands, options.offset_x or 0, options.offset_y or 0)
	return entry
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
	local desired_mode = resolve_requested_cache_mode(gfx, options, CACHE_MODE_ON)
	if desired_mode == CACHE_MODE_OFF then
		record_cache_counter(gfx, "render.cache.bypass", 1)
		draw_immediate_builder(gfx, x, y, builder)
		return nil
	end
	local global_mode = get_global_cache_mode(gfx)
	if options.force_cache ~= true and global_mode == CACHE_MODE_AUTO then
		local profile = get_auto_profile(gfx, options)
		if profile and profile.decision ~= nil then
			note_auto_decision(gfx, profile)
			if profile.decision == "off" then
				record_cache_counter(gfx, "render.cache.bypass", 1)
				draw_immediate_builder(gfx, x, y, builder)
				return nil
			end
			return draw_cached_surface_mode(gfx, key, x, y, builder, options)
		end
		if not claim_auto_profile_sampling(gfx, profile) then
			return draw_cached_surface_mode(gfx, key, x, y, builder, options)
		end
		local sample_mode = get_auto_sample_mode(profile)
		if sample_mode == "immediate" then
			local _, cpu_cost, ms_cost = measure_cache_cost(function()
				draw_immediate_builder(gfx, x, y, builder)
			end)
			record_cache_counter(gfx, "render.cache.bypass", 1)
			record_auto_sample(gfx, profile, "immediate", cpu_cost, ms_cost)
			return nil
		end
		local entry, cpu_cost, ms_cost = measure_cache_cost(function()
			return draw_cached_surface_mode(gfx, key, x, y, builder, options)
		end)
		record_auto_sample(gfx, profile, "cached", cpu_cost, ms_cost)
		if choose_auto_decision(profile) == "off" and gfx.render_cache then
			remove_render_cache_entry(gfx, resolve_cache_key(key, options), "decision_off")
		end
		return entry
	end
	local resolved_mode = resolve_cache_mode(gfx, options, CACHE_MODE_ON)
	if resolved_mode == CACHE_MODE_OFF then
		record_cache_counter(gfx, "render.cache.bypass", 1)
		draw_immediate_builder(gfx, x, y, builder)
		return nil
	end
	return draw_cached_surface_mode(gfx, key, x, y, builder, options)
end

GraphicsCache.drawCachedSurfaceScreen = function(gfx, key, x, y, builder, options)
	return GraphicsCache.drawCachedScreen(gfx, key, x, y, builder, options)
end

GraphicsCache.drawCachedLayer = function(gfx, key, layer_x, layer_y, builder, options)
	if not gfx or not gfx.camera then
		return nil
	end
	local sx, sy = gfx.camera:layerToScreenXY(layer_x, layer_y)
	return GraphicsCache.drawCachedScreen(gfx, key, sx, sy, builder, options)
end

GraphicsCache.drawCachedSurfaceLayer = function(gfx, key, layer_x, layer_y, builder, options)
	return GraphicsCache.drawCachedLayer(gfx, key, layer_x, layer_y, builder, options)
end

GraphicsCache.drawSharedSurfaceStampLayer = function(gfx, key, layer_x, layer_y, builder, options)
	if not gfx or not gfx.camera then
		return nil
	end
	local sx, sy = gfx.camera:layerToScreenXY(layer_x, layer_y)
	return draw_shared_surface_stamp_mode(gfx, key, sx, sy, builder, options)
end

GraphicsCache.drawSharedRotatedSurfaceStampLayer = function(gfx, key, layer_x, layer_y, builder, options)
	if not gfx or not gfx.camera then
		return nil
	end
	options = options or {}
	local bucket_count = max(1, flr(options.rotation_bucket_count or 1))
	if bucket_count <= 1 then
		return GraphicsCache.drawSharedSurfaceStampLayer(gfx, key, layer_x, layer_y, builder, options)
	end
	local rotated = ensure_rotated_surface_entry(gfx, key, builder, options)
	if not rotated or not rotated.entry or not rotated.entry.surface then
		record_cache_counter(gfx, "render.cache.bypass", 1)
		local sx, sy = gfx.camera:layerToScreenXY(layer_x, layer_y)
		draw_immediate_builder(gfx, sx, sy, builder)
		return nil
	end
	local draw_x = layer_x + (rotated.draw_offset_x or 0)
	local draw_y = layer_y + (rotated.draw_offset_y or 0)
	local sx, sy = gfx.camera:layerToScreenXY(draw_x, draw_y)
	touch_render_cache_entry(rotated.entry)
	spr(rotated.entry.surface, sx, sy)
	record_cache_counter(gfx, "render.cache.surface_blits", 1)
	record_cache_counter(gfx, "render.cache.shared_stamp_blits", 1)
	record_cache_counter(gfx, "render.cache.rotated_surface_blits", 1)
	record_cache_observe(gfx, "render.cache.surface_blit_area", (rotated.entry.w or 0) * (rotated.entry.h or 0))
	return rotated.entry
end

GraphicsCache.drawCachedTextLayer = function(gfx, key, text, layer_x, layer_y, color, options)
	options = options or {}
	local width, height = estimate_text_size(text)
	local draw_options = {}
	for opt_key, opt_value in pairs(options) do
		draw_options[opt_key] = opt_value
	end
	draw_options.w = draw_options.w or width
	draw_options.h = draw_options.h or height
	draw_options.cache_auto_profile_key = draw_options.cache_auto_profile_key or "primitive.text"
	key = key or ("text:" .. tostr(text or "") .. ":" .. tostr(color or 7) .. ":" .. tostr(draw_options.w) .. ":" .. tostr(draw_options.h))
	if should_use_live_fast_path(gfx, draw_options) then
		local sx, sy = gfx.camera:layerToScreenXY(layer_x, layer_y)
		gfx:screenPrint(text, sx, sy, color)
		record_cache_counter(gfx, "render.cache.bypass", 1)
		return nil
	end
	return GraphicsCache.drawCachedLayer(gfx, key, layer_x, layer_y, function(target)
		target:print(text, 0, 0, color)
	end, draw_options)
end

GraphicsCache.drawCachedTextScreen = function(gfx, key, text, x, y, color, options)
	options = options or {}
	local width, height = estimate_text_size(text)
	local draw_options = {}
	for opt_key, opt_value in pairs(options) do
		draw_options[opt_key] = opt_value
	end
	draw_options.w = draw_options.w or width
	draw_options.h = draw_options.h or height
	draw_options.cache_auto_profile_key = draw_options.cache_auto_profile_key or "primitive.text"
	key = key or ("text:" .. tostr(text or "") .. ":" .. tostr(color or 7) .. ":" .. tostr(draw_options.w) .. ":" .. tostr(draw_options.h))
	if should_use_live_fast_path(gfx, draw_options) then
		gfx:screenPrint(text, x, y, color)
		record_cache_counter(gfx, "render.cache.bypass", 1)
		return nil
	end
	return GraphicsCache.drawCachedScreen(gfx, key, x, y, function(target)
		target:print(text, 0, 0, color)
	end, draw_options)
end

GraphicsCache.drawCachedSpriteLayer = function(gfx, key, sprite_id, layer_x, layer_y, flip_x, flip_y, options)
	options = options or {}
	local draw_options = {}
	for opt_key, opt_value in pairs(options) do
		draw_options[opt_key] = opt_value
	end
	draw_options.w = draw_options.w or 16
	draw_options.h = draw_options.h or 16
	draw_options.cache_auto_profile_key = draw_options.cache_auto_profile_key or "primitive.sprite"
	key = key or table.concat({
		"sprite",
		tostr(sprite_id or -1),
		tostr(flip_x == true),
		tostr(flip_y == true),
		tostr(draw_options.w),
		tostr(draw_options.h),
	}, ":")
	if should_use_live_fast_path(gfx, draw_options) then
		local sx, sy = gfx.camera:layerToScreenXY(layer_x, layer_y)
		spr(sprite_id, sx, sy, flip_x, flip_y)
		record_cache_counter(gfx, "render.cache.bypass", 1)
		return nil
	end
	if max(1, flr(draw_options.rotation_bucket_count or 1)) > 1 and abs(draw_options.rotation_angle or 0) > 0.0001 then
		return GraphicsCache.drawSharedRotatedSurfaceStampLayer(gfx, key, layer_x, layer_y, function(target)
			target:spr(sprite_id, 0, 0, flip_x, flip_y)
		end, draw_options)
	end
	return GraphicsCache.drawSharedSurfaceStampLayer(gfx, key, layer_x, layer_y, function(target)
		target:spr(sprite_id, 0, 0, flip_x, flip_y)
	end, draw_options)
end

return GraphicsCache
