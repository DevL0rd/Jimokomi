local Class = include("src/Engine/Core/Class.lua")
local Sprite = include("src/Engine/Objects/Sprite.lua")

local function clone_table(source)
	if type(source) ~= "table" then
		return source
	end
	local copy = {}
	for key, value in pairs(source) do
		copy[key] = clone_table(value)
	end
	return copy
end

local AnimationController = Class:new({
	_type = "AnimationController",
	owner = nil,
	slot_name = "visual",
	definitions = nil,
	sprite = nil,
	current_state = nil,
	current_config = nil,

	init = function(self)
		self.definitions = clone_table(self.definitions or {})
	end,

	setDefinitions = function(self, definitions)
		self.definitions = clone_table(definitions or {})
	end,

	getDefinition = function(self, state)
		return self.definitions and self.definitions[state] or nil
	end,

	ensureSprite = function(self)
		if self.sprite or not self.owner then
			return self.sprite
		end
		self.sprite = Sprite:new({
			parent = self.owner,
			parent_slot = self.slot_name,
		})
		return self.sprite
	end,

	clear = function(self)
		if self.sprite then
			self.sprite:destroy()
			self.sprite = nil
		end
		self.current_state = nil
		self.current_config = nil
	end,

	applyConfig = function(self, config, reset_animation)
		local sprite = self:ensureSprite()
		if not sprite then
			return nil
		end

		if config.shape then
			sprite:setShape(config.shape)
		end
		if config.offset then
			sprite:setLocalPosition(config.offset)
		else
			sprite:setLocalPosition({ x = 0, y = 0 })
		end
		sprite.sprite = config.sprite or -1
		sprite.end_sprite = config.end_sprite or -1
		sprite.speed = config.speed or sprite.speed
		sprite.loop = config.loop ~= false
		sprite.flip_x = config.flip_x == true
		sprite.flip_y = config.flip_y == true

		if reset_animation and sprite.reset then
			sprite:reset()
		end

		self.current_config = clone_table(config)
		return sprite
	end,

	play = function(self, state, overrides)
		local definition = clone_table(self:getDefinition(state))
		if not definition then
			return nil
		end
		overrides = overrides or {}
		for key, value in pairs(overrides) do
			definition[key] = clone_table(value)
		end
		local reset_animation = self.current_state ~= state
		self.current_state = state
		return self:applyConfig(definition, reset_animation)
	end,

	setFlip = function(self, flip_x, flip_y)
		local sprite = self.sprite
		if not sprite then
			return
		end
		if flip_x ~= nil then
			sprite.flip_x = flip_x == true
		end
		if flip_y ~= nil then
			sprite.flip_y = flip_y == true
		end
	end,

	getSprite = function(self)
		return self.sprite
	end,

	getCurrentState = function(self)
		return self.current_state
	end,
})

return AnimationController
