-- Court volumes observed in the user's live geometry report; no map coordinates hard-coded.
do (function()
 local cacheAt=-math.huge
 local volume,divider,halfSign=nil,nil,1
 local function refresh()
  if os.clock()-cacheAt<.5 then return end
  cacheAt=os.clock();volume=nil;divider=nil
  local own=model();local root=rootOf(own);if not root then return end
  local assets=workspace:FindFirstChild("Assets")
  local bounds=assets and assets:FindFirstChild("CourtBounds")
  local nearest=math.huge
  if bounds then
   for _,part in ipairs(bounds:GetChildren())do
    if part:IsA("BasePart")then
     local localPoint=part.CFrame:PointToObjectSpace(root.Position)
     local excess=math.max(0,math.abs(localPoint.X)-part.Size.X*.5)^2+math.max(0,math.abs(localPoint.Z)-part.Size.Z*.5)^2
     if excess<nearest then nearest=excess;volume=part end
    end
   end
  end
  local map=workspace:FindFirstChild("Map");local courts=map and map:FindFirstChild("Courts")
  if not courts or not volume then return end
  local number=attr(own,"CourtNumber")
  for _,half in ipairs(courts:GetDescendants())do
   if half:IsA("Model")and half:GetAttribute("StartedGame")==true and half:GetAttribute("FullCourt")==false
    and tostring(half:GetAttribute("CourtNumber"))==tostring(number)then
    local out=half:FindFirstChild("OutOfBounds")
    local edge=out and out:FindFirstChild("HalfCourtBound")
    local violations=half:FindFirstChild("Violations")
    local inside=violations and violations:FindFirstChild("Backcourt")
    if edge and edge:IsA("BasePart")and inside and inside:IsA("BasePart")
     and (edge.Position-volume.Position).Magnitude<math.max(volume.Size.X,volume.Size.Z)then
     divider=edge
     halfSign=edge.CFrame:PointToObjectSpace(inside.Position).Z>=0 and 1 or -1
     break
    end
   end
  end
 end
 settings._courtClamp=function(point,margin)
  refresh();margin=margin or 2
  if not volume then return point end
  local q=volume.CFrame:PointToObjectSpace(point)
  local hx=math.max(.1,volume.Size.X*.5-margin);local hz=math.max(.1,volume.Size.Z*.5-margin)
  local result=volume.CFrame:PointToWorldSpace(Vector3.new(math.clamp(q.X,-hx,hx),q.Y,math.clamp(q.Z,-hz,hz)))
  if divider then
   local d=divider.CFrame:PointToObjectSpace(result)
   local limit=divider.Size.Z*.5+margin
   if d.Z*halfSign<limit then result=divider.CFrame:PointToWorldSpace(Vector3.new(d.X,d.Y,limit*halfSign))end
  end
  return Vector3.new(result.X,point.Y,result.Z)
 end
 settings._courtInside=function(point,margin)
  local limited=settings._courtClamp(point,margin)
  return flat(limited-point).Magnitude<.05,volume~=nil
 end
 settings._courtSteer=function(root,direction)
  if not root or not direction or direction.Magnitude<.01 then return direction end
  local origin=root.Position
  local inset=settings._courtClamp(origin,2.5)
  if flat(inset-origin).Magnitude>.2 then return flat(inset-origin).Unit end
  local lead=math.clamp(flat(root.AssemblyLinearVelocity).Magnitude*.25,3,8)
  local requested=origin+direction*lead
  local endpoint=settings._courtClamp(requested,2.5)
  local delta=flat(endpoint-origin)
  return delta.Magnitude>.2 and delta.Unit or Vector3.zero
 end
 local previous=featureCleanup
 featureCleanup=function()
  settings._courtClamp=nil;settings._courtInside=nil;settings._courtSteer=nil
  previous()
 end
end)() end

