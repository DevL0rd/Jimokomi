local WorldObject = include("src/Engine/Objects/WorldObject.lua")
local Item = include("src/Game/Mixins/Item.lua")

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
	can_drop = true,
	pickup_on_touch = true,
	pickup_radius_padding = 2,
	use_action_name = "eat",
	heal_amount = 1,
	max_stack_size = 12,
	spawn_item_path = "src/Game/Actors/Items/Food/Cherry.lua",
	inventory_sprite = 47,
	destroy_on_pickup = false,
	visual_definitions = {
		idle = {
			shape = { kind = "rect", w = 10, h = 10 },
			sprite = 47,
		},
	},
	spawn_rule = {
		kind = "tile_random",
		scope = "map",
		tile_ids = { 32, 44 },
		padding = 24,
		attempts = 24,
	},
	init = function(self)
		WorldObject.init(self)
		Item.init(self)
		self.is_spawned = self.spawn_rule == nil
		self:playVisual("idle")
	end,

	getWorld = function(self)
		if not self.layer then
			return nil
		end
		return self.layer.world
	end,

	setSpawnPosition = function(self, pos)
		self.pos.x = pos.x
		self.pos.y = pos.y
		self.vel.x = 0
		self.vel.y = 0
	end,

	respawn = function(self, require_offscreen)
		local world = self:getWorld()
		if not world or not self.spawn_rule then
			return false
		end

		local spawn_pos = world:resolveRule(self.spawn_rule, require_offscreen, self:getPickupRadius())
		if not spawn_pos then
			return false
		end

		self:setSpawnPosition(spawn_pos)
		self.is_spawned = true
		return true
	end,

	trySpawn = function(self, require_offscreen)
		if not self.layer then
			return false
		end
		return self:respawn(require_offscreen)
	end,

	onPickedUp = function(self, collector, picked_up)
		if not self:trySpawn(true) then
			self:destroy()
		end
	end,

	update = function(self)
		if self.spawn_rule and not self.is_spawned then
			if not self:trySpawn(false) then
				return
			end
		end

		if Item.update(self) == false then
			return
		end

		WorldObject.update(self)
	end
})

return Cherry
