local Class = include("src/classes/Class.lua")
local Vector = include("src/classes/Vector.lua")
local Timer = include("src/classes/Timer.lua")
local Transform = include("src/classes/Transform.lua")
local Collider = include("src/classes/Collider.lua")
local WorldObjectSnapshot = include("src/primitives/world_object/WorldObjectSnapshot.lua")
local WorldObjectEvents = include("src/primitives/world_object/WorldObjectEvents.lua")

local function clone_vector(value)
	return Vector:new({
		x = value and value.x or 0,
		y = value and value.y or 0,
	})
end

local function world_object_update(self)
	if self.lifetime ~= -1 then
		self.percent_expired = self.timer:elapsed() / self.lifetime
		if self.timer:hasElapsed(self.lifetime) then
			self:destroy()
		end
	end
end

local function world_object_draw(self)
	if self.fill then
		self:fill()
	elseif self.fill_color > -1 then
		self:fillShape(self.fill_color)
	end
	if self.stroke then
		self:stroke()
	elseif self.stroke_color > -1 then
		self:strokeShape(self.stroke_color)
	end
	if self.debug then
		self:draw_debug()
		local vx = self.vel.x * 0.25
		local vy = self.vel.y * 0.25
		self.layer.gfx:line(self.pos.x, self.pos.y, self.pos.x + vx, self.pos.y + vy, 8)
	end
end

local WorldObject = Class:new({
	_type = "WorldObject",
	name = "WorldObject",
	object_id = nil,
	shape = nil,
	pos = nil,
	vel = Vector:new(),
	accel = Vector:new(),
	friction = 10,
	stroke_color = -1,
	fill_color = -1,
	debug = false,
	snapshot_enabled = true,
	lifetime = -1,
	ignore_physics = false,
	ignore_gravity = false,
	ignore_friction = false,
	ignore_collisions = false,
	collision_layer = "default",
	collision_mask = nil,
	is_trigger = false,
	resolve_entity_collisions = false,
	parent = nil,
	parent_slot = nil,
	layer = nil,
	slot_defs = nil,
	slot_offset = nil,
	slots_enabled = true,
	default_slot_destroy_policy = "detach",

	getTransform = function(self)
		return self.transform
	end,

	getCollider = function(self)
		return self.collider
	end,

	setShape = function(self, shape)
		if self.collider then
			self.collider:setShape(shape)
		else
			self.shape = shape
		end
	end,

	setCircleShape = function(self, radius)
		self:setShape({
			kind = "circle",
			r = radius,
		})
	end,

	setRectShape = function(self, width, height)
		self:setShape({
			kind = "rect",
			w = width,
			h = height,
		})
	end,

	getShapeKind = function(self)
		return self.collider and self.collider:getShapeKind() or nil
	end,

	isCircleShape = function(self)
		return self.collider and self.collider:isCircle() or false
	end,

	isRectShape = function(self)
		return self.collider and self.collider:isRect() or false
	end,

	getRadius = function(self)
		return self.collider and self.collider:getRadius() or 0
	end,

	getWidth = function(self)
		return self.collider and self.collider:getWidth() or 0
	end,

	getHeight = function(self)
		return self.collider and self.collider:getHeight() or 0
	end,

	getHalfWidth = function(self)
		return self.collider and self.collider:getHalfWidth() or 0
	end,

	getHalfHeight = function(self)
		return self.collider and self.collider:getHalfHeight() or 0
	end,

	getTopLeft = function(self)
		return Vector:new({
			x = self.pos.x - self:getHalfWidth(),
			y = self.pos.y - self:getHalfHeight(),
		})
	end,

	setWorldPosition = function(self, pos)
		return self.transform:setWorldPosition(pos)
	end,

	setLocalPosition = function(self, pos)
		return self.transform:setLocalPosition(pos)
	end,

	getLocalPosition = function(self)
		return self.transform:getLocalPosition()
	end,

	translate = function(self, vec, scale_with_dt)
		return self.transform:translate(vec, scale_with_dt)
	end,

	emitEvent = WorldObjectEvents.emitEvent,

	getCollisionLayer = function(self)
		return self.collider and self.collider:getCollisionLayer() or "default"
	end,

	setCollisionLayer = function(self, layer_name)
		if self.collider then
			self.collider:setCollisionLayer(layer_name)
		else
			self.collision_layer = layer_name or "default"
		end
	end,

	allowsCollisionLayer = function(self, layer_name)
		return self.collider and self.collider:allowsCollisionLayer(layer_name) or true
	end,

	setCollisionMask = function(self, mask)
		if self.collider then
			self.collider:setCollisionMask(mask)
		else
			self.collision_mask = mask
		end
	end,

	isTrigger = function(self)
		return self.collider and self.collider:isTriggerCollider() or false
	end,

	canCollideWith = function(self, other)
		if self.ignore_collisions or (other and other.ignore_collisions) then
			return false
		end
		if not self.collider or not other or not other.getCollider then
			return false
		end
		return self.collider:canCollideWith(other:getCollider())
	end,

	ensureSlots = function(self)
		return self.transform:ensureSlots()
	end,

	defineSlot = function(self, name, config)
		return self.transform:defineSlot(name, config)
	end,

	getSlot = function(self, name)
		return self.transform:getSlot(name)
	end,

	hasSlot = function(self, name)
		return self.transform:hasSlot(name)
	end,

	setSlotPosition = function(self, name, pos)
		return self.transform:setSlotPosition(name, pos)
	end,

	getSlotLocalPosition = function(self, name)
		return self.transform:getSlotLocalPosition(name)
	end,

	getSlotWorldPosition = function(self, name)
		return self.transform:getSlotWorldPosition(name)
	end,

	isSlotEnabled = function(self, name)
		return self.transform:isSlotEnabled(name)
	end,

	setSlotEnabled = function(self, name, enabled)
		return self.transform:setSlotEnabled(name, enabled)
	end,

	setSlotsEnabled = function(self, enabled)
		return self.transform:setSlotsEnabled(enabled)
	end,

	setSlotOffset = function(self, pos)
		return self.transform:setLocalPosition(pos)
	end,

	getSlotOffset = function(self)
		return self.transform:getSlotOffset()
	end,

	getAttachment = function(self)
		return self.transform:getAttachment()
	end,

	isAttached = function(self)
		return self.transform:isAttached()
	end,

	isAttachmentEnabled = function(self)
		return self.transform:isAttachmentEnabled()
	end,

	setAttachmentEnabled = function(self, enabled)
		return self.transform:setAttachmentEnabled(enabled)
	end,

	shouldSyncToParent = function(self)
		return self.transform:shouldSync()
	end,

	attachTo = function(self, parent, slot_name, config)
		if not parent or not parent.transform then
			return false
		end
		return self.transform:attachTo(parent.transform, slot_name, config)
	end,

	attachChild = function(self, child, slot_name, config)
		child:attachTo(self, slot_name, config)
		return child
	end,

	getAttachedChildren = function(self, slot_name)
		return self.transform:getAttachedChildren(slot_name)
	end,

	detach = function(self)
		return self.transform:detach()
	end,

	syncToParent = function(self)
		return self.transform:sync()
	end,

	destroySlot = function(self, name, destroy_policy)
		return self.transform:destroySlot(name, destroy_policy)
	end,

	destroySlots = function(self)
		return self.transform:destroySlots()
	end,

	getAssets = function(self)
		if self.layer and self.layer.engine then
			return self.layer.engine.assets
		end
		return nil
	end,

	isSnapshotEnabled = function(self)
		return self.snapshot_enabled ~= false
	end,

	getWorld = function(self)
		return self.layer and self.layer.world or nil
	end,

	getSpawner = function(self)
		return self.layer and self.layer.spawner or nil
	end,

	getDebugSnapshot = WorldObjectSnapshot.getDebugSnapshot,
	toSnapshot = WorldObjectSnapshot.toSnapshot,
	applySnapshot = function(self, snapshot)
		return WorldObjectSnapshot.applySnapshot(self, snapshot, clone_vector)
	end,

	exportState = function(self)
		return nil
	end,

	importState = function(self, state)
	end,

	init = function(self)
		local initial_pos = self.pos or Vector:new()
		self.transform = Transform:new({
			owner = self,
			position = initial_pos,
			local_position = self.slot_offset,
			slot_defs = self.slot_defs,
			slots_enabled = self.slots_enabled ~= false,
			default_slot_destroy_policy = self.default_slot_destroy_policy,
		})
		self.pos = self.transform.position
		self.slot_offset = self.transform:getSlotOffset()

		self.collider = Collider:new({
			owner = self,
			shape = self.shape,
			collision_layer = self.collision_layer,
			collision_mask = self.collision_mask,
			is_trigger = self.is_trigger,
			resolve_dynamic = self.resolve_entity_collisions,
			enabled = not self.ignore_collisions,
		})

		self.vel = clone_vector(self.vel)
		self.accel = clone_vector(self.accel)
		self.timer = Timer:new()
		self.percent_expired = 0

		if self.layer and self.layer.engine and self.object_id == nil and self.layer.engine.nextObjectId then
			self.object_id = self.layer.engine:nextObjectId()
		end

		if self.parent then
			self:attachTo(self.parent, self.parent_slot, {
				offset = self.slot_offset,
			})
		end
		if self.layer then
			self.layer:add(self)
		end
	end,

	didAddToLayer = WorldObjectEvents.didAddToLayer,
	willRemoveFromLayer = WorldObjectEvents.willRemoveFromLayer,
	didRemoveFromLayer = WorldObjectEvents.didRemoveFromLayer,

	update = world_object_update,

	needsUpdate = function(self)
		return self.update ~= world_object_update or self.lifetime ~= -1
	end,

	strokeShape = function(self, color)
		if not self.layer then
			return
		end
		if self:isCircleShape() then
			self.layer.gfx:circ(self.pos.x, self.pos.y, self:getRadius(), color)
			return
		end
		if self:isRectShape() then
			local top_left = self:getTopLeft()
			self.layer.gfx:rect(
				top_left.x,
				top_left.y,
				top_left.x + self:getWidth() - 1,
				top_left.y + self:getHeight() - 1,
				color
			)
		end
	end,

	fillShape = function(self, color)
		if not self.layer then
			return
		end
		if self:isCircleShape() then
			self.layer.gfx:circfill(self.pos.x, self.pos.y, self:getRadius(), color)
			return
		end
		if self:isRectShape() then
			local top_left = self:getTopLeft()
			self.layer.gfx:rectfill(
				top_left.x,
				top_left.y,
				top_left.x + self:getWidth() - 1,
				top_left.y + self:getHeight() - 1,
				color
			)
		end
	end,

	draw = world_object_draw,

	needsDraw = function(self)
		return self.draw ~= world_object_draw or self.fill ~= nil or self.stroke ~= nil or
			self.fill_color > -1 or self.stroke_color > -1 or self.debug
	end,

	draw_debug = function(self)
		self:strokeShape(8)
	end,

	on_collision = WorldObjectEvents.on_collision,
	on_overlap = WorldObjectEvents.on_overlap,
	destroy = WorldObjectEvents.destroy,

	unInit = function(self)
	end,
})

return WorldObject
