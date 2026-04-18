local SpatialIndex = {}

local function clear_array(values)
	if not values then
		return {}
	end
	for i = #values, 1, -1 do
		values[i] = nil
	end
	return values
end

local function get_bucket(self, cell_x, cell_y)
	local row = self.bucket_rows[cell_y]
	if not row then
		row = {}
		self.bucket_rows[cell_y] = row
	end
	local bucket = row[cell_x]
	if not bucket then
		bucket = {
			stamp = 0,
			cell_x = cell_x,
			cell_y = cell_y,
		}
		row[cell_x] = bucket
	end
	return bucket
end

local function begin_rebuild(self)
	self.build_stamp = (self.build_stamp or 0) + 1
	self.entity_count = 0
	clear_array(self.active_buckets)
end

local function get_item_bounds(self, item)
	if not item then
		return nil
	end
	if item.getSpatialBounds then
		return item:getSpatialBounds()
	end
	local pos = item.pos or item
	if not pos or pos.x == nil or pos.y == nil then
		return nil
	end
	if item.isCircleShape and item:isCircleShape() then
		local radius = item.getRadius and item:getRadius() or 0
		return pos.x - radius, pos.y - radius, pos.x + radius, pos.y + radius
	end
	local half_width = item.getHalfWidth and item:getHalfWidth() or 0
	local half_height = item.getHalfHeight and item:getHalfHeight() or 0
	return pos.x - half_width, pos.y - half_height, pos.x + half_width, pos.y + half_height
end

SpatialIndex.new = function(config)
	local self = config or {}
	self.cell_size = self.cell_size or 32
	self.bucket_rows = self.bucket_rows or {}
	self.active_buckets = self.active_buckets or {}
	self.query_marks = self.query_marks or {}
	self.build_stamp = self.build_stamp or 0
	self.query_stamp = self.query_stamp or 0
	self.entity_count = self.entity_count or 0
	return setmetatable(self, { __index = SpatialIndex })
end

SpatialIndex.rebuild = function(self, items, filter_fn)
	begin_rebuild(self)
	if not items then
		return 0
	end
	local cell_size = self.cell_size or 32
	for i = 1, #items do
		local item = items[i]
		if item and (filter_fn == nil or filter_fn(item) == true) then
			local left, top, right, bottom = get_item_bounds(self, item)
			if left ~= nil then
				local start_x = flr(left / cell_size)
				local end_x = flr(right / cell_size)
				local start_y = flr(top / cell_size)
				local end_y = flr(bottom / cell_size)
				for cell_y = start_y, end_y do
					for cell_x = start_x, end_x do
						local bucket = get_bucket(self, cell_x, cell_y)
						if bucket.stamp ~= self.build_stamp then
							clear_array(bucket)
							bucket.stamp = self.build_stamp
							add(self.active_buckets, bucket)
						end
						add(bucket, item)
					end
				end
				self.entity_count += 1
			end
		end
	end
	return self.entity_count
end

SpatialIndex.queryRect = function(self, left, top, right, bottom, results, filter_fn)
	results = results or {}
	local cell_size = self.cell_size or 32
	local start_x = flr(left / cell_size)
	local end_x = flr(right / cell_size)
	local start_y = flr(top / cell_size)
	local end_y = flr(bottom / cell_size)
	self.query_stamp = (self.query_stamp or 0) + 1
	local query_stamp = self.query_stamp
	local marks = self.query_marks

	for cell_y = start_y, end_y do
		local row = self.bucket_rows[cell_y]
		if row then
			for cell_x = start_x, end_x do
				local bucket = row[cell_x]
				if bucket and bucket.stamp == self.build_stamp then
					for i = 1, #bucket do
						local item = bucket[i]
						if marks[item] ~= query_stamp then
							marks[item] = query_stamp
							if filter_fn == nil or filter_fn(item) == true then
								add(results, item)
							end
						end
					end
				end
			end
		end
	end

	return results
end

SpatialIndex.queryRadius = function(self, x, y, radius, results, filter_fn)
	local radius_sq = radius * radius
	return self:queryRect(x - radius, y - radius, x + radius, y + radius, results, function(item)
		local pos = item and item.pos or item
		if not pos or pos.x == nil or pos.y == nil then
			return false
		end
		local dx = pos.x - x
		local dy = pos.y - y
		if dx * dx + dy * dy > radius_sq then
			return false
		end
		return filter_fn == nil or filter_fn(item) == true
	end)
end

return SpatialIndex
