local AttachmentNode = include("src/Engine/Nodes/AttachmentNode.lua")
local SharedVisualCache = include("src/Engine/Mixins/SharedVisualCache.lua")

local SpriteNode = AttachmentNode:new({
	_type = "SpriteNode",
	node_kind = 1,
	shape = {
		kind = "rect",
		w = 16,
		h = 16,
	},
	flip_x = false,
	flip_y = false,
	sprite = -1,
	end_sprite = -1,
	cache_mode = 2,
	cache_tag = "node.sprite",
	cache_override_mode = nil,
	sprite_cache_entry_ttl_ms = 30000,
	speed = 30,
	loop = true,
	rotation_bucket_count = 32,
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

	getSpriteCacheProfileKey = function(self)
		return "node.sprite:" .. self:getSpriteSharedIdentity()
	end,

	getSpriteSharedIdentity = function(self)
		return table.concat({
			tostr(self.cache_tag or "node.sprite"),
			tostr(self.sprite or -1),
			tostr(self.end_sprite or -1),
		}, ":")
	end,

	getSpriteCacheProfileSignature = function(self)
		local total_frames = self.end_sprite >= 0 and (self.end_sprite - self.sprite + 1) or 1
		return table.concat({
			tostr(self:getWidth()),
			tostr(self:getHeight()),
			tostr(self.flip_x == true),
			tostr(self.flip_y == true),
			tostr(total_frames),
		}, ":")
	end,

	getSpriteRotationAngle = function(self)
		return -(self.angle or 0)
	end,

	getSpriteRotationBucketCount = function(self)
		return SharedVisualCache.resolveRotationBucketCount(
			self,
			self:getSpriteRotationAngle(),
			self.rotation_bucket_count or 1
		)
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
		local shape = self.shape or nil
		local width = shape and shape.w or self:getWidth()
		local height = shape and shape.h or self:getHeight()
		local x = self.pos.x - width * 0.5
		local y = self.pos.y - height * 0.5
		local sprite_id = self:getSpriteId()
		local cache_mode = self.cache_mode or 2
		local rotation_angle = self:getSpriteRotationAngle()
		local rotation_bucket_count = self:getSpriteRotationBucketCount()
		local wants_rotated_cache = rotation_bucket_count > 1 and abs(rotation_angle or 0) > 0.0001
		if (cache_mode == 2 and not wants_rotated_cache) or not self.layer.gfx.cache_renders then
			local transform = self.layer.camera:getRenderTransform()
			spr(sprite_id, x - transform.x, y - transform.y, self.flip_x, self.flip_y)
			return
		end
		local cache_key = "sprite_node:" ..
			self:getSpriteSharedIdentity() .. ":" ..
			tostr(sprite_id) .. ":" ..
			tostr(self.flip_x == true) .. ":" ..
			tostr(self.flip_y == true) .. ":" ..
			tostr(width) .. ":" ..
			tostr(height)
		self.layer.gfx:drawCachedSpriteLayer(
			cache_key,
			sprite_id,
			x,
			y,
			self.flip_x,
			self.flip_y,
			{
				w = width,
				h = height,
				cache_mode = cache_mode,
				cache_tag = self.cache_tag or "node.sprite",
				cache_profile_key = self:getSpriteCacheProfileKey(),
				cache_profile_signature = self:getSpriteCacheProfileSignature(),
				cache_override_mode = self.cache_override_mode,
				rotation_angle = rotation_angle,
				rotation_bucket_count = rotation_bucket_count,
				entry_ttl_ms = SharedVisualCache.resolveEntryTtlMs(
					self,
					self.sprite_cache_entry_ttl_ms,
					30000
				),
				rotation_entry_ttl_ms = SharedVisualCache.resolveRotationEntryTtlMs(
					self,
					self.sprite_rotation_cache_entry_ttl_ms,
					self.sprite_cache_entry_ttl_ms,
					30000
				),
			}
		)
	end,
})

return SpriteNode
