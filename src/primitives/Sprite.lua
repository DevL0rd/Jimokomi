local Graphic = include("src/primitives/Graphic.lua")
local Vector = include("src/classes/Vector.lua")

local Sprite = Graphic:new({
    _type = "Sprite",
    ignore_collisions = true,
    sprite = -1,
    end_sprite = -1,
    speed = 30,
    loop = true,
    snapshot_enabled = false,
    _timer = 0,
    _current_frame_idx = 0,
    is_done = false,
    init = function(self)
        Graphic.init(self)
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
    needsUpdate = function(self)
        return self.end_sprite >= 0 or self.lifetime ~= -1
    end,

    get_sprite_id = function(self)
        if self.end_sprite < 0 then
            return self.sprite
        end
        return self.sprite + self._current_frame_idx
    end,

    draw = function(self)
        local x = self.pos.x - self:getHalfWidth()
        local y = self.pos.y - self:getHalfHeight()
        self.layer.gfx:spr(self:get_sprite_id(), x, y, self.flip_x, self.flip_y)
        Graphic.draw(self)
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
})
return Sprite
