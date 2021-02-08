file = io.open("./Cars/"..ccinfo.."/Info.lua", "r")
compcar = Image.load("Cars/"..ccinfo.."/Car.png")

--Lines refers to number of lines to read
function readFile(file, lines)
	for i = 1, lines do
		file:read()
	end
end

--Reads after 2 after "="
function subLine(line)
	return string.sub(line, string.find(line,"=")+2)
end
readFile(file,5)

--Always Same for Every Car
cmph = 0
cgear = 1
cpos = 0
cvpos = 0
cgearratio = {}

--Car Info
ourline = file:read()
cacceleration = subLine(ourline)

ourline = file:read()
credline = subLine(ourline)

ourline = file:read()
cfinal = subLine(ourline)

--Gets gear ratio's
ourline = file:read()
cgearratio[1] = subLine(ourline)

ourline = file:read()
cgearratio[2] = subLine(ourline)

ourline = file:read()
cgearratio[3] = subLine(ourline)

ourline = file:read()
cgearratio[4] = subLine(ourline)

ourline = file:read()
cgearratio[5] = subLine(ourline)

ourline = file:read()
cgearratio[6] = subLine(ourline)

--Gets Tire Circumfrence
ourline = file:read()
ctcircumference = subLine(ourline)

--Gets Max Speed/How high gear can go/etc.
cmaxspeed = {}
cmphoverhaul = (440 * cgearratio[cgear] * cfinal) / ctcircumference
cperfectshift = (cgearratio[cgear] * cfinal * 88) / ctcircumference
for u = 1, 6 do
	cmaxspeed[u] = ((credline * ctcircumference) / (cgearratio[u] * cfinal * 88))
end
cacceleration = cacceleration + 0
