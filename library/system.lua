---@meta

---print a string to the host operating system's console for debugging.
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#printh)
---@param str any
function printh(str) end

---Returns a table of environment variables given to the process at the time of creation.
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#env)
---@return table
function env() end

---stop the cart and optionally print a message
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#stop)
---@param message any?
function stop(message) end

---if condition is false, stop the program and print message if it is given. this can be useful for debugging cartridges, by assert()'ing that things that you expect to be true are indeed true.
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#assert)
---@param condition any
---@param message any?
function assert(condition, message) end

---Returns the number of seconds elapsed since the cartridge was run.
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#time)
---@return number
function time() end

---Returns the number of seconds elapsed since the cartridge was run.
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#time)
---@return number
function t() end

---Returns the current day and time formatted using Lua's standard date strings.
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#date)
---@param format string -- specifies the output string format, and defaults to "!%Y-%m-%d %H:%M:%S" (UTC) when not given.
---@param t string -- specifies the moment in time to be encoded as a string
---@param delta number --  number of seconds to add to t.
---@return string
function date(format, t, delta) end

---Read the contents of the clipboard. The value is always a single string;
---
---to copy structured objects to the clipboard, use pod() and unpod().
---
---For security reasons, get_clipboard() only has access to the host clipboard after ctrl-v is pressed while Picotron is active. Until ctrl-v is pressed, changes to the host clipboard have no effect on the return value of get_clipboard(). The same is true for sandboxed applications (e.g. bbs carts): they are only able to access clipboard contents from other processes once ctrl-v is pressed while that app has keyboard focus.
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#get_clipboard)
---@return string
function get_clipboard() end

---Write the contents of the clipboard. The value is always a single string;
---
------to copy structured objects to the clipboard, use pod() and unpod().
---
---[View Online](https://www.lexaloffle.com/dl/docs/picotron_manual.html#set_clipboard)
---@param text string
function set_clipboard(text) end
