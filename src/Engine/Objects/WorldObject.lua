local Class = include("src/Engine/Core/Class.lua")
local WorldObjectLifecycle = include("src/Engine/Objects/WorldObject/Lifecycle.lua")
local WorldObjectTransform = include("src/Engine/Objects/WorldObject/Transform.lua")
local WorldObjectNodes = include("src/Engine/Objects/WorldObject/Nodes.lua")
local WorldObjectSnapshot = include("src/Engine/Objects/WorldObject/Snapshot.lua")
local WorldObjectEvents = include("src/Engine/Objects/WorldObject/Events.lua")
local PhysicsBody = include("src/Engine/Mixins/PhysicsBody.lua")
local ColliderBody = include("src/Engine/Mixins/ColliderBody.lua")

local WorldObject = Class:new({
	_type = "WorldObject",
	entity_kind = 0,
	is_world_object = true,
	name = "WorldObject",
	object_id = nil,
	pos = nil,
	debug = false,
	inherit_layer_debug = true,
	snapshot_enabled = true,
	lifetime = -1,
	parent = nil,
	parent_slot = nil,
	layer = nil,
	slot_defs = nil,
	slot_offset = nil,
	slots_enabled = true,
	default_slot_destroy_policy = "detach",

	getTransform = WorldObjectTransform.getTransform,
	getCollider = ColliderBody.getCollider,

	setShape = ColliderBody.setShape,
	setCircleShape = ColliderBody.setCircleShape,
	setRectShape = ColliderBody.setRectShape,
	getShapeKind = ColliderBody.getShapeKind,
	isCircleShape = ColliderBody.isCircleShape,
	isRectShape = ColliderBody.isRectShape,
	getRadius = ColliderBody.getRadius,
	getWidth = ColliderBody.getWidth,
	getHeight = ColliderBody.getHeight,
	getHalfWidth = ColliderBody.getHalfWidth,
	getHalfHeight = ColliderBody.getHalfHeight,
	getTopLeft = ColliderBody.getTopLeft,

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

	getCollisionLayer = ColliderBody.getCollisionLayer,
	setCollisionLayer = ColliderBody.setCollisionLayer,
	allowsCollisionLayer = ColliderBody.allowsCollisionLayer,
	setCollisionMask = ColliderBody.setCollisionMask,
	isTrigger = ColliderBody.isTrigger,
	canCollideWith = ColliderBody.canCollideWith,

	ensureSlots = WorldObjectTransform.ensureSlots,
	defineSlot = WorldObjectTransform.defineSlot,
	getSlot = WorldObjectTransform.getSlot,
	getSlotNames = WorldObjectTransform.getSlotNames,
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
	drawSingleAttachmentSpriteFast = WorldObjectNodes.drawSingleAttachmentSpriteFast,
	destroyAttachmentNodes = WorldObjectNodes.destroyAttachmentNodes,

	isSnapshotEnabled = function(self)
		return self.snapshot_enabled ~= false
	end,

	getWorld = WorldObjectLifecycle.getWorld,
	getSpriteRotationAngle = WorldObjectLifecycle.getSpriteRotationAngle,
	getSpriteRotationBucketCount = WorldObjectLifecycle.getSpriteRotationBucketCount,
	drawSharedSprite = WorldObjectLifecycle.drawSharedSprite,
	markSpatialDirty = WorldObjectLifecycle.markSpatialDirty,
	isSpatialStatic = WorldObjectLifecycle.isSpatialStatic,
	initPhysicsBody = PhysicsBody.init,
	canSleepPhysics = PhysicsBody.canSleepPhysics,
	isPhysicsSleeping = PhysicsBody.isPhysicsSleeping,
	sleepPhysics = PhysicsBody.sleepPhysics,
	wakePhysics = PhysicsBody.wakePhysics,
	updatePhysicsSleepState = PhysicsBody.updatePhysicsSleepState,
	canRotatePhysics = PhysicsBody.canRotatePhysics,
	isRotationLocked = PhysicsBody.isRotationLocked,
	setRotationLocked = PhysicsBody.setRotationLocked,
	getSharedCacheIdentity = function(self)
		return tostr(self._type or self.name or "world_object")
	end,

	getDebugSnapshot = WorldObjectSnapshot.getDebugSnapshot,
	toSnapshot = WorldObjectSnapshot.toSnapshot,
	applySnapshot = function(self, snapshot)
		return WorldObjectSnapshot.applySnapshot(self, snapshot, PhysicsBody.cloneVector)
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
	strokeShape = ColliderBody.strokeShape,
	fillShape = ColliderBody.fillShape,
	draw = WorldObjectLifecycle.draw,
	needsDraw = WorldObjectLifecycle.needsDraw,
	draw_debug = ColliderBody.draw_debug,

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

for key, value in pairs(PhysicsBody) do
	if key ~= "mixin" and key ~= "init" and WorldObject[key] == nil then
		WorldObject[key] = value
	end
end

for key, value in pairs(ColliderBody) do
	if key ~= "mixin" and key ~= "init" and WorldObject[key] == nil then
		WorldObject[key] = value
	end
end

return WorldObject
