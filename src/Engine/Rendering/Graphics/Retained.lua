local GraphicsCache = include("src/Engine/Rendering/Graphics/Cache.lua")

local RetainedNode = {}
RetainedNode.__index = RetainedNode

local function record_counter(gfx, name, amount)
	if gfx and gfx.profiler and gfx.profiler.addCounter then
		gfx.profiler:addCounter(name, amount or 1)
	end
end

local function record_observe(gfx, name, value)
	if gfx and gfx.profiler and gfx.profiler.observe then
		gfx.profiler:observe(name, value)
	end
end

local function attach_gfx(node, gfx)
	node.gfx = gfx or node.gfx
	if node.children then
		for i = 1, #node.children do
			attach_gfx(node.children[i], node.gfx)
		end
	end
	return node
end

local function mark_dirty(node)
	if not node then
		return
	end
	if node.dirty then
		return
	end
	node.dirty = true
	record_counter(node.gfx, "render.retained.dirty_marks", 1)
	if node.parent then
		mark_dirty(node.parent)
	end
end

function RetainedNode:setStateKey(state_key)
	if self.state_key ~= state_key then
		self.state_key = state_key
		mark_dirty(self)
	end
	return self
end

function RetainedNode:setBuilder(builder)
	if self.builder ~= builder then
		self.builder = builder
		mark_dirty(self)
	end
	return self
end

function RetainedNode:setBounds(w, h)
	if self.w ~= w or self.h ~= h then
		self.w = w
		self.h = h
		mark_dirty(self)
	end
	return self
end

function RetainedNode:setPosition(x, y)
	if self.x ~= x or self.y ~= y then
		self.x = x
		self.y = y
		mark_dirty(self)
	end
	return self
end

function RetainedNode:setCacheMode(cache_mode)
	if self.cache_mode ~= cache_mode then
		self.cache_mode = cache_mode or "retained"
		mark_dirty(self)
	end
	return self
end

function RetainedNode:markDirty()
	mark_dirty(self)
	return self
end

function RetainedNode:addChild(child)
	if not child then
		return nil
	end
	child.parent = self
	attach_gfx(child, self.gfx)
	self.children = self.children or {}
	add(self.children, child)
	mark_dirty(self)
	return child
end

function RetainedNode:clearChildren()
	if not self.children then
		return self
	end
	for i = 1, #self.children do
		self.children[i].parent = nil
	end
	self.children = {}
	mark_dirty(self)
	return self
end

function RetainedNode:replaceChildren(children)
	self:clearChildren()
	for i = 1, #(children or {}) do
		self:addChild(children[i])
	end
	return self
end

local function ensure_leaf_commands(node)
	if node.cache_mode == "off" then
		return nil
	end
	if not node.dirty and node.commands then
		record_counter(node.gfx, "render.retained.hits", 1)
		return node.commands
	end
	record_counter(node.gfx, "render.retained.rebuilds", 1)
	record_counter(node.gfx, "render.retained.leaf_rebuilds", 1)
	local commands = {}
	if node.builder then
		commands = GraphicsCache.recordCommandList(node.gfx, function(target)
			node.builder(target, node)
		end)
	end
	node.commands = commands
	node.dirty = false
	record_observe(node.gfx, "render.retained.commands_per_node", #commands)
	return commands
end

local function ensure_group_commands(node)
	if node.cache_mode == "off" then
		return nil
	end
	if not node.dirty and node.commands then
		record_counter(node.gfx, "render.retained.hits", 1)
		return node.commands
	end
	record_counter(node.gfx, "render.retained.rebuilds", 1)
	record_counter(node.gfx, "render.retained.group_rebuilds", 1)
	local commands = {}
	if node.builder then
		local own_commands = GraphicsCache.recordCommandList(node.gfx, function(target)
			node.builder(target, node)
		end)
		GraphicsCache.appendOffsetCommands(commands, own_commands, 0, 0)
	end
	for i = 1, #(node.children or {}) do
		local child = node.children[i]
		local child_commands = child:ensureCommands()
		if child_commands then
			GraphicsCache.appendOffsetCommands(commands, child_commands, child.x or 0, child.y or 0)
		elseif child.cache_mode == "off" then
			mark_dirty(node)
		end
	end
	node.commands = commands
	node.dirty = false
	record_observe(node.gfx, "render.retained.commands_per_node", #commands)
	return commands
end

function RetainedNode:ensureCommands()
	if self.kind == "group" then
		return ensure_group_commands(self)
	end
	return ensure_leaf_commands(self)
end

local function draw_immediate(node, draw_x, draw_y)
	local target = GraphicsCache.buildImmediateTarget(node.gfx, draw_x, draw_y)
	if node.builder then
		node.builder(target, node)
	end
	for i = 1, #(node.children or {}) do
		local child = node.children[i]
		child:draw(draw_x, draw_y)
	end
end

local function get_node_cache_profile_key(node)
	if node.getCacheProfileKey then
		return node:getCacheProfileKey()
	end
	return node.cache_profile_key or ("retained:" .. tostr(node.cache_tag or node.kind or "node"))
end

local function get_node_cache_profile_signature(node)
	if node.getCacheProfileSignature then
		return node:getCacheProfileSignature()
	end
	if node.cache_profile_signature ~= nil then
		return node.cache_profile_signature
	end
	return table.concat({
		tostr(node.kind or "leaf"),
		tostr(node.cache_tag or "-"),
		tostr(node.state_key or "-"),
		tostr(node.w or 0),
		tostr(node.h or 0),
		tostr(#(node.children or {})),
		tostr(node.builder ~= nil),
	}, ":")
end

local function get_node_surface_cache_key(node)
	if node.getSurfaceCacheKey then
		return node:getSurfaceCacheKey()
	end
	if node.key ~= nil then
		return node.key
	end
	return get_node_cache_profile_key(node) .. ":" .. get_node_cache_profile_signature(node)
end

function RetainedNode:draw(base_x, base_y)
	local draw_x = (base_x or 0) + (self.x or 0)
	local draw_y = (base_y or 0) + (self.y or 0)
	record_counter(self.gfx, "render.retained.draws", 1)
	local resolved_mode = self.gfx and self.gfx.resolveCacheMode and self.gfx:resolveCacheMode({
		cache_mode = self.cache_mode,
		cache_tag = self.cache_tag,
		cache_override_mode = self.cache_override_mode,
	}, self.cache_mode or "retained") or self.cache_mode
	if resolved_mode == "off" or not (self.gfx and self.gfx.cache_renders) then
		record_counter(self.gfx, "render.retained.off_draws", 1)
		draw_immediate(self, draw_x, draw_y)
		return
	end

	local was_dirty = self.dirty
	local commands = self:ensureCommands() or {}
	if resolved_mode == "surface" then
		record_counter(self.gfx, "render.retained.surface_draws", 1)
		self.gfx:drawCachedSurfaceScreen(get_node_surface_cache_key(self), draw_x, draw_y, function(target)
			GraphicsCache.emitCommands(target, commands, 0, 0)
		end, {
			w = self.w,
			h = self.h,
			rebuild = was_dirty,
			clear_color = self.clear_color or 0,
			cache_tag = self.cache_tag,
			cache_profile_key = get_node_cache_profile_key(self),
			cache_profile_signature = get_node_cache_profile_signature(self),
			cache_override_mode = resolved_mode,
		})
		return
	end

	record_counter(self.gfx, "render.retained.off_draws", 1)
	draw_immediate(self, draw_x, draw_y)
end

local GraphicsRetained = {}

local function new_node(gfx, config, kind)
	local node = setmetatable({
		gfx = gfx,
		kind = kind or "leaf",
		key = config and config.key or nil,
		x = (config and config.x) or 0,
		y = (config and config.y) or 0,
		w = (config and config.w) or 0,
		h = (config and config.h) or 0,
		state_key = (config and config.state_key) or nil,
		cache_mode = (config and config.cache_mode) or "retained",
		cache_tag = (config and config.cache_tag) or ("retained." .. tostr(kind or "leaf")),
		cache_profile_key = config and config.cache_profile_key or nil,
		cache_profile_signature = config and config.cache_profile_signature or nil,
		cache_override_mode = config and config.cache_override_mode or nil,
		clear_color = config and config.clear_color or 0,
		builder = config and config.builder or nil,
		children = {},
		commands = nil,
		parent = nil,
		dirty = true,
	}, RetainedNode)
	record_counter(gfx, "render.retained.nodes_created", 1)
	return node
end

GraphicsRetained.newRetainedLeaf = function(gfx, config)
	return new_node(gfx, config, "leaf")
end

GraphicsRetained.newRetainedGroup = function(gfx, config)
	return new_node(gfx, config, "group")
end

GraphicsRetained.attachRetainedGfx = function(gfx, node)
	return attach_gfx(node, gfx)
end

return GraphicsRetained
