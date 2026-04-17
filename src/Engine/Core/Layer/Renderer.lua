local Class = include("src/Engine/Core/Class.lua")
local Screen = include("src/Engine/Core/Screen.lua")

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
		if not self.layer.debug then
			return
		end

		local layer = self.layer
		local grid_size = 16
		local camera_obj = layer.camera
		local grid_color = layer.layer_id + 5
		local profiler = layer and layer.engine and layer.engine.profiler or nil
		local origin_sx, origin_sy = camera_obj:layerToScreenXY(0, 0)
		local offset_x = ((flr(origin_sx) % grid_size) + grid_size) % grid_size
		local offset_y = ((flr(origin_sy) % grid_size) + grid_size) % grid_size
		local vertical_lines = flr((Screen.w - offset_x + grid_size - 1) / grid_size) + 1
		local horizontal_lines = flr((Screen.h - offset_y + grid_size - 1) / grid_size) + 1
		local line_count = vertical_lines + horizontal_lines
		layer.gfx:drawCachedScreen(
			"debug_grid:" .. grid_size .. ":" .. grid_color .. ":" .. offset_x .. ":" .. offset_y,
			0,
			0,
			function(target)
				for sx = offset_x, Screen.w, grid_size do
					target:line(sx, 0, sx, Screen.h, grid_color)
				end
				for sy = offset_y, Screen.h, grid_size do
					target:line(0, sy, Screen.w, sy, grid_color)
				end
			end,
			{
				w = Screen.w,
				h = Screen.h,
			}
		)
		if profiler then
			profiler:observe("layer.render.debug_grid_lines", line_count)
		end
	end,

	drawMap = function(self)
		local layer = self.layer
		if layer.map_id == nil then
			return
		end

		local tile_size = layer.tile_size
		local camera_obj = layer.camera
		local transform = camera_obj:getRenderTransform()
		local cam_x = flr(transform.x)
		local cam_y = flr(transform.y)
		local start_tile_x = max(0, flr(cam_x / tile_size))
		local end_tile_x = max(start_tile_x, flr((cam_x + Screen.w - 1) / tile_size))
		local start_tile_y = max(0, flr(cam_y / tile_size))
		local end_tile_y = max(start_tile_y, flr((cam_y + Screen.h - 1) / tile_size))
		local visible_tiles_w = end_tile_x - start_tile_x + 1
		local visible_tiles_h = end_tile_y - start_tile_y + 1
		local profiler = layer and layer.engine and layer.engine.profiler or nil
		local visible_tile_rect = visible_tiles_w * visible_tiles_h
		local dest_x = start_tile_x * tile_size - cam_x
		local dest_y = start_tile_y * tile_size - cam_y
		map(
			start_tile_x,
			start_tile_y,
			dest_x,
			dest_y,
			visible_tiles_w,
			visible_tiles_h
		)

		if profiler then
			profiler:observe("layer.render.visible_tiles", visible_tile_rect)
			profiler:observe("layer.render.drawn_tiles", visible_tile_rect)
			profiler:observe("layer.render.map_calls", 1)
		end

		if not layer.debug then
			return
		end

		if layer.debug_tile_labels == true then
			for tx = start_tile_x, end_tile_x do
				for ty = start_tile_y, end_tile_y do
					local tile_id = layer.collision:getTileIDAt(tx * tile_size, ty * tile_size, layer.map_id)
					if tile_id ~= 0 then
						local layer_x = tx * tile_size + tile_size / 2
						local layer_y = ty * tile_size + tile_size / 2
						local sx, sy = camera_obj:layerToScreenXY(layer_x, layer_y)
						if sx >= 0 and sx < Screen.w and sy >= 0 and sy < Screen.h then
							local label = tostr(tile_id)
							layer.gfx:drawCachedScreen(
								"debug_tile_label:" .. label,
								sx - 6,
								sy - 4,
								function(target)
									target:print(label, 0, 0, 7)
								end,
								{
									w = #label * 4 + 2,
									h = 6,
								}
							)
						end
					end
				end
			end
		end
	end,

	drawEntities = function(self, behind_map)
		local view_bounds = self.view_bounds or self.layer.camera:getViewBounds()
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		local drawable_entities = self.layer.drawable_entities
		local visible_count = 0
		local culled_count = 0
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
				ent:draw()
				visible_count += 1
			else
				culled_count += 1
			end
			::continue::
		end
		if profiler and behind_map ~= true then
			profiler:observe("layer.render.drawable_entities", #drawable_entities)
			profiler:observe("layer.render.visible_entities", visible_count)
			profiler:observe("layer.render.culled_entities", culled_count)
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
		local entities_scope = profiler and profiler:start("layer.render.entities") or nil
		self:drawEntities(false)
		if profiler then profiler:stop(entities_scope) end
		self.view_bounds = nil
	end,
})

return Renderer
