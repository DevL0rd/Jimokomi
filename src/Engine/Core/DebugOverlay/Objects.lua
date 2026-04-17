local DebugOverlayObjects = {}

DebugOverlayObjects.getObjectLine = function(self, object)
	if not object then
		return "object: none"
	end
	local attachment = object.getAttachment and object:getAttachment() or nil
	local slot_name = attachment and attachment.slot_name or "-"
	local overlaps = object.overlaps and #object.overlaps or 0
	local collisions = object.collisions and #object.collisions or 0
	return object._type ..
		" #" .. tostr(object.object_id or "?") ..
		" pos(" .. flr(object.pos.x) .. "," .. flr(object.pos.y) .. ")" ..
		" slot:" .. slot_name ..
		" c:" .. collisions ..
		" o:" .. overlaps
end

DebugOverlayObjects.collectLayerLines = function(self, layer)
	local lines = {}
	self:appendLine(lines, "layer " .. layer.layer_id .. " objects:" .. #layer.entities)
	self:appendLine(lines, "player: " .. (layer.player and layer.player._type or "none"))

	local inspect = {}
	if layer.player then
		add(inspect, layer.player)
	end
	for _, ent in pairs(layer.entities) do
		if ent ~= layer.player and #inspect < self.max_entities then
			add(inspect, ent)
		end
	end

	for _, ent in pairs(inspect) do
		self:appendLine(lines, self:getObjectLine(ent))
	end

	return lines
end

return DebugOverlayObjects
