local ProceduralSprite = include("src/Engine/Objects/ProceduralSprite.lua")
local WorldObjectCollider = include("src/Engine/Objects/WorldObject/Collider.lua")

local SPACE_BALL_FRAMES = 8

local function wave01(value)
	return (sin(value) + 1) * 0.5
end

local SpaceBall = ProceduralSprite:new({
	_type = "SpaceBall",
	shape = {
		kind = "circle",
		r = 8,
	},
	ignore_physics = false,
	ignore_gravity = false,
	ignore_friction = false,
	ignore_collisions = false,
	resolve_entity_collisions = true,
	draw_debug = function(self)
		if WorldObjectCollider and WorldObjectCollider.strokeShape then
			WorldObjectCollider.strokeShape(self, 8)
		end
		if self.layer and self.layer.gfx and not self:isRotationLocked() then
			local angle = -(self.angle or 0)
			local radius = max(4, self:getRadius())
			local x1 = self.pos.x
			local y1 = self.pos.y
			local x2 = x1 + cos(angle) * radius
			local y2 = y1 + sin(angle) * radius
			self.layer.gfx:line(x1, y1, x2, y2, 10)
		end
	end,
	cache_procedural_sprite = true,
	rotation_bucket_count = 16,
	sprite = 0,
	length = SPACE_BALL_FRAMES,
	fps = 4,
	loop = true,
	stress_test_ball = true,

	init = function(self)
		ProceduralSprite.init(self)
		if self.layer then
			self.layer._stress_space_ball_count = (self.layer._stress_space_ball_count or 0) + 1
			self._stress_ball_registered = true
		end
	end,

	on_destroy = function(self)
		if self._stress_ball_registered and self.layer then
			self.layer._stress_space_ball_count = max(0, (self.layer._stress_space_ball_count or 0) - 1)
			self._stress_ball_registered = false
		end
	end,

	getProceduralSpriteSharedIdentity = function(self)
		return "SpaceBall"
	end,

	getProceduralSpriteVisualSignature = function(self)
		return "space_ball:" .. SPACE_BALL_FRAMES
	end,

	draw = function(self)
		ProceduralSprite.draw(self)
		local layer = self.layer
		local gfx = layer and layer.gfx or nil
		if not gfx then
			return
		end
		local top_left = self:getTopLeft()
		local w = self:getWidth()
		local h = self:getHeight()
		gfx:drawSharedSurfaceStampLayer(
			"space_ball_gloss:" .. tostr(w) .. ":" .. tostr(h),
			top_left.x,
			top_left.y,
			function(target)
				local cx = flr(w * 0.5)
				local cy = flr(h * 0.5)
				local radius = min(cx, cy) - 1
				local glow_x = cx - flr(radius * 0.35)
				local glow_y = cy - flr(radius * 0.4)
				target:circfill(glow_x, glow_y, max(1, flr(radius * 0.3)), 7)
				target:pset(glow_x - 1, glow_y, 7)
				target:pset(glow_x, glow_y - 1, 7)
			end,
			{
				w = w,
				h = h,
				cache_tag = "space_ball.gloss",
				cache_profile_key = "space_ball.gloss",
				cache_profile_signature = tostr(w) .. ":" .. tostr(h),
				entry_ttl_ms = 30000,
			}
		)
	end,

	drawProceduralSprite = function(self, target, w, h, frame_id, frame_index)
		local cx = flr(w * 0.5)
		local cy = flr(h * 0.5)
		local radius = min(cx, cy) - 1
		local phase = ((frame_index or 0) % SPACE_BALL_FRAMES) / SPACE_BALL_FRAMES
		local swirl_shift = flr(phase * 6 + 0.5)
		local marker_cos = 1
		local marker_sin = 0

		target:circfill(cx, cy, radius, 1)
		target:circ(cx, cy, radius, 6)
		target:circ(cx, cy, max(1, radius - 1), 13)

		for y = -radius, radius do
			local row_half = flr(sqrt(max(0, radius * radius - y * y)))
			local highlight = wave01((y + swirl_shift) * 0.55 + phase * 6.28318)
			local band_color = highlight > 0.66 and 13 or (highlight > 0.33 and 12 or 5)
			local left = cx - row_half
			local right = cx + row_half
			if right > left then
				target:line(left, cy + y, right, cy + y, band_color)
			end
		end

		local marker_radius = max(2, flr(radius * 0.45))
		local marker_x = cx + flr(marker_cos * marker_radius)
		local marker_y = cy + flr(marker_sin * marker_radius * 0.75)
		local marker_size = max(2, flr(radius * 0.28))
		target:circfill(marker_x, marker_y, marker_size, 8)
		target:circ(marker_x, marker_y, marker_size, 14)
		target:line(
			cx + flr(marker_cos * 2),
			cy + flr(marker_sin * 2),
			cx + flr(marker_cos * max(3, radius - 2)),
			cy + flr(marker_sin * max(2, radius - 3) * 0.75),
			14
		)
		target:line(
			cx + flr(cos(0.9) * 2),
			cy + flr(sin(0.9) * 2),
			cx + flr(cos(0.9) * max(2, radius - 4)),
			cy + flr(sin(0.9) * max(2, radius - 5) * 0.75),
			8
		)

		local notch_angle = 1.8
		local notch_x = cx + flr(cos(notch_angle) * max(2, radius - 1))
		local notch_y = cy + flr(sin(notch_angle) * max(2, radius - 1) * 0.8)
		target:circfill(notch_x, notch_y, 2, 0)

		for i = 0, radius - 2, 3 do
			local spark_y = cy - radius + 2 + ((i + swirl_shift) % max(1, radius * 2 - 3))
			local spark_x = cx + flr(sin((i + phase * 8) * 0.7) * max(1, radius - 3))
			target:pset(spark_x, spark_y, 10)
		end
	end,
})

return SpaceBall
