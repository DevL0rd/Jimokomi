local Class = include("src/Engine/Core/Class.lua")
local Timer = include("src/Engine/Core/Timer.lua")

local Pathing = Class:new({
	_type = "Pathing",
	owner = nil,
	policy = nil,
	path = nil,
	path_index = 1,
	path_goal = nil,
	repath_distance = 20,
	repath_rate_hz = 2,
	repath_fail_rate_hz = 1,
	repath_interval = nil,
	repath_fail_interval = nil,
	last_path_info = nil,

	init = function(self)
		self.path = self.path or nil
		self.path_index = self.path_index or 1
		self.path_goal = self.path_goal or nil
		self.path_timer = Timer:new()
		self.last_path_info = self.last_path_info or {
			iterations = 0,
			found = false,
		}
	end,

	getWorld = function(self)
		return self.owner and self.owner.getWorld and self.owner:getWorld() or nil
	end,

	setPolicy = function(self, policy)
		self.policy = policy
	end,

	getRepathInterval = function(self)
		if self.repath_interval ~= nil then
			return self.repath_interval
		end
		local rate = self.repath_rate_hz or 2
		if rate <= 0 then
			return 0
		end
		return 1000 / rate
	end,

	getRepathFailInterval = function(self)
		if self.repath_fail_interval ~= nil then
			return self.repath_fail_interval
		end
		local rate = self.repath_fail_rate_hz or self.repath_rate_hz or 1
		if rate <= 0 then
			return 0
		end
		return 1000 / rate
	end,

	invalidate = function(self)
		self.path = nil
		self.path_index = 1
		self.path_goal = nil
	end,

	setGoal = function(self, goal)
		self.path_goal = goal and { x = goal.x, y = goal.y } or nil
	end,

	getGoalDistanceSquared = function(self, goal)
		if not goal or not self.path_goal then
			return nil
		end
		local dx = self.path_goal.x - goal.x
		local dy = self.path_goal.y - goal.y
		return dx * dx + dy * dy
	end,

	needsRepath = function(self, goal)
		if not goal then
			return false
		end
		if not self.path or #self.path == 0 or not self.path_goal then
			return true
		end
		local goal_dist2 = self:getGoalDistanceSquared(goal)
		return goal_dist2 ~= nil and goal_dist2 > self.repath_distance * self.repath_distance
	end,

	canRepath = function(self)
		local world = self:getWorld()
		if self.path_goal == nil then
			return world == nil or world:consumePathBudget()
		end
		local min_wait = self.last_path_info and self.last_path_info.found and self:getRepathInterval() or self:getRepathFailInterval()
		return self.path_timer:elapsed() >= min_wait and (world == nil or world:consumePathBudget())
	end,

	resolveDirectPath = function(self, goal)
		self.path = {
			{ x = goal.x, y = goal.y }
		}
		self.path_index = 1
		self:setGoal(goal)
		self.last_path_info = {
			iterations = 0,
			found = true,
			direct = true,
		}
		self.path_timer:reset()
		return self.path
	end,

	rebuild = function(self, goal)
		local profiler = self.owner and self.owner.layer and self.owner.layer.engine and self.owner.layer.engine.profiler or nil
		local scope = profiler and profiler:start("game.pathing.rebuild") or nil
		local world = self:getWorld()
		if not world or not goal then
			if profiler then profiler:stop(scope) end
			self:invalidate()
			return nil
		end

		local policy = self.policy
		if not policy then
			if profiler then profiler:stop(scope) end
			self:invalidate()
			return nil
		end

		if policy.findPath then
			local direct_path, direct_info = policy:findPath(self, goal)
			self.path = direct_path
			self.path_index = 1
			self:setGoal(goal)
			self.last_path_info = direct_info or {
				iterations = 0,
				found = direct_path ~= nil,
			}
			self.path_timer:reset()
			if profiler then
				profiler:addCounter("game.pathing.direct_paths", 1)
				profiler:stop(scope)
			end
			return self.path
		end

		local start_node = policy:getStartNode(self)
		local goal_node = policy:getGoalNode(self, goal)
		if not start_node or not goal_node then
			if profiler then profiler:stop(scope) end
			self:invalidate()
			return nil
		end

		local path_nodes, path_info = world:findPath(start_node, goal_node, {
			max_iterations = policy.max_iterations or 256,
			getNodeKey = function(node)
				return policy:getNodeKey(self, node)
			end,
			getHeuristic = function(from_node, to_node)
				return policy:getHeuristic(self, from_node, to_node)
			end,
			getNeighbors = function(node)
				return policy:getNeighbors(self, node, goal_node)
			end,
			isGoal = function(node, wanted_goal)
				return policy:isGoal(self, node, wanted_goal)
			end,
		})

		self.path = path_nodes and policy:convertPath(self, path_nodes, goal) or nil
		self.path_index = 1
		self:setGoal(goal)
		self.last_path_info = path_info or {
			iterations = 0,
			found = self.path ~= nil,
		}
		self.path_timer:reset()
		if profiler then
			profiler:observe("game.pathing.rebuild_iterations", self.last_path_info.iterations or 0)
			profiler:stop(scope)
		end
		return self.path
	end,

	ensurePath = function(self, goal)
		if not goal then
			return nil
		end
		if self:needsRepath(goal) and self:canRepath() then
			self:rebuild(goal)
		end
		return self.path
	end,

	getWaypoint = function(self, goal, reach_distance)
		local owner = self.owner
		if not owner or not goal then
			return goal
		end

		self:ensurePath(goal)
		if not self.path or #self.path == 0 then
			return goal
		end

		local min_reach = reach_distance or 12
		local waypoint = self.path[self.path_index]
		while waypoint do
			local dx = waypoint.x - owner.pos.x
			local dy = waypoint.y - owner.pos.y
			if dx * dx + dy * dy > min_reach * min_reach then
				break
			end
			self.path_index += 1
			waypoint = self.path[self.path_index]
		end

		return waypoint or goal
	end,
})

return Pathing
