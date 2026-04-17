local AttachmentNode = include("src/Engine/Nodes/AttachmentNode.lua")

local SpriteNode = AttachmentNode:new({
	_type = "SpriteNode",
	shape = {
		kind = "rect",
		w = 16,
		h = 16,
	},
	flip_x = false,
	flip_y = false,
	sprite = -1,
	end_sprite = -1,
	speed = 30,
	loop = true,
	_timer = 0,
	_current_frame_idx = 0,
	is_done = false,

	setShape = function(self, shape)
		self.shape = shape or self.shape
	end,

	getWidth = function(self)
		return self.shape and self.shape.w or 0
	end,

	getHeight = function(self)
		return self.shape and self.shape.h or 0
	end,

	getHalfWidth = function(self)
		return self:getWidth() * 0.5
	end,

	getHalfHeight = function(self)
		return self:getHeight() * 0.5
	end,

	reset = function(self)
		self._timer = 0
		self._current_frame_idx = 0
		self.is_done = false
	end,

	stop = function(self)
		self._timer = 0
		self._current_frame_idx = 0
		self.is_done = true
	end,

	start = function(self)
		self.is_done = false
	end,

	needsNodeUpdate = function(self)
		return self.end_sprite >= 0 and (self.loop or not self.is_done)
	end,

	getSpriteId = function(self)
		if self.end_sprite < 0 then
			return self.sprite
		end
		return self.sprite + self._current_frame_idx
	end,

	updateNode = function(self)
		if self.end_sprite < 0 then
			return
		end
		if self.is_done and not self.loop then
			return
		end
		self._timer += _dt
		local total_frames = self.end_sprite - self.sprite + 1
		if total_frames <= 0 then
			return
		end
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
	end,

	drawNode = function(self)
		if not self.layer then
			return
		end
		if self.sprite < 0 then
			return
		end
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		if profiler then
			profiler:addCounter("render.sprite_node.draws", 1)
			if self.end_sprite >= 0 then
				profiler:addCounter("render.sprite_node.animated_draws", 1)
			else
				profiler:addCounter("render.sprite_node.static_draws", 1)
			end
		end
		local x = self.pos.x - self:getHalfWidth()
		local y = self.pos.y - self:getHalfHeight()
		self.layer.gfx:spr(self:getSpriteId(), x, y, self.flip_x, self.flip_y)
	end,
})

return SpriteNode
