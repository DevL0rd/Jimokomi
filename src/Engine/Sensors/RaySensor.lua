local Timer = include("src/Engine/Core/Timer.lua")
local Vector = include("src/Engine/Math/Vector.lua")
local AttachmentNode = include("src/Engine/Nodes/AttachmentNode.lua")

local RaySensor = AttachmentNode:new({
	_type = "RaySensor",
	vec = Vector:new({ y = 1 }),
	length = 50,
	rate = 50,

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
})

return RaySensor
