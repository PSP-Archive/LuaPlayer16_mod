screen:clear()
function Gu.drawLine(sX, sY, fX, fY, sColor, fColor)
	local line = {
	{sColor,sX,sY,0},
	{fColor or sColor,fX,fY,0},
	}
	Gu.start3d()
	Gu.disable(Gu.TEXTURE_2D)
	Gum.drawArray(Gu.LINES, Gu.COLOR_8888+Gu.VERTEX_32BITF+Gu.TRANSFORM_2D, line)
	Gu.end3d()
end
green = Color.new(0, 255, 0)
red = Color.new(0,0,255)

screen:print(200, 100, "Hello World!", green)

for i=0,20 do
	x0 = i/20*479
	y1 = 271-i/20*271
	Gu.drawLine(x0, 271, 479, y1, red, green)
end

screen.flip()
while true do
	if Controls.read():start() then break end
	screen.waitVblankStart()
end
