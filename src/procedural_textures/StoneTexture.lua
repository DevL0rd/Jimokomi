return function(ProceduralTextureRenderer)
	ProceduralTextureRenderer.stone = function(self, x, y, w, h, seed, scale, use_cache)
		scale = scale or 0.05

		local render_function = function(renderer, sx, sy, width, height, render_seed, render_scale)
			for i = 0, width - 1 do
				for j = 0, height - 1 do
					local noise_val = renderer.noise:fractalNoise2d((sx + i) * render_scale, (sy + j) * render_scale, 3, 0.6, 1, render_seed)
					local color = 13

					if noise_val > 0.7 then
						color = 6
					elseif noise_val > 0.4 then
						color = 5
					end

					pset(sx + i, sy + j, color)
				end
			end
		end

		self:renderTexture("stone", x, y, w, h, seed, use_cache, render_function, scale)
	end
end
