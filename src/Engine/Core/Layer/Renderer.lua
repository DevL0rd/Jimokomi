local Class = include("src/Engine/Core/Class.lua")
local Screen = include("src/Engine/Core/Screen.lua")

local Renderer = Class:new({
	_type = "Renderer",
	layer = nil,

	drawDebugGrid = function(self)
		if not DEBUG then
			return
		end

		local layer = self.layer
		local grid_size = 16
		local camera_obj = layer.camera
		local view_bounds = self.view_bounds or camera_obj:getViewBounds()

		local start_x = flr(view_bounds.left / grid_size) * grid_size
		local start_y = flr(view_bounds.top / grid_size) * grid_size
		local grid_color = layer.layer_id + 5
		local profiler = layer and layer.engine and layer.engine.profiler or nil
		local line_count = 0

		for x = start_x, view_bounds.right, grid_size do
			local sx1, sy1 = camera_obj:layerToScreenXY(x, view_bounds.top)
			local sx2, sy2 = camera_obj:layerToScreenXY(x, view_bounds.bottom)
			line(sx1, sy1, sx2, sy2, grid_color)
			line_count += 1
		end

		for y = start_y, view_bounds.bottom, grid_size do
			local sx1, sy1 = camera_obj:layerToScreenXY(view_bounds.left, y)
			local sx2, sy2 = camera_obj:layerToScreenXY(view_bounds.right, y)
			line(sx1, sy1, sx2, sy2, grid_color)
			line_count += 1
		end
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
		local view_bounds = self.view_bounds or camera_obj:getViewBounds()
		local start_tile_x = max(0, flr(view_bounds.left / tile_size))
		local end_tile_x = max(start_tile_x, ceil(view_bounds.right / tile_size))
		local start_tile_y = max(0, flr(view_bounds.top / tile_size))
		local end_tile_y = max(start_tile_y, ceil(view_bounds.bottom / tile_size))
		local visible_tiles_w = end_tile_x - start_tile_x + 1
		local visible_tiles_h = end_tile_y - start_tile_y + 1
		local profiler = layer and layer.engine and layer.engine.profiler or nil
		if profiler then
			profiler:observe("layer.render.visible_tiles", visible_tiles_w * visible_tiles_h)
		end

		local transform = camera_obj:getRenderTransform()
		local cam_x = flr(transform.x)
		local cam_y = flr(transform.y)

		camera(cam_x, cam_y)
		map(
			start_tile_x,
			start_tile_y,
			start_tile_x * tile_size,
			start_tile_y * tile_size,
			visible_tiles_w,
			visible_tiles_h
		)
		camera()

		if not DEBUG then
			return
		end

		for tx = start_tile_x, end_tile_x do
			for ty = start_tile_y, end_tile_y do
				local tile_id = layer.collision:getTileIDAt(tx * tile_size, ty * tile_size, layer.map_id)
				if tile_id ~= 0 then
					local layer_x = tx * tile_size + tile_size / 2
					local layer_y = ty * tile_size + tile_size / 2
					local sx, sy = camera_obj:layerToScreenXY(layer_x, layer_y)
					if sx >= 0 and sx < Screen.w and sy >= 0 and sy < Screen.h then
						print(tile_id, sx - 6, sy - 4, 7)
					end
				end
			end
		end
	end,

	drawEntities = function(self)
		local view_bounds = self.view_bounds or self.layer.camera:getViewBounds()
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		local drawable_entities = self.layer.drawable_entities
		local visible_count = 0
		local culled_count = 0
		for i = 1, #drawable_entities do
			local ent = drawable_entities[i]
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
		end
		if profiler then
			profiler:observe("layer.render.drawable_entities", #drawable_entities)
			profiler:observe("layer.render.visible_entities", visible_count)
			profiler:observe("layer.render.culled_entities", culled_count)
		end
	end,

	draw = function(self)
		self.view_bounds = self.layer.camera:getViewBounds()
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		local grid_scope = profiler and profiler:start("layer.render.debug_grid") or nil
		self:drawDebugGrid()
		if profiler then profiler:stop(grid_scope) end
		local map_scope = profiler and profiler:start("layer.render.map") or nil
		self:drawMap()
		if profiler then profiler:stop(map_scope) end
		local entities_scope = profiler and profiler:start("layer.render.entities") or nil
		self:drawEntities()
		if profiler then profiler:stop(entities_scope) end
		self.view_bounds = nil
	end,
})

return Renderer
