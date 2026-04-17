local Class = include("src/Engine/Core/Class.lua")

local PlayerVisuals = Class:new({
	_type = "PlayerVisuals",
	owner = nil,
	states = nil,
	directions = nil,

	getVisualState = function(self, is_moving)
		if self.owner.state == self.states.Running then
			return is_moving and "run_move" or "run_idle", self.owner.direction == self.directions.right, false
		end

		if self.owner.state == self.states.Gliding then
			return "glide", self.owner.direction == self.directions.right, false
		end

		if self.owner.direction == self.directions.up or self.owner.direction == self.directions.down then
			return is_moving and "climb_vertical_move" or "climb_vertical_idle", false,
				self.owner.direction == self.directions.down
		end

		return is_moving and "climb_horizontal_move" or "climb_horizontal_idle",
			self.owner.direction == self.directions.right, false
	end,

	updateVisualState = function(self, is_moving)
		local visual_state, flip_x, flip_y = self:getVisualState(is_moving)
		local state_has_changed = visual_state ~= self.owner.current_visual_state or
			flip_x ~= self.owner.current_visual_flip_x or
			flip_y ~= self.owner.current_visual_flip_y

		if not state_has_changed then
			return
		end

		if self.owner.state == self.states.Running and self.owner.current_visual_state ~= visual_state and
			not self.owner.did_ground_pound then
			self.owner.layer.camera:startShaking(5, 500)
			self.owner.did_ground_pound = true
		end

		self.owner.current_visual_state = visual_state
		self.owner.current_visual_flip_x = flip_x
		self.owner.current_visual_flip_y = flip_y
		self.owner:playVisual(visual_state, {
			flip_x = flip_x,
			flip_y = flip_y,
		})
	end,
})

return PlayerVisuals
