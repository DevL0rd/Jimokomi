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

local function is_higher_priority(entry_a, entry_b)
	if entry_a.f == entry_b.f then
		return entry_a.g > entry_b.g
	end
	return entry_a.f < entry_b.f
end

local function heap_swap(heap, a_index, b_index)
	local a_entry = heap[a_index]
	local b_entry = heap[b_index]
	heap[a_index] = b_entry
	heap[b_index] = a_entry
	a_entry.heap_index = b_index
	b_entry.heap_index = a_index
end

local function sift_up(heap, index)
	while index > 1 do
		local parent_index = flr(index / 2)
		if not is_higher_priority(heap[index], heap[parent_index]) then
			break
		end
		heap_swap(heap, index, parent_index)
		index = parent_index
	end
end

local function sift_down(heap, index)
	local heap_size = #heap
	while true do
		local left_index = index * 2
		local right_index = left_index + 1
		local best_index = index
		if left_index <= heap_size and is_higher_priority(heap[left_index], heap[best_index]) then
			best_index = left_index
		end
		if right_index <= heap_size and is_higher_priority(heap[right_index], heap[best_index]) then
			best_index = right_index
		end
		if best_index == index then
			break
		end
		heap_swap(heap, index, best_index)
		index = best_index
	end
end

local function heap_push(heap, entry)
	add(heap, entry)
	entry.heap_index = #heap
	sift_up(heap, entry.heap_index)
	return entry
end

local function heap_pop(heap)
	local heap_size = #heap
	if heap_size == 0 then
		return nil
	end
	local root = heap[1]
	local last = heap[heap_size]
	heap[heap_size] = nil
	if heap_size > 1 then
		heap[1] = last
		last.heap_index = 1
		sift_down(heap, 1)
	end
	root.heap_index = nil
	return root
end

local function heap_update(heap, entry)
	if not entry or not entry.heap_index then
		return
	end
	sift_up(heap, entry.heap_index)
	sift_down(heap, entry.heap_index)
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

		local open = {}
		local start_entry = {
			node = start_node,
			key = start_key,
			g = 0,
			f = self:getHeuristic(start_node, goal_node, options),
		}
		heap_push(open, start_entry)
		local open_lookup = {
			[start_key] = start_entry,
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
			local current = heap_pop(open)
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
						local open_entry = open_lookup[neighbor_key]
						if open_entry then
							open_entry.node = neighbor
							open_entry.g = tentative_score
							open_entry.f = estimate
							heap_update(open, open_entry)
						else
							open_entry = {
								node = neighbor,
								key = neighbor_key,
								g = tentative_score,
								f = estimate,
							}
							heap_push(open, open_entry)
							open_lookup[neighbor_key] = open_entry
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
