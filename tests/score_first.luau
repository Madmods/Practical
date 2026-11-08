 local function farmChoice(c)
  if c.finish then return "finish" end
  if c.easyDunk then return "drive" end
  if c.wideOpen and c.shotEnabled and c.inRange and c.ready and not c.dribbling then return "shoot" end
  if c.confirmedStepback and c.stepbackWindow and c.shotEnabled and c.inRange and c.open and c.ready and not c.dribbling then return "shoot" end
  if c.stepback then return "stepback" end
  if c.beaten or c.laneClear then return "drive" end
  -- Covered possessions keep creating a drive or a safe stepback.
  if c.stepback then return "stepback" end
  return "create"
 end

assert(farmChoice({easyDunk=true,stepback=true})=="drive")
assert(farmChoice({laneClear=true,stepback=true})=="stepback")
assert(farmChoice({beaten=true,stepback=true})=="stepback")
assert(farmChoice({laneClear=true,stepback=false})=="drive")
assert(farmChoice({stepback=true,confirmedStepback=true,stepbackWindow=true,shotEnabled=true,inRange=true,open=true,ready=true})=="shoot")
print("Scoring priority: 5 scenarios passed")
