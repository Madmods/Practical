 local function farmChoice(c)
  if c.finish then return "finish" end
  if c.beaten or c.laneClear then return "drive" end
  if c.confirmedStepback and c.stepbackWindow and c.shotEnabled and c.inRange and c.open and c.ready and not c.dribbling then return "shoot" end
  if c.smartRetreat and c.stepback and not c.beaten then return "stepback" end
  if c.beaten or c.laneClear then return "drive" end
  -- Ordinary offense keeps creating a drive. Only a completed stepback can choose a jumper.
  if c.stepback then return "stepback" end
  return "create"
 end

assert(farmChoice({finish=true,laneClear=true})=="finish")
assert(farmChoice({laneClear=true,smartRetreat=true,stepback=true})=="drive")
assert(farmChoice({beaten=true,confirmedStepback=true,stepbackWindow=true,shotEnabled=true,inRange=true,open=true,ready=true})=="drive")
assert(farmChoice({laneClear=true,confirmedStepback=true,stepbackWindow=true,shotEnabled=true,inRange=true,open=true,ready=true})=="drive")
assert(farmChoice({smartRetreat=true,stepback=true})=="stepback")
print("Dunk priority: 5 scenarios passed")
