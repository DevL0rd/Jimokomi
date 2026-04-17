return function(ProceduralTextureRenderer)
	ProceduralTextureRenderer.water = function(self, x, y, w, h, seed, offset, use_cache)
		offset = (offset or 0) * 0.01

		local render_function = function(renderer, sx, sy, width, height, render_seed, render_offset)
			for i = 0, width - 1 do
				for j = 0, height - 1 do
					local wave1 = renderer.noise:smoothNoise2d((sx + i) * 0.02 + render_offset * 0.5, (sy + j) * 0.02, render_seed)
					local wave2 = renderer.noise:smoothNoise2d((sx + i) * 0.05, (sy + j) * 0.05 + render_offset * 0.3, render_seed + 100)
					local water_val = (wave1 + wave2) / 2
					local color = 5

					if water_val > 0.7 then
						color = 12
					elseif water_val > 0.4 then
						color = 1
					end

					pset(sx + i, sy + j, color)
				end
			end
		end

		self:renderTexture("water", x, y, w, h, seed, use_cache, render_function, offset)
	end
end
