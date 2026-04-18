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
	if (self.debug_overlay_enabled or self.performance_panel_enabled) and not self.debug_overlay then
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

local function normalize_engine_render_cache_mode(mode)
	if mode == nil or mode == true or mode == "default" then
		return 3
	end
	if mode == false or mode == "off" or mode == "immediate" or mode == "disabled" then
		return 2
	end
	if mode == "auto" then
		return 1
	end
	if mode == 1 or mode == 2 or mode == 3 or mode == 4 or mode == 5 then
		return mode
	end
	return 3
end

Services.setRenderCacheMode = function(self, mode)
	self.render_cache_mode = normalize_engine_render_cache_mode(mode)
	self.render_cache_enabled = self.render_cache_mode ~= 2
	for _, layer in pairs(self.layers or {}) do
		if layer and layer.gfx then
			if layer.gfx.setRenderCacheMode then
				layer.gfx:setRenderCacheMode(self.render_cache_mode)
			else
				layer.gfx:setRenderCacheEnabled(self.render_cache_enabled)
			end
			layer.gfx.cache_tag_modes = self.render_cache_tag_modes or {}
			layer.gfx.render_cache_entry_ttl_ms = self.render_cache_entry_ttl_ms or layer.gfx.render_cache_entry_ttl_ms
			layer.gfx.render_cache_max_entries = self.render_cache_max_entries or layer.gfx.render_cache_max_entries
			layer.gfx.render_cache_prune_interval_ms = self.render_cache_prune_interval_ms or layer.gfx.render_cache_prune_interval_ms
			if layer.gfx.clearRenderCache then
				layer.gfx:clearRenderCache()
			end
			if layer.gfx.clearPanelCache then
				layer.gfx:clearPanelCache()
			end
		end
	end
end

Services.setRenderCacheEnabled = function(self, enabled)
	self:setRenderCacheMode(enabled == true and 3 or 2)
end

local function refresh_layer_cache_modes(self)
	for _, layer in pairs(self.layers or {}) do
		if layer and layer.gfx then
			layer.gfx.cache_tag_modes = self.render_cache_tag_modes or {}
			layer.gfx.render_cache_entry_ttl_ms = self.render_cache_entry_ttl_ms or layer.gfx.render_cache_entry_ttl_ms
			layer.gfx.render_cache_max_entries = self.render_cache_max_entries or layer.gfx.render_cache_max_entries
			layer.gfx.render_cache_prune_interval_ms = self.render_cache_prune_interval_ms or layer.gfx.render_cache_prune_interval_ms
			layer.gfx.debug_text_cache_enabled = self.debug_text_cache_enabled ~= false
			layer.gfx.debug_shape_cache_enabled = self.debug_shape_cache_enabled == true
			if layer.gfx.clearRenderCache then
				layer.gfx:clearRenderCache()
			end
			if layer.gfx.clearPanelCache then
				layer.gfx:clearPanelCache()
			end
		end
	end
end

Services.setDebugTextCacheEnabled = function(self, enabled)
	self.debug_text_cache_enabled = enabled == true
	refresh_layer_cache_modes(self)
end

Services.setDebugShapeCacheEnabled = function(self, enabled)
	self.debug_shape_cache_enabled = enabled == true
	refresh_layer_cache_modes(self)
end

local function refresh_cache_tags(self)
	self.render_cache_tag_modes = self.render_cache_tag_modes or {}
	for _, layer in pairs(self.layers or {}) do
		if layer and layer.gfx then
			layer.gfx.cache_tag_modes = self.render_cache_tag_modes
			layer.gfx.render_cache_entry_ttl_ms = self.render_cache_entry_ttl_ms or layer.gfx.render_cache_entry_ttl_ms
			layer.gfx.render_cache_max_entries = self.render_cache_max_entries or layer.gfx.render_cache_max_entries
			layer.gfx.render_cache_prune_interval_ms = self.render_cache_prune_interval_ms or layer.gfx.render_cache_prune_interval_ms
			if layer.gfx.clearRenderCache then
				layer.gfx:clearRenderCache()
			end
			if layer.gfx.clearPanelCache then
				layer.gfx:clearPanelCache()
			end
		end
	end
end

local function refresh_spatial_settings(self)
	for _, layer in pairs(self.layers or {}) do
		if layer then
			local spatial_cell_size = self.spatial_index_cell_size or layer.spatial_index_cell_size or 32
			local broadphase_cell_size = self.collision_broadphase_cell_size or spatial_cell_size
			layer.spatial_index_cell_size = spatial_cell_size
			layer.collision_broadphase_cell_size = broadphase_cell_size
			if layer.entity_spatial_index then
				layer.entity_spatial_index.cell_size = spatial_cell_size
			end
			if layer.entity_static_spatial_index then
				layer.entity_static_spatial_index.cell_size = spatial_cell_size
			end
			if layer.collidable_spatial_index then
				layer.collidable_spatial_index.cell_size = spatial_cell_size
			end
			if layer.collidable_static_spatial_index then
				layer.collidable_static_spatial_index.cell_size = spatial_cell_size
			end
			if layer.collision_dynamic_spatial_index then
				layer.collision_dynamic_spatial_index.cell_size = broadphase_cell_size
			end
			if layer.collision_static_spatial_index then
				layer.collision_static_spatial_index.cell_size = broadphase_cell_size
			end
			if layer.collision and layer.collision.broadphase then
				layer.collision.broadphase.cell_size = broadphase_cell_size
			end
			layer.entity_spatial_dirty = true
			layer.collidable_spatial_dirty = true
			layer.entity_static_spatial_dirty = true
			layer.collidable_static_spatial_dirty = true
			layer.collision_dynamic_spatial_dirty = true
			layer.collision_static_spatial_dirty = true
		end
	end
end

Services.setRenderCacheTagMode = function(self, tag, mode)
	if not tag or tag == "" then
		return
	end
	self.render_cache_tag_modes = self.render_cache_tag_modes or {}
	self.render_cache_tag_modes[tag] = mode
	refresh_cache_tags(self)
end

Services.clearRenderCacheTagMode = function(self, tag)
	if not self.render_cache_tag_modes or not tag then
		return
	end
	self.render_cache_tag_modes[tag] = nil
	refresh_cache_tags(self)
end

Services.clearRenderCacheTagModes = function(self)
	self.render_cache_tag_modes = {}
	refresh_cache_tags(self)
end

Services.setSpatialIndexCellSize = function(self, cell_size)
	self.spatial_index_cell_size = max(8, flr(cell_size or self.spatial_index_cell_size or 32))
	if self.collision_broadphase_cell_size == nil then
		self.collision_broadphase_cell_size = self.spatial_index_cell_size
	end
	refresh_spatial_settings(self)
end

Services.setCollisionBroadphaseCellSize = function(self, cell_size)
	self.collision_broadphase_cell_size = max(8, flr(cell_size or self.spatial_index_cell_size or 32))
	refresh_spatial_settings(self)
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
