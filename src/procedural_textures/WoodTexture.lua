return function(ProceduralTextureRenderer)
	ProceduralTextureRenderer.wood = function(self, x, y, w, h, seed, grain_scale, use_cache)
		grain_scale = grain_scale or 0.1

		local render_function = function(renderer, sx, sy, width, height, render_seed, render_grain_scale)
			for i = 0, width - 1 do
				for j = 0, height - 1 do
					local grain = sin(
						(sx + i) * render_grain_scale +
						renderer.noise:fractalNoise2d((sx + i) * 0.02, (sy + j) * 0.02, 2, 0.5, 1, render_seed) * 10
					)
					local noise_val = (grain + 1) / 2
					local color = noise_val > 0.6 and 4 or 15
					pset(sx + i, sy + j, color)
				end
			end
		end

		self:renderTexture("wood", x, y, w, h, seed, use_cache, render_function, grain_scale)
	end
end
