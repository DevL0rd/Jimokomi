local Ground = {
	max_iterations = 48,
	patrol_step = 2,
}

local function make_search_context(self, pathing)
	local world = pathing:getWorld()
	local bounds = world and world:getTileBounds() or nil
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
		passable = {},
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

function Ground:prepareSearch(pathing, goal, start_node, goal_node)
	return make_search_context(self, pathing)
end

function Ground:isTileInBounds(pathing, tile_x, tile_y, context)
	local bounds = context and context.bounds or pathing:getWorld() and pathing:getWorld():getTileBounds() or nil
	return bounds ~= nil and
		tile_x >= bounds.min_x and tile_x <= bounds.max_x and
		tile_y >= bounds.min_y and tile_y <= bounds.max_y
end

function Ground:isTilePassable(pathing, tile_x, tile_y, context)
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
		local tile_id = collision and collision:getTileIDAt(tile_x * context.tile_size, tile_y * context.tile_size, context.map_id) or 0
		local passable = collision and not collision:isSolidTile(tile_id) or false
		if key ~= nil then
			context.passable[key] = passable
		end
		return passable
	end
	local pos = world:tileToWorld(tile_x, tile_y)
	return not world:isSolidAt(pos.x, pos.y)
end

function Ground:hasGroundBelow(pathing, tile_x, tile_y, context)
	local world = pathing:getWorld()
	local bounds = context and context.bounds or world and world:getTileBounds() or nil
	if not world or not bounds then
		return false
	end
	if tile_y + 1 > bounds.max_y then
		return true
	end
	return not self:isTilePassable(pathing, tile_x, tile_y + 1, context)
end

function Ground:canStandAt(pathing, tile_x, tile_y, context)
	if context then
		local key = tile_key(context, tile_x, tile_y)
		local cached = key ~= nil and context.standable[key] or nil
		if cached ~= nil then
			return cached
		end
		local standable = self:isTilePassable(pathing, tile_x, tile_y, context) and self:hasGroundBelow(pathing, tile_x, tile_y, context)
		if key ~= nil then
			context.standable[key] = standable
		end
		return standable
	end
	return self:isTilePassable(pathing, tile_x, tile_y) and self:hasGroundBelow(pathing, tile_x, tile_y)
end

function Ground:getStartNode(pathing)
	local owner = pathing.owner
	local world = pathing:getWorld()
	if not owner or not world then
		return nil
	end
	local tile_x, tile_y = world:worldToTile(owner.pos)
	return { x = tile_x, y = tile_y }
end

function Ground:getGoalNode(pathing, goal)
	local world = pathing:getWorld()
	if not world or not goal then
		return nil
	end
	local tile_x, tile_y = world:worldToTile(goal)
	return { x = tile_x, y = tile_y }
end

function Ground:getNodeKey(pathing, node)
	local world = pathing:getWorld()
	local bounds = world and world:getTileBounds() or nil
	if not bounds then
		return tostr(node.x) .. "," .. tostr(node.y)
	end
	local stride = (bounds.max_x - bounds.min_x) + 1
	return ((node.y - bounds.min_y) * stride) + (node.x - bounds.min_x)
end

function Ground:getHeuristic(pathing, from_node, to_node)
	return abs(to_node.x - from_node.x) + abs(to_node.y - from_node.y)
end

function Ground:isGoal(pathing, node, goal_node)
	return node.x == goal_node.x and node.y == goal_node.y
end

function Ground:canDirectPath(pathing, start_node, goal_node, context)
	if not start_node or not goal_node or start_node.y ~= goal_node.y then
		return false
	end
	local step_x = goal_node.x > start_node.x and 1 or -1
	local current_x = start_node.x
	while current_x ~= goal_node.x do
		current_x += step_x
		if not self:canStandAt(pathing, current_x, start_node.y, context) then
			return false
		end
	end
	return true
end

function Ground:getNeighbors(pathing, node, goal_node, context)
	local neighbors = {}
	local next_y = node.y
	if self:canStandAt(pathing, node.x - 1, next_y, context) then
		add(neighbors, { x = node.x - 1, y = next_y, cost = 1 })
	end
	if self:canStandAt(pathing, node.x + 1, next_y, context) then
		add(neighbors, { x = node.x + 1, y = next_y, cost = 1 })
	end
	return neighbors
end

function Ground:convertPath(pathing, node_path, goal, context)
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

return Ground
