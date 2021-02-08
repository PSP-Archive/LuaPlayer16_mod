-- - - - - - - - - - - - - - - - - - - - - -
--The Basix
white = Color.new(255,255,255)
select_car = Image.load("car_select.png")
x = 0
y = 0
cont = 0
oldpad=Controls.read()

-- - - - - - - - - - - - - - - - - - - - - -
--Folder Stuff
selectstatus=3
Carfolder="default"

Car=System.listDirectory("Cars")
lastnumber=table.getn(Car)

for i=3,lastnumber do
	Car[i].main=Image.load("Cars/"..Car[i].name.."/Car.png")
end
dofile("Cars/"..Car[selectstatus].name.."/Info.lua")
-- - - - - - - - - - - - - - - - - - - - - -
--finds x for placing text in center
function find_x(letters)
	x = (480 / 2) - ((string.len(letters)*7) / 2)
end
-- - - - - - - - - - - - - - - - - - - - - -
--finds x for pic in center
function find_y(pic)
	y = (272 / 2) - (pic:height()/2)
end
-- - - - - - - - - - - - - - - - - - - - - -
--Returns # of max gears
function max_gear()
	if gearratio[6] == 0 then
		return 5
	else return 6
	end
end
-- - - - - - - - - - - - - - - - - - - - - -
--Rounds Numbers
function round(num)
	if (num-math.floor(num))<0.5 then
		return(math.floor(num))
	else
		return(math.ceil(num))
	end
end -- Thanks Magician FB!
-- - - - - - - - - - - - - - - - - - - - - -
while true do
	pad = Controls.read()

	if pad:right() then
		selectstatus = selectstatus - 1
		if selectstatus==2 then
			selectstatus=selectstatus+lastnumber-2
		end
		dofile("Cars/"..Car[selectstatus].name.."/Info.lua")
		screen.waitVblankStart(20)
	end

	if pad:left() then
		selectstatus = selectstatus + 1
		if selectstatus==lastnumber+1 then
			selectstatus=selectstatus-lastnumber+2
		end	
		dofile("Cars/"..Car[selectstatus].name.."/Info.lua")
		screen.waitVblankStart(20)
	end

	if selectstatus==2 then
		selectstatus=selectstatus+lastnumber-2
	end

	if selectstatus==lastnumber+1 then
		selectstatus=selectstatus-lastnumber+2
	end

	if pad:cross() and oldpad:cross() ~= pad:cross() then
		cinfo=Car[selectstatus].name
		if mode == "online" then
			Car = nil
			collectgarbage()
			break
		elseif mode == "engine" then
			Car = nil
			collectgarbage()
			break
		end
	end

	find_x(Car[selectstatus].name)
	find_y(Car[selectstatus].main)
	screen:blit(0,0,select_car)
	screen:blit(100, y, Car[selectstatus].main)
	screen:print(x, 44, Car[selectstatus].name,white)
	screen:print(248,90,"Price: $"..price,white)
	screen:print(255,100,"0-60: "..acceleration.. " sec",white)
	screen:print(255,110,"Redline: "..redline,white)
	screen:print(240,120,"Top Speed: "..round((redline * tcircumference) / (gearratio[max_gear()] * final * 88)).." mph",white)
	screen:print(260,130,"Transmission",white)
	for i = 1, 6 do
		screen:print(235,(13+i)*10,"Gear Ratio "..i.. ": "..gearratio[i],white)
	end
	screen:print(218,200,"Final Drive Ratio: " ..final,white)
	screen:print(200,220,"Tire Circumference: "..tcircumference.." ft.",white)
	screen.flip()
	if pad:start() and oldpad:start() ~= pad:start() then cont = 1 break end
	oldpad = pad
end
if cont == 0 then
	if mode == "online" then
		dofile(mode..".lua")
	elseif mode == "engine" then
		dofile("ccar_select.lua")
	end
end
