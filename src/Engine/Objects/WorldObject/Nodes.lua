local AttachmentNode = include("src/Engine/Nodes/AttachmentNode.lua")
local Screen = include("src/Engine/Core/Screen.lua")

local WorldObjectNodes = {}
local ATTACHMENT_FAMILY_NONE = 0
local ATTACHMENT_FAMILY_SPRITE = 1
local ATTACHMENT_FAMILY_EMITTER = 2
local ATTACHMENT_FAMILY_MIXED = 3
local ATTACHMENT_TOPOLOGY_NONE = 0
local ATTACHMENT_TOPOLOGY_SINGLE_SPRITE = 1
local ATTACHMENT_TOPOLOGY_FLAT_SPRITES = 2
local ATTACHMENT_TOPOLOGY_TREE = 3

local function is_sprite_node(node)
	return node and node.node_kind == 1 or false
end

local function classify_attachment_family_from_counts(sprite_nodes, emitters, other_nodes)
	sprite_nodes = sprite_nodes or 0
	emitters = emitters or 0
	other_nodes = other_nodes or 0
	if sprite_nodes > 0 and emitters == 0 and other_nodes == 0 then
		return ATTACHMENT_FAMILY_SPRITE
	end
	if emitters > 0 and sprite_nodes == 0 and other_nodes == 0 then
		return ATTACHMENT_FAMILY_EMITTER
	end
	if sprite_nodes > 0 or emitters > 0 or other_nodes > 0 then
		return ATTACHMENT_FAMILY_MIXED
	end
	return ATTACHMENT_FAMILY_NONE
end

local function recalc_attachment_family(self)
	local nodes = self and self.attachment_nodes or nil
	local sprite_nodes = 0
	local emitters = 0
	local other_nodes = 0
	local flat_sprite_nodes = {}
	local flat_sprite_only = true
	if nodes then
		for i = 1, #nodes do
			local node = nodes[i]
			local node_kind = node and node.node_kind or 0
			if node_kind == 1 then
				sprite_nodes += 1
				if node and ((node.children and #node.children > 0) or not is_sprite_node(node)) then
					flat_sprite_only = false
				else
					add(flat_sprite_nodes, node)
				end
			elseif node_kind == 2 then
				emitters += 1
				flat_sprite_only = false
			else
				other_nodes += 1
				flat_sprite_only = false
			end
		end
	end
	self._attachment_sprite_node_count = sprite_nodes
	self._attachment_emitter_node_count = emitters
	self._attachment_other_node_count = other_nodes
	self._attachment_family_kind = classify_attachment_family_from_counts(sprite_nodes, emitters, other_nodes)
	if not nodes or #nodes == 0 then
		self._attachment_draw_topology = ATTACHMENT_TOPOLOGY_NONE
		self._flat_attachment_sprite_nodes = nil
	elseif self._attachment_family_kind == ATTACHMENT_FAMILY_SPRITE and flat_sprite_only then
		self._flat_attachment_sprite_nodes = flat_sprite_nodes
		if #flat_sprite_nodes == 1 then
			self._attachment_draw_topology = ATTACHMENT_TOPOLOGY_SINGLE_SPRITE
		else
			self._attachment_draw_topology = ATTACHMENT_TOPOLOGY_FLAT_SPRITES
		end
	else
		self._attachment_draw_topology = ATTACHMENT_TOPOLOGY_TREE
		self._flat_attachment_sprite_nodes = nil
	end
	self._draw_family = nil
end

local function add_attachment_node_kind(self, node_kind)
	if node_kind == 1 then
		self._attachment_sprite_node_count = (self._attachment_sprite_node_count or 0) + 1
	elseif node_kind == 2 then
		self._attachment_emitter_node_count = (self._attachment_emitter_node_count or 0) + 1
	else
		self._attachment_other_node_count = (self._attachment_other_node_count or 0) + 1
	end
	self._attachment_family_kind = classify_attachment_family_from_counts(
		self._attachment_sprite_node_count,
		self._attachment_emitter_node_count,
		self._attachment_other_node_count
	)
	self._draw_family = nil
end

local function remove_attachment_node_kind(self, node_kind)
	if node_kind == 1 then
		self._attachment_sprite_node_count = max(0, (self._attachment_sprite_node_count or 0) - 1)
	elseif node_kind == 2 then
		self._attachment_emitter_node_count = max(0, (self._attachment_emitter_node_count or 0) - 1)
	else
		self._attachment_other_node_count = max(0, (self._attachment_other_node_count or 0) - 1)
	end
	self._attachment_family_kind = classify_attachment_family_from_counts(
		self._attachment_sprite_node_count,
		self._attachment_emitter_node_count,
		self._attachment_other_node_count
	)
	self._draw_family = nil
end

local function has_custom_draw_node(node)
	return node and node.drawNode ~= AttachmentNode.drawNode
end

local function is_drawless_leaf_node(node)
	if not node then
		return true
	end
	if is_sprite_node(node) then
		return false
	end
	if node.children and #node.children > 0 then
		return false
	end
	return node.drawNode == AttachmentNode.drawNode
end

local function draw_sprite_node_fast(node, context)
	if not node or not node.layer or node.sprite == nil or node.sprite < 0 then
		return false
	end
	local shape = node.shape or nil
	local width = shape and shape.w or (node.getWidth and node:getWidth() or 0)
	local height = shape and shape.h or (node.getHeight and node:getHeight() or 0)
	local x = node.pos.x - width * 0.5
	local y = node.pos.y - height * 0.5
	local sprite_id = node.getSpriteId and node:getSpriteId() or node.sprite
	local transform = context and context.transform or node.layer.camera:getRenderTransform()
	local sx = x - transform.x
	local sy = y - transform.y
	if sx + width <= 0 or sy + height <= 0 or sx >= Screen.w or sy >= Screen.h then
		return false
	end
	if context then
		context.sprite_draws = (context.sprite_draws or 0) + 1
		if node.end_sprite and node.end_sprite >= 0 then
			context.animated_sprite_draws = (context.animated_sprite_draws or 0) + 1
		else
			context.static_sprite_draws = (context.static_sprite_draws or 0) + 1
		end
	end
	spr(sprite_id, sx, sy, node.flip_x, node.flip_y)
	return true
end

local function draw_attachment_node_fast(node, assume_visible, context)
	if not node then
		return
	end
	if is_drawless_leaf_node(node) then
		return
	end
	local visible = assume_visible or node.always_draw_offscreen == true
	if not visible then
		visible = node:isVisibleInLayer()
	end
	if not visible then
		return
	end

	if context then
		context.nodes_drawn = (context.nodes_drawn or 0) + 1
	end

	local drew_fast = false
	local cache_mode = node.cache_mode or 2
	local node_layer = node.layer
	if is_sprite_node(node) and
		(cache_mode == 2 or node_layer == nil or node_layer.gfx == nil or not node_layer.gfx.cache_renders) then
		drew_fast = draw_sprite_node_fast(node, context)
	end
	if not drew_fast and has_custom_draw_node(node) then
		node:drawNode()
	end

	local children = node.children
	if not children then
		return
	end
	for i = 1, #children do
		draw_attachment_node_fast(children[i], false, context)
	end
end

local function draw_sprite_attachment_list_fast(nodes, context)
	for i = 1, #nodes do
		local node = nodes[i]
		if node and node.always_draw_offscreen ~= true and not is_drawless_leaf_node(node) and draw_sprite_node_fast(node, context) then
			if context then
				context.nodes_drawn = (context.nodes_drawn or 0) + 1
			end
		elseif node then
			draw_attachment_node_fast(node, false, context)
		end
	end
end

local function draw_single_attachment_sprite_fast(self)
	local nodes = self and self._flat_attachment_sprite_nodes or nil
	if not nodes or #nodes ~= 1 or self._attachment_draw_topology ~= ATTACHMENT_TOPOLOGY_SINGLE_SPRITE then
		return false
	end
	local node = nodes[1]
	if not node or not is_sprite_node(node) or (node.children and #node.children > 0) then
		return false
	end
	local context = {
		transform = self.layer and self.layer.camera and self.layer.camera:getRenderTransform() or nil,
		sprite_draws = 0,
		animated_sprite_draws = 0,
		static_sprite_draws = 0,
	}
	if node.always_draw_offscreen == true then
		local shape = node.shape or nil
		local width = shape and shape.w or (node.getWidth and node:getWidth() or 0)
		local height = shape and shape.h or (node.getHeight and node:getHeight() or 0)
		local x = node.pos.x - width * 0.5
		local y = node.pos.y - height * 0.5
		local sprite_id = node.getSpriteId and node:getSpriteId() or node.sprite
		local transform = context.transform
		context.sprite_draws = 1
		if node.end_sprite and node.end_sprite >= 0 then
			context.animated_sprite_draws = 1
		else
			context.static_sprite_draws = 1
		end
		spr(sprite_id, x - transform.x, y - transform.y, node.flip_x, node.flip_y)
	else
		if not draw_sprite_node_fast(node, context) then
			return false
		end
	end
	if context.sprite_draws <= 0 then
		return false
	end
	local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
	if profiler then
		profiler:addCounter("layer.nodes.drawn", 1)
		profiler:addCounter("render.sprite_node.draws", 1)
		if node.end_sprite and node.end_sprite >= 0 then
			profiler:addCounter("render.sprite_node.animated_draws", 1)
		else
			profiler:addCounter("render.sprite_node.static_draws", 1)
		end
	end
	return true
end

local function draw_flat_attachment_sprite_list_fast(self)
	local nodes = self and self._flat_attachment_sprite_nodes or nil
	if not nodes or #nodes == 0 or self._attachment_draw_topology ~= ATTACHMENT_TOPOLOGY_FLAT_SPRITES then
		return false
	end
	local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
	local context = {
		transform = self.layer and self.layer.camera and self.layer.camera:getRenderTransform() or nil,
		nodes_drawn = 0,
		sprite_draws = 0,
		animated_sprite_draws = 0,
		static_sprite_draws = 0,
	}
	draw_sprite_attachment_list_fast(nodes, context)
	if profiler then
		if context.nodes_drawn > 0 then
			profiler:addCounter("layer.nodes.drawn", context.nodes_drawn)
		end
		if context.sprite_draws > 0 then
			profiler:addCounter("render.sprite_node.draws", context.sprite_draws)
		end
		if context.animated_sprite_draws > 0 then
			profiler:addCounter("render.sprite_node.animated_draws", context.animated_sprite_draws)
		end
		if context.static_sprite_draws > 0 then
			profiler:addCounter("render.sprite_node.static_draws", context.static_sprite_draws)
		end
	end
	return context.sprite_draws > 0
end

WorldObjectNodes.addAttachmentNode = function(self, node)
	self.attachment_nodes = self.attachment_nodes or {}
	add(self.attachment_nodes, node)
	node.layer = self.layer
	add_attachment_node_kind(self, node and node.node_kind or 0)
	recalc_attachment_family(self)
	if self.layer and self.layer.refreshEntityBuckets then
		self.layer:refreshEntityBuckets(self)
	end
	return node
end

WorldObjectNodes.removeAttachmentNode = function(self, node)
	if not self.attachment_nodes then
		return
	end
	del(self.attachment_nodes, node)
	remove_attachment_node_kind(self, node and node.node_kind or 0)
	recalc_attachment_family(self)
	if self.layer and self.layer.refreshEntityBuckets then
		self.layer:refreshEntityBuckets(self)
	end
end

WorldObjectNodes.updateAttachmentNodes = function(self)
	if not self.attachment_nodes then
		return
	end
	if self._attachment_draw_topology == ATTACHMENT_TOPOLOGY_SINGLE_SPRITE or
		self._attachment_draw_topology == ATTACHMENT_TOPOLOGY_FLAT_SPRITES then
		local nodes = self._flat_attachment_sprite_nodes or self.attachment_nodes
		for i = 1, #nodes do
			local node = nodes[i]
			if node and node.updateNode then
				node:updateNode()
			end
		end
		return
	end
	for i = 1, #self.attachment_nodes do
		self.attachment_nodes[i]:updateRecursive()
	end
end

WorldObjectNodes.drawAttachmentNodes = function(self)
	if not self.attachment_nodes then
		return
	end
	if self._attachment_draw_topology == ATTACHMENT_TOPOLOGY_FLAT_SPRITES then
		if draw_flat_attachment_sprite_list_fast(self) then
			return
		end
	end
	local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
	local context = {
		transform = self.layer and self.layer.camera and self.layer.camera:getRenderTransform() or nil,
		nodes_drawn = 0,
		sprite_draws = 0,
		animated_sprite_draws = 0,
		static_sprite_draws = 0,
	}
	if self._attachment_family_kind == ATTACHMENT_FAMILY_SPRITE then
		draw_sprite_attachment_list_fast(self.attachment_nodes, context)
	else
		for i = 1, #self.attachment_nodes do
			draw_attachment_node_fast(self.attachment_nodes[i], true, context)
		end
	end
	if profiler then
		if context.nodes_drawn > 0 then
			profiler:addCounter("layer.nodes.drawn", context.nodes_drawn)
		end
		if context.sprite_draws > 0 then
			profiler:addCounter("render.sprite_node.draws", context.sprite_draws)
		end
		if context.animated_sprite_draws > 0 then
			profiler:addCounter("render.sprite_node.animated_draws", context.animated_sprite_draws)
		end
		if context.static_sprite_draws > 0 then
			profiler:addCounter("render.sprite_node.static_draws", context.static_sprite_draws)
		end
	end
end

WorldObjectNodes.drawSingleAttachmentSpriteFast = draw_single_attachment_sprite_fast
WorldObjectNodes.drawFlatAttachmentSpriteListFast = draw_flat_attachment_sprite_list_fast

WorldObjectNodes.destroyAttachmentNodes = function(self)
	if not self.attachment_nodes then
		return
	end
	for i = #self.attachment_nodes, 1, -1 do
		self.attachment_nodes[i]:destroy()
	end
	self.attachment_nodes = {}
	recalc_attachment_family(self)
end

WorldObjectNodes.recalcAttachmentFamily = recalc_attachment_family

return WorldObjectNodes
