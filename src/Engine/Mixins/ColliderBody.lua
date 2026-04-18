local WorldObjectCollider = include("src/Engine/Objects/WorldObject/Collider.lua")

local ColliderBody = {
	shape = nil,
	stroke_color = -1,
	fill_color = -1,
	debug_collision_interest = false,
	ignore_collisions = false,
	collision_layer = "default",
	collision_mask = nil,
	is_trigger = false,
	resolve_entity_collisions = false,
}

ColliderBody.mixin = function(self)
	if self._collider_body_mixin_applied then
		return
	end
	self._collider_body_mixin_applied = true
	for key, value in pairs(ColliderBody) do
		if key ~= "mixin" and key ~= "init" and self[key] == nil then
			self[key] = value
		end
	end
end

ColliderBody.init = function(self)
	ColliderBody.mixin(self)
end

ColliderBody.getCollider = WorldObjectCollider.getCollider
ColliderBody.setShape = WorldObjectCollider.setShape
ColliderBody.setCircleShape = WorldObjectCollider.setCircleShape
ColliderBody.setRectShape = WorldObjectCollider.setRectShape
ColliderBody.getShapeKind = WorldObjectCollider.getShapeKind
ColliderBody.isCircleShape = WorldObjectCollider.isCircleShape
ColliderBody.isRectShape = WorldObjectCollider.isRectShape
ColliderBody.getRadius = WorldObjectCollider.getRadius
ColliderBody.getWidth = WorldObjectCollider.getWidth
ColliderBody.getHeight = WorldObjectCollider.getHeight
ColliderBody.getHalfWidth = WorldObjectCollider.getHalfWidth
ColliderBody.getHalfHeight = WorldObjectCollider.getHalfHeight
ColliderBody.getTopLeft = WorldObjectCollider.getTopLeft
ColliderBody.getCollisionLayer = WorldObjectCollider.getCollisionLayer
ColliderBody.setCollisionLayer = WorldObjectCollider.setCollisionLayer
ColliderBody.allowsCollisionLayer = WorldObjectCollider.allowsCollisionLayer
ColliderBody.setCollisionMask = WorldObjectCollider.setCollisionMask
ColliderBody.isTrigger = WorldObjectCollider.isTrigger
ColliderBody.canCollideWith = WorldObjectCollider.canCollideWith
ColliderBody.strokeShape = WorldObjectCollider.strokeShape
ColliderBody.fillShape = WorldObjectCollider.fillShape
ColliderBody.draw_debug = WorldObjectCollider.draw_debug

return ColliderBody
