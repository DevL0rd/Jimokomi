local Graphic = include("src/Engine/Objects/Graphic.lua")
local SharedVisualCache = include("src/Engine/Mixins/SharedVisualCache.lua")

local ProceduralSprite = Graphic:new({
	_type = "ProceduralSprite",
	entity_kind = 2,
	sprite = 0,
	end_sprite = nil,
	length = nil,
	speed = 30,
	fps = nil,
	loop = true,
	cache_procedural_sprite = true,
	rotation_bucket_count = 32,
	procedural_cache_entry_ttl_ms = 30000,

	drawProceduralSprite = function(self, target, w, h, frame_id, frame_index)
	end,

	getProceduralSpriteFps = function(self)
		return self.fps or self.speed or 30
	end,

	getProceduralSpriteLength = function(self)
		if self.length ~= nil then
			return max(1, self.length)
		end
		if self.end_sprite ~= nil then
			return max(1, (self.end_sprite - self.sprite) + 1)
		end
		return 1
	end,

	getProceduralFrameIndex = function(self)
		local frame_count = self:getProceduralSpriteLength()
		if frame_count <= 1 then
			return 0
		end
		local fps = self:getProceduralSpriteFps()
		if fps <= 0 then
			return 0
		end
		local elapsed_frames = time() * fps
		if self.loop == false then
			return min(frame_count - 1, flr(elapsed_frames))
		end
		return flr(elapsed_frames) % frame_count
	end,

	getProceduralFrameId = function(self)
		return (self.sprite or 0) + self:getProceduralFrameIndex()
	end,

	getProceduralSpriteSharedIdentity = function(self)
		if self.getSharedCacheIdentity then
			return self:getSharedCacheIdentity()
		end
		return tostr(self._type or "procedural_sprite")
	end,

	getProceduralSpriteVisualSignature = function(self)
		return table.concat({
			tostr(self.sprite or 0),
			tostr(self.end_sprite or -1),
			tostr(self:getProceduralSpriteLength()),
			tostr(self:getProceduralSpriteFps()),
			tostr(self.loop ~= false),
		}, ":")
	end,

	getProceduralSpriteCachePrefix = function(self, w, h)
		local shared_identity = self:getProceduralSpriteSharedIdentity()
		local visual_signature = self:getProceduralSpriteVisualSignature()
		local width = max(1, flr(w or 0))
		local height = max(1, flr(h or 0))
		local cached_prefix = self._procedural_cache_prefix or nil
		if cached_prefix and
			self._procedural_cache_prefix_identity == shared_identity and
			self._procedural_cache_prefix_signature == visual_signature and
			self._procedural_cache_prefix_w == width and
			self._procedural_cache_prefix_h == height then
			return cached_prefix
		end
		cached_prefix = "procedural_sprite:" ..
			shared_identity .. ":" ..
			visual_signature .. ":" ..
			tostr(width) .. ":" ..
			tostr(height) .. ":"
		self._procedural_cache_prefix = cached_prefix
		self._procedural_cache_prefix_identity = shared_identity
		self._procedural_cache_prefix_signature = visual_signature
		self._procedural_cache_prefix_w = width
		self._procedural_cache_prefix_h = height
		return cached_prefix
	end,

	getProceduralSpriteCacheKey = function(self, w, h, frame_index)
		return self:getProceduralSpriteCachePrefix(w, h) .. tostr(frame_index or 0)
	end,

	getProceduralSpriteProfileKey = function(self)
		return "procedural_sprite:" .. self:getProceduralSpriteSharedIdentity()
	end,

	getProceduralSpriteProfileSignature = function(self, w, h)
		return table.concat({
			self:getProceduralSpriteVisualSignature(),
			tostr(w),
			tostr(h),
		}, ":")
	end,

	getProceduralSpriteRotationAngle = function(self)
		return -(self.angle or 0)
	end,

	getProceduralSpriteRotationBucketCount = function(self)
		return SharedVisualCache.resolveRotationBucketCount(
			self,
			self:getProceduralSpriteRotationAngle(),
			self.rotation_bucket_count or 1
		)
	end,

	draw = function(self)
		local gfx = self.layer and self.layer.gfx or nil
		if not gfx then
			Graphic.draw(self)
			return
		end
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		local top_left = self:getTopLeft()
		local width = self:getWidth()
		local height = self:getHeight()
		local frame_index = self:getProceduralFrameIndex()
		local frame_id = (self.sprite or 0) + frame_index
		if profiler then
			profiler:addCounter("render.procedural_sprite.draws", 1)
			profiler:observe("render.procedural_sprite.area", width * height)
		end

		if self.cache_procedural_sprite and self.drawProceduralSprite then
			if profiler then
				profiler:addCounter("render.procedural_sprite.cached_draws", 1)
			end
			local shared_identity = self:getProceduralSpriteSharedIdentity()
			local visual_signature = self:getProceduralSpriteVisualSignature()
			local cache_tag = self._procedural_cache_tag
			if cache_tag == nil or self._procedural_cache_tag_identity ~= shared_identity then
				cache_tag = "procedural_sprite." .. shared_identity
				self._procedural_cache_tag = cache_tag
				self._procedural_cache_tag_identity = shared_identity
			end
			local rotation_angle = self:getProceduralSpriteRotationAngle()
			local rotation_bucket_count = self:getProceduralSpriteRotationBucketCount()
			local draw_builder = function(target)
				self:drawProceduralSprite(target, width, height, frame_id, frame_index)
			end
			local draw_options = {
				w = width,
				h = height,
				cache_tag = cache_tag,
				cache_profile_key = "procedural_sprite:" .. shared_identity,
				cache_profile_signature = visual_signature .. ":" .. tostr(width) .. ":" .. tostr(height),
				rotation_angle = rotation_angle,
				rotation_bucket_count = rotation_bucket_count,
				entry_ttl_ms = SharedVisualCache.resolveEntryTtlMs(
					self,
					self.procedural_cache_entry_ttl_ms,
					30000
				),
				rotation_entry_ttl_ms = SharedVisualCache.resolveRotationEntryTtlMs(
					self,
					self.procedural_rotation_cache_entry_ttl_ms,
					self.procedural_cache_entry_ttl_ms,
					30000
				),
			}
			if rotation_bucket_count > 1 and abs(rotation_angle or 0) > 0.0001 then
				gfx:drawSharedRotatedSurfaceStampLayer(
					self:getProceduralSpriteCacheKey(width, height, frame_index),
					top_left.x,
					top_left.y,
					draw_builder,
					draw_options
				)
			else
				gfx:drawSharedSurfaceStampLayer(
					self:getProceduralSpriteCacheKey(width, height, frame_index),
					top_left.x,
					top_left.y,
					draw_builder,
					draw_options
				)
			end
		elseif self.drawProceduralSprite then
			if profiler then
				profiler:addCounter("render.procedural_sprite.immediate_draws", 1)
			end
			local sx, sy = gfx.camera:layerToScreenXY(top_left.x, top_left.y)
			local target = gfx:buildImmediateTarget(sx, sy)
			self:drawProceduralSprite(target, width, height, frame_id, frame_index)
		end

		Graphic.draw(self)
	end,
})

return ProceduralSprite
