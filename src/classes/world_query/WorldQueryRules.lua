local WorldQueryRules = {}

WorldQueryRules.rules = {
	randomMap = function(options)
		options = options or {}
		return {
			kind = "map_random",
			scope = options.scope or "map",
			margin = options.margin or 0,
			padding = options.padding or 0,
			attempts = options.attempts or 24,
			bounds = options.bounds,
			offset = options.offset,
		}
	end,

	randomSurface = function(options)
		options = options or {}
		return {
			kind = "surface_random",
			scope = options.scope or "map",
			margin = options.margin or 0,
			padding = options.padding or 0,
			attempts = options.attempts or 48,
			bounds = options.bounds,
			offset = options.offset,
		}
	end,

	randomTile = function(options)
		options = options or {}
		return {
			kind = "tile_random",
			scope = options.scope or "map",
			margin = options.margin or 0,
			padding = options.padding or 0,
			attempts = options.attempts or 24,
			tile_ids = options.tile_ids or {},
			bounds = options.bounds,
			offset = options.offset,
		}
	end,
}

return WorldQueryRules
