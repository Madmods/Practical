 local function farmChoice(c)
  if c.finish then return "finish" end
  if c.easyDunk then return "drive" end
  if c.wideOpen and c.shotEnabled and c.inRange and c.ready and not c.dribbling then return "shoot" end
  if c.confirmedStepback and c.stepbackWindow and c.shotEnabled and c.inRange and c.open and c.ready and not c.dribbling then return "shoot" end
  if c.smartRetreat and c.stepback and not c.beaten then return "stepback" end
  if c.beaten or c.laneClear then return "drive" end
  -- Covered possessions keep creating a drive or a safe stepback.
  if c.stepback then return "stepback" end
  return "create"
 end

local function choice(overrides)
 local c={wideOpen=true,shotEnabled=true,inRange=true,ready=true,laneClear=true}
 for k,v in pairs(overrides or {})do c[k]=v end
 return farmChoice(c)
end
assert(choice()=="shoot")
assert(choice({easyDunk=true})=="drive")
assert(choice({inRange=false})=="drive")
assert(choice({shotEnabled=false})=="drive")
assert(choice({dribbling=true})=="drive")
assert(choice({wideOpen=false})=="drive")
assert(choice({ready=false})=="drive")
print("Wide-open shot policy: 7 scenarios passed")
