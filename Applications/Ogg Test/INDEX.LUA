Ogg.load("REALTEST.OGG")
isplaying = "false"
nickn = System.nickName()
while true do
pad = Controls.read()

osec = Ogg.getSec()
omin = Ogg.getMin()
ohour = Ogg.getHour()

if osec < 10 then osec = "0"..Ogg.getSec() end
if omin < 10 then omin = "0"..Ogg.getMin() end
if ohour < 10 then ohour = "0"..Ogg.getHour() end

screen:clear()
screen:print(0,0,"Ogg Test",Color.new(255,255,255))
screen:print(0,10,"Press X to Play",Color.new(255,255,255))
screen:print(0,20,"Press O to Pause",Color.new(255,255,255))
screen:print(0,30,"Press Start to Exit",Color.new(255,255,255))
screen:print(0,50,"Nickname: "..nickn, Color.new(255,255,255))

if pad:cross() then Ogg.play() isplaying = "true" end

if isplaying == "true" then
screen:print(0,40,"This song has been playing for: "..ohour..":"..omin..":"..osec,Color.new(255,255,255))
end

--Sample Code on How to use, wont work in this situation

if isplaying == "true" and Ogg.EndOfStream() ~= "false" then
isplaying = "false"
Ogg.stop()
end

if pad:circle() then Ogg.pause() end

if Controls.read():start() then Ogg.stop() break end
screen.flip()
screen.waitVblankStart()
end