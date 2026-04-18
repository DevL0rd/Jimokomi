local WorldObject = include("src/Engine/Objects/WorldObject.lua")
local Item = include("src/Game/Mixins/Item.lua")

local CHERRY_DRAW_W = 10
local CHERRY_DRAW_H = 10

local Cherry = WorldObject:new({
	_type = "Cherry",
	shape = {
		kind = "circle",
		r = 5,
	},
	inherit_layer_debug = false,
	debug_force_off = true,
	debug_collision_force_off = true,
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
	max_stack_size = 1,
	spawn_item_path = "src/Game/Actors/Items/Food/Cherry.lua",
	inventory_sprite = 47,
	destroy_on_pickup = false,
	touch_pickup_active_padding = 24,
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
		self:clearVisual()
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
	end,

	draw = function(self)
		local layer = self.layer
		if layer and self.inventory_sprite ~= nil and self.inventory_sprite >= 0 and self.drawSharedSprite then
			self:drawSharedSprite(
				self.inventory_sprite,
				self.pos.x - CHERRY_DRAW_W * 0.5,
				self.pos.y - CHERRY_DRAW_H * 0.5,
				CHERRY_DRAW_W,
				CHERRY_DRAW_H,
				{
					cache_tag = "object.sprite.cherry",
					cache_profile_key = "object.sprite:Cherry",
				}
			)
		end
		if self.debug or self.fill or self.stroke or self.fill_color > -1 or self.stroke_color > -1 or
			(self.attachment_nodes and #self.attachment_nodes > 0) then
			WorldObject.draw(self)
		end
	end,

	needsUpdate = function(self)
		if self.spawn_rule and not self.is_spawned then
			return true
		end
		if self.pickup_on_touch ~= true then
			return false
		end
		local player = self.getPlayer and self:getPlayer() or nil
		if not player or not self.isCollectorNearPickupRange then
			return false
		end
		return self:isCollectorNearPickupRange(player, self.touch_pickup_active_padding or 24)
	end
})

return Cherry
