local Class = include("src/Engine/Core/Class.lua")

local function buildEvent(name, payload, bus)
	local event = {
		name = name,
		payload = payload or {},
		bus = bus,
		stopped = false,
		stop = function(self)
			self.stopped = true
		end,
	}
	return event
end

local EventBus = Class:new({
	_type = "EventBus",
	listeners = nil,
	parent_bus = nil,

	init = function(self)
		self.listeners = {}
	end,

	getListeners = function(self, name)
		if self.listeners[name] == nil then
			self.listeners[name] = {}
		end
		return self.listeners[name]
	end,

	on = function(self, name, handler)
		local listeners = self:getListeners(name)
		local listener = {
			handler = handler,
			once = false,
		}
		add(listeners, listener)
		return listener
	end,

	once = function(self, name, handler)
		local listeners = self:getListeners(name)
		local listener = {
			handler = handler,
			once = true,
		}
		add(listeners, listener)
		return listener
	end,

	off = function(self, name, handler_or_listener)
		local listeners = self.listeners[name]
		if listeners == nil then
			return false
		end

		for i = #listeners, 1, -1 do
			local listener = listeners[i]
			if listener == handler_or_listener or listener.handler == handler_or_listener then
				deli(listeners, i)
				return true
			end
		end

		return false
	end,

	clear = function(self, name)
		if name ~= nil then
			self.listeners[name] = {}
			return
		end
		self.listeners = {}
	end,

	hasLocalListeners = function(self, name)
		local listeners = self.listeners[name]
		if listeners and #listeners > 0 then
			return true
		end
		local wildcard_listeners = self.listeners["*"]
		return wildcard_listeners ~= nil and #wildcard_listeners > 0
	end,

	hasListeners = function(self, name, bubble)
		if self:hasLocalListeners(name) then
			return true
		end
		if bubble ~= false and self.parent_bus and self.parent_bus.hasListeners then
			return self.parent_bus:hasListeners(name, true)
		end
		return false
	end,

	dispatchListeners = function(self, name, event)
		local listeners = self.listeners[name]
		if listeners == nil then
			return
		end

		for i = #listeners, 1, -1 do
			local listener = listeners[i]
			listener.handler(event)
			if listener.once then
				deli(listeners, i)
			end
			if event.stopped then
				return
			end
		end
	end,

	emit = function(self, name, payload, bubble)
		if not self:hasListeners(name, bubble) then
			return nil
		end
		local event = buildEvent(name, payload, self)
		self:dispatchListeners(name, event)
		if not event.stopped then
			self:dispatchListeners("*", event)
		end

		if bubble ~= false and not event.stopped and self.parent_bus then
			self.parent_bus:emit(name, payload, true)
		end

		return event
	end,
})

return EventBus
