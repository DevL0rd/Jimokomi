local WalkClimbJump = {
	max_iterations = 160,
}

local function make_search_context(self, pathing)
	local world = pathing:getWorld()
	local bounds = self:getTileBounds(pathing)
	if not world or not bounds then
		return nil
	end
	return {
		world = world,
		collision = world.layer and world.layer.collision or nil,
		map_id = world.layer and world.layer.map_id or nil,
		tile_size = world:getTileSize(),
		bounds = bounds,
		min_x = bounds.min_x,
		min_y = bounds.min_y,
		stride = (bounds.max_x - bounds.min_x) + 1,
		tile_ids = {},
		passable = {},
		climbable = {},
		standable = {},
	}
end

local function tile_key(context, tile_x, tile_y)
	if not context then
		return nil
	end
	return ((tile_y - context.min_y) * context.stride) + (tile_x - context.min_x)
end

local function world_pos_for_tile(context, tile_x, tile_y)
	local tile_size = context and context.tile_size or 16
	return {
		x = tile_x * tile_size + tile_size * 0.5,
		y = tile_y * tile_size + tile_size * 0.5,
	}
end

function WalkClimbJump:prepareSearch(pathing, goal, start_node, goal_node)
	return make_search_context(self, pathing)
end

function WalkClimbJump:getOwnerRadius(pathing)
	local owner = pathing.owner
	return owner and owner.getRadius and owner:getRadius() or 0
end

function WalkClimbJump:getTileBounds(pathing, context)
	if context and context.bounds then
		return context.bounds
	end
	local world = pathing:getWorld()
	return world and world:getTileBounds() or nil
end

function WalkClimbJump:isTileInBounds(pathing, tile_x, tile_y, context)
	local bounds = self:getTileBounds(pathing, context)
	return bounds ~= nil and
		tile_x >= bounds.min_x and tile_x <= bounds.max_x and
		tile_y >= bounds.min_y and tile_y <= bounds.max_y
end

function WalkClimbJump:getTileId(pathing, tile_x, tile_y, context)
	if context then
		local key = tile_key(context, tile_x, tile_y)
		local cached = key ~= nil and context.tile_ids[key] or nil
		if cached ~= nil then
			return cached
		end
		local collision = context.collision
		local tile_id = collision and collision:getTileIDAt(tile_x * context.tile_size, tile_y * context.tile_size, context.map_id) or 0
		if key ~= nil then
			context.tile_ids[key] = tile_id
		end
		return tile_id
	end
	local world = pathing:getWorld()
	if not world then
		return 0
	end
	local pos = world:tileToWorld(tile_x, tile_y)
	return world:getTileAt(pos.x, pos.y)
end

function WalkClimbJump:isTilePassable(pathing, tile_x, tile_y, context)
	local world = pathing:getWorld()
	if not world or not self:isTileInBounds(pathing, tile_x, tile_y, context) then
		return false
	end
	if context then
		local key = tile_key(context, tile_x, tile_y)
		local cached = key ~= nil and context.passable[key] or nil
		if cached ~= nil then
			return cached
		end
		local collision = context.collision
		local passable = collision and not collision:isSolidTile(self:getTileId(pathing, tile_x, tile_y, context)) or false
		if key ~= nil then
			context.passable[key] = passable
		end
		return passable
	end
	local pos = world:tileToWorld(tile_x, tile_y)
	return not world:isSolidAt(pos.x, pos.y)
end

function WalkClimbJump:isTileClimbable(pathing, tile_x, tile_y, context)
	local owner = pathing.owner
	if not owner or not owner.locomotion or not owner.locomotion.isClimbableTile then
		return false
	end
	if context then
		local key = tile_key(context, tile_x, tile_y)
		local cached = key ~= nil and context.climbable[key] or nil
		if cached ~= nil then
			return cached
		end
		local climbable = owner.locomotion:isClimbableTile(self:getTileId(pathing, tile_x, tile_y, context)) == true
		if key ~= nil then
			context.climbable[key] = climbable
		end
		return climbable
	end
	return owner.locomotion:isClimbableTile(self:getTileId(pathing, tile_x, tile_y)) == true
end

function WalkClimbJump:hasGroundBelow(pathing, tile_x, tile_y, context)
	local bounds = self:getTileBounds(pathing, context)
	if not bounds then
		return false
	end
	if tile_y + 1 > bounds.max_y then
		return true
	end
	return not self:isTilePassable(pathing, tile_x, tile_y + 1, context)
end

function WalkClimbJump:canStandAt(pathing, tile_x, tile_y, context)
	if context then
		local key = tile_key(context, tile_x, tile_y)
		local cached = key ~= nil and context.standable[key] or nil
		if cached ~= nil then
			return cached
		end
		local standable = self:isTilePassable(pathing, tile_x, tile_y, context) and
			(self:hasGroundBelow(pathing, tile_x, tile_y, context) or self:isTileClimbable(pathing, tile_x, tile_y, context))
		if key ~= nil then
			context.standable[key] = standable
		end
		return standable
	end
	if not self:isTilePassable(pathing, tile_x, tile_y) then
		return false
	end
	return self:hasGroundBelow(pathing, tile_x, tile_y) or self:isTileClimbable(pathing, tile_x, tile_y)
end

function WalkClimbJump:canMoveHorizontal(pathing, from_x, from_y, to_x, to_y, context)
	if not self:isTilePassable(pathing, to_x, to_y, context) then
		return false
	end
	if self:isTileClimbable(pathing, from_x, from_y, context) or self:isTileClimbable(pathing, to_x, to_y, context) then
		return true
	end
	return self:canStandAt(pathing, from_x, from_y, context) and self:canStandAt(pathing, to_x, to_y, context)
end

function WalkClimbJump:canMoveVertical(pathing, from_x, from_y, to_x, to_y, context)
	if not self:isTilePassable(pathing, to_x, to_y, context) then
		return false
	end
	return self:isTileClimbable(pathing, from_x, from_y, context) or self:isTileClimbable(pathing, to_x, to_y, context)
end

function WalkClimbJump:canJumpTo(pathing, from_x, from_y, to_x, to_y, context)
	if not self:canStandAt(pathing, from_x, from_y, context) then
		return false
	end
	if not self:isTilePassable(pathing, to_x, to_y, context) then
		return false
	end
	local mid_y = from_y - 1
	if mid_y < 0 then
		return false
	end
	return self:isTilePassable(pathing, from_x, mid_y, context)
end

function WalkClimbJump:getStartNode(pathing)
	local owner = pathing.owner
	local world = pathing:getWorld()
	if not owner or not world then
		return nil
	end
	local tile_x, tile_y = world:worldToTile(owner.pos)
	return { x = tile_x, y = tile_y }
end

function WalkClimbJump:getGoalNode(pathing, goal)
	local world = pathing:getWorld()
	if not world or not goal then
		return nil
	end
	local tile_x, tile_y = world:worldToTile(goal)
	return { x = tile_x, y = tile_y }
end

function WalkClimbJump:getNodeKey(pathing, node)
	local bounds = self:getTileBounds(pathing)
	if not bounds then
		return tostr(node.x) .. "," .. tostr(node.y)
	end
	local stride = (bounds.max_x - bounds.min_x) + 1
	return ((node.y - bounds.min_y) * stride) + (node.x - bounds.min_x)
end

function WalkClimbJump:getHeuristic(pathing, from_node, to_node)
	return abs(to_node.x - from_node.x) + abs(to_node.y - from_node.y)
end

function WalkClimbJump:isGoal(pathing, node, goal_node)
	return node.x == goal_node.x and node.y == goal_node.y
end

function WalkClimbJump:canDirectPath(pathing, start_node, goal_node, context)
	if not start_node or not goal_node then
		return false
	end
	if start_node.x == goal_node.x and start_node.y == goal_node.y then
		return true
	end
	if start_node.y ~= goal_node.y then
		return false
	end
	local step_x = goal_node.x > start_node.x and 1 or -1
	local current_x = start_node.x
	while current_x ~= goal_node.x do
		local next_x = current_x + step_x
		if not self:canMoveHorizontal(pathing, current_x, start_node.y, next_x, start_node.y, context) then
			return false
		end
		current_x = next_x
	end
	return true
end

function WalkClimbJump:getNeighbors(pathing, node, goal_node, context)
	local tile_x = node.x
	local tile_y = node.y
	local neighbors = {}

	local function try_add(next_x, next_y, cost)
		if self:isTileInBounds(pathing, next_x, next_y, context) then
			add(neighbors, { x = next_x, y = next_y, cost = cost or 1 })
		end
	end

	if self:canMoveHorizontal(pathing, tile_x, tile_y, tile_x - 1, tile_y, context) then
		try_add(tile_x - 1, tile_y, 1)
	end
	if self:canMoveHorizontal(pathing, tile_x, tile_y, tile_x + 1, tile_y, context) then
		try_add(tile_x + 1, tile_y, 1)
	end
	if self:canMoveVertical(pathing, tile_x, tile_y, tile_x, tile_y - 1, context) then
		try_add(tile_x, tile_y - 1, 1)
	end
	if self:canMoveVertical(pathing, tile_x, tile_y, tile_x, tile_y + 1, context) then
		try_add(tile_x, tile_y + 1, 1)
	end
	if self:canJumpTo(pathing, tile_x, tile_y, tile_x - 1, tile_y - 1, context) then
		try_add(tile_x - 1, tile_y - 1, 2)
	end
	if self:canJumpTo(pathing, tile_x, tile_y, tile_x + 1, tile_y - 1, context) then
		try_add(tile_x + 1, tile_y - 1, 2)
	end

	return neighbors
end

function WalkClimbJump:convertPath(pathing, node_path, goal, context)
	local world = context and context.world or pathing:getWorld()
	if not world then
		return nil
	end
	local path = {}
	for i = 1, #node_path do
		local node = node_path[i]
		add(path, world_pos_for_tile(context, node.x, node.y))
	end
	return path
end

function WalkClimbJump:getRandomReachableTarget(pathing, rule)
	local world = pathing:getWorld()
	if not world then
		return nil
	end

	rule = rule or {}
	local attempts = rule.attempts or 24
	local bounds = world:getQueryBounds(rule)
	local tile_size = world:getTileSize()
	local radius = self:getOwnerRadius(pathing)

	local function tile_center(tile_x, tile_y)
		return world:tileToWorld(tile_x, tile_y)
	end

	local function is_valid_tile(tile_x, tile_y)
		if not self:isTileInBounds(pathing, tile_x, tile_y) then
			return false
		end
		if self:isTileClimbable(pathing, tile_x, tile_y) then
			return true
		end
		return self:canStandAt(pathing, tile_x, tile_y)
	end

	for _ = 1, attempts do
		local x = random_int(bounds.left, bounds.right)
		local y = random_int(bounds.top, bounds.bottom)
		local tile_x, tile_y = world:worldToTile({ x = x, y = y })
		if is_valid_tile(tile_x, tile_y) then
			return tile_center(tile_x, tile_y)
		end
	end

	for y = bounds.top, bounds.bottom, tile_size do
		for x = bounds.left, bounds.right, tile_size do
			local tile_x, tile_y = world:worldToTile({ x = x, y = y })
			if is_valid_tile(tile_x, tile_y) then
				return tile_center(tile_x, tile_y)
			end
		end
	end

	return world:getRandomSurfacePosition({
		margin = rule.margin,
		padding = rule.padding,
		attempts = attempts,
	}, false, radius)
end

return WalkClimbJump
