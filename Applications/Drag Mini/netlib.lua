--[[
netlib v2.01
Copyright 2006 by youresam
Also thanks to PsPmad for help with netcall and netsms!


FUNCTIONS:
------------

netconnect()
netclose()

netsend(id, data, attribute)
netget(id)
netreg(id)
netunreg(id)
netrecv()
netvalue(id)

netmail(to,from,subject,message)
netcall(contact, destination)
netsms(to,message,from)

VARIABLES:
-----------
netlib_server
netlib_port

]]--

netlib_server = netlib_server or "66.25.53.65"
netlib_port   = netlib_port or 3388

local id_names = {}
local id_data  = {}
netlib_timer_timeout = Timer.new()
local amws = {}


function netconnect()
udpSocket = Socket.udpConnect(netlib_server,netlib_port)
netlib_timer_timeout:reset()
netlib_timer_timeout:start()
end

function netclose()
	if table.getn(amws) ~= 0 then
		for a=1,table.getn(amws) do
			udpSocket:udpSend("u:"..amws[a]..":")
		end
		amws = {}
	end
udpSocket:close()
netlib_timer_timeout:stop()
netlib_timer_timeout = nil
end


function replace(thestring,char1,char2)
lastfound = 1
while true do
hasfound = string.find(thestring,char1)
if hasfound == nil then break end
thestring = string.sub(thestring,1,hasfound-1)..char2..string.sub(thestring,hasfound+1)
lastfound = hasfound + 1
end
return thestring
end

function table.search(table1,value1)
if table.getn(table1) == 0 then return 0 end
for a=1,table.getn(table1) do
if table1[a] == value1 then return a end
end
end

function table.findandremove(table1,value1)
value2 = table.search(table1,value1)
if value2 == 0 then return table1 end
		table.remove(table1,value2)
		return table1
end

function table.replace(table1,value1,value2)
if table.getn(table1) == 0 then return table1,false end
for a=1,table.getn(table1) do
if table1[a] == value1 then table1[a] = value2; return table1,true end
end
return table1,false
end

local function reconnect(string1)
if (err == -1 and Wlan.getIPAddress()) == false then return end
	while err == -1 and Wlan.getIPAddress() do --reconnect
		udpSocket:close()
		udpSocket = Socket.udpConnect(netlib_server,netlib_port)
		err = udpSocket:udpSend(string1)
	end
	--send all AMWs
	if table.getn(amws) == 0 then return end
	for a=1,table.getn(amws) do
		udpSocket:udpSend("r:"..amws[a]..":")
	end

end

function netsend(id, data, attribute)
	if id == nil or data == nil then error("wrong arguments for netsend",2) end
	if id == "" then error("invalid id") end
	if not (attribute == "a" or attribute == "w") then
		error("bad attribute, must be 'w' or 'a'")
	end
err = udpSocket:udpSend(attribute..":"..id..":"..data)
		reconnect(attribute..":"..id..":"..data)
netlib_timer_timeout:reset()
netlib_timer_timeout:start()
end

function netmail(to,from,subject,message)
if to == nil or from == nil or subject == nil or message == nil then error("",2) end
message = replace(message,"\n","§")
message = replace(message,"\r","")
message = replace(message," ","%20")
message = replace(message,":","%3A")
subject = replace(subject," ","%20")
subject = replace(subject,":","%3A")
socket = Socket.connect("208.97.136.133", 80)
while not socket:isConnected() do 
if Controls.read():start() then return end
System.sleep(100) 
end
bytesSent = socket:send("GET http://www.psp-programming.com/youresam/net/mail.php?email_from=" .. from .. "&email_to=" .. to .. "&email_subject=" .. subject .. "&email_message=" .. message .. "&blah=toomany HTTP/1.0\r\n")
bytesSent = socket:send("host: www.psp-programming.com\r\n\r\n")
socket:close()
end


function netcall(contact, destination)
socket = Socket.connect("69.46.27.53", 80)
while not socket:isConnected() do System.sleep(100) end
bytesSent = socket:send("GET /call_phone2phone.php?txt_contact_phone2phone="..contact.."&txt_dest_phone2phone="..destination.." HTTP/1.0\r\n")
bytesSent = socket:send("host: www.searchitcallit.com\r\n\r\n")
socket:close()
end


function netsms(to,message,from)
if to == nil or message == nil then error("",2) end
from = from or "PSP"
if string.len(to) ~= 10 then error("cell number too short",2) end
message = replace(message,"\n"," ")
message = replace(message,"\r","")
message = replace(message," ","%20")
message = replace(message,":","%3A")
socket = Socket.connect("64.226.43.242", 80)
while not socket:isConnected() do 
if Controls.read():start() then return end
System.sleep(100) 
end
bytesSent = socket:send("GET http://www.textr.net/?recipients=" .. to .. "&message=" .. message .. "&reply=" .. from .. "&submit=Send%20SMS HTTP/1.0\r\n")
bytesSent = socket:send("host: www.textr.net\r\n\r\n")
socket:close()
end


function netget(id)
	if id == "" or id == nil then error("invalid id") end
err = udpSocket:udpSend("g:"..id..":")
		reconnect("g:"..id..":")
netlib_timer_timeout:reset()
netlib_timer_timeout:start()
end

--[[ not supported...
function netcheck(id)
if id == "" then error("no id") end
socket = Socket.connect("208.97.136.133", 80)
while not socket:isConnected() do System.sleep(100) end
bytesSent = socket:send("GET /youresam/net/read.php?id="..id.." HTTP/1.0\r\n")
bytesSent = socket:send("host: www.psp-programming.com\r\n\r\n")
total = ""
request = 0
while true do
	buffer = socket:recv()
		if string.len(buffer) > 0 then 
			total = total..buffer
			request = 1
		else
			if request == 1 then request = 2 end
		end
	
	if request == 2 then 
		socket:close()
		break
	end
screen.waitVblankStart(6)
end
if string.find(total,"READ ERROR") == nil then
return true
else
return false
end
end

--not supported either...
function netdelete(id)
if Wlan.getIPAddress() == nil then return end
if id == "" then error("invalid id") end
socket = Socket.connect("208.97.136.133", 80)
while not socket:isConnected() do System.sleep(100) end
bytesSent = socket:send("GET /youresam/net/delete.php?id="..id.." HTTP/1.0\r\n")
bytesSent = socket:send("host: www.psp-programming.com\r\n\r\n")
socket:close()
end
]]

function netreg(id)
	if id == "" or id == nil then error("invalid id") end
err = udpSocket:udpSend("r:"..id..":")
		reconnect("r:"..id..":")
table.insert(amws,id)
netlib_timer_timeout:reset()
netlib_timer_timeout:start()
end

function netunreg(id)
	if id == "" or id == nil then error("invalid id") end
if table.search(amws,id) == 0 then return end
	if id == "" then error("invalid id") end
err = udpSocket:udpSend("u:"..id..":")
		reconnect("u:"..id..":")
amws = table.findandremove(amws,id)
netlib_timer_timeout:reset()
netlib_timer_timeout:start()
end

local function search(thestring,tosearch)
local found = 0
for a=1,string.len(thestring) do
if string.sub(thestring,a,a) == tosearch then
found = found +1
end
end
return found
end


local function sortdata(data)
if search(data,":") == 0 then return nil,nil end
local find = string.find(data,":")
local string1 = string.sub(data,1,find-1)
data = string.sub(data,find+1)
return string1,data
end


function netrecv()
if netlib_timer_timeout:time() >= 5000 then --check to make sure socket is still open
	err = udpSocket:udpSend("z:z:z")
		reconnect("z:z:z")
	netlib_timer_timeout:reset()
	netlib_timer_timeout:start()
end

		buffer,err = udpSocket:recv()
			if err ~= -1 then
				buffer,buffer2 = sortdata(buffer)
					if buffer ~= nil then
						index_of_id = table.search(id_names,buffer)
						if index_of_id>0 then
							id_data[index_of_id] = buffer2
						elseif index_of_id == 0 then
							table.insert(id_names,buffer)
							table.insert(id_data,buffer2)
						end
					end
			end
end

function netvalue(id)
	if id == "" or id == nil then error("invalid id") end
	index_id = table.search(id_names,id)
	if index_id == 0 then return "" end
	data_to_return = id_data[index_id]
	table.remove(id_names,index_id)
	table.remove(id_data,index_id)
	return data_to_return
	
end