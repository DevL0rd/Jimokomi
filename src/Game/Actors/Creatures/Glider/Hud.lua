local Class = include("src/Engine/Core/Class.lua")

local HUD_PANEL_STYLE = {
	key = "hud_panel",
	shadow_offset = 1,
	shadow_color = 1,
	bg_top = 2,
	bg_bottom = 1,
	border_color = 6,
	inner_border_color = 5,
	highlight_color = 7,
	speckle_density = 0,
}

local HUD_SLOT_STYLE = {
	key = "hud_slot",
	shadow_offset = 1,
	shadow_color = 1,
	bg_top = 1,
	bg_bottom = 2,
	border_color = 13,
	inner_border_color = 5,
	highlight_color = 6,
	speckle_density = 0,
}

local Hud = Class:new({
	_type = "Hud",
	party = nil,
	gfx = nil,
	margin = 6,
	panel_height = 44,
	panel_width = 126,
	slot_size = 16,
	slot_gap = 6,
	root_node = nil,
	panel_node = nil,
	title_node = nil,
	divider_node = nil,
	health_bar_node = nil,
	health_text_node = nil,
	slot_nodes = nil,
	slot_content_nodes = nil,

	resolveGfx = function(self)
		if self.gfx then
			return self.gfx
		end
		if self.party and self.party.layer and self.party.layer.gfx then
			self.gfx = self.party.layer.gfx
		end
		return self.gfx
	end,

	ensureRetainedTree = function(self)
		local gfx = self:resolveGfx()
		if not gfx then
			return false
		end
		if self.root_node then
			return true
		end

		self.root_node = gfx:newRetainedGroup({
			key = "hud.root",
			cache_mode = "surface",
			w = self.panel_width,
			h = self.panel_height,
		})

		self.panel_node = gfx:newRetainedPanel({
			key = "hud.panel",
			cache_mode = "retained",
			w = self.panel_width,
			h = self.panel_height,
			style = HUD_PANEL_STYLE,
		})
		self.title_node = gfx:newRetainedLeaf({
			key = "hud.title",
			cache_mode = "retained",
			x = 6,
			y = 5,
			w = self.panel_width - 12,
			h = 6,
		})
		self.divider_node = gfx:newRetainedLeaf({
			key = "hud.divider",
			cache_mode = "retained",
			w = self.panel_width - 12,
			h = 1,
			x = 6,
			y = 13,
			builder = function(target, node)
				target:line(0, 0, node.w, 0, 5)
			end,
		})
		self.health_bar_node = gfx:newRetainedLeaf({
			key = "hud.health_bar",
			cache_mode = "retained",
			x = 6,
			y = 18,
			w = 78,
			h = 8,
		})
		self.health_text_node = gfx:newRetainedLeaf({
			key = "hud.health_text",
			cache_mode = "retained",
			x = 90,
			y = 18,
			w = 30,
			h = 6,
		})

		self.root_node:addChild(self.panel_node)
		self.root_node:addChild(self.title_node)
		self.root_node:addChild(self.divider_node)
		self.root_node:addChild(self.health_bar_node)
		self.root_node:addChild(self.health_text_node)

		self.slot_nodes = {}
		self.slot_content_nodes = {}
		local total_slots_w = (3 * self.slot_size) + (2 * self.slot_gap)
		local slots_x = flr((self.panel_width - total_slots_w) * 0.5)
		for i = 1, 3 do
			local slot_x = slots_x + ((i - 1) * (self.slot_size + self.slot_gap))
			local slot_node = gfx:newRetainedGroup({
				key = "hud.slot." .. i,
				cache_mode = "retained",
				x = slot_x,
				y = 27,
				w = self.slot_size,
				h = self.slot_size,
			})
			local slot_panel_node = gfx:newRetainedPanel({
				key = "hud.slot.panel." .. i,
				cache_mode = "retained",
				w = self.slot_size,
				h = self.slot_size,
				style = HUD_SLOT_STYLE,
			})
			local slot_content_node = gfx:newRetainedLeaf({
				key = "hud.slot.content." .. i,
				cache_mode = "retained",
				w = self.slot_size,
				h = self.slot_size,
			})
			slot_node:addChild(slot_panel_node)
			slot_node:addChild(slot_content_node)
			self.slot_nodes[i] = slot_node
			self.slot_content_nodes[i] = slot_content_node
			self.root_node:addChild(slot_node)
		end

		return true
	end,

	getInventory = function(self, actor)
		return actor and actor.getPrimaryInventory and actor:getPrimaryInventory() or nil
	end,

	getSlotLabel = function(self, stack)
		if not stack then
			return ""
		end
		local name = stack.display_name or stack.item_id or "?"
		return sub(tostr(name), 1, 1)
	end,

	buildHealthBar = function(self, target, w, h, health, max_health, color)
		local safe_max = max(1, max_health or 1)
		local clamped_health = mid(0, health or 0, safe_max)
		local fill_w = flr((clamped_health / safe_max) * max(0, w - 2))
		target:rect(0, 0, w, h, 13)
		target:rect(1, 1, w - 1, h - 1, 5)
		target:rectfill(2, 2, w - 2, h - 2, 1)
		if fill_w > 0 then
			target:rectfill(2, 2, 1 + fill_w, h - 2, color or 11)
		end
	end,

	configureSlotNode = function(self, slot_node, stack)
		local gfx = self:resolveGfx()
		if not gfx then
			return
		end
		local state_key = stack and (
			tostr(stack.inventory_sprite) .. ":" ..
			tostr(stack.amount or 0) .. ":" ..
			tostr(stack.display_name or stack.item_id or "")
		) or "empty"
		if slot_node.state_key ~= state_key then
			slot_node:setStateKey(state_key)
			slot_node.builder = function(target)
				if not stack then
					return
				end
				if stack.inventory_sprite ~= nil and stack.inventory_sprite >= 0 then
					target:spr(stack.inventory_sprite, 3, 3)
				else
					target:print(self:getSlotLabel(stack), 5, 5, 7)
				end
				if (stack.amount or 0) > 1 then
					target:print(tostr(stack.amount), 2, self.slot_size - 5, 10)
				end
			end
		end
	end,

	configureActorNodes = function(self, actor)
		local gfx = self:resolveGfx()
		if not gfx or not self:ensureRetainedTree() then
			return
		end
		local inventory = self:getInventory(actor)
		local slot_keys = inventory and inventory.getSlotKeys and inventory:getSlotKeys() or { 1, 2, 3 }

		local title = actor.display_name or actor._type or "glider"
		if self.title_node.state_key ~= title then
			self.title_node:setStateKey(title)
			self.title_node.builder = function(target)
				target:print(title, 0, 0, 10)
			end
		end

		local health_state_key = tostr(actor.health or 0) .. ":" .. tostr(actor.max_health or 0)
		if self.health_bar_node.state_key ~= health_state_key then
			self.health_bar_node:setStateKey(health_state_key)
			self.health_bar_node.builder = function(target, node)
				self:buildHealthBar(target, node.w, node.h, actor.health, actor.max_health, 11)
			end
		end

		local health_text = (actor.health or 0) .. "/" .. (actor.max_health or 0)
		if self.health_text_node.state_key ~= health_text then
			self.health_text_node:setStateKey(health_text)
			self.health_text_node.builder = function(target)
				target:print(health_text, 0, 0, 7)
			end
		end

		for i = 1, 3 do
			local slot_key = slot_keys[i] or i
			local stack = inventory and inventory.getSlot and inventory:getSlot(slot_key) or nil
			self:configureSlotNode(self.slot_content_nodes[i], stack)
		end
	end,

	draw = function(self)
		if not self.party or not self.party.players or #self.party.players == 0 then
			return
		end
		local actor = self.party.getActivePlayer and self.party:getActivePlayer() or self.party.players[self.party.active_index]
		if actor and not actor._delete then
			if not self:ensureRetainedTree() then
				return
			end
			self:configureActorNodes(actor)
			self.root_node:draw(self.margin, self.margin)
		end
	end,
})

return Hud
