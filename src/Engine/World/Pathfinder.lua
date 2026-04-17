local Class = include("src/Engine/Core/Class.lua")

local function default_node_key(node)
	if node == nil then
		return nil
	end
	if node.key ~= nil then
		return node.key
	end
	if node.id ~= nil then
		return node.id
	end
	if node.x ~= nil and node.y ~= nil then
		return tostr(node.x) .. "," .. tostr(node.y)
	end
	return tostr(node)
end

local function default_heuristic(from_node, goal_node)
	if from_node == nil or goal_node == nil then
		return 0
	end
	if from_node.x ~= nil and from_node.y ~= nil and goal_node.x ~= nil and goal_node.y ~= nil then
		return abs(goal_node.x - from_node.x) + abs(goal_node.y - from_node.y)
	end
	return 0
end

local Pathfinder = Class:new({
	_type = "Pathfinder",

	getNodeKey = function(self, node, options)
		local get_node_key = options and options.getNodeKey or nil
		if get_node_key then
			return get_node_key(node)
		end
		return default_node_key(node)
	end,

	getHeuristic = function(self, from_node, goal_node, options)
		local get_heuristic = options and options.getHeuristic or nil
		if get_heuristic then
			return get_heuristic(from_node, goal_node)
		end
		return default_heuristic(from_node, goal_node)
	end,

	isGoal = function(self, current_node, goal_node, options)
		local is_goal = options and options.isGoal or nil
		if is_goal then
			return is_goal(current_node, goal_node) == true
		end
		return self:getNodeKey(current_node, options) == self:getNodeKey(goal_node, options)
	end,

	reconstructPath = function(self, came_from, current_key)
		local reverse_path = {}
		local key = current_key
		while key do
			local node = came_from[key]
			if not node then
				break
			end
			add(reverse_path, node.node)
			key = node.previous
		end

		local path = {}
		for i = #reverse_path, 1, -1 do
			add(path, reverse_path[i])
		end
		return path
	end,

	findPath = function(self, start_node, goal_node, options)
		local profiler = self.world and self.world.layer and self.world.layer.engine and self.world.layer.engine.profiler or nil
		local scope = profiler and profiler:start("world.pathfinder.find_path") or nil
		if not start_node or not goal_node then
			if profiler then
				profiler:addCounter("world.pathfinder.failures", 1)
				profiler:stop(scope)
			end
			return nil, { iterations = 0, found = false }
		end

		options = options or {}
		local get_neighbors = options.getNeighbors
		if not get_neighbors then
			if profiler then
				profiler:addCounter("world.pathfinder.failures", 1)
				profiler:stop(scope)
			end
			return nil, { iterations = 0, found = false, error = "missing getNeighbors" }
		end

		local max_iterations = options.max_iterations or 256
		local start_key = self:getNodeKey(start_node, options)
		local goal_key = self:getNodeKey(goal_node, options)
		if start_key == nil or goal_key == nil then
			if profiler then
				profiler:addCounter("world.pathfinder.failures", 1)
				profiler:stop(scope)
			end
			return nil, { iterations = 0, found = false, error = "missing node key" }
		end

		local open = {
			{
				node = start_node,
				key = start_key,
				g = 0,
				f = self:getHeuristic(start_node, goal_node, options),
			}
		}
		local open_lookup = {
			[start_key] = true,
		}
		local closed = {}
		local came_from = {
			[start_key] = {
				node = start_node,
				previous = nil,
			}
		}
		local scores = {
			[start_key] = 0,
		}

		local iterations = 0
		while #open > 0 and iterations < max_iterations do
			iterations += 1

			local best_index = 1
			local best_entry = open[1]
			for i = 2, #open do
				local entry = open[i]
				if entry.f < best_entry.f then
					best_index = i
					best_entry = entry
				end
			end

			local current = best_entry
			del(open, current)
			open_lookup[current.key] = nil

			if current.key == goal_key or self:isGoal(current.node, goal_node, options) then
				if profiler then
					profiler:addCounter("world.pathfinder.successes", 1)
					profiler:observe("world.pathfinder.iterations", iterations)
					profiler:stop(scope)
				end
				local path = self:reconstructPath(came_from, current.key)
				if profiler then
					profiler:observe("world.pathfinder.path_length", #path)
				end
				return path, {
					iterations = iterations,
					found = true,
				}
			end

			closed[current.key] = true
			local neighbors = get_neighbors(current.node) or {}
			for i = 1, #neighbors do
				local neighbor = neighbors[i]
				local neighbor_key = self:getNodeKey(neighbor, options)
				if neighbor_key ~= nil and not closed[neighbor_key] then
					local step_cost = neighbor.cost or 1
					local tentative_score = current.g + step_cost
					local known_score = scores[neighbor_key]

					if known_score == nil or tentative_score < known_score then
						scores[neighbor_key] = tentative_score
						came_from[neighbor_key] = {
							node = neighbor,
							previous = current.key,
						}

						local estimate = tentative_score + self:getHeuristic(neighbor, goal_node, options)
						if open_lookup[neighbor_key] then
							for j = 1, #open do
								local open_entry = open[j]
								if open_entry.key == neighbor_key then
									open_entry.node = neighbor
									open_entry.g = tentative_score
									open_entry.f = estimate
									break
								end
							end
						else
							add(open, {
								node = neighbor,
								key = neighbor_key,
								g = tentative_score,
								f = estimate,
							})
							open_lookup[neighbor_key] = true
						end
					end
				end
			end
		end

		if profiler then
			profiler:addCounter("world.pathfinder.failures", 1)
			profiler:observe("world.pathfinder.iterations", iterations)
			profiler:stop(scope)
		end
		return nil, {
			iterations = iterations,
			found = false,
		}
	end,
})

return Pathfinder
