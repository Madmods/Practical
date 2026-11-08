local Enum={KeyCode={LeftShift="Shift"}}
 local function passGesture(style)
  return {shift=style=="Chest"or style=="Spin",taps=(style=="Bounce"or style=="Spin")and 2 or 1}
 end
 -- Run only the native input sequence; no pass payload or receiver is fabricated.
 local function emitGesture(key,style,send,wait,cancelled)
  local gesture=passGesture(style)
  local modifier=false;local down=false
  local ok,err=pcall(function()
   if cancelled()then return end
   if gesture.shift then send(true,Enum.KeyCode.LeftShift);modifier=true end
   for i=1,gesture.taps do
    if cancelled()then break end
    send(true,key);down=true;wait(.025)
    send(false,key);down=false
    if i<gesture.taps then wait(.055)end
   end
  end)
  -- Always release our synthetic keys, including cancellation and input failures.
  if down then pcall(send,false,key)end
  if modifier then pcall(send,false,Enum.KeyCode.LeftShift)end
  return ok,err
 end

local function sequence(style)
 local events={};local waits={}
 local ok=emitGesture("2",style,function(down,key)events[#events+1]=(down and "+"or "-")..key end,function(seconds)waits[#waits+1]=seconds end,function()return false end)
 assert(ok)
 return table.concat(events,","),#waits
end
assert(sequence("Overhead")=="+2,-2")
assert(sequence("Chest")=="+Shift,+2,-2,-Shift")
assert(sequence("Bounce")=="+2,-2,+2,-2")
assert(sequence("Spin")=="+Shift,+2,-2,+2,-2,-Shift")
local sent=0
assert(emitGesture("2","Spin",function()sent+=1 end,function()end,function()return true end))
assert(sent==0,"Cancel before dispatch sends no keys")
local events={};local cancel=false
assert(emitGesture("2","Spin",function(down,key)events[#events+1]=(down and "+"or "-")..key end,function()cancel=true end,function()return cancel end))
assert(table.concat(events,",")=="+Shift,+2,-2,-Shift","Mid-sequence cancellation releases keys")
events={}
local ok=emitGesture("2","Chest",function(down,key)events[#events+1]=(down and "+"or "-")..key end,function()error("wait failed")end,function()return false end)
assert(not ok and table.concat(events,",")=="+Shift,+2,-2,-Shift","Errors release both injected keys")
print("7 native gesture and cleanup checks passed")
