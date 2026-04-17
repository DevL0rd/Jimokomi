local DebugOverlayLines = {}

DebugOverlayLines.appendLine = function(self, lines, text)
	add(lines, text)
end

DebugOverlayLines.drawLines = function(self, lines, x, y, color)
	color = color or 8
	for index = 1, #lines do
		print(lines[index], x, y + (index - 1) * 8, color)
	end
end

return DebugOverlayLines
