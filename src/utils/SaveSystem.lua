local SaveSystem = {
	registry = {},

	register_class = function(self, class_obj)
		if class_obj and class_obj._type then
			self.registry[class_obj._type] = class_obj
		end
	end,

	register_classes = function(self, classes)
		for _, class_obj in pairs(classes or {}) do
			self:register_class(class_obj)
		end
	end,

	save_snapshot = function(self, snapshot, filename, snapshot_type)
		if not snapshot then
			return false, "Snapshot is nil"
		end

		store(filename, {
			version = "2.0",
			timestamp = time(),
			snapshot_type = snapshot_type or "snapshot",
			snapshot = snapshot,
		})

		return true, "Saved successfully"
	end,

	load_snapshot = function(self, filename)
		local save_data = fetch(filename)
		if not save_data or not save_data.snapshot then
			return nil, "Snapshot not found"
		end
		return save_data.snapshot, "Loaded successfully"
	end,

	save_object = function(self, obj, filename)
		if not obj or not obj.toSnapshot then
			return false, "Object does not support snapshots"
		end
		return self:save_snapshot(obj:toSnapshot(), filename, obj._type or "object")
	end,

	load_object = function(self, filename)
		local snapshot, err = self:load_snapshot(filename)
		if not snapshot then
			return nil, err
		end
		local class_obj = self.registry[snapshot._type]
		if not class_obj then
			return nil, "Unregistered type: " .. tostr(snapshot._type)
		end
		local obj = class_obj:new()
		if obj.applySnapshot then
			obj:applySnapshot(snapshot)
		end
		return obj, "Loaded successfully"
	end,

	save_engine = function(self, engine, filename)
		if not engine or not engine.toSnapshot then
			return false, "Engine does not support snapshots"
		end
		return self:save_snapshot(engine:toSnapshot(), filename or "engine_snapshot.dat", "engine")
	end,

	load_engine = function(self, engine, filename)
		if not engine or not engine.applySnapshot then
			return false, "Engine does not support snapshot loading"
		end
		local snapshot, err = self:load_snapshot(filename or "engine_snapshot.dat")
		if not snapshot then
			return false, err
		end
		engine:applySnapshot(snapshot, self.registry)
		return true, "Loaded successfully"
	end,

	save_exists = function(self, filename)
		return fetch(filename) ~= nil
	end,

	get_save_info = function(self, filename)
		local save_data = fetch(filename)
		if not save_data then
			return nil, "File not found"
		end
		return {
			version = save_data.version or "unknown",
			timestamp = save_data.timestamp or 0,
			snapshot_type = save_data.snapshot_type or "snapshot",
			has_snapshot = save_data.snapshot ~= nil,
		}
	end,
}

return SaveSystem
