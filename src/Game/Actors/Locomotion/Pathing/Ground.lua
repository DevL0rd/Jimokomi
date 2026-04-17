local Ground = {
	max_iterations = 48,
	patrol_step = 2,
}

function Ground:isTileInBounds(pathing, tile_x, tile_y)
	local world = pathing:getWorld()
	local bounds = world and world:getTileBounds() or nil
	return bounds ~= nil and
		tile_x >= bounds.min_x and tile_x <= bounds.max_x and
		tile_y >= bounds.min_y and tile_y <= bounds.max_y
end

function Ground:isTilePassable(pathing, tile_x, tile_y)
	local world = pathing:getWorld()
	if not world or not self:isTileInBounds(pathing, tile_x, tile_y) then
		return false
	end
	local pos = world:tileToWorld(tile_x, tile_y)
	return not world:isSolidAt(pos.x, pos.y)
end

function Ground:hasGroundBelow(pathing, tile_x, tile_y)
	local world = pathing:getWorld()
	local bounds = world and world:getTileBounds() or nil
	if not world or not bounds then
		return false
	end
	if tile_y + 1 > bounds.max_y then
		return true
	end
	return not self:isTilePassable(pathing, tile_x, tile_y + 1)
end

function Ground:canStandAt(pathing, tile_x, tile_y)
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
	return tostr(node.x) .. "," .. tostr(node.y)
end

function Ground:getHeuristic(pathing, from_node, to_node)
	return abs(to_node.x - from_node.x) + abs(to_node.y - from_node.y)
end

function Ground:isGoal(pathing, node, goal_node)
	return node.x == goal_node.x and node.y == goal_node.y
end

function Ground:getNeighbors(pathing, node)
	local neighbors = {}
	local next_y = node.y
	if self:canStandAt(pathing, node.x - 1, next_y) then
		add(neighbors, { x = node.x - 1, y = next_y, cost = 1 })
	end
	if self:canStandAt(pathing, node.x + 1, next_y) then
		add(neighbors, { x = node.x + 1, y = next_y, cost = 1 })
	end
	return neighbors
end

function Ground:convertPath(pathing, node_path, goal)
	local world = pathing:getWorld()
	if not world then
		return nil
	end
	local path = {}
	for i = 1, #node_path do
		local node = node_path[i]
		add(path, world:tileToWorld(node.x, node.y))
	end
	return path
end

return Ground
