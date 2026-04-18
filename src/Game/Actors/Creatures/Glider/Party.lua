local Class = include("src/Engine/Core/Class.lua")
local Agent = include("src/Game/Mixins/Agent.lua")

local Party = Class:new({
	_type = "Party",
	layer = nil,
	players = nil,
	active_index = 1,
	last_control_swap_frame = nil,

	init = function(self)
		self.players = self.players or {}
	end,

	addPlayer = function(self, player)
		if not player then
			return
		end
		add(self.players, player)
		player.party = self
	end,

	getActivePlayer = function(self)
		return self.players[self.active_index]
	end,

	applyActivePlayer = function(self)
		local active_player = self:getActivePlayer()
		for i = 1, #self.players do
			local player = self.players[i]
			if player and player.setControlMode then
				player:setControlMode(i == self.active_index and Agent.ControlModes.Player or Agent.ControlModes.Autonomous)
			end
		end
		if self.layer then
			self.layer:setPlayer(active_player)
			if self.layer.camera and active_player then
				self.layer.camera:setTarget(active_player)
			end
		end
	end,

	setActiveIndex = function(self, index)
		if #self.players == 0 then
			return nil
		end
		self.active_index = mid(1, index, #self.players)
		self:applyActivePlayer()
		return self:getActivePlayer()
	end,

	cycleControlledPlayer = function(self)
		if #self.players == 0 then
			return nil
		end
		local next_index = self.active_index + 1
		if next_index > #self.players then
			next_index = 1
		end
		return self:setActiveIndex(next_index)
	end,

	handleControlSwapInput = function(self)
		if not btnp or not btnp(5) then
			return nil
		end
		local frame_id = flr(time() * 60)
		if self.last_control_swap_frame == frame_id then
			return nil
		end
		self.last_control_swap_frame = frame_id
		return self:cycleControlledPlayer()
	end,
})

return Party
