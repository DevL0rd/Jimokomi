return function(ProceduralTextureRenderer)
	ProceduralTextureRenderer.fire = function(self, x, y, w, h, seed, time_offset, use_cache)
		time_offset = time_offset or 0

		local render_function = function(renderer, sx, sy, width, height, render_seed, render_time_offset)
			local current_time = time() + render_time_offset

			for i = 0, width - 1 do
				for j = 0, height - 1 do
					local height_factor = 1 - (j / height)
					local turbulence = renderer.noise:fractalNoise2d(
						(sx + i) * 0.05,
						(sy + j) * 0.05 + current_time,
						3,
						0.5,
						1,
						render_seed
					)
					local fire_val = (turbulence + height_factor) / 2
					local color = nil

					if fire_val > 0.8 then
						color = 7
					elseif fire_val > 0.6 then
						color = 10
					elseif fire_val > 0.3 then
						color = 9
					elseif fire_val > 0.1 then
						color = 8
					end

					if color then
						pset(sx + i, sy + j, color)
					end
				end
			end
		end

		self:renderTexture("fire", x, y, w, h, seed, use_cache, render_function, time_offset)
	end
end
