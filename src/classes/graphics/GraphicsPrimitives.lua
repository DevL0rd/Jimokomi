local GraphicsPrimitives = {}

GraphicsPrimitives.line = function(self, x1, y1, x2, y2, color)
	local screen_pos1 = self.camera:layerToScreen({ x = x1, y = y1 })
	local screen_pos2 = self.camera:layerToScreen({ x = x2, y = y2 })
	line(screen_pos1.x, screen_pos1.y, screen_pos2.x, screen_pos2.y, color)
end

GraphicsPrimitives.circ = function(self, x, y, r, color)
	local screen_pos = self.camera:layerToScreen({ x = x, y = y })
	circ(screen_pos.x, screen_pos.y, r, color)
end

GraphicsPrimitives.circfill = function(self, x, y, r, color)
	local screen_pos = self.camera:layerToScreen({ x = x, y = y })
	circfill(screen_pos.x, screen_pos.y, r, color)
end

GraphicsPrimitives.rect = function(self, x1, y1, x2, y2, color)
	local screen_pos1 = self.camera:layerToScreen({ x = x1, y = y1 })
	local screen_pos2 = self.camera:layerToScreen({ x = x2, y = y2 })
	rect(screen_pos1.x, screen_pos1.y, screen_pos2.x, screen_pos2.y, color)
end

GraphicsPrimitives.rectfill = function(self, x1, y1, x2, y2, color)
	local screen_pos1 = self.camera:layerToScreen({ x = x1, y = y1 })
	local screen_pos2 = self.camera:layerToScreen({ x = x2, y = y2 })
	rectfill(screen_pos1.x, screen_pos1.y, screen_pos2.x, screen_pos2.y, color)
end

GraphicsPrimitives.spr = function(self, sprite_id, x, y, flip_x, flip_y)
	local screen_pos = self.camera:layerToScreen({ x = x, y = y })
	spr(sprite_id, screen_pos.x, screen_pos.y, flip_x, flip_y)
end

GraphicsPrimitives.print = function(self, text, x, y, color)
	local screen_pos = self.camera:layerToScreen({ x = x, y = y })
	print(text, screen_pos.x, screen_pos.y, color)
end

return GraphicsPrimitives
