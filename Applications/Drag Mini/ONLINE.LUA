screen:clear()
screen:print(0,0,"Connecting...",Color.new(255,255,255))
screen.flip()
dofile("netlib.lua")
netlib_server = "66.25.53.65"
Wlan.init()
Wlan.useConnectionConfig(1)
System.sleep(50)
screen.flip()
screen:print(0,10,"Connecting to netlib...",Color.new(255,255,255))
screen.flip()
netconnect()
System.sleep(50)
screen.flip()
screen:print(0,20,"Getting room data...",Color.new(255,255,255))
screen.flip()
netget("Drag_Mini_room")
System.sleep(50)
Drag_Mini_timer = Timer.new()
Drag_Mini_timer:start()
while Wlan.getIPAddress() do
netrecv()
roomData = netvalue("Drag_Mini_room")
if roomData ~= "" then break end
if Drag_Mini_timer:time() >= 4000 then --resend request
	netget("Drag_Mini_room")
	Drag_Mini_timer:reset()
	Drag_Mini_timer:start()
end
screen.waitVblankStart()
end

if roomData == "error_missing_id" then --server has never had a Drag_Mini connection, send variables
	roomData = "empty"
	System.sleep(50)
	netsend("Drag_Mini_number","1","w")
	System.sleep(50)
end

screen.flip()
screen:print(0,20,"Getting room data...done, value: "..roomData,Color.new(255,255,255))
screen.flip()


if roomData == "empty" then
player_ = 1
--wait for someone to join
netreg("Drag_Mini_room")
System.sleep(50)
netsend("Drag_Mini_room",cinfo,"w")
	while Wlan.getIPAddress() do
		if Controls.read():start() then --will cut connection from netlib
			System.sleep(50)
			netsend("Drag_Mini_room","empty","w")
			System.sleep(50)
			netunreg("Drag_Mini_room")
			System.sleep(50)
			netclose()
			Wlan.term()
			return
		end
		netrecv()
		buffer = netvalue("Drag_Mini_room")
		if buffer ~= "" then --Assume that someone has joined!
		screen.flip()
		screen:print(0,30,buffer.." has joined!",Color.new(255,255,255))
		screen.flip()
			opon = buffer
			netunreg("Drag_Mini_room") --no longer need that
			System.sleep(50)
			netget("Drag_Mini_number")
			screen.flip()
			screen:print(0,40,"Getting room number...",Color.new(255,255,255))
			screen.flip()
				while Wlan.getIPAddress() do -- get the current room number
					netrecv()
					room_number = netvalue("Drag_Mini_number")
					if room_number ~= "" then break end --done, lets play!
					screen.waitVblankStart()
				end
			System.sleep(700)
			break
		end
	screen.waitVblankStart()
	end
else --someone is waiting for another player
player_ = 2
	opon = roomData
		screen.flip()
		screen:print(0,30,"Room has player, sending data.",Color.new(255,255,255))
		screen.flip()
	System.sleep(50)
	netsend("Drag_Mini_room",cinfo,"w")
	System.sleep(50)
	netunreg("Drag_Mini_room")
	System.sleep(50)
	netget("Drag_Mini_number")
			screen.flip()
			screen:print(0,40,"Getting room number...",Color.new(255,255,255))
			screen.flip()
				while Wlan.getIPAddress() do -- get the current room number
					netrecv()
					room_number = netvalue("Drag_Mini_number")
					if room_number ~= "" then break end --done, lets play!
					screen.waitVblankStart()
				end
			System.sleep(500) --wait for it...
			netsend("Drag_Mini_room","empty","w")
			System.sleep(50)  --wait for it...
			netsend("Drag_Mini_number",room_number + 1,"w") --BAM! Add 1 to the current room number
end

			System.sleep(50) --------------(
			netreg("Drag_Mini_room"..room_number)
			System.sleep(50)
			netreg("Drag_Mini_room"..room_number)
			System.sleep(50)
			netreg("Drag_Mini_room"..room_number) --)

--at this point, there should be 2 players in a room, "Drag_Mini_room"..room_number
--if player_ is 1, you are player variable
--if player_ is 2, you are ai variable
if not Wlan.getIPAddress() then
screen.flip()
screen:print(0,50,"WLAN not connected, press START to return.",Color.new(255,0,0))
screen.flip()
while not Controls.read():start() do screen.waitVblankStart() end
return
end


function explode(text_ex, needle) --thanks to LuMo.
	local array={}
	_, count = string.gsub(text_ex, needle, needle)
	for index=1,count+1 do
		i, j = string.find(text_ex, needle)
		if i == nil or j==nil then
			i=0
			j=0
		end
	  table.insert(array,string.sub(text_ex, 1, i-1))
		text_ex=string.sub(text_ex, j+1 )
  end
  return array
end

screen:clear()

--Colors
white = Color.new(255,255,255)
green = Color.new(0,255,0)
red = Color.new(255,0,0)
grey = Color.new(128, 128, 128)
black = Color.new(0,0,0)

--Variables
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

mph = 0
gear = 1
pos = 0
vpos = 0
mphoverhaul = (440 * gearratio[gear] * final) / tcircumference
perfectshift = (gearratio[gear] * final * 88) / tcircumference
comp = ""
player = ""
blown_eng = ""

--Opponent (AI Vars)
compcar = Image.load("Cars/"..opon.."/Car.png")
cmph = 0
cgear = 1
cpos = 0
cvpos = 0

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

function round(num)
	if (num-math.floor(num))<0.5 then
		return(math.floor(num))
	else
		return(math.ceil(num))
	end
end --Thanks Magician FB!

function main_init()
	screen:blit(0,0,bg)
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
	if cpos >= 10000 then comp = "win" counter:stop() cpos = 10000 end
	if player == "win" then screen:blit(190,108,win)
	elseif comp == "win" then screen:blit(190,108,lose)
	elseif pos == 10000 and cpos == 10000 then screen:blit(190,108,tie) end
end
--------------------------------------
--  Main Loop
--------------------------------------
while true do

if not Wlan.getIPAddress() then
screen.flip()
screen:print(0,50,"WLAN not connected, press START to return.",Color.new(255,0,0))
screen.flip()
while not Controls.read():start() do screen.waitVblankStart() end
return
end

netrecv()
buffer = netvalue("Drag_Mini_room"..room_number)
buffer2 = {}
if buffer ~= "" then
	buffer2 = explode(buffer,":")
	if table.getn(buffer2) == 4 then
		cmph = tonumber(buffer2[1])
		cgear = tonumber(buffer2[2])
		cpos = tonumber(buffer2[3])
		cvpos = tonumber(buffer2[4])
	end
end

netsend("Drag_Mini_room"..room_number,mph..":"..gear..":"..pos..":"..vpos,"w")
if pad:start() then break end

	FPS_timer:reset()
	FPS_timer:start()
	main_init()
	car_controls()
	redraw_cars()
	final_info()
	oldpad=pad
	screen.flip()
	screen.waitVblankStart()
end

for _ = 1,3 do --make sure it gets processed.
System.sleep(50)
netunreg("Drag_Mini_room"..room_number)
end
System.sleep(50)
netclose()
System.sleep(50)
Wlan.term()
