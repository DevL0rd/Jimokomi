local WorldVisibility = {}

WorldVisibility.isOnScreen = function(self, pos, radius, padding)
	radius = radius or 0
	padding = padding or 0
	return self.layer.camera:isRectVisible(
		pos.x - radius - padding,
		pos.y - radius - padding,
		(radius + padding) * 2,
		(radius + padding) * 2
	)
end

return WorldVisibility
