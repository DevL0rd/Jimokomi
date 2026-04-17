local FrameClock = include("src/Engine/Core/FrameClock.lua")
local DebugBuffer = include("src/Engine/Core/DebugBuffer.lua")
local EventBus = include("src/Engine/Core/EventBus.lua")
local DebugOverlay = include("src/Engine/Core/DebugOverlay.lua")
local SaveSystem = include("src/Engine/Core/SaveSystem.lua")

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
	if not self.save_system then
		self.save_system = SaveSystem:new()
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

EngineServices.registerClass = function(self, class_obj)
	self:ensureServices()
	return self.save_system:registerClass(class_obj)
end

EngineServices.registerClasses = function(self, classes)
	self:ensureServices()
	return self.save_system:registerClasses(classes)
end

EngineServices.saveSnapshot = function(self, snapshot, filename, snapshot_type)
	self:ensureServices()
	return self.save_system:saveSnapshot(snapshot, filename, snapshot_type)
end

EngineServices.loadSnapshot = function(self, filename)
	self:ensureServices()
	return self.save_system:loadSnapshot(filename)
end

EngineServices.saveObject = function(self, obj, filename)
	self:ensureServices()
	return self.save_system:saveObject(obj, filename)
end

EngineServices.loadObject = function(self, filename)
	self:ensureServices()
	return self.save_system:loadObject(filename)
end

EngineServices.save = function(self, filename)
	self:ensureServices()
	return self.save_system:saveEngine(self, filename)
end

EngineServices.load = function(self, filename)
	self:ensureServices()
	return self.save_system:loadEngine(self, filename)
end

EngineServices.saveExists = function(self, filename)
	self:ensureServices()
	return self.save_system:saveExists(filename)
end

EngineServices.getSaveInfo = function(self, filename)
	self:ensureServices()
	return self.save_system:getSaveInfo(filename)
end

return EngineServices
