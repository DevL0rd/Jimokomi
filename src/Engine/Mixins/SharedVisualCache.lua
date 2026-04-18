local SharedVisualCache = {}

SharedVisualCache.resolveRotationBucketCount = function(self, angle, bucket_count)
	local resolved_angle = abs(angle or 0)
	if resolved_angle <= 0.0001 and self.force_rotation_cache ~= true then
		return 1
	end
	return max(1, flr(bucket_count or 1))
end

SharedVisualCache.resolveEntryTtlMs = function(self, ttl_ms, fallback_ms)
	return max(0, flr(ttl_ms or fallback_ms or 0))
end

SharedVisualCache.resolveRotationEntryTtlMs = function(self, rotation_ttl_ms, ttl_ms, fallback_ms)
	return max(0, flr(rotation_ttl_ms or ttl_ms or fallback_ms or 0))
end

return SharedVisualCache
