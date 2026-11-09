local CONFIG={ZeroContestDistance=12,FullContestDistance=2,Curve=1.6,CloseRangeDistance=6,CloseRangeMultiplier=1.5,CloseRangeBonusGain=3,DistanceFalloffStart=6,DistanceDropPerStud=0}
local function proximity(distance)
    local span = math.max(0.1, CONFIG.ZeroContestDistance - CONFIG.FullContestDistance)
    local closeness = math.clamp((CONFIG.ZeroContestDistance - distance) / span, 0, 1)
    local closeSpan = math.max(.1, CONFIG.CloseRangeDistance - CONFIG.FullContestDistance)
    local closeWeight = math.clamp((CONFIG.CloseRangeDistance - distance) / closeSpan, 0, 1)
    local multiplier = 1 + (CONFIG.CloseRangeMultiplier - 1) * CONFIG.CloseRangeBonusGain * closeWeight
    local extraStuds = math.max(0, distance - CONFIG.DistanceFalloffStart)
    local distanceMultiplier = (1 - CONFIG.DistanceDropPerStud) ^ extraStuds
    return math.clamp(closeness ^ CONFIG.Curve * multiplier * distanceMultiplier, 0, 1)
end

local near,far=proximity(4),proximity(8)
CONFIG.DistanceDropPerStud=.1
assert(proximity(4)==near,"No extra drop before starting distance")
assert(math.abs(proximity(8)-far*.9^2)<1e-8,"Two studs compounds reduction twice")
CONFIG.DistanceDropPerStud=.5
assert(proximity(8)<far*.9^2,"Stronger slider lowers distant estimate")
local previous=0
for d=15,0,-.1 do local v=proximity(d);assert(v>=previous-1e-8 and v>=0 and v<=1);previous=v end
print("Distance falloff checks passed")
