local Ecology = {
	Factions = {
		Player = "player",
		Wildlife = "wildlife",
	},
	Diets = {
		None = "none",
		Herbivore = "herbivore",
		Omnivore = "omnivore",
		Carnivore = "carnivore",
		Insectivore = "insectivore",
		Scavenger = "scavenger",
	},
	Temperaments = {
		Passive = "passive",
		Neutral = "neutral",
		Defensive = "defensive",
		Timid = "timid",
		Aggressive = "aggressive",
		Territorial = "territorial",
	},
	Actions = {
		Idle = "idle",
		Ignore = "ignore",
		Wander = "wander",
		Flee = "flee",
		Watch = "watch",
		Perch = "perch",
		Patrol = "patrol",
		Warn = "warn",
		Defend = "defend",
		Chase = "chase",
		SeekFood = "seek_food",
		Hunt = "hunt",
	},
}

Ecology.getDefaultActionForTemperament = function(temperament)
	if temperament == Ecology.Temperaments.Timid then
		return Ecology.Actions.Flee
	end
	if temperament == Ecology.Temperaments.Defensive then
		return Ecology.Actions.Defend
	end
	if temperament == Ecology.Temperaments.Aggressive then
		return Ecology.Actions.Chase
	end
	if temperament == Ecology.Temperaments.Territorial then
		return Ecology.Actions.Warn
	end
	if temperament == Ecology.Temperaments.Passive then
		return Ecology.Actions.Ignore
	end
	return Ecology.Actions.Watch
end

return Ecology
