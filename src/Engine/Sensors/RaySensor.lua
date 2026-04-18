local Timer = include("src/Engine/Core/Timer.lua")
local Vector = include("src/Engine/Math/Vector.lua")
local AttachmentNode = include("src/Engine/Nodes/AttachmentNode.lua")

local RaySensor = AttachmentNode:new({
	_type = "RaySensor",
	vec = Vector:new({ y = 1 }),
	length = 50,
	rate = 50,
	always_update_offscreen = false,
	ignore_entity_hits = true,

	init = function(self)
		AttachmentNode.init(self)
		self.line_dist = self.length
		self.timer = Timer:new()
		self.cached_result = { nil, self.length }
	end,

	cast = function(self)
		if self.transform and self.transform:shouldSync() then
			self.transform:sync()
		end
		if self.timer:hasElapsed(self.rate) then
			local world = self:getWorld()
			if world then
				local hit_obj, distance = world:castRay(self)
				self.line_dist = distance
				self.cached_result = { hit_obj, distance }
			end
		end
		return self.cached_result[1], self.cached_result[2]
	end,

	drawNode = function(self)
		local parent = self.parent
		local layer = self.layer
		if not layer or not parent or parent.debug ~= true then
			return
		end
		local hit_obj, distance = self:cast()
		local hit_dist = distance or self.length or 0
		local max_length = self.length or 0
		local dir_x = self.vec and self.vec.x or 0
		local dir_y = self.vec and self.vec.y or 0
		local dir_len = sqrt(dir_x * dir_x + dir_y * dir_y)
		if dir_len <= 0 then
			return
		end
		dir_x /= dir_len
		dir_y /= dir_len

		local start_x = self.pos.x
		local start_y = self.pos.y
		local max_end_x = start_x + dir_x * max_length
		local max_end_y = start_y + dir_y * max_length
		local hit_end_x = start_x + dir_x * hit_dist
		local hit_end_y = start_y + dir_y * hit_dist
		local active_color = hit_obj and 10 or 11
		local max_color = 5
		local sx, sy = layer.camera:layerToScreenXY(start_x, start_y)
		local max_sx, max_sy = layer.camera:layerToScreenXY(max_end_x, max_end_y)
		local hit_sx, hit_sy = layer.camera:layerToScreenXY(hit_end_x, hit_end_y)
		layer.gfx:screenLine(sx, sy, max_sx, max_sy, max_color)
		layer.gfx:screenLine(sx, sy, hit_sx, hit_sy, active_color)
		layer.gfx:screenCircfill(sx, sy, 1, active_color)
		layer.gfx:screenCirc(hit_sx, hit_sy, 2, active_color)
	end,
})

return RaySensor
