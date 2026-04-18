local Class = include("src/Engine/Core/Class.lua")
local Screen = include("src/Engine/Core/Screen.lua")

local MAP_CACHE_CHUNK_TILES = 8
local DEBUG_TILE_OVERLAY_CHUNK_TILES = MAP_CACHE_CHUNK_TILES
local ATTACHMENT_FAMILY_SPRITE = 1
local ATTACHMENT_FAMILY_EMITTER = 2
local ATTACHMENT_FAMILY_MIXED = 3
local ENTITY_KIND_PARTICLE = 1
local ENTITY_KIND_PROCEDURAL = 2

local function estimate_debug_label_size(label)
	local safe_label = label and tostr(label) or ""
	return max(1, #safe_label * 4 + 2), 6
end

local function classify_attachment_family(ent)
	local family_kind = ent and ent._attachment_family_kind or 0
	if family_kind == ATTACHMENT_FAMILY_SPRITE then
		return "attachment_sprite_nodes"
	end
	if family_kind == ATTACHMENT_FAMILY_EMITTER then
		return "attachment_emitters"
	end
	if family_kind == ATTACHMENT_FAMILY_MIXED then
		return "attachment_nodes"
	end
	if not ent or not ent.attachment_nodes or #ent.attachment_nodes == 0 then
		return nil
	end
	if ent.recalcAttachmentFamily then
		ent:recalcAttachmentFamily()
		return classify_attachment_family(ent)
	end
	return nil
end

local function classify_entity_draw_family(ent)
	if ent and ent._draw_family ~= nil then
		return ent._draw_family
	end
	local family = "generic"
	local entity_kind = ent and ent.entity_kind or 0
	if ent and (entity_kind == ENTITY_KIND_PROCEDURAL or ent.drawProceduralSprite ~= nil) then
		family = "procedural_sprite"
	elseif entity_kind == ENTITY_KIND_PARTICLE then
		family = "particle"
	else
		local attachment_family = classify_attachment_family(ent)
		if attachment_family ~= nil then
			family = attachment_family
		elseif ent and ent.visual and ent.visual.getSprite and ent.visual:getSprite() ~= nil then
			family = "visual_sprite"
		elseif ent and ent.debug then
			family = "debug_object"
		end
	end
	if ent then
		ent._draw_family = family
	end
	return family
end

local function get_entity_type_key(ent)
	if ent and ent._prof_type_key ~= nil then
		return ent._prof_type_key
	end
	local ent_type = ent and ent._type or nil
	if ent_type == nil or ent_type == "" then
		ent_type = "unknown"
	else
		ent_type = tostr(ent_type)
	end
	if ent then
		ent._prof_type_key = ent_type
	end
	return ent_type
end

local function get_entity_object_key(ent)
	local object_id = ent and ent.object_id or nil
	local cached_key = ent and ent._prof_object_key or nil
	local cached_id = ent and ent._prof_object_id or nil
	if cached_key ~= nil and cached_id == object_id then
		return cached_key
	end
	local object_key = get_entity_type_key(ent) .. "#" .. (object_id == nil and "?" or tostr(object_id))
	if ent then
		ent._prof_object_key = object_key
		ent._prof_object_id = object_id
	end
	return object_key
end

local function get_draw_type_scope_name(ent, behind_map)
	local field = behind_map == true and "_prof_draw_type_scope_name_behind" or "_prof_draw_type_scope_name_front"
	if ent and ent[field] ~= nil then
		return ent[field]
	end
	local prefix = behind_map == true and "layer.render.entities_behind_map.type." or "layer.render.entities.type."
	local scope_name = prefix .. get_entity_type_key(ent)
	if ent then
		ent[field] = scope_name
	end
	return scope_name
end

local function get_draw_object_scope_name(ent, behind_map)
	local id_field = behind_map == true and "_prof_draw_object_scope_id_behind" or "_prof_draw_object_scope_id_front"
	local name_field = behind_map == true and "_prof_draw_object_scope_name_behind" or "_prof_draw_object_scope_name_front"
	if ent and ent[name_field] ~= nil and ent[id_field] == ent.object_id then
		return ent[name_field]
	end
	local prefix = behind_map == true and "layer.render.entities_behind_map.object." or "layer.render.entities.object."
	local scope_name = prefix .. get_entity_object_key(ent)
	if ent then
		ent[id_field] = ent.object_id
		ent[name_field] = scope_name
	end
	return scope_name
end

local function get_visible_tile_bounds(layer)
	local tile_size = layer.tile_size
	local camera_obj = layer.camera
	local transform = camera_obj:getRenderTransform()
	local cam_x = flr(transform.x)
	local cam_y = flr(transform.y)
	return {
		tile_size = tile_size,
		cam_x = cam_x,
		cam_y = cam_y,
		start_tile_x = max(0, flr(cam_x / tile_size)),
		end_tile_x = max(0, flr((cam_x + Screen.w - 1) / tile_size)),
		start_tile_y = max(0, flr(cam_y / tile_size)),
		end_tile_y = max(0, flr((cam_y + Screen.h - 1) / tile_size)),
	}
end

local function get_visible_chunk_bounds(tile_bounds, chunk_tiles)
	return {
		start_chunk_x = flr(tile_bounds.start_tile_x / chunk_tiles),
		end_chunk_x = flr(tile_bounds.end_tile_x / chunk_tiles),
		start_chunk_y = flr(tile_bounds.start_tile_y / chunk_tiles),
		end_chunk_y = flr(tile_bounds.end_tile_y / chunk_tiles),
	}
end

local function build_tile_chunk_overlay(layer, chunk_tile_x, chunk_tile_y, chunk_tiles)
	local tile_size = layer.tile_size
	local grid_color = layer.layer_id + 5
	local chunk_pixel_size = chunk_tiles * tile_size
	local label_positions = {}
	local label_count = 0
	local tile_hash = 17
	local grid_line_count = 0

	for tx = chunk_tile_x, chunk_tile_x + chunk_tiles - 1 do
		for ty = chunk_tile_y, chunk_tile_y + chunk_tiles - 1 do
			local tile_id = layer.collision:getTileIDAt(tx * tile_size, ty * tile_size, layer.map_id)
			tile_hash = (tile_hash * 131 + tile_id + tx * 7 + ty * 13) % 2147483647
			if tile_id ~= 0 then
				local label = tostr(tile_id)
				local positions = label_positions[label]
				if positions == nil then
					positions = {}
					label_positions[label] = positions
				end
				add(positions, {
					x = (tx - chunk_tile_x) * tile_size + tile_size / 2 - 6,
					y = (ty - chunk_tile_y) * tile_size + tile_size / 2 - 4,
				})
				label_count += 1
			end
		end
	end

	for local_x = 0, chunk_pixel_size, tile_size do
		grid_line_count += 1
	end
	for local_y = 0, chunk_pixel_size, tile_size do
		grid_line_count += 1
	end

	return {
		chunk_tile_x = chunk_tile_x,
		chunk_tile_y = chunk_tile_y,
		chunk_tiles = chunk_tiles,
		chunk_pixel_size = chunk_pixel_size,
		grid_color = grid_color,
		grid_line_count = grid_line_count,
		label_positions = label_positions,
		label_count = label_count,
		tile_hash = tile_hash,
	}
end

local function append_tile_chunk_overlay(target, chunk_overlay, tile_size)
	for local_x = 0, chunk_overlay.chunk_pixel_size, tile_size do
		target:line(local_x, 0, local_x, chunk_overlay.chunk_pixel_size, chunk_overlay.grid_color)
	end
	for local_y = 0, chunk_overlay.chunk_pixel_size, tile_size do
		target:line(0, local_y, chunk_overlay.chunk_pixel_size, local_y, chunk_overlay.grid_color)
	end
end

local function append_tile_chunk_labels(gfx, target, chunk_overlay)
	for label, positions in pairs(chunk_overlay.label_positions) do
		local label_w, label_h = estimate_debug_label_size(label)
		local label_entry = gfx:ensureCachedSurface("debug.tile_label_surface:" .. label, function(surface_target)
			surface_target:print(label, 0, 0, 7)
		end, {
			w = label_w,
			h = label_h,
			cache_tag = "debug.tile_label.surface",
			cache_profile_key = "debug.tile_label.surface",
			cache_profile_signature = label,
			clear_color = 0,
		})
		local label_surface = label_entry and label_entry.surface or nil
		for i = 1, #positions do
			local pos = positions[i]
			if label_surface ~= nil then
				target:spr(label_surface, pos.x, pos.y)
			else
				target:print(label, pos.x, pos.y, 7)
			end
		end
	end
end

local Renderer = Class:new({
	_type = "Renderer",
	layer = nil,

	drawBackground = function(self)
		local layer = self.layer
		local background = layer and layer.background or nil
		if not background or not background.drawBackground then
			return
		end
		background:drawBackground(layer)
	end,

	drawDebugGrid = function(self)
		return
	end,

	drawMap = function(self)
		local layer = self.layer
		if layer.map_id == nil then
			return
		end

		local tile_bounds = get_visible_tile_bounds(layer)
		local tile_size = tile_bounds.tile_size
		local visible_tiles_w = tile_bounds.end_tile_x - tile_bounds.start_tile_x + 1
		local visible_tiles_h = tile_bounds.end_tile_y - tile_bounds.start_tile_y + 1
		local chunk_bounds = get_visible_chunk_bounds(tile_bounds, MAP_CACHE_CHUNK_TILES)
		local profiler = layer and layer.engine and layer.engine.profiler or nil
		local visible_tile_rect = visible_tiles_w * visible_tiles_h
		local map_chunk_count = 0
		local overlay_chunk_count = 0
		local total_grid_lines = 0
		local total_labels = 0
		local render_debug_tile_overlay = false
		local base_chunks_scope = profiler and profiler:start("layer.render.map.base_chunks") or nil
		for chunk_x = chunk_bounds.start_chunk_x, chunk_bounds.end_chunk_x do
			for chunk_y = chunk_bounds.start_chunk_y, chunk_bounds.end_chunk_y do
				local chunk_tile_x = chunk_x * MAP_CACHE_CHUNK_TILES
				local chunk_tile_y = chunk_y * MAP_CACHE_CHUNK_TILES
				local chunk_key = table.concat({
					"layer_map_chunk",
					tostr(layer.map_id),
					tostr(chunk_x),
					tostr(chunk_y),
				}, ":")
				layer.gfx:drawCachedLayer(chunk_key, chunk_tile_x * tile_size, chunk_tile_y * tile_size, function(target)
					target:map(
						chunk_tile_x,
						chunk_tile_y,
						0,
						0,
						MAP_CACHE_CHUNK_TILES,
						MAP_CACHE_CHUNK_TILES
					)
				end, {
					w = MAP_CACHE_CHUNK_TILES * tile_size,
					h = MAP_CACHE_CHUNK_TILES * tile_size,
					cache_mode = 2,
					cache_override_mode = 2,
					cache_tag = "layer.map.chunk",
					cache_profile_key = "layer.map.chunk:" .. tostr(layer.map_id),
					cache_profile_signature = tostr(MAP_CACHE_CHUNK_TILES),
				})
				map_chunk_count += 1
			end
		end
		if profiler then profiler:stop(base_chunks_scope) end

		if render_debug_tile_overlay then
			local overlay_chunks_scope = profiler and profiler:start("layer.render.map.debug_overlay_chunks") or nil
			for chunk_x = chunk_bounds.start_chunk_x, chunk_bounds.end_chunk_x do
				for chunk_y = chunk_bounds.start_chunk_y, chunk_bounds.end_chunk_y do
					local chunk_tile_x = chunk_x * DEBUG_TILE_OVERLAY_CHUNK_TILES
					local chunk_tile_y = chunk_y * DEBUG_TILE_OVERLAY_CHUNK_TILES
					local chunk_overlay = build_tile_chunk_overlay(layer, chunk_tile_x, chunk_tile_y, DEBUG_TILE_OVERLAY_CHUNK_TILES)
					local overlay_key = table.concat({
						"debug_tile_overlay_chunk",
						tostr(layer.layer_id or 0),
						tostr(layer.map_id),
						tostr(chunk_x),
						tostr(chunk_y),
						tostr(chunk_overlay.tile_hash),
					}, ":")
					local overlay_entry = layer.gfx:ensureCachedSurface(overlay_key, function(target)
						local grid_scope = profiler and profiler:start("layer.render.map.grid_chunk_overlay") or nil
						append_tile_chunk_overlay(target, chunk_overlay, tile_size)
						if profiler then profiler:stop(grid_scope) end
						local label_scope = profiler and profiler:start("layer.render.map.debug_label_stamp") or nil
						append_tile_chunk_labels(layer.gfx, target, chunk_overlay)
						if profiler then profiler:stop(label_scope) end
					end, {
						w = DEBUG_TILE_OVERLAY_CHUNK_TILES * tile_size,
						h = DEBUG_TILE_OVERLAY_CHUNK_TILES * tile_size,
						force_cache = true,
						cache_tag = "debug.tile_overlay.chunk",
						cache_profile_key = "debug.tile_overlay.chunk:" .. tostr(layer.layer_id or 0),
						cache_profile_signature = table.concat({
							tostr(DEBUG_TILE_OVERLAY_CHUNK_TILES),
							tostr(chunk_overlay.tile_hash),
						}, ":"),
						clear_color = 0,
					})
					if overlay_entry and overlay_entry.surface then
						layer.gfx:spr(overlay_entry.surface, chunk_tile_x * tile_size, chunk_tile_y * tile_size)
					end
					total_grid_lines += chunk_overlay.grid_line_count
					total_labels += chunk_overlay.label_count
					overlay_chunk_count += 1
				end
			end
			if profiler then profiler:stop(overlay_chunks_scope) end
		end

		if profiler then
			profiler:observe("layer.render.visible_tiles", visible_tile_rect)
			profiler:observe("layer.render.drawn_tiles", visible_tile_rect)
			profiler:observe("layer.render.map_calls", map_chunk_count)
			profiler:observe("layer.render.map.base_chunk_calls", map_chunk_count)
			if render_debug_tile_overlay then
				profiler:observe("layer.render.map.debug_overlay_chunk_calls", overlay_chunk_count)
			end
			if render_debug_tile_overlay then
				profiler:observe("layer.render.debug_grid_lines", total_grid_lines)
				profiler:observe("layer.render.debug_tile_labels", total_labels)
			end
		end
	end,

	drawDebugTileLabels = function(self)
		return
	end,

	drawEntities = function(self, behind_map)
		local view_bounds = self.view_bounds or self.layer.camera:getViewBounds()
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		local detailed_entity_profiling = self.layer and self.layer.engine and self.layer.engine.detailed_entity_profiling_enabled == true
		local drawable_entities = self.layer.drawable_entities
		local visible_count = 0
		local culled_count = 0
		local family_counts = {}
		local scope_prefix = behind_map == true and "layer.render.entities_behind_map" or "layer.render.entities"
		for i = 1, #drawable_entities do
			local ent = drawable_entities[i]
			local ent_behind_map = ent.draw_behind_map == true
			if (behind_map == true and not ent_behind_map) or (behind_map ~= true and ent_behind_map) then
				goto continue
			end
			local width = ent.getWidth and ent:getWidth() or 0
			local height = ent.getHeight and ent:getHeight() or 0
			local x = ent.pos and ent.pos.x or 0
			local y = ent.pos and ent.pos.y or 0
			if width <= 0 or height <= 0 or
				not (x + width * 0.5 < view_bounds.left or x - width * 0.5 > view_bounds.right or
					y + height * 0.5 < view_bounds.top or y - height * 0.5 > view_bounds.bottom) then
				local family = classify_entity_draw_family(ent)
				local family_scope = profiler and profiler:start(scope_prefix .. "." .. family) or nil
				local type_scope = nil
				local object_scope = nil
				if profiler and detailed_entity_profiling then
					type_scope = profiler:start(get_draw_type_scope_name(ent, behind_map == true))
					object_scope = profiler:start(get_draw_object_scope_name(ent, behind_map == true))
				end
				ent:draw()
				if profiler then
					profiler:stop(object_scope)
					profiler:stop(type_scope)
					profiler:stop(family_scope)
				end
				family_counts[family] = (family_counts[family] or 0) + 1
				visible_count += 1
			else
				culled_count += 1
			end
			::continue::
		end
		if profiler then
			if behind_map ~= true then
				profiler:observe("layer.render.drawable_entities", #drawable_entities)
				profiler:observe("layer.render.visible_entities", visible_count)
				profiler:observe("layer.render.culled_entities", culled_count)
			end
			for family, count in pairs(family_counts) do
				profiler:observe(scope_prefix .. ".family." .. family, count)
			end
		end
	end,

	draw = function(self)
		self.view_bounds = self.layer.camera:getViewBounds()
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		local background_scope = profiler and profiler:start("layer.render.background") or nil
		self:drawBackground()
		if profiler then profiler:stop(background_scope) end
		local grid_scope = profiler and profiler:start("layer.render.debug_grid") or nil
		self:drawDebugGrid()
		if profiler then profiler:stop(grid_scope) end
		local behind_scope = profiler and profiler:start("layer.render.entities_behind_map") or nil
		self:drawEntities(true)
		if profiler then profiler:stop(behind_scope) end
		local map_scope = profiler and profiler:start("layer.render.map") or nil
		self:drawMap()
		if profiler then profiler:stop(map_scope) end
		local tile_label_scope = profiler and profiler:start("layer.render.debug_tile_labels") or nil
		self:drawDebugTileLabels()
		if profiler then profiler:stop(tile_label_scope) end
		local entities_scope = profiler and profiler:start("layer.render.entities") or nil
		self:drawEntities(false)
		if profiler then profiler:stop(entities_scope) end
		self.view_bounds = nil
	end,
})

return Renderer
