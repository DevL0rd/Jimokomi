local AnimationController = include("src/classes/AnimationController.lua")

local VisualOwner = {}

VisualOwner.mixin = function(self)
	if self._visual_owner_mixin_applied then
		return
	end
	self._visual_owner_mixin_applied = true
	for key, value in pairs(VisualOwner) do
		if key ~= "mixin" and key ~= "init" and type(value) == "function" and self[key] == nil then
			self[key] = value
		end
	end
end

VisualOwner.init = function(self)
	VisualOwner.mixin(self)
	if self.defineSlot and not self:hasSlot("visual") then
		self:defineSlot("visual", {
			x = 0,
			y = 0,
			destroy_policy = "destroy_children",
			max_children = 1,
		})
	end
	if self.hasSlot and self:hasSlot("visual") then
		local slot = self:getSlot("visual")
		slot.destroy_policy = "destroy_children"
		slot.max_children = 1
	end
	local definitions = self.visual_definitions
	if self.layer and self.layer.engine and self.layer.engine.assets and self.visual_key then
		definitions = self.layer.engine.assets:getVisualSet(self.visual_key) or definitions
	end
	self.visual = AnimationController:new({
		owner = self,
		slot_name = self.visual_slot or "visual",
		definitions = definitions or {},
	})
end

VisualOwner.setVisualDefinitions = function(self, definitions)
	if not self.visual then
		VisualOwner.init(self)
	end
	self.visual:setDefinitions(definitions or {})
end

VisualOwner.playVisual = function(self, state, overrides)
	if not self.visual then
		VisualOwner.init(self)
	end
	return self.visual:play(state, overrides)
end

VisualOwner.setVisualFlip = function(self, flip_x, flip_y)
	if self.visual then
		self.visual:setFlip(flip_x, flip_y)
	end
end

VisualOwner.getVisualSprite = function(self)
	return self.visual and self.visual:getSprite() or nil
end

VisualOwner.clearVisual = function(self)
	if self.visual then
		self.visual:clear()
	end
end

return VisualOwner
