local GraphicsPrimitives = {}

GraphicsPrimitives.screenLine = function(self, x1, y1, x2, y2, color)
	line(x1, y1, x2, y2, color)
end

GraphicsPrimitives.screenCirc = function(self, x, y, r, color)
	circ(x, y, r, color)
end

GraphicsPrimitives.screenCircfill = function(self, x, y, r, color)
	circfill(x, y, r, color)
end

GraphicsPrimitives.screenRect = function(self, x1, y1, x2, y2, color)
	rect(x1, y1, x2, y2, color)
end

GraphicsPrimitives.screenRectfill = function(self, x1, y1, x2, y2, color)
	rectfill(x1, y1, x2, y2, color)
end

GraphicsPrimitives.screenPrint = function(self, text, x, y, color)
	print(text, x, y, color)
end

GraphicsPrimitives.line = function(self, x1, y1, x2, y2, color)
	local sx1, sy1 = self.camera:layerToScreenXY(x1, y1)
	local sx2, sy2 = self.camera:layerToScreenXY(x2, y2)
	line(sx1, sy1, sx2, sy2, color)
end

GraphicsPrimitives.circ = function(self, x, y, r, color)
	local sx, sy = self.camera:layerToScreenXY(x, y)
	circ(sx, sy, r, color)
end

GraphicsPrimitives.circfill = function(self, x, y, r, color)
	local sx, sy = self.camera:layerToScreenXY(x, y)
	circfill(sx, sy, r, color)
end

GraphicsPrimitives.rect = function(self, x1, y1, x2, y2, color)
	local sx1, sy1 = self.camera:layerToScreenXY(x1, y1)
	local sx2, sy2 = self.camera:layerToScreenXY(x2, y2)
	rect(sx1, sy1, sx2, sy2, color)
end

GraphicsPrimitives.rectfill = function(self, x1, y1, x2, y2, color)
	local sx1, sy1 = self.camera:layerToScreenXY(x1, y1)
	local sx2, sy2 = self.camera:layerToScreenXY(x2, y2)
	rectfill(sx1, sy1, sx2, sy2, color)
end

GraphicsPrimitives.spr = function(self, sprite_id, x, y, flip_x, flip_y)
	local sx, sy = self.camera:layerToScreenXY(x, y)
	spr(sprite_id, sx, sy, flip_x, flip_y)
end

GraphicsPrimitives.print = function(self, text, x, y, color)
	local sx, sy = self.camera:layerToScreenXY(x, y)
	print(text, sx, sy, color)
end

return GraphicsPrimitives
