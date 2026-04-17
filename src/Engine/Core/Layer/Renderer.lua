local Class = include("src/Engine/Core/Class.lua")
local Screen = include("src/Engine/Core/Screen.lua")

local LayerRenderer = Class:new({
	_type = "LayerRenderer",
	layer = nil,

	drawDebugGrid = function(self)
		if not DEBUG then
			return
		end

		local layer = self.layer
		local grid_size = 16
		local view_bounds = layer.camera:getViewBounds()

		local start_x = flr(view_bounds.left / grid_size) * grid_size
		local start_y = flr(view_bounds.top / grid_size) * grid_size
		local grid_color = layer.layer_id + 5

		for x = start_x, view_bounds.right, grid_size do
			local start_pos = layer.camera:layerToScreen({ x = x, y = view_bounds.top })
			local end_pos = layer.camera:layerToScreen({ x = x, y = view_bounds.bottom })
			line(start_pos.x, start_pos.y, end_pos.x, end_pos.y, grid_color)
		end

		for y = start_y, view_bounds.bottom, grid_size do
			local start_pos = layer.camera:layerToScreen({ x = view_bounds.left, y = y })
			local end_pos = layer.camera:layerToScreen({ x = view_bounds.right, y = y })
			line(start_pos.x, start_pos.y, end_pos.x, end_pos.y, grid_color)
		end
	end,

	drawMap = function(self)
		local layer = self.layer
		if layer.map_id == nil then
			return
		end

		local tile_size = layer.tile_size
		local view_bounds = layer.camera:getViewBounds()
		local start_tile_x = max(0, flr(view_bounds.left / tile_size))
		local end_tile_x = max(start_tile_x, ceil(view_bounds.right / tile_size))
		local start_tile_y = max(0, flr(view_bounds.top / tile_size))
		local end_tile_y = max(start_tile_y, ceil(view_bounds.bottom / tile_size))
		local visible_tiles_w = end_tile_x - start_tile_x + 1
		local visible_tiles_h = end_tile_y - start_tile_y + 1

		local shake = layer.camera:getShakeOffset()
		local cam_x = flr(layer.camera.pos.x * layer.camera.parallax_factor.x + shake.x)
		local cam_y = flr(layer.camera.pos.y * layer.camera.parallax_factor.y + shake.y)

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
					local screen_pos = layer.camera:layerToScreen({ x = layer_x, y = layer_y })
					if screen_pos.x >= 0 and screen_pos.x < Screen.w and screen_pos.y >= 0 and screen_pos.y < Screen.h then
						print(tile_id, screen_pos.x - 6, screen_pos.y - 4, 7)
					end
				end
			end
		end
	end,

	drawEntities = function(self)
		local view_bounds = self.layer.camera:getViewBounds()
		for i = 1, #self.layer.drawable_entities do
			local ent = self.layer.drawable_entities[i]
			local width = ent.getWidth and ent:getWidth() or 0
			local height = ent.getHeight and ent:getHeight() or 0
			local x = ent.pos and ent.pos.x or 0
			local y = ent.pos and ent.pos.y or 0
			if width <= 0 or height <= 0 or
				not (x + width * 0.5 < view_bounds.left or x - width * 0.5 > view_bounds.right or
					y + height * 0.5 < view_bounds.top or y - height * 0.5 > view_bounds.bottom) then
				ent:draw()
			end
		end
	end,

	draw = function(self)
		self:drawDebugGrid()
		self:drawMap()
		self:drawEntities()
	end,
})

return LayerRenderer
