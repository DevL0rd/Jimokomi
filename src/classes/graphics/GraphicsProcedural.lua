local ProceduralTextureRenderer = include("src/classes/ProceduralTextureRenderer.lua")

local GraphicsProcedural = {}

GraphicsProcedural.initProceduralRenderer = function(self)
	self.procedural_textures = ProceduralTextureRenderer:new({
		camera = self.camera
	})
end

GraphicsProcedural.drawProceduralTexture = function(self, texture_name, x, y, w, h, ...)
	local renderer = self.procedural_textures
	if renderer and renderer[texture_name] then
		return renderer[texture_name](renderer, x, y, w, h, ...)
	end
	return nil
end

GraphicsProcedural.fire = function(self, x, y, w, h, ...)
	return self:drawProceduralTexture("fire", x, y, w, h, ...)
end

GraphicsProcedural.water = function(self, x, y, w, h, ...)
	return self:drawProceduralTexture("water", x, y, w, h, ...)
end

GraphicsProcedural.wood = function(self, x, y, w, h, ...)
	return self:drawProceduralTexture("wood", x, y, w, h, ...)
end

GraphicsProcedural.stone = function(self, x, y, w, h, ...)
	return self:drawProceduralTexture("stone", x, y, w, h, ...)
end

return GraphicsProcedural
