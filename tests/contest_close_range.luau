local CONFIG={ZeroContestDistance=12,FullContestDistance=2,Curve=1.6,CloseRangeDistance=6,CloseRangeMultiplier=1.5,CloseRangeBonusGain=3}
local function proximity(distance)
    local span = math.max(0.1, CONFIG.ZeroContestDistance - CONFIG.FullContestDistance)
    local closeness = math.clamp((CONFIG.ZeroContestDistance - distance) / span, 0, 1)
    local closeSpan = math.max(.1, CONFIG.CloseRangeDistance - CONFIG.FullContestDistance)
    local closeWeight = math.clamp((CONFIG.CloseRangeDistance - distance) / closeSpan, 0, 1)
    local multiplier = 1 + (CONFIG.CloseRangeMultiplier - 1) * CONFIG.CloseRangeBonusGain * closeWeight
    return math.clamp(closeness ^ CONFIG.Curve * multiplier, 0, 1)
end

assert(proximity(12)==0)
assert(proximity(2)==1)
local far=proximity(8)
local close=proximity(4)
CONFIG.CloseRangeMultiplier=1
assert(proximity(8)==far,"Outside bonus radius must be unchanged")
assert(proximity(4)<close,"Near defender receives additional multiplier")
CONFIG.CloseRangeMultiplier=3
local previous=0
for d=12,0,-.1 do local v=proximity(d);assert(v>=previous-1e-8 and v<=1);previous=v end
print("Close-range scaling checks passed")
