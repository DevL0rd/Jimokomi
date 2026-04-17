local cart_path = "/desktop/Jimokomi_Debug.p64"
local disable_flag = "/appdata/system/disable_jimokomi_autostart.txt"

if fstat(disable_flag) == "file" then
	return
end

create_process("/system/util/open.lua", {
	argv = { cart_path },
	pwd = "/desktop",
})
