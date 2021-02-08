--Colors
white = Color.new(255,255,255)
green = Color.new(0,255,0)
red = Color.new(255,0,0)
grey = Color.new(128, 128, 128)
black = Color.new(0,0,0)

--Pad
oldpad = Controls.read()

--Images
bg = Image.load("bg.png")
win = Image.load("win.png")
lose = Image.load("lose.png")
tie = Image.load("tie.png")

--Cars
yourcar = Image.load("Cars/"..cinfo.."/Car.png")
dofile("Cars/"..cinfo.."/Info.lua")

--Max Speed
maxspeed = {}
for y = 1, 6 do
	maxspeed[y] = ((redline * tcircumference) / (gearratio[y] * final * 88))
end

--Timer Variable
counter = Timer.new()
FPS_timer = Timer.new()

--Car Variables
mph = 0
gear = 1
pos = 0
vpos = 0

mphoverhaul = (440 * gearratio[gear] * final) / tcircumference
perfectshift = (gearratio[gear] * final * 88) / tcircumference

--comp, player has won
comp = ""
player = ""
blown_eng = ""

--ai variables
dofile("aivar.lua")

function redliner()
	rpm = {}
	rpm[gear] = ((mph * gearratio[gear] * final * 88) / tcircumference)
	if rpm[gear] < redline - mphoverhaul then color = grey
		if pad:r() and oldpad:r() ~= pad:r() and comp ~= "win" and player ~= "win" and gear ~= max_gear() then
			acceleration = acceleration + .2
			screen:print(90,210,"Bad Shift!",white)
		end
	elseif rpm[gear] > redline + mphoverhaul then color = red
	else color = green end
	if rpm[gear] > redline - perfectshift and rpm[gear] > redline - perfectshift then
		if pad:r() and oldpad:r() ~= pad:r() and comp ~= "win" and player ~= "win" and gear ~= max_gear() and acceleration > 0 then
			acceleration = acceleration - .2
			screen:print(90,210,"Perfect Shift!",white)
		end
	end
	screen:fillRect(90, 220, rpm[gear] / 100, 10, color)
	screen:print(92, 222, rpm[gear], black) 
	if color == red then blown_eng = player engine_blown() counter:stop() end
end

function change_gear()
	if pad:r() and oldpad:r() ~= pad:r() and comp ~= "win" and player ~= "win" then gear = gear + 1 end
	if pad:l() and oldpad:l() ~= pad:l() and comp ~= "win" and player ~= "win" then gear = gear - 1 end
	if gear < 1 then gear = 1 end
	if gear > max_gear() then gear = max_gear() end
end

function draw_screen()
	redliner()
	change_gear()
	screen:print(0,0,"Perfect Shifts:",white)
	for i = 1, max_gear()-1 do
		screen:print(0,i*10,"Gear "..i..": "..round(maxspeed[i]).." MPH",white)
	end
	screen:print(50,232,"MPH: " ..round(mph),white)
	screen:print(50,242,"Gear: " ..gear,white)
	screen:print(50,252,"Distance: " ..round(pos).. " px.",white)
	screen:print(335.5,232,"Computer: "..round(cmph).." MPH",white)
	screen:print(335.5,242,"Computer: "..cgear,white)
	screen:print(335.5,252,"Computer: "..round(cpos).. " px.",white)
end

function engine_blown()
	if blown_eng == player then
		for q = 7, 20 do
			screen:print(5,q*10,"Your Engine has blown!",white)
			comp = "win"
		end
	end
	if blown_eng == comp then
		for q = 7, 20 do
			screen:print(5,q*10,"Opponent's Engine has blown!",white)
			player = "win"
		end
	end
end

function max_gear()
	if gearratio[6] == 0 then
		return 5
	else return 6
	end
end

function cmax_gear()
	if cgearratio[6] == 0 then
		return 5
	else return 6
	end
end

function round(num)
	if (num-math.floor(num))<0.5 then
		return(math.floor(num))
	else
		return(math.ceil(num))
	end
end -- Thanks Magician FB!

function ai()
if comp ~= "win" and player ~= "win" then cmph = cmph + (3.2 / cacceleration) cpos = cpos + cmph cvpos = cvpos + cmph end
if cmph >= cmaxspeed[cmax_gear()] then cmph = cmaxspeed[cmax_gear()] end

crpm = {}
crpm[cgear] = ((cmph * cgearratio[cgear] * final * 88) / tcircumference)

math.randomseed(os.time())
rand = math.random(4)

if crpm[cgear] > credline - cmphoverhaul and crpm[cgear] < credline + cmphoverhaul then
	--shift in grey
		if rand == 1 then greyshift = true end

	--shift in red
		if rand == 2 then greyshift = true end

	--shift in perfectshift
		if rand == 3 then perfshift = true end

	--shift in green
		if rand == 4 then greenshift = true end

end

if greyshift == true then
	if comp ~= "win" and player ~= "win" and cgear ~= cmax_gear() then
		cacceleration = cacceleration + .2
		cgear = cgear + 1
		greyshift = false
	end
end

if perfshift == true then
	if comp ~= "win" and player ~= "win" and cgear ~= cmax_gear() and cacceleration > 0 then
		cacceleration = cacceleration - .2
		cgear = cgear + 1
		perfshift = false
	end
end

if greenshift == true then
	if comp ~= "win" and player ~= "win" and cgear ~= cmax_gear() then cgear = cgear + 1 greenshift = false end
end

if cpos >= 10000 then comp = "win" counter:stop() cpos = 10000 end
end

function main_init()
	screen:blit(0,0,bg)
	ai()
	pad = Controls.read()
	draw_screen()
end

function car_controls()
	--Accelerate, Decelerate, and Brake (NOS COMMING SOON!)
	if pad:cross() and comp ~= "win" and player ~= "win" then mph = mph + (3.2 / acceleration) pos = pos + mph vpos = vpos + mph end
	if not pad:cross() and mph > 0 and comp ~= "win" and player ~= "win" then mph = mph - (acceleration * .1) pos = pos + math.abs(mph) vpos = vpos + math.abs(mph) end
	if pad:triangle() and mph > 0  and comp ~= "win" and player~= "win" then mph = mph - (acceleration * .1) pos = pos + math.abs(mph) vpos = vpos + math.abs(mph) end
end

function redraw_cars()
	if vpos > 480 then vpos = 0 end
	screen:blit(vpos,160,yourcar)
	if cvpos > 480 then cvpos = 0 end
	screen:blit(cvpos,80,compcar)
end

function final_info()
	screen:print(201.5,262,"Time: " .. counter:time()/1000,white)
	screen:print(0,60,"FPS: "..math.floor(1000/FPS_timer:time()),white)
	if mph >= maxspeed[max_gear()] then mph = maxspeed[max_gear()]
		elseif mph <= 0 then mph = 0 end
	if pos >= 10000 then player = "win" counter:stop() pos = 10000 end
	if player == "win" then screen:blit(190,108,win)
	elseif comp == "win" then screen:blit(190,108,lose)
	elseif pos == 10000 and cpos == 10000 then screen:blit(190,108,tie) end
end

screen:clear()
screen:print(219,133.5,"Ready?",white)
screen.flip()
screen.waitVblankStart(100)
screen:clear()
screen:print(229.5,133.5,"Set",white)
screen.flip()
screen.waitVblankStart(100)
screen:clear()
screen:print(229.5,133.5,"Go!",white)
screen.flip()
screen.waitVblankStart(30)

while true do
	FPS_timer:reset()
	FPS_timer:start()
	if Controls.read():triangle() then screen:save("dm2.png") end
	main_init()
	car_controls()
	redraw_cars()
	final_info()
	screen.flip()
	screen.waitVblankStart()
	oldpad = pad
	if pad:start() then break end
end