local VisualOwner = include("src/primitives/VisualOwner.lua")

local ItemVisuals = {}

ItemVisuals.playVisual = function(self, state, overrides)
	return VisualOwner.playVisual(self, state, overrides)
end

ItemVisuals.clearVisual = function(self)
	VisualOwner.clearVisual(self)
end

return ItemVisuals
