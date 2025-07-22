---@meta

---Returns a binary string encoding val.
---
---flags determine the encoding format (default: 0x0)
---
---metadata is an optional value that is encoded into the string and stores additional information about the pod.
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#pod)
---@param val any
---@param flags any?
---@param metadata any?
---@return string
function pod(val, flags, metadata) end

---returns the decoded value, and the decoded metadata as a second result.
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#unpod)
---@param str string
---@return any,metadata
function unpod(str) end
