-- Exercise the actual scoped initializer and its captured callbacks.
local source=readfile("src/engine/09_suite_api_profiles.luau")
local callbacks,buttons={},{}
local heartbeat={};local possession=false;local events=0;local cleaned=0;local refreshed=0
local settings={guardGap=5,guardReaction=.16,assignmentLock=false}
local suite={}
local fixture=setmetatable({
 settings=settings,env=suite,readfile=false,writefile=false,
 settingRows={{key="guardGap",min=2,max=8,refresh=function()refreshed=refreshed+1 end}},
 finite=function(v)return type(v)=="number"and v==v end,
 on=function(signal,fn)callbacks[signal]=fn end,
 row=function(_,name)local b={Text=name,Activated={},MouseButton2Click={}};buttons[name]=b;return b end,
 label=function()end,featureToggle=function()end,
 nav={Stats={}},pages={Stats={}},UDim2={fromOffset=function()return {}end},
 status={},Input={InputBegan={}},Run={Heartbeat=heartbeat},
 model=function()return {}end,attr=function()return possession end,
 holdsBall=function()return possession end,lastSteal=-math.huge,
 recordDefense=function()events=events+1 end,
 syncSoloToggles=function()end,featureCleanup=function()cleaned=cleaned+1 end,
 reportEvents={},actionCounts={},records={},
}, {__index=_G})
local init=assert(loadstring(source.."\nreturn api,numericLimits,featureCleanup"))
setfenv(init,fixture)
local api,limits,cleanup=init()
assert(api==suite.PracticalSuiteAPI,"API must remain exported to later features")
assert(limits.guardGap[1]==2 and limits.guardGap[2]==8,"Shared limits must remain available")
limits.laterFeature={0,10};settings.laterFeature=1
assert(api.SetNumber("laterFeature",20)and settings.laterFeature==10,"Later modules must share the same limits table")
assert(api.SaveProfile("Match"))
settings.guardGap=7
assert(api.ApplyProfile("Match")and settings.guardGap==5,"Profile state must survive initializer return")
local profileButton
for name,button in pairs(buttons)do if name:sub(1,7)=="Profile"then profileButton=button end end
assert(profileButton and callbacks[profileButton.Activated])
callbacks[profileButton.Activated]()
assert(api.GetState().profile=="Practice"and refreshed>0,"Profile UI callback must retain private state")
callbacks[heartbeat]();assert(events==0)
possession=true;callbacks[heartbeat]();callbacks[heartbeat]()
assert(events==1,"Possession observer must retain its previous value")
api.RegisterUICleanup(function()cleaned=cleaned+10 end)
api.RegisterUICleanup(function()cleaned=cleaned+100 end)
cleanup()
assert(cleaned==111 and suite.PracticalSuiteAPI==nil,"Cleanup must still call each owner once and unpublish API")
assert(suite.PracticalMatchReport.profile=="Practice","Cleanup report must retain active profile")
assert(fixture.currentProfile==nil and fixture.previousPossession==nil,"Private state must not become globals")
print("Scoped API: exports, limits, profile callbacks, possession tracking and cleanup passed")
