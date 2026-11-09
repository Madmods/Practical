do
 api.GetContestCalibration=function()return table.clone(settings._contestModel.config)end
 api.EstimateContest=function(character,horizon)
  character=character or model();local r=rootOf(character)
  if not r then return nil,"Character unavailable"end
  return settings._contestModel.evaluate(r.Position,settings._contestModel.collect(character),horizon or 0,r.AssemblyLinearVelocity)
 end
end
