return function(ProceduralTextureRenderer)
	ProceduralTextureRenderer.clouds = function(self, x, y, w, h, seed, density, use_cache)
		density = density or 0.02

		local render_function = function(renderer, sx, sy, width, height, render_seed, render_density)
			for i = 0, width - 1 do
				for j = 0, height - 1 do
					local noise_val = renderer.noise:fractalNoise2d(
						(sx + i) * render_density,
						(sy + j) * render_density,
						4,
						0.5,
						1,
						render_seed
					)

					if noise_val > 0.6 then
						pset(sx + i, sy + j, 7)
					elseif noise_val > 0.4 then
						pset(sx + i, sy + j, 6)
					end
				end
			end
		end

		self:renderTexture("clouds", x, y, w, h, seed, use_cache, render_function, density)
	end
end
