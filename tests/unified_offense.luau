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

assert(farmChoice({easyDunk=true,wideOpen=true,shotEnabled=true,inRange=true,ready=true})=="drive")
assert(farmChoice({easyDunk=true,smartRetreat=true,stepback=true})=="drive")
assert(farmChoice({confirmedStepback=true,stepbackWindow=true,shotEnabled=true,inRange=true,open=false,ready=true})=="create")
assert(farmChoice({confirmedStepback=true,stepbackWindow=true,shotEnabled=true,inRange=true,open=true,ready=true})=="shoot")
print("Unified offense: 4 priority scenarios passed")
