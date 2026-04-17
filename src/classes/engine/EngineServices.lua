local FrameClock = include("src/classes/FrameClock.lua")
local DebugBuffer = include("src/classes/DebugBuffer.lua")
local EventBus = include("src/classes/EventBus.lua")
local AssetRegistry = include("src/classes/AssetRegistry.lua")
local DebugOverlay = include("src/classes/DebugOverlay.lua")

local EngineServices = {}

EngineServices.ensureServices = function(self)
	if not self.clock then
		self.clock = FrameClock:new()
	end
	if not self.debug_buffer then
		self.debug_buffer = DebugBuffer:new()
	end
	if not self.events then
		self.events = EventBus:new()
	end
	if not self.assets then
		self.assets = AssetRegistry:new()
		local world_config = self.assets:getWorldConfig()
		if world_config.size then
			self.w = world_config.size.w or self.w
			self.h = world_config.size.h or self.h
		end
	end
	if self.debug and not self.debug_overlay then
		self.debug_overlay = DebugOverlay:new()
	end
end

EngineServices.nextObjectId = function(self)
	self.next_object_id += 1
	return self.next_object_id
end

EngineServices.on = function(self, name, handler)
	self:ensureServices()
	return self.events:on(name, handler)
end

EngineServices.once = function(self, name, handler)
	self:ensureServices()
	return self.events:once(name, handler)
end

EngineServices.off = function(self, name, handler)
	if not self.events then
		return false
	end
	return self.events:off(name, handler)
end

EngineServices.emit = function(self, name, payload)
	self:ensureServices()
	return self.events:emit(name, payload, false)
end

EngineServices.appendDebug = function(self, value)
	self:ensureServices()
	self.debug_buffer:append(value)
end

EngineServices.clearDebug = function(self)
	if self.debug_buffer ~= nil then
		self.debug_buffer:clear()
	end
end

return EngineServices
