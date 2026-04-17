local Class = include("src/Engine/Core/Class.lua")
local Screen = include("src/Engine/Core/Screen.lua")

local Hud = Class:new({
	_type = "Hud",
	party = nil,
	margin = 6,
	panel_height = 42,
	panel_width = 118,
	slot_size = 14,
	slot_gap = 4,

	getInventory = function(self, player)
		return player and player.getPrimaryInventory and player:getPrimaryInventory() or nil
	end,

	getSlotLabel = function(self, stack)
		if not stack then
			return ""
		end
		local name = stack.display_name or stack.item_id or "?"
		return sub(tostr(name), 1, 1)
	end,

	drawHealthBar = function(self, x, y, w, h, health, max_health, color)
		local safe_max = max(1, max_health or 1)
		local clamped_health = mid(0, health or 0, safe_max)
		local fill_w = flr((clamped_health / safe_max) * (w - 2))
		rect(x, y, x + w, y + h, 5)
		rectfill(x + 1, y + 1, x + w - 1, y + h - 1, 1)
		if fill_w > 0 then
			rectfill(x + 1, y + 1, x + fill_w, y + h - 1, color or 11)
		end
	end,

	drawInventorySlot = function(self, x, y, slot_size, stack)
		rect(x, y, x + slot_size, y + slot_size, 6)
		rectfill(x + 1, y + 1, x + slot_size - 1, y + slot_size - 1, 0)
		if not stack then
			return
		end
		if stack.inventory_sprite ~= nil and stack.inventory_sprite >= 0 then
			spr(stack.inventory_sprite, x + 3, y + 3)
		else
			print(self:getSlotLabel(stack), x + 4, y + 4, 7)
		end
		if (stack.amount or 0) > 1 then
			print(tostr(stack.amount), x + 1, y + slot_size - 5, 10)
		end
	end,

	drawPlayerPanel = function(self, player, is_active)
		local x = self.margin
		local y = self.margin
		local panel_w = self.panel_width
		local name_color = is_active and 10 or 7
		local health_color = is_active and 11 or 8
		local inventory = self:getInventory(player)
		local slot_keys = inventory and inventory.getSlotKeys and inventory:getSlotKeys() or { 1, 2, 3 }

		rectfill(x, y, x + panel_w, y + self.panel_height, 1)
		rect(x, y, x + panel_w, y + self.panel_height, is_active and 10 or 5)

		local marker = is_active and ">" or "-"
		print(marker .. " " .. (player.display_name or player._type or "player"), x + 4, y + 3, name_color)
		print((player.control_mode or "?"), x + 4, y + 10, 6)

		self:drawHealthBar(x + 4, y + 18, 72, 7, player.health, player.max_health, health_color)
		print((player.health or 0) .. "/" .. (player.max_health or 0), x + 80, y + 17, 7)

		local slots_x = x + 4
		local slots_y = y + 27
		for i = 1, 3 do
			local slot_key = slot_keys[i] or i
			local stack = inventory and inventory.getSlot and inventory:getSlot(slot_key) or nil
			local slot_x = slots_x + ((i - 1) * (self.slot_size + self.slot_gap))
			self:drawInventorySlot(slot_x, slots_y, self.slot_size, stack)
		end
	end,

	draw = function(self)
		if not self.party or not self.party.players or #self.party.players == 0 then
			return
		end
		local player = self.party.getActivePlayer and self.party:getActivePlayer() or self.party.players[self.party.active_index]
		if player and not player._delete then
			self:drawPlayerPanel(player, true)
		end
	end,
})

return Hud
