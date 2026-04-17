local WalkClimbJump = {
	max_iterations = 160,
}

function WalkClimbJump:getOwnerRadius(pathing)
	local owner = pathing.owner
	return owner and owner.getRadius and owner:getRadius() or 0
end

function WalkClimbJump:getTileBounds(pathing)
	local world = pathing:getWorld()
	return world and world:getTileBounds() or nil
end

function WalkClimbJump:isTileInBounds(pathing, tile_x, tile_y)
	local bounds = self:getTileBounds(pathing)
	return bounds ~= nil and
		tile_x >= bounds.min_x and tile_x <= bounds.max_x and
		tile_y >= bounds.min_y and tile_y <= bounds.max_y
end

function WalkClimbJump:getTileId(pathing, tile_x, tile_y)
	local world = pathing:getWorld()
	if not world then
		return 0
	end
	local pos = world:tileToWorld(tile_x, tile_y)
	return world:getTileAt(pos.x, pos.y)
end

function WalkClimbJump:isTilePassable(pathing, tile_x, tile_y)
	local world = pathing:getWorld()
	if not world or not self:isTileInBounds(pathing, tile_x, tile_y) then
		return false
	end
	local pos = world:tileToWorld(tile_x, tile_y)
	return not world:isSolidAt(pos.x, pos.y)
end

function WalkClimbJump:isTileClimbable(pathing, tile_x, tile_y)
	local owner = pathing.owner
	if not owner or not owner.locomotion or not owner.locomotion.isClimbableTile then
		return false
	end
	return owner.locomotion:isClimbableTile(self:getTileId(pathing, tile_x, tile_y)) == true
end

function WalkClimbJump:hasGroundBelow(pathing, tile_x, tile_y)
	local bounds = self:getTileBounds(pathing)
	if not bounds then
		return false
	end
	if tile_y + 1 > bounds.max_y then
		return true
	end
	return not self:isTilePassable(pathing, tile_x, tile_y + 1)
end

function WalkClimbJump:canStandAt(pathing, tile_x, tile_y)
	if not self:isTilePassable(pathing, tile_x, tile_y) then
		return false
	end
	return self:hasGroundBelow(pathing, tile_x, tile_y) or self:isTileClimbable(pathing, tile_x, tile_y)
end

function WalkClimbJump:canMoveHorizontal(pathing, from_x, from_y, to_x, to_y)
	if not self:isTilePassable(pathing, to_x, to_y) then
		return false
	end
	if self:isTileClimbable(pathing, from_x, from_y) or self:isTileClimbable(pathing, to_x, to_y) then
		return true
	end
	return self:canStandAt(pathing, from_x, from_y) and self:canStandAt(pathing, to_x, to_y)
end

function WalkClimbJump:canMoveVertical(pathing, from_x, from_y, to_x, to_y)
	if not self:isTilePassable(pathing, to_x, to_y) then
		return false
	end
	return self:isTileClimbable(pathing, from_x, from_y) or self:isTileClimbable(pathing, to_x, to_y)
end

function WalkClimbJump:canJumpTo(pathing, from_x, from_y, to_x, to_y)
	if not self:canStandAt(pathing, from_x, from_y) then
		return false
	end
	if not self:isTilePassable(pathing, to_x, to_y) then
		return false
	end
	local mid_y = from_y - 1
	if mid_y < 0 then
		return false
	end
	return self:isTilePassable(pathing, from_x, mid_y)
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
	return tostr(node.x) .. "," .. tostr(node.y)
end

function WalkClimbJump:getHeuristic(pathing, from_node, to_node)
	return abs(to_node.x - from_node.x) + abs(to_node.y - from_node.y)
end

function WalkClimbJump:isGoal(pathing, node, goal_node)
	return node.x == goal_node.x and node.y == goal_node.y
end

function WalkClimbJump:getNeighbors(pathing, node)
	local tile_x = node.x
	local tile_y = node.y
	local neighbors = {}

	local function try_add(next_x, next_y, cost)
		if self:isTileInBounds(pathing, next_x, next_y) then
			add(neighbors, { x = next_x, y = next_y, cost = cost or 1 })
		end
	end

	if self:canMoveHorizontal(pathing, tile_x, tile_y, tile_x - 1, tile_y) then
		try_add(tile_x - 1, tile_y, 1)
	end
	if self:canMoveHorizontal(pathing, tile_x, tile_y, tile_x + 1, tile_y) then
		try_add(tile_x + 1, tile_y, 1)
	end
	if self:canMoveVertical(pathing, tile_x, tile_y, tile_x, tile_y - 1) then
		try_add(tile_x, tile_y - 1, 1)
	end
	if self:canMoveVertical(pathing, tile_x, tile_y, tile_x, tile_y + 1) then
		try_add(tile_x, tile_y + 1, 1)
	end
	if self:canJumpTo(pathing, tile_x, tile_y, tile_x - 1, tile_y - 1) then
		try_add(tile_x - 1, tile_y - 1, 2)
	end
	if self:canJumpTo(pathing, tile_x, tile_y, tile_x + 1, tile_y - 1) then
		try_add(tile_x + 1, tile_y - 1, 2)
	end

	return neighbors
end

function WalkClimbJump:convertPath(pathing, node_path, goal)
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
