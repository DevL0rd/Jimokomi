return {
	visual_definitions = {
		run_idle = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 64,
		},
		run_move = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 65,
			end_sprite = 69,
			speed = 16,
		},
		glide = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 70,
			end_sprite = 71,
			speed = 6,
		},
		climb_vertical_idle = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 72,
		},
		climb_vertical_move = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 73,
			end_sprite = 74,
			speed = 12,
		},
		climb_horizontal_idle = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 80,
		},
		climb_horizontal_move = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 81,
			end_sprite = 82,
			speed = 12,
		},
	},
}
