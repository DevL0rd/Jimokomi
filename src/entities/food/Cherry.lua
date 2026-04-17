local WorldObject = include("src/primitives/WorldObject.lua")
local Consumable = include("src/primitives/Item/Consumable/Consumable.lua")
local WorldQuery = include("src/classes/WorldQuery.lua")

local Cherry = WorldObject:new({
	_type = "Cherry",
	shape = {
		kind = "circle",
		r = 5,
	},
	item_id = "cherry",
	display_name = "Cherry",
	ignore_physics = true,
	ignore_gravity = true,
	ignore_friction = true,
	ignore_collisions = true,
	can_pickup = true,
	can_use = true,
	consume_action_name = "eat",
	max_stack_size = 12,
	consume_radius_padding = 2,
	spawn_rule = WorldQuery.rules.randomTile({
		scope = "map",
		tile_ids = { 32, 44 },
		padding = 24,
		attempts = 24,
	}),
	init = function(self)
		local assets = self.layer and self.layer.engine and self.layer.engine.assets or nil
		if assets then
			assets:applyEntityConfig(self)
		end
		WorldObject.init(self)
		Consumable.init(self)
		self:playVisual("idle")
	end,
	onConsumed = function(self, consumer)
	end,
	update = function(self)
		if Consumable.update(self) == false then
			return
		end
		WorldObject.update(self)
	end
})

return Cherry
