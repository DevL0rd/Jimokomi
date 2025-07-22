--[[pod_format="raw",created="2025-07-21 20:55:12",modified="2025-07-22 01:56:51",revision=41]]
Screen = {
	w = 480,
	h = 270
}

function random_float(min_val, max_val)
	return min_val + rnd(max_val - min_val)
end

function random_int(min_val, max_val)
	return flr(min_val + rnd(max_val - min_val + 1))
end

function round(num, decimal_places)
	if decimal_places == nil then
		return flr(num + 0.5)
	else
		local factor = 10 ^ decimal_places
		return flr(num * factor + 0.5) / factor
	end
end

Vector = Class:new({
	_type = "Vector",
	x = 0,
	y = 0,
	randomize = function(self, min_val_x, max_val_x, min_val_y, max_val_y)
		min_val_x = min_val_x or -1
		max_val_x = max_val_x or 1
		min_val_y = min_val_y or -1
		max_val_y = max_val_y or 1
		self.x = random_float(min_val_x, max_val_x)
		self.y = random_float(min_val_y, max_val_y)
		return self
	end,
	round = function(self)
		self.x = round(self.x)
		self.y = round(self.y)
		return self
	end,
	add = function(self, vec2, useDelta)
		useDelta = useDelta or false
		if type(vec2) == "number" then
			vec2 = {
				x = vec2,
				y = vec2
			}
		end
		if useDelta then
			self.x += vec2.x * _dt
			self.y += vec2.y * _dt
		else
			self.x += vec2.x
			self.y += vec2.y
		end
		return self
	end,
	sub = function(self, vec2, useDelta)
		useDelta = useDelta or false
		if type(vec2) == "number" then
			vec2 = {
				x = vec2,
				y = vec2
			}
		end
		if useDelta then
			self.x -= vec2.x * _dt
			self.y -= vec2.y * _dt
		else
			self.x -= vec2.x
			self.y -= vec2.y
		end
		return self
	end,
	len = function(self)
		return sqrt(self.x * self.x + self.y * self.y)
	end,
	normalize = function(self)
		local l = self:len()
		if l > 0 then
			self.x /= l
			self.y /= l
		end
		return self
	end,
	drag = function(self, dragV, useDelta)
		useDelta = useDelta or false
		local d = 1 - dragV
		if useDelta then
			local xDiff = self.x - (self.x * d)
			local yDiff = self.y - (self.y * d)
			self.x -= xDiff * _dt
			self.y -= yDiff * _dt
		else
			self.x *= d
			self.y *= d
		end
		return self
	end,
	dot = function(self, other_vec)
		return self.x * other_vec.x + self.y * other_vec.y
	end
})

Entity = Class:new({
	_type = "Entity",
	name = "Entity",
	pos = Vector:new({
		x = 0,
		y = 0
	}),
	vel = Vector:new(),
	friction = 10,
	stroke_color = -1,
	fill_color = -1,
	debug = DEBUG,
	lifetime = -1,
	ignore_physics = false,
	ignore_gravity = false,
	ignore_friction = false,
	ignore_collisions = false,
	parent = nil,
	world = nil,
	init = function(self)
		self.init_pos = Vector:new({
			x = self.pos.x,
			y = self.pos.y
		})
		self.timer = Timer:new()
		self.percent_expired = 0
		if self.parent then
			self.world = self.parent.world
		end
		if self.world then
			self.world:add(self)
		end
	end,
	update = function(self)
		if self.lifetime ~= -1 then
			self.percent_expired = self.timer:elapsed() / self.lifetime
			if self.timer:hasElapsed(self.lifetime) then
				self:destroy()
			end
		end
		if not self.ignore_physics then
			if not self.ignore_gravity then
				self.vel:add(self.world.gravity, true)
			end
			if not self.ignore_friction then
				self.vel:drag(self.world.friction, false)
			end
			self.pos:add(self.vel, true)
		end
	end,
	draw = function(self)
		if self.fill then
			self:fill()
		end
		if self.stroke then
			self:stroke()
		end
		if self.debug then
			self:draw_debug()
			local vx = self.vel.x * 0.25
			local vy = self.vel.y * 0.25
			self.world.gfx:line(self.pos.x, self.pos.y, self.pos.x + vx, self.pos.y + vy, 8)
		end
	end,
	draw_debug = function(self)
	end,
	on_collision = function(self, ent, vector)
		if self.debug then
			local collision_scale = 10
			local end_x = self.pos.x + (vector.x * collision_scale)
			local end_y = self.pos.y + (vector.y * collision_scale)

			self.world.gfx:line(self.pos.x, self.pos.y, end_x, end_y, 8)

			self.world.gfx:circfill(end_x, end_y, 2, 8)
		end
		if self.parent then
			self.parent:on_collision(ent, vector)
		end
	end,
	destroy = function(self)
		self._delete = true
	end,
})

Circle = Entity:new({
	_type = "Circle",
	r = 8,
	stroke = function(self)
		if self.stroke_color > -1 then
			self.world.gfx:circ(self.pos.x, self.pos.y, self.r, self.stroke_color)
		end
	end,
	fill = function(self)
		if self.fill_color > -1 then
			self.world.gfx:circfill(self.pos.x, self.pos.y, self.r, self.fill_color)
		end
	end,
	getCenter = function(self)
		return Vector:new({
			x = self.pos.x,
			y = self.pos.y
		})
	end,
	draw_debug = function(self)
		self.world.gfx:circ(self.pos.x, self.pos.y, self.r, 8)
	end
})

Rectangle = Entity:new({
	_type = "Rectangle",
	w = 16,
	h = 16,
	stroke = function(self)
		if self.stroke_color > -1 then
			local x = self.pos.x - self.w / 2
			local y = self.pos.y - self.h / 2
			self.world.gfx:rect(x, y, x + self.w - 1, y + self.h - 1, self.stroke_color)
		end
	end,
	fill = function(self)
		if self.fill_color > -1 then
			local x = self.pos.x - self.w / 2
			local y = self.pos.y - self.h / 2
			self.world.gfx:rectfill(x, y, x + self.w - 1, y + self.h - 1, self.fill_color)
		end
	end,
	getCenter = function(self)
		return Vector:new({
			x = self.pos.x,
			y = self.pos.y
		})
	end,
	getTopLeft = function(self)
		return Vector:new({
			x = self.pos.x - self.w / 2,
			y = self.pos.y - self.h / 2
		})
	end,
	draw_debug = function(self)
		local x = self.pos.x - self.w / 2
		local y = self.pos.y - self.h / 2
		self.world.gfx:rect(x, y, x + self.w - 1, y + self.h - 1, 8)
	end
})

Particle = Circle:new({
	_type = "Particle",
	lifetime = 3000,
	fill_color = 7,
	init = function(self)
		Circle.init(self)
		self.constant_vel = Vector:new()
		self.constant_vel:randomize(-5, 5, -50, -10)
		self.r = random_int(2, 5)
		self.initial_radius = self.r
	end,
	update = function(self)
		local d = 1 - self.percent_expired
		self.r = self.initial_radius * d
		self.vel = self.constant_vel
		DEBUG_TEXT = "Particle Update:\n" ..
			"Position: " .. tostring(self.pos) .. "\n" ..
			"Velocity: " .. tostring(self.vel) .. "\n" ..
			"Radius: " .. tostring(self.r) .. "\n"
		Circle.update(self)
	end
})

ParticleEmitter = Rectangle:new({
	_type = "ParticleEmitter",
	rate = 100,
	rate_variation = 0,
	particle_lifetime = 500,
	particle_lifetime_variation = 0,
	ignore_physics = true,
	state = true,
	Particle = Particle,
	init = function(self)
		Rectangle.init(self)
		self.spawn_timer = Timer:new()
		self.waiting_time = self.rate
	end,
	update = function(self)
		if self.state and self.spawn_timer:hasElapsed(self.waiting_time) then
			if self.rate_variation > 0 then
				self.waiting_time = self.rate + random_int(-self.rate_variation, self.rate_variation)
			else
				self.waiting_time = self.rate
			end
			local particle_lifetime = self.particle_lifetime
			if self.particle_lifetime_variation > 0 then
				particle_lifetime = self.particle_lifetime +
					random_int(-self.particle_lifetime_variation, self.particle_lifetime_variation)
			end
			local p = self.Particle:new({
				world = self.world,
				lifetime = particle_lifetime,
			})
			local min_x = self.pos.x - self.w / 2
			local max_x = self.pos.x + self.w / 2
			local min_y = self.pos.y - self.h / 2
			local max_y = self.pos.y + self.h / 2
			p.pos:randomize(min_x, max_x, min_y, max_y)
		end
		Rectangle.update(self)
	end,
	toggle = function(self)
		self.state = not self.state
	end,
	on = function(self)
		self.state = true
	end,
	off = function(self)
		self.state = false
	end
})

Graphic = Rectangle:new({
	_type = "Graphic",
	flip_x = false,
	flip_y = false,
	ignore_physics = true,
	init = function(self)
		Rectangle.init(self)
		self.pos = Vector:new()
	end,
})


Sprite = Graphic:new({
	_type = "Graphic",
	sprite = -1,
	end_sprite = -1,
	speed = 30,
	loop = true,
	_timer = 0,
	_current_frame_idx = 0,
	is_done = false,
	init = function(self)
		Graphic.init(self)
		self.pos = Vector:new()
	end,
	update = function(self)
		if self.end_sprite < 0 then
			return
		end
		if self.is_done and not self.loop then return end

		self._timer += _dt

		local total_frames = self.end_sprite - self.sprite + 1
		if total_frames <= 0 then return end

		local frame_duration = 1 / self.speed
		local elapsed_frames = self._timer / frame_duration

		if self.loop then
			self._current_frame_idx = flr(elapsed_frames) % total_frames
		else
			if elapsed_frames >= total_frames then
				self._current_frame_idx = total_frames - 1
				if not self.is_done then
					self.is_done = true
				end
			else
				self._current_frame_idx = flr(elapsed_frames)
			end
		end
		Graphic.update(self)
	end,

	get_sprite_id = function(self)
		if self.end_sprite < 0 then
			return self.sprite
		end
		return self.sprite + self._current_frame_idx
	end,

	draw = function(self)
		local x = self.pos.x - self.w / 2
		local y = self.pos.y - self.h / 2
		self.world.gfx:spr(self:get_sprite_id(), x, y, self.flip_x, self.flip_y)
		Graphic.draw(self)
	end,
	reset = function(self)
		self._timer = 0
		self._current_frame_idx = 0
		self.is_done = false
	end
})

Skybox = Graphic:new({
	_type = "Skybox",
	cycle_duration = 60000, -- 60 seconds for full day/night cycle (in milliseconds)
	world = nil,
	init = function(self)
		Graphic.init(self)
		if self.world then
			self.w = self.world.w
			self.h = self.world.h
			self.pos.x = self.world.w / 2
			self.pos.y = self.world.h / 2
		end
		self.cycle_timer = Timer:new()
	end,
	update = function(self)
		-- Reset timer when cycle completes
		if self.cycle_timer:hasElapsed(self.cycle_duration) then
			self.cycle_timer:reset()
		end
		Graphic.update(self)
	end,
	get_time_of_day = function(self)
		-- Returns a value from 0 to 1 representing the time of day
		-- 0 = midnight, 0.25 = dawn, 0.5 = noon, 0.75 = dusk, 1 = midnight again
		return self.cycle_timer:elapsed() / self.cycle_duration
	end,
	get_sky_colors = function(self)
		local time = self:get_time_of_day()

		-- Define color phases for different times of day with smoother transitions
		local phases = {
			{ time = 0.0,  top = 0,  bottom = 1 }, -- Midnight: black to dark blue
			{ time = 0.15, top = 0,  bottom = 1 }, -- Late night: black to dark blue
			{ time = 0.25, top = 1,  bottom = 9 }, -- Dawn: dark blue to orange
			{ time = 0.35, top = 9,  bottom = 10 }, -- Early morning: orange to yellow
			{ time = 0.5,  top = 12, bottom = 7 }, -- Noon: light blue to white
			{ time = 0.65, top = 12, bottom = 10 }, -- Afternoon: light blue to yellow
			{ time = 0.75, top = 8,  bottom = 9 }, -- Dusk: red to orange
			{ time = 0.85, top = 2,  bottom = 1 }, -- Evening: purple to dark blue
			{ time = 1.0,  top = 0,  bottom = 1 } -- Midnight: black to dark blue
		}

		-- Find the two phases to interpolate between
		local phase1, phase2
		for i = 1, #phases - 1 do
			if time >= phases[i].time and time <= phases[i + 1].time then
				phase1 = phases[i]
				phase2 = phases[i + 1]
				break
			end
		end

		-- If we didn't find phases, handle wrap-around
		if not phase1 then
			phase1 = phases[#phases]
			phase2 = phases[1]
		end

		-- Calculate interpolation factor
		local t = 0
		if phase2.time > phase1.time then
			t = (time - phase1.time) / (phase2.time - phase1.time)
		else
			-- Handle wrap-around case
			local total_time = (1.0 - phase1.time) + phase2.time
			if time >= phase1.time then
				t = (time - phase1.time) / total_time
			else
				t = (1.0 - phase1.time + time) / total_time
			end
		end

		-- Smooth the transition with easing
		t = t * t * (3 - 2 * t) -- Smoothstep function

		-- For now, just return the closest phase colors (we'll improve interpolation next)
		if t < 0.5 then
			return phase1.top, phase1.bottom
		else
			return phase2.top, phase2.bottom
		end
	end,
	draw = function(self)
		local top_color, bottom_color = self:get_sky_colors()
		local time = self:get_time_of_day()

		-- Create a more sophisticated gradient
		for y = 0, self.world.h - 1 do
			local gradient_factor = y / self.world.h

			-- Use a smooth gradient transition
			local current_color
			if gradient_factor < 0.3 then
				-- Top third uses top color
				current_color = top_color
			elseif gradient_factor > 0.7 then
				-- Bottom third uses bottom color
				current_color = bottom_color
			else
				-- Middle third blends between colors
				local blend_factor = (gradient_factor - 0.3) / 0.4
				-- Use smoothstep for the blend
				blend_factor = blend_factor * blend_factor * (3 - 2 * blend_factor)

				if blend_factor < 0.5 then
					current_color = top_color
				else
					current_color = bottom_color
				end
			end

			-- Add some atmospheric effects during certain times
			if time > 0.2 and time < 0.35 and gradient_factor > 0.6 then
				-- Dawn horizon glow
				if rnd(1) < 0.3 then
					current_color = 10 -- Yellow glow
				end
			elseif time > 0.7 and time < 0.8 and gradient_factor > 0.5 then
				-- Dusk horizon glow
				if rnd(1) < 0.2 then
					current_color = 8 -- Red glow
				end
			end

			-- Draw the line
			self.world.gfx:line(0, y, self.world.w - 1, y, current_color)
		end

		Graphic.draw(self)
	end
})
