 local function retreatingDefender(gap,ahead,lateral,rimSpeed,awaySpeed)
  return gap>=5 and gap<=14 and ahead>2 and lateral<=6
   and rimSpeed>=3 and awaySpeed>=2
 end
assert(retreatingDefender(8,7,2,8,7))
assert(not retreatingDefender(3,3,0,8,8))
assert(not retreatingDefender(18,18,0,8,8))
assert(not retreatingDefender(8,7,2,-8,-7))
assert(not retreatingDefender(8,-7,2,8,7))
assert(not retreatingDefender(9,3,8,8,7))
assert(not retreatingDefender(8,7,2,0,0))
print("Retreat read: 7 scenarios passed")
