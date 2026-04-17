local Class = include("src/Engine/Core/Class.lua")

local Spawner = Class:new({
	_type = "Spawner",
	layer = nil,
	world = nil,

	spawn = function(self, ObjectClass, params, rule, require_offscreen, radius)
		params = params or {}
		if not ObjectClass or not self.layer then
			return nil
		end

		local spawn_pos = nil
		if rule and self.world then
			spawn_pos = self.world:resolveRule(rule, require_offscreen, radius)
			if not spawn_pos then
				return nil
			end
		end

		params.layer = params.layer or self.layer
		if spawn_pos then
			params.pos = spawn_pos
		end

		return ObjectClass:new(params)
	end,

	spawnMany = function(self, count, ObjectClass, params, rule, require_offscreen, radius)
		local spawned = {}
		for i = 1, count do
			local ent = self:spawn(ObjectClass, params, rule, require_offscreen, radius)
			if ent then
				add(spawned, ent)
			end
		end
		return spawned
	end,
})

return Spawner
