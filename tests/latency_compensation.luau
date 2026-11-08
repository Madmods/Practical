-- Run in a Luau harness providing readfile and loadstring, from the project root.
-- Load the actual implementation instead of duplicating the learning formula.
local source=readfile("src/engine/03_autotime_peak_predictor.luau")
local first=assert(source:find("settings._latency={buckets={}}",1,true))
local last=assert(source:find("local function rate(samples)",first,true))
local settings={latencyCompensation=false}
local finite=function(v)return type(v)=="number" and v==v and math.abs(v)<math.huge end
assert(loadstring("return function(settings,finite)\n"..source:sub(first,last-1).."\nend"))()(settings,finite)
local learner=settings._latency
local c,m,b=learner.adjust(.100,.050)
assert(c==0 and m==0 and b==nil,"off must bypass compensation")
settings.latencyCompensation=true
c,m,b=learner.adjust(.100,.050)
assert(c==0 and m==.012 and b==5,"jitter must be capped; new buckets start neutral")
assert(learner.bucket(.101)==learner.bucket(.099),"nearby pings share a bucket")
assert(learner.bucket(.140)~=b,"distant pings are isolated")
local function shot(error,correction,margin)
 return {compensated=true,started=0,sentAt=.9+error,ping=.08,baseDelay=.02,offset=0,
  correction=correction or 0,jitterMargin=margin or 0,pingBucket=5,
  trace={{time=.96,progress=.95},{time=1,progress=1},{time=1.04,progress=.94}}}
end
local record={}
learner.learn(shot(.010),record)
assert(math.abs(learner.buckets[5].correction-.002)<1e-9,"late releases must advance the next release")
assert(record.estimatedReleaseError>0 and record.estimatedPeakTime==1)
learner.buckets={}
learner.learn(shot(-.010),{})
assert(learner.buckets[5].correction<0,"early releases must delay the next release")
learner.buckets={}
learner.learn(shot(-.012,0,.012),{})
assert(math.abs(learner.buckets[5].correction)<1e-9,"learning must not cancel the jitter margin")
learner.buckets={}
for i=1,100 do learner.learn(shot(.060,.020),{}) end
assert(learner.buckets[5].correction<=.020,"correction must stay bounded")
local count=learner.buckets[5].count
learner.learn(shot(.100),{})
local truncated=shot(.010);truncated.trace[3]=nil;learner.learn(truncated,{})
local low=shot(.010);low.trace[2].progress=.97;learner.learn(low,{})
local failed=shot(.010);failed.cancelled=true;learner.learn(failed,{})
settings.latencyCompensation=false;learner.learn(shot(.010),{})
assert(learner.buckets[5].count==count,"unreliable or disabled shots must not train")
print("latency compensation scenarios passed")
