local WorldBounds = {}

WorldBounds.getMapBounds = function(self, margin)
	margin = margin or 0
	return {
		left = margin,
		top = margin,
		right = self.layer.w - margin,
		bottom = self.layer.h - margin,
	}
end

WorldBounds.clampBoundsToMap = function(self, bounds, margin)
	margin = margin or 0
	if not bounds then
		return nil
	end

	local map_bounds = self:getMapBounds(margin)
	return {
		left = max(map_bounds.left, bounds.left),
		top = max(map_bounds.top, bounds.top),
		right = min(map_bounds.right, bounds.right),
		bottom = min(map_bounds.bottom, bounds.bottom),
	}
end

WorldBounds.getViewBounds = function(self, margin)
	margin = margin or 0
	local map_bounds = self:getMapBounds(0)
	local view_bounds = self.layer.camera:getViewBounds()
	return {
		left = max(map_bounds.left + margin, flr(view_bounds.left) + margin),
		top = max(map_bounds.top + margin, flr(view_bounds.top) + margin),
		right = min(map_bounds.right - margin, ceil(view_bounds.right) - margin),
		bottom = min(map_bounds.bottom - margin, ceil(view_bounds.bottom) - margin),
	}
end

WorldBounds.getQueryBounds = function(self, rule)
	rule = rule or {}
	if rule.bounds then
		return self:clampBoundsToMap(rule.bounds, rule.margin or 0)
	end
	local scope = rule.scope or "map"
	local margin = rule.margin or 0
	if scope == "view" then
		return self:getViewBounds(margin)
	end
	return self:getMapBounds(margin)
end

WorldBounds.getOffset = function(self, rule)
	rule = rule or {}
	local offset = rule.offset or {}
	return {
		x = rule.offset_x or offset.x or 0,
		y = rule.offset_y or offset.y or 0,
	}
end

return WorldBounds
