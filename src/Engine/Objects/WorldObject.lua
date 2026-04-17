local Class = include("src/Engine/Core/Class.lua")
local Vector = include("src/Engine/Math/Vector.lua")
local WorldObjectLifecycle = include("src/Engine/Objects/WorldObject/Lifecycle.lua")
local WorldObjectTransform = include("src/Engine/Objects/WorldObject/Transform.lua")
local WorldObjectCollider = include("src/Engine/Objects/WorldObject/Collider.lua")
local WorldObjectNodes = include("src/Engine/Objects/WorldObject/Nodes.lua")
local WorldObjectSnapshot = include("src/Engine/Objects/WorldObject/Snapshot.lua")
local WorldObjectEvents = include("src/Engine/Objects/WorldObject/Events.lua")

local WorldObject = Class:new({
	_type = "WorldObject",
	is_world_object = true,
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
	inherit_layer_debug = true,
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

	getTransform = WorldObjectTransform.getTransform,
	getCollider = WorldObjectCollider.getCollider,

	setShape = WorldObjectCollider.setShape,
	setCircleShape = WorldObjectCollider.setCircleShape,
	setRectShape = WorldObjectCollider.setRectShape,
	getShapeKind = WorldObjectCollider.getShapeKind,
	isCircleShape = WorldObjectCollider.isCircleShape,
	isRectShape = WorldObjectCollider.isRectShape,
	getRadius = WorldObjectCollider.getRadius,
	getWidth = WorldObjectCollider.getWidth,
	getHeight = WorldObjectCollider.getHeight,
	getHalfWidth = WorldObjectCollider.getHalfWidth,
	getHalfHeight = WorldObjectCollider.getHalfHeight,
	getTopLeft = WorldObjectCollider.getTopLeft,

	setWorldPosition = WorldObjectTransform.setWorldPosition,
	setLocalPosition = WorldObjectTransform.setLocalPosition,
	getLocalPosition = WorldObjectTransform.getLocalPosition,
	translate = WorldObjectTransform.translate,

	emitEvent = WorldObjectEvents.emitEvent,
	hasCollisionEnterInterest = WorldObjectEvents.hasCollisionEnterInterest,
	hasCollisionStayInterest = WorldObjectEvents.hasCollisionStayInterest,
	hasCollisionExitInterest = WorldObjectEvents.hasCollisionExitInterest,
	hasAnyCollisionInterest = WorldObjectEvents.hasAnyCollisionInterest,
	hasOverlapEnterInterest = WorldObjectEvents.hasOverlapEnterInterest,
	hasOverlapStayInterest = WorldObjectEvents.hasOverlapStayInterest,
	hasOverlapExitInterest = WorldObjectEvents.hasOverlapExitInterest,
	hasAnyOverlapInterest = WorldObjectEvents.hasAnyOverlapInterest,

	getCollisionLayer = WorldObjectCollider.getCollisionLayer,
	setCollisionLayer = WorldObjectCollider.setCollisionLayer,
	allowsCollisionLayer = WorldObjectCollider.allowsCollisionLayer,
	setCollisionMask = WorldObjectCollider.setCollisionMask,
	isTrigger = WorldObjectCollider.isTrigger,
	canCollideWith = WorldObjectCollider.canCollideWith,

	ensureSlots = WorldObjectTransform.ensureSlots,
	defineSlot = WorldObjectTransform.defineSlot,
	getSlot = WorldObjectTransform.getSlot,
	hasSlot = WorldObjectTransform.hasSlot,
	setSlotPosition = WorldObjectTransform.setSlotPosition,
	getSlotLocalPosition = WorldObjectTransform.getSlotLocalPosition,
	getSlotWorldPosition = WorldObjectTransform.getSlotWorldPosition,
	isSlotEnabled = WorldObjectTransform.isSlotEnabled,
	setSlotEnabled = WorldObjectTransform.setSlotEnabled,
	setSlotsEnabled = WorldObjectTransform.setSlotsEnabled,
	setSlotOffset = WorldObjectTransform.setSlotOffset,
	getSlotOffset = WorldObjectTransform.getSlotOffset,
	getAttachment = WorldObjectTransform.getAttachment,
	isAttached = WorldObjectTransform.isAttached,
	isAttachmentEnabled = WorldObjectTransform.isAttachmentEnabled,
	setAttachmentEnabled = WorldObjectTransform.setAttachmentEnabled,
	shouldSyncToParent = WorldObjectTransform.shouldSyncToParent,
	attachTo = WorldObjectTransform.attachTo,
	attachChild = WorldObjectTransform.attachChild,
	getAttachedChildren = WorldObjectTransform.getAttachedChildren,
	detach = WorldObjectTransform.detach,
	syncToParent = WorldObjectTransform.syncToParent,
	destroySlot = WorldObjectTransform.destroySlot,
	destroySlots = WorldObjectTransform.destroySlots,

	getAssets = WorldObjectLifecycle.getAssets,
	addAttachmentNode = WorldObjectNodes.addAttachmentNode,
	removeAttachmentNode = WorldObjectNodes.removeAttachmentNode,
	updateAttachmentNodes = WorldObjectNodes.updateAttachmentNodes,
	drawAttachmentNodes = WorldObjectNodes.drawAttachmentNodes,
	destroyAttachmentNodes = WorldObjectNodes.destroyAttachmentNodes,

	isSnapshotEnabled = function(self)
		return self.snapshot_enabled ~= false
	end,

	getWorld = WorldObjectLifecycle.getWorld,

	getDebugSnapshot = WorldObjectSnapshot.getDebugSnapshot,
	toSnapshot = WorldObjectSnapshot.toSnapshot,
	applySnapshot = function(self, snapshot)
		return WorldObjectSnapshot.applySnapshot(self, snapshot, WorldObjectLifecycle.cloneVector)
	end,

	exportState = function(self)
		return nil
	end,

	importState = function(self, state)
	end,

	init = WorldObjectLifecycle.init,
	didAddToLayer = WorldObjectEvents.didAddToLayer,
	willRemoveFromLayer = WorldObjectEvents.willRemoveFromLayer,
	didRemoveFromLayer = WorldObjectEvents.didRemoveFromLayer,

	update = WorldObjectLifecycle.update,
	needsUpdate = WorldObjectLifecycle.needsUpdate,
	strokeShape = WorldObjectCollider.strokeShape,
	fillShape = WorldObjectCollider.fillShape,
	draw = WorldObjectLifecycle.draw,
	needsDraw = WorldObjectLifecycle.needsDraw,
	draw_debug = WorldObjectCollider.draw_debug,

	on_collision = WorldObjectEvents.on_collision,
	on_collision_enter = WorldObjectEvents.on_collision_enter,
	on_collision_stay = WorldObjectEvents.on_collision_stay,
	on_collision_exit = WorldObjectEvents.on_collision_exit,
	on_overlap = WorldObjectEvents.on_overlap,
	on_overlap_enter = WorldObjectEvents.on_overlap_enter,
	on_overlap_stay = WorldObjectEvents.on_overlap_stay,
	on_overlap_exit = WorldObjectEvents.on_overlap_exit,
	destroy = WorldObjectEvents.destroy,

	unInit = WorldObjectLifecycle.unInit,
})

return WorldObject
