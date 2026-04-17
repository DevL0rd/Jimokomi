local Graphic = include("src/Engine/Objects/Graphic.lua")

local Material = Graphic:new({
	_type = "Material",

	drawMaterial = function(self, gfx, x, y, w, h)
	end,

	draw = function(self)
		local top_left = self:getTopLeft()
		self:drawMaterial(
			self.layer.gfx,
			top_left.x,
			top_left.y,
			self:getWidth(),
			self:getHeight()
		)
		Graphic.draw(self)
	end,
})

return Material
