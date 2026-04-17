local Graphic = include("src/Engine/Objects/Graphic.lua")

local ProceduralSprite = Graphic:new({
	_type = "ProceduralSprite",
	sprite = 0,
	end_sprite = nil,
	length = nil,
	speed = 30,
	fps = nil,
	loop = true,
	cache_procedural_sprite = true,
	procedural_sprite_cache_key = nil,
	procedural_sprite_cache_mode = "command",

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

	getProceduralSpriteCacheKey = function(self, w, h, frame_id)
		local stable_key = self.procedural_sprite_cache_key or self._type or "procedural_sprite"
		return "procedural_sprite:" .. stable_key .. ":" .. w .. ":" .. h .. ":" .. frame_id
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
			gfx:drawCachedLayer(
				self:getProceduralSpriteCacheKey(width, height, frame_id),
				top_left.x,
				top_left.y,
				function(target)
					self:drawProceduralSprite(target, width, height, frame_id, frame_index)
				end,
				{
					w = width,
					h = height,
					cache_mode = self.procedural_sprite_cache_mode or "command",
				}
			)
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
