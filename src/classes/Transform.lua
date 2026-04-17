local Class = include("src/classes/Class.lua")
local TransformPosition = include("src/classes/scene/TransformPosition.lua")
local TransformSlots = include("src/classes/scene/TransformSlots.lua")
local TransformAttachments = include("src/classes/scene/TransformAttachments.lua")
local TransformSnapshot = include("src/classes/scene/TransformSnapshot.lua")

local Transform = Class:new({
	_type = "Transform",
	owner = nil,
	position = nil,
	local_position = nil,
	slots = nil,
	slot_defs = nil,
	attachment = nil,
	slots_enabled = true,
	default_slot_destroy_policy = "detach",

	init = function(self)
		TransformPosition.init(self)
		self.slots = {}
		self:ensureSlots()
		self:applySlotDefinitions()
	end,

	emit = TransformAttachments.emit,
	ensureSlots = TransformSlots.ensureSlots,
	createSlotRecord = TransformSlots.createSlotRecord,
	applySlotDefinitions = TransformSlots.applySlotDefinitions,
	defineSlot = TransformSlots.defineSlot,
	getSlot = TransformSlots.getSlot,
	getSlotNames = TransformSlots.getSlotNames,
	hasSlot = TransformSlots.hasSlot,
	setSlotPosition = TransformSlots.setSlotPosition,
	getSlotLocalPosition = TransformSlots.getSlotLocalPosition,
	getSlotWorldPosition = TransformSlots.getSlotWorldPosition,
	isSlotEnabled = TransformSlots.isSlotEnabled,
	setSlotEnabled = TransformSlots.setSlotEnabled,
	setSlotsEnabled = TransformSlots.setSlotsEnabled,
	canAttachToSlot = TransformSlots.canAttachToSlot,
	setLocalPosition = TransformPosition.setLocalPosition,
	getLocalPosition = TransformPosition.getLocalPosition,
	setWorldPosition = TransformPosition.setWorldPosition,
	translate = TransformPosition.translate,
	getSlotOffset = TransformPosition.getSlotOffset,
	createAttachment = TransformAttachments.createAttachment,
	getAttachment = TransformAttachments.getAttachment,
	isAttached = TransformAttachments.isAttached,
	isAttachmentEnabled = TransformAttachments.isAttachmentEnabled,
	setAttachmentEnabled = TransformAttachments.setAttachmentEnabled,
	shouldSync = TransformAttachments.shouldSync,
	attachTo = TransformAttachments.attachTo,
	removeChildFromSlot = TransformSlots.removeChildFromSlot,
	getAttachedChildren = TransformSlots.getAttachedChildren,
	detach = TransformAttachments.detach,
	sync = TransformAttachments.sync,
	destroySlot = TransformSlots.destroySlot,
	destroySlots = TransformSlots.destroySlots,
	toSnapshot = TransformSnapshot.toSnapshot,
	applySnapshot = TransformSnapshot.applySnapshot,
})

return Transform
