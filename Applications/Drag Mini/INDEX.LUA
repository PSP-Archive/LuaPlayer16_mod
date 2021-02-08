--Drag Mini, A minigame made by cools
selector = Image.load("Red_Bar.png")
main_menu = Image.load("race_select.png")
selected = 1
mode = ""
oldpad = Controls.read()

while true do
screen:blit(0,0,main_menu)

pad = Controls.read()
if pad:start() then break end
if pad:down() then selected = 2 end
if pad:up() then selected = 1 end

if selected == 1 then screen:blit(126,84,selector) screen:print(208.5,200,"Good Luck",Color.new(255,255,255)) if pad:cross() and oldpad:cross() ~= pad:cross() then mode = "engine" selector = nil main_menu = nil collectgarbage() dofile("car_select.lua") end end
if selected == 2 and Wlan.getSwitchState() == false then 
	screen:blit(126,114,selector)
	screen:print(140,200,"Turn on your wifi switch!",Color.new(255,255,255))
elseif selected == 2 and Wlan.getSwitchState() == true then
	screen:blit(126,114,selector)
	screen:print(172,200,"Good Luck Online!",Color.new(255,255,255))
	if pad:cross() and oldpad:cross() ~= pad:cross() then
		mode = "online"
		dofile("car_select.lua")
	end
end
screen.flip()
screen.waitVblankStart()
oldpad = pad
end
