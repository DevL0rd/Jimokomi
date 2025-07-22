--[[pod_format="raw",created="2025-07-21 21:47:56",modified="2025-07-22 00:49:12",revision=8]]
include "src/class.lua"
include "src/timer.lua"
include "src/primitives.lua"
include "src/ray.lua"
include "src/graphics_helper.lua"
local last_time = 0
_dt = 0
Layer = Class:new({
	_type = "Layer",
	entities = {},
	gravity = Vector:new({ y = 200 }),
	friction = 0.01,
	wall_friction = 0.05,
	collisionPasses = 2,
	running = false,
	layer_id = 0,
	physics_enabled = true,
	map_id = nil,
	tile_size = 8,
	tile_properties = {
		[0] = { solid = false, name = "empty" },
		[1] = { solid = true, name = "wall" },
		[2] = { solid = false, name = "background" },
		[3] = { solid = false, name = "decoration" },
	},

	init = function(self)
		self.entities = {}
		self.camera = {
			pos = Vector:new({ x = 0, y = 0 }),
			target = nil,
			offset = Vector:new({ x = Screen.w / 2, y = Screen.h / 2 }),
			smoothing = 0.1,
			bounds = {
				min_x = nil,
				max_x = nil,
				min_y = nil,
				max_y = nil
			},
			shake = {
				initial_intensity = 0,
				intensity = 0,
				duration = -1,
				timer = Timer:new()
			},
			parallax_factor = Vector:new({ x = 1, y = 1 }),
			update = function(self)
				if self.target then
					local target_pos = self.target.pos
					local desired_x = target_pos.x - self.offset.x
					local desired_y = target_pos.y - self.offset.y

					self.pos.x += (desired_x - self.pos.x) * self.smoothing
					self.pos.y += (desired_y - self.pos.y) * self.smoothing
				end

				if self.bounds.min_x then
					self.pos.x = max(self.pos.x, self.bounds.min_x)
				end
				if self.bounds.max_x then
					self.pos.x = min(self.pos.x, self.bounds.max_x)
				end
				if self.bounds.min_y then
					self.pos.y = max(self.pos.y, self.bounds.min_y)
				end
				if self.bounds.max_y then
					self.pos.y = min(self.pos.y, self.bounds.max_y)
				end

				if self.shake.duration ~= -1 then
					local percent_elapsed = self.shake.timer:elapsed() / self.shake.duration
					self.shake.intensity = self.shake.initial_intensity * (1 - percent_elapsed)
					if self.shake.timer:hasElapsed(self.shake.duration, false) then
						self.shake.intensity = 0
						self.shake.duration = -1
					end
				end
			end,

			getShakeOffset = function(self)
				if self.shake.intensity <= 0 then
					return { x = 0, y = 0 }
				end
				return {
					x = (rnd(2) - 1) * self.shake.intensity,
					y = (rnd(2) - 1) * self.shake.intensity
				}
			end,

			startShaking = function(self, intensity, duration)
				intensity = intensity or 5
				duration = duration or -1
				self.shake.initial_intensity = intensity
				self.shake.duration = duration
				if duration ~= -1 then
					self.shake.timer:reset()
				end
			end,

			stopShaking = function(self)
				self.shake.intensity = 0
				self.shake.duration = -1
			end,

			setTarget = function(self, entity)
				self.target = entity
			end,

			setBounds = function(self, min_x, max_x, min_y, max_y)
				self.bounds.min_x = min_x
				self.bounds.max_x = max_x
				self.bounds.min_y = min_y
				self.bounds.max_y = max_y
			end,

			worldToScreen = function(self, world_pos)
				local shake = self:getShakeOffset()
				return {
					x = world_pos.x - (self.pos.x * self.parallax_factor.x) + shake.x,
					y = world_pos.y - (self.pos.y * self.parallax_factor.y) + shake.y
				}
			end,

			screenToWorld = function(self, screen_pos)
				local shake = self:getShakeOffset()
				return {
					x = screen_pos.x + (self.pos.x * self.parallax_factor.x) - shake.x,
					y = screen_pos.y + (self.pos.y * self.parallax_factor.y) - shake.y
				}
			end,

			linkToCamera = function(self, master_camera, parallax_x, parallax_y)
				parallax_x = parallax_x or 1
				parallax_y = parallax_y or 1
				self.parallax_factor.x = parallax_x
				self.parallax_factor.y = parallax_y
				self.pos.x = master_camera.pos.x * parallax_x
				self.pos.y = master_camera.pos.y * parallax_y
			end
		}

		self.gfx = GraphicsHelper:new({
			camera = self.camera
		})
	end,
	collectCollisions = function(self)
		if not self.physics_enabled then return end

		for index, ent in pairs(self.entities) do
			if ent.ignore_physics or ent.ignore_collisions then
				goto skip
			end
			ent.collisions = {}

			local world_coll = nil
			if ent:is_a(Rectangle) then
				world_coll = self:rectToWorld(ent)
			elseif ent:is_a(Circle) then
				world_coll = self:circleToWorld(ent)
			end

			if world_coll then
				add(ent.collisions, {
					object = "world",
					vector = world_coll
				})
			end

			local map_coll = self:checkMapCollision(ent)
			if map_coll then
				add(ent.collisions, {
					object = "map",
					vector = map_coll
				})
			end

			::skip::
		end
	end,

	processCollisions = function(self)
		if not self.physics_enabled then return end

		for index, ent in pairs(self.entities) do
			if ent.ignore_physics or ent.ignore_collisions or not ent.collisions then
				goto skip
			end

			local total_push = Vector:new({ x = 0, y = 0 })
			local has_x_collision = false
			local has_y_collision = false

			for _, collision in pairs(ent.collisions) do
				total_push:add(collision.vector)
				if collision.vector.x ~= 0 then
					has_x_collision = true
				end
				if collision.vector.y ~= 0 then
					has_y_collision = true
				end
			end

			if total_push.x ~= 0 or total_push.y ~= 0 then
				ent.pos:add(total_push)
				ent.vel:drag(self.wall_friction, false)

				if has_x_collision then
					ent.vel.x = 0
				end
				if has_y_collision then
					ent.vel.y = 0
				end
			end

			::skip::
		end
	end,

	resolveCollisions = function(self)
		if not self.physics_enabled then return end

		for i = 1, self.collisionPasses do
			self:collectCollisions()
			self:processCollisions()
		end
	end,


	handleCollisionEvents = function(self)
		if not self.physics_enabled then return end

		for index, ent in pairs(self.entities) do
			if ent.collisions and #ent.collisions > 0 and ent.on_collision then
				for _, collision in pairs(ent.collisions) do
					ent:on_collision(collision.object, collision.vector)
				end
			end
		end
	end,

	update = function(self)
		if not self.running then
			return
		end

		for i = #self.entities, 1, -1 do
			local ent = self.entities[i]
			if not ent._delete then
				ent:update()
			else
				if ent.unInit then
					ent:unInit()
				end
				del(self.entities, ent)
			end
		end

		self.camera:update()
		self:resolveCollisions()
		self:handleCollisionEvents()
		for _, ent in pairs(self.entities) do
			if ent.parent then
				ent.pos.x = ent.parent.pos.x + ent.init_pos.x
				ent.pos.y = ent.parent.pos.y + ent.init_pos.y
			end
		end
	end,

	draw = function(self)
		if not self.running then
			return
		end

		if self.map_id ~= nil then
			-- Apply camera transformation for map rendering
			local shake = self.camera:getShakeOffset()
			local cam_x = flr(self.camera.pos.x * self.camera.parallax_factor.x + shake.x)
			local cam_y = flr(self.camera.pos.y * self.camera.parallax_factor.y + shake.y)

			-- Debug output if DEBUG is enabled
			if DEBUG then
				print("Rendering map " .. self.map_id .. " at offset: " .. cam_x .. ", " .. cam_y)
			end

			-- Set camera for map rendering (positive values to scroll map opposite to camera)
			camera(cam_x, cam_y)
			map(self.map_id)
			camera() -- Reset camera
		end

		-- Draw grid for this layer if DEBUG is enabled
		if DEBUG then
			local grid_size = 16
			local view_top_left = self.camera:screenToWorld({ x = 0, y = 0 })
			local view_bottom_right = self.camera:screenToWorld({ x = Screen.w, y = Screen.h })

			local start_x = flr(view_top_left.x / grid_size) * grid_size
			local start_y = flr(view_top_left.y / grid_size) * grid_size

			-- Use different colors for different layers
			local grid_color = self.layer_id + 5 -- Different color per layer

			-- Vertical lines
			for x = start_x, view_bottom_right.x, grid_size do
				local start_pos = self.camera:worldToScreen({ x = x, y = view_top_left.y })
				local end_pos = self.camera:worldToScreen({ x = x, y = view_bottom_right.y })
				line(start_pos.x, start_pos.y, end_pos.x, end_pos.y, grid_color)
			end

			-- Horizontal lines
			for y = start_y, view_bottom_right.y, grid_size do
				local start_pos = self.camera:worldToScreen({ x = view_top_left.x, y = y })
				local end_pos = self.camera:worldToScreen({ x = view_bottom_right.x, y = y })
				line(start_pos.x, start_pos.y, end_pos.x, end_pos.y, grid_color)
			end
		end

		for _, ent in pairs(self.entities) do
			ent:draw()
		end
	end,

	add = function(self, ent)
		add(self.entities, ent)
		ent.world = self
	end,

	start = function(self)
		self.running = true
	end,

	stop = function(self)
		self.running = false
	end,

	circleToWorld = function(self, cir)
		local push_x = 0
		local push_y = 0

		if cir.pos.x - cir.r < 0 then
			push_x = cir.r - cir.pos.x
		elseif cir.pos.x + cir.r > self.w then
			push_x = self.w - (cir.pos.x + cir.r)
		end

		if cir.pos.y - cir.r < 0 then
			push_y = cir.r - cir.pos.y
		elseif cir.pos.y + cir.r > self.h then
			push_y = self.h - (cir.pos.y + cir.r)
		end

		if push_x ~= 0 or push_y ~= 0 then
			return Vector:new({ x = push_x, y = push_y })
		else
			return false
		end
	end,

	rectToWorld = function(self, rec)
		local push_x = 0
		local push_y = 0

		local left = rec.pos.x - rec.w / 2
		local right = rec.pos.x + rec.w / 2
		local top = rec.pos.y - rec.h / 2
		local bottom = rec.pos.y + rec.h / 2

		if left < 0 then
			push_x = -left
		elseif right > self.w then
			push_x = self.w - right
		end

		if top < 0 then
			push_y = -top
		elseif bottom > self.h then
			push_y = self.h - bottom
		end

		if push_x ~= 0 or push_y ~= 0 then
			return Vector:new({ x = push_x, y = push_y })
		else
			return false
		end
	end,

	setMap = function(self, map_id)
		self.map_id = map_id
	end,

	getTileAt = function(self, x, y)
		if self.map_id == nil then return 0 end
		local tile_x = flr(x / self.tile_size)
		local tile_y = flr(y / self.tile_size)
		return mget(tile_x, tile_y)
	end,

	isSolidTile = function(self, tile_id)
		local properties = self.tile_properties[tile_id]
		return properties and properties.solid or false
	end,

	getTileProperties = function(self, tile_id)
		return self.tile_properties[tile_id] or { solid = false, name = "unknown" }
	end,

	addTileType = function(self, tile_id, properties)
		self.tile_properties[tile_id] = properties
	end,

	checkMapCollision = function(self, ent)
		if self.map_id == nil then return false end

		local push_x = 0
		local push_y = 0

		if ent:is_a(Rectangle) then
			local left = ent.pos.x - ent.w / 2
			local right = ent.pos.x + ent.w / 2
			local top = ent.pos.y - ent.h / 2
			local bottom = ent.pos.y + ent.h / 2

			local tl = self:getTileAt(left, top)
			local tr = self:getTileAt(right, top)
			local bl = self:getTileAt(left, bottom)
			local br = self:getTileAt(right, bottom)

			if self:isSolidTile(tl) or self:isSolidTile(tr) or self:isSolidTile(bl) or self:isSolidTile(br) then
				local tile_x = flr(ent.pos.x / self.tile_size) * self.tile_size
				local tile_y = flr(ent.pos.y / self.tile_size) * self.tile_size

				local overlap_left = (tile_x + self.tile_size) - left
				local overlap_right = right - tile_x
				local overlap_top = (tile_y + self.tile_size) - top
				local overlap_bottom = bottom - tile_y

				if overlap_left < overlap_right and overlap_left < overlap_top and overlap_left < overlap_bottom then
					push_x = overlap_left
				elseif overlap_right < overlap_top and overlap_right < overlap_bottom then
					push_x = -overlap_right
				elseif overlap_top < overlap_bottom then
					push_y = overlap_top
				else
					push_y = -overlap_bottom
				end
			end
		elseif ent:is_a(Circle) then
			local tile = self:getTileAt(ent.pos.x, ent.pos.y)
			if self:isSolidTile(tile) then
				local tile_x = flr(ent.pos.x / self.tile_size) * self.tile_size
				local tile_y = flr(ent.pos.y / self.tile_size) * self.tile_size
				local tile_center_x = tile_x + self.tile_size / 2
				local tile_center_y = tile_y + self.tile_size / 2

				local dx = ent.pos.x - tile_center_x
				local dy = ent.pos.y - tile_center_y
				local dist = sqrt(dx * dx + dy * dy)

				if dist < ent.r + self.tile_size / 2 then
					local push_dist = (ent.r + self.tile_size / 2) - dist
					push_x = (dx / dist) * push_dist
					push_y = (dy / dist) * push_dist
				end
			end
		end

		if push_x ~= 0 or push_y ~= 0 then
			return Vector:new({ x = push_x, y = push_y })
		else
			return false
		end
	end
})

Engine = {
	layers = {},
	master_camera = nil,

	w = 16 * 32,
	h = 16 * 32,
	gravity = Vector:new({
		y = 200
	}),
	fill_color = 32,
	stroke_color = 5,
	createLayer = function(self, layer_id, physics_enabled, map_id)
		physics_enabled = physics_enabled or true
		local layer = Layer:new()
		layer.layer_id = layer_id
		layer.physics_enabled = physics_enabled
		layer.gravity = self.gravity
		layer.map_id = map_id

		layer.w = self.w
		layer.h = self.h

		layer.camera:setBounds(0, self.w - Screen.w, 0, self.h - Screen.h)

		self.layers[layer_id] = layer

		if not self.master_camera then
			self.master_camera = layer.camera
		end

		return layer
	end,

	getLayer = function(self, layer_id)
		return self.layers[layer_id]
	end,

	linkCameras = function(self, layer_id, parallax_x, parallax_y)
		local layer = self.layers[layer_id]
		if layer and self.master_camera then
			layer.camera:linkToCamera(self.master_camera, parallax_x, parallax_y)
		end
	end,

	setMasterCamera = function(self, layer_id)
		local layer = self.layers[layer_id]
		if layer then
			self.master_camera = layer.camera
		end
	end,

	update = function(self)
		_dt = time() - last_time
		last_time = time()

		if self.master_camera then
			self.master_camera:update()
		end

		for layer_id, world in pairs(self.layers) do
			if world.camera ~= self.master_camera and world.camera.parallax_factor then
				world.camera:linkToCamera(self.master_camera,
					world.camera.parallax_factor.x,
					world.camera.parallax_factor.y)
			end

			world:update()
		end
	end,

	draw = function(self)
		cls()

		if self.master_camera then
			local cam_screen = self.master_camera:worldToScreen({ x = 0, y = 0 })

			if self.fill_color > -1 then
				rectfill(cam_screen.x, cam_screen.y,
					cam_screen.x + self.w - 1,
					cam_screen.y + self.h - 1,
					self.fill_color)
			end

			if self.stroke_color > -1 then
				rect(cam_screen.x, cam_screen.y,
					cam_screen.x + self.w - 1,
					cam_screen.y + self.h - 1,
					self.stroke_color)
			end
		end

		local layer_keys = {}
		for k, v in pairs(self.layers) do
			add(layer_keys, k)
		end

		for i = 1, #layer_keys - 1 do
			for j = 1, #layer_keys - i do
				if layer_keys[j] > layer_keys[j + 1] then
					local temp = layer_keys[j]
					layer_keys[j] = layer_keys[j + 1]
					layer_keys[j + 1] = temp
				end
			end
		end

		for _, layer_id in pairs(layer_keys) do
			self.layers[layer_id]:draw()
		end

		-- Reset camera after drawing all layers
		camera()
	end,

	start = function(self)
		last_time = time()
		for _, world in pairs(self.layers) do
			world:start()
		end
	end,

	stop = function(self)
		for _, world in pairs(self.layers) do
			world:stop()
		end
	end
}
