local Class = include("src/classes/Class.lua")
local GameData = include("src/data/GameData.lua")

local function deep_copy(value)
	if type(value) ~= "table" then
		return value
	end
	local copy = {}
	for key, nested in pairs(value) do
		copy[key] = deep_copy(nested)
	end
	return copy
end

local function get_leaf_key(key)
	if not key then
		return nil
	end
	local last_colon = nil
	for i = 1, #key do
		if sub(key, i, i) == ":" then
			last_colon = i
		end
	end
	if last_colon then
		return sub(key, last_colon + 1)
	end
	return key
end

local function resolve_key(section, key)
	if not key then
		return nil
	end
	if section[key] ~= nil then
		return key
	end
	return get_leaf_key(key)
end

local function get_section_value(section, key)
	local resolved_key = resolve_key(section, key)
	if not resolved_key then
		return nil
	end
	return deep_copy(section[resolved_key])
end

local AssetRegistry = Class:new({
	_type = "AssetRegistry",
	visuals = nil,
	entity_config = nil,
	world_config = nil,

	init = function(self)
		self.visuals = deep_copy(GameData.visuals or {})
		self.entity_config = deep_copy(GameData.entity_config or {})
		self.world_config = deep_copy(GameData.world or {})
	end,

	getVisualSet = function(self, key)
		return get_section_value(self.visuals, key)
	end,

	getVisual = function(self, key, state)
		if not key or not state then
			return nil
		end
		local visual_set = get_section_value(self.visuals, key)
		if not visual_set then
			return nil
		end
		return deep_copy(visual_set[state])
	end,

	getEntityConfig = function(self, key)
		return get_section_value(self.entity_config, key)
	end,

	applyEntityConfig = function(self, entity, key)
		local config = self:getEntityConfig(key or entity._type)
		if not config then
			return nil
		end
		for field, value in pairs(config) do
			if entity[field] == nil then
				entity[field] = value
			end
		end
		return config
	end,

	getWorldConfig = function(self)
		return deep_copy(self.world_config)
	end,
})

return AssetRegistry
