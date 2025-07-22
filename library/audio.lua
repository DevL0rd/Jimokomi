---@meta

---Play sfx n (0..63) on channel (0..15) from note offset (0..63 in notes) for length notes.
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#sfx)
---@param n number -- 0~63
---@param channel number? -- 0~15
---@param offset number? -- 0~63
---@param length number?
function sfx(n, channel, offset, length) end

---Play music starting from pattern n. -1 to stop music
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#music)
---@param n number
---@param fade_len number? default 0, in ms
---@param channel_mask number?
function music(n, fade_len, channel_mask) end

---This provides low level control over the state of a channel. It is useful in more niche situations, like audio authoring tools and size-coding.
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#note)
---@param pitch number? -- channel pitch (default 48 -- middle C)
---@param inst number? -- instrument index (default 0)
---@param vol number? -- channel volume (default 64)
---@param effect number? -- channel effect (default 0)
---@param effect_p number?  -- effect parameter (default 0)
---@param channel number? -- channel index (0..15 -- default 0)
---@param retrig number? -- (boolean) force retrigger -- default to false
---@param panning number? -- set channel panning (-128..127)
function note(pitch, inst, vol, effect, effect_p, channel, retrig, panning) end
