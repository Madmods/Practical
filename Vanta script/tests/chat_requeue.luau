 local function chatTriggerWord(message)
  local words={cheat=true,cheats=true,cheater=true,cheaters=true,cheating=true,hack=true,hacks=true,hacker=true,hackers=true,hacking=true,exploit=true,exploits=true,exploiter=true,exploiters=true,exploiting=true,aimbot=true,aimbotting=true,aimlock=true}
  for word in string.lower(tostring(message)):gmatch("%a+")do
   if words[word]then return word end
  end
 end

assert(chatTriggerWord("HE IS CHEATING!")=="cheating")
assert(chatTriggerWord("hacks??")=="hacks")
assert(chatTriggerWord("an exploiter")=="exploiter")
assert(chatTriggerWord("aimbot")=="aimbot")
assert(not chatTriggerWord("that was a hackingly odd word"))
assert(not chatTriggerWord("nice shot"))
assert(not chatTriggerWord("hacksaw"))
print("7 chat keyword checks passed")
