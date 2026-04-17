return function(ProceduralTextureRenderer)
	ProceduralTextureRenderer.grass = function(self, x, y, w, h, seed, wind_strength)
		wind_strength = wind_strength or 0.5

		local screen_pos = self.camera:layerToScreen({ x = x, y = y })
		local sx = screen_pos.x
		local sy = screen_pos.y
		local current_time = time()

		for i = 0, w - 1 do
			for j = 0, h - 1 do
				local base_noise = self.noise:fractalNoise2d((sx + i) * 0.1, (sy + j) * 0.1, 3, 0.5, 1, seed)
				local wind_noise = self.noise:smoothNoise2d(
					(sx + i) * 0.02 + current_time * wind_strength,
					(sy + j) * 0.02,
					seed + 200
				)
				local grass_val = base_noise + wind_noise * 0.3
				local color = 19

				if grass_val > 0.7 then
					color = 11
				elseif grass_val > 0.3 then
					color = 3
				end

				pset(sx + i, sy + j, color)
			end
		end
	end
end
