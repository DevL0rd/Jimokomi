local FrameClock = include("src/Engine/Core/FrameClock.lua")
local DebugBuffer = include("src/Engine/Core/DebugBuffer.lua")
local EventBus = include("src/Engine/Core/EventBus.lua")
local DebugOverlay = include("src/Engine/Core/DebugOverlay.lua")
local SaveSystem = include("src/Engine/Core/SaveSystem.lua")
local Profiler = include("src/Engine/Core/Profiler.lua")

local Services = {}

Services.ensureServices = function(self)
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
	if not self.profiler then
		self.profiler = Profiler:new()
	end
	if self.profiler and self.profiler.setEnabled then
		self.profiler:setEnabled(self.profiler_enabled ~= false)
	end
	if self.debug_overlay_enabled and not self.debug_overlay then
		self.debug_overlay = DebugOverlay:new()
	end
end

Services.nextObjectId = function(self)
	self.next_object_id += 1
	return self.next_object_id
end

Services.on = function(self, name, handler)
	self:ensureServices()
	return self.events:on(name, handler)
end

Services.once = function(self, name, handler)
	self:ensureServices()
	return self.events:once(name, handler)
end

Services.off = function(self, name, handler)
	if not self.events then
		return false
	end
	return self.events:off(name, handler)
end

Services.emit = function(self, name, payload)
	self:ensureServices()
	return self.events:emit(name, payload, false)
end

Services.appendDebug = function(self, value)
	self:ensureServices()
	self.debug_buffer:append(value)
end

Services.clearDebug = function(self)
	if self.debug_buffer ~= nil then
		self.debug_buffer:clear()
	end
end

Services.registerClass = function(self, class_obj)
	self:ensureServices()
	return self.save_system:registerClass(class_obj)
end

Services.registerClasses = function(self, classes)
	self:ensureServices()
	return self.save_system:registerClasses(classes)
end

Services.saveSnapshot = function(self, snapshot, filename, snapshot_type)
	self:ensureServices()
	return self.save_system:saveSnapshot(snapshot, filename, snapshot_type)
end

Services.loadSnapshot = function(self, filename)
	self:ensureServices()
	return self.save_system:loadSnapshot(filename)
end

Services.saveObject = function(self, obj, filename)
	self:ensureServices()
	return self.save_system:saveObject(obj, filename)
end

Services.loadObject = function(self, filename)
	self:ensureServices()
	return self.save_system:loadObject(filename)
end

Services.save = function(self, filename)
	self:ensureServices()
	return self.save_system:saveEngine(self, filename)
end

Services.load = function(self, filename)
	self:ensureServices()
	return self.save_system:loadEngine(self, filename)
end

Services.saveExists = function(self, filename)
	self:ensureServices()
	return self.save_system:saveExists(filename)
end

Services.getSaveInfo = function(self, filename)
	self:ensureServices()
	return self.save_system:getSaveInfo(filename)
end

return Services
