white = Color.new(255,255,255)
bg = Image.load("NEOFLASH.PNG")
local sc=1

local square = {
{1,1,white,1,-1,0},
{0,0,white,0,0,0},
{1,0,white,1,0,0},

{0,0,white,0,0,0},
{1,1,white,1,-1,0},
{0,1,white,0,-1,0},
}

while true do
	local hprm = Hprm.read()
	screen:clear()
	local pad = Controls.read()
	
	if not Hprm.remote() then error("Remote needed to work correctly") end

	Gu.start3d()
	Gu.clearDepth(0)
	Gu.clear(Gu.COLOR_BUFFER_BIT+Gu.DEPTH_BUFFER_BIT)
	Gum.matrixMode(Gu.PROJECTION)
	Gum.loadIdentity()
	Gum.perspective(50, 16/9, .5, 1000)
	Gum.matrixMode(Gu.VIEW)
	Gum.loadIdentity()
	Gu.enable(Gu.BLEND)
	Gu.blendFunc(Gu.ADD, Gu.SRC_ALPHA, Gu.ONE_MINUS_SRC_ALPHA, 0, 0)
	Gu.enable(Gu.TEXTURE_2D)
	Gu.texImage(bg)
	Gu.texFunc(Gu.TFX_MODULATE, Gu.TCC_RGBA)
	Gu.texEnvColor(white)
	Gu.texFilter(Gu.LINEAR, Gu.LINEAR)
	Gu.texScale(1, 1)
	Gu.texOffset(0, 0)
	Gu.ambientColor(white)
	Gum.matrixMode(Gu.MODEL)
	Gum.loadIdentity()
	x = sc/-2
	y = sc/2
	Gum.translate(x,y,-1)
	Gum.scale(sc,sc,1)
	Gum.drawArray(Gu.TRIANGLES, Gu.TEXTURE_32BITF+Gu.COLOR_8888+Gu.VERTEX_32BITF+Gu.TRANSFORM_3D, square)
	Gu.end3d()

	screen:print(0,0,"Press +/- or forward/back on remote to zoom in on picture",white)
	screen:print(0,10,"Press Play/Pause or start to exit",white)

	if hprm:volup() or hprm:forward() then sc = sc + .02 end
	if hprm:back() or hprm:voldown() then sc = sc - .02 end

	if pad:start() or pad:cross() or hprm:playpause() then break end
	screen.flip()
	screen.waitVblankStart()

end

screen:clear()
bg = nil
collectgarbage()