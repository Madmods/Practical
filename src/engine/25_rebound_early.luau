-- Early rebound positioning: while a shot is in the air, head for a spot on the line from the
-- hoop to you, `reboundSpot` studs from the rim. The existing landing-spot chase takes over as
-- soon as the ball is falling. Used only when Auto rebound + Pursue are on; WASD still overrides.
do (function()
 settings.reboundEarly=false;settings.reboundSpot=5.5
 numericLimits.reboundSpot={3,9}
 settings._reboundEarlySpot=function(root,ballPosition,ball)
  if not ball or not ball.Parent or attr(ball,"MidShot")~=true then return nil end
  -- The hoop this shot is heading for: the nearest assigned Goal to the ball.
  local hoop,best
  for _,character in ipairs(defenseCharacters())do
   local goal=goalPosition(attr(character,"Goal"))
   if goal then
    local distance=flat(goal-ballPosition).Magnitude
    if distance<45 and(not best or distance<best)then hoop=goal;best=distance end
   end
  end
  if not hoop then return nil end
  local fromHoop=flat(root.Position-hoop)
  if fromHoop.Magnitude<.5 or fromHoop.Magnitude>30 then return nil end
  -- The ball must still be up in the air near the rim, not already on its way down to the floor.
  if ballPosition.Y<root.Position.Y+2 then return nil end
  local spot=hoop+fromHoop.Unit*(settings.reboundSpot or 5.5)
  return clampCourt(Vector3.new(spot.X,root.Position.Y,spot.Z))
 end
 local previous=featureCleanup
 featureCleanup=function()settings._reboundEarlySpot=nil;previous()end
end)() end
