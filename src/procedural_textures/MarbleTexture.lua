return function(ProceduralTextureRenderer)
	ProceduralTextureRenderer.marble = function(self, x, y, w, h, seed, scale, use_cache)
		scale = scale or 0.03

		local render_function = function(renderer, sx, sy, width, height, render_seed, render_scale)
			for i = 0, width - 1 do
				for j = 0, height - 1 do
					local marble_val = renderer.noise:warpedNoise2d((sx + i) * render_scale, (sy + j) * render_scale, 8, render_seed)
					marble_val = sin(marble_val * 10) * 0.5 + 0.5
					local color = 13

					if marble_val > 0.8 then
						color = 7
					elseif marble_val > 0.6 then
						color = 6
					elseif marble_val > 0.3 then
						color = 5
					end

					pset(sx + i, sy + j, color)
				end
			end
		end

		self:renderTexture("marble", x, y, w, h, seed, use_cache, render_function, scale)
	end
end
