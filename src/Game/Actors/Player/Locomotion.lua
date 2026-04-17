local Class = include("src/Engine/Core/Class.lua")

local PlayerLocomotion = Class:new({
	_type = "PlayerLocomotion",
	owner = nil,
	states = nil,
	directions = nil,

	getCurrentTile = function(self)
		local world = self.owner:getWorld()
		return world and world:getTileAt(self.owner.pos.x, self.owner.pos.y) or 0
	end,

	isClimbableTile = function(self, tile_id)
		return tile_id ~= 0 and tile_id ~= 15
	end,

	distToGround = function(self)
		local _, dist = self.owner.down_ray:cast()
		return dist
	end,

	isOnGround = function(self)
		return self:distToGround() <= 0.001
	end,

	isCloseToGround = function(self)
		return self:distToGround() <= 2
	end,

	jump = function(self, input)
		if not self.owner.jump_timer:hasElapsed(500) then
			return false
		end

		if self.owner.state == self.states.Running then
			self.owner.vel.y -= self.owner.jump_speed
			if input.left or input.right or input.up or input.down then
				if self.owner.direction == self.directions.left then
					self.owner.vel.x -= self.owner.jump_speed * 0.5
				elseif self.owner.direction == self.directions.right then
					self.owner.vel.x += self.owner.jump_speed * 0.5
				end
			end
			return true
		end

		if self.owner.state == self.states.Climbing then
			self.owner.state = self.states.Gliding
			if self.owner.direction == self.directions.left then
				self.owner.vel.y -= self.owner.jump_speed
				self.owner.vel.x -= self.owner.jump_speed * 0.5
			elseif self.owner.direction == self.directions.right then
				self.owner.vel.y -= self.owner.jump_speed
				self.owner.vel.x += self.owner.jump_speed * 0.5
			elseif self.owner.direction == self.directions.up then
				self.owner.vel.y -= self.owner.jump_speed
			elseif self.owner.direction == self.directions.down then
				self.owner.vel.y += self.owner.jump_speed
			end
			return true
		end

		if self.owner.state == self.states.Gliding and self:isClimbableTile(self:getCurrentTile()) then
			self.owner.state = self.states.Climbing
			return true
		end

		return false
	end,

	updateStateFromWorld = function(self, input)
		local current_tile = self:getCurrentTile()
		if self.owner.layer and self.owner.layer.engine and self.owner.layer.engine.debug then
			self.owner.layer.engine:appendDebug("Current tile: " .. current_tile)
		end

		if self:isClimbableTile(current_tile) and (input.up or input.down) then
			self.owner.state = self.states.Climbing
		end

		if self.owner.state == self.states.Climbing and not self:isClimbableTile(current_tile) then
			self.owner.state = self.states.Gliding
		end

		if self.owner.state ~= self.states.Climbing then
			if self:isCloseToGround() then
				self.owner.state = self.states.Running
			else
				self.owner.state = self.states.Gliding
			end
		end
	end,

	applyMovement = function(self, input, is_moving)
		self.owner.runEmitter:off()
		self.owner.ignore_gravity = false

		if self.owner.state == self.states.Climbing then
			self.owner.climb_behavior:updateMovement(input)
			return
		end

		self.owner.climb_behavior:stop()

		if self.owner.state == self.states.Running then
			if is_moving then
				self.owner.run_behavior:updateMovement(input)
				self.owner.runEmitter.vec.x = -self.owner.vel.x * 0.2
				self.owner.runEmitter.vec.y = -30
				self.owner.runEmitter:on()
			end
			return
		end

		if self.owner.state == self.states.Gliding then
			self.owner.did_ground_pound = false
			self.owner.glide_behavior:updateMovement(input)
		end
	end,
})

return PlayerLocomotion
