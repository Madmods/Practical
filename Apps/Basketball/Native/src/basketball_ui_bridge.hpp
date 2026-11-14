#pragma once
#include "engine.hpp"
#include "webui/json_lite.hpp"
#include "vanta_release.hpp"
#include <filesystem>
#include <sstream>

// The UI adapter maps to the existing atomics; planners/input rules remain in engine.hpp.
namespace basketballUi {
struct Number { const wchar_t* key; const wchar_t* file; std::atomic<int>* value; int low,high; };
inline Number numbers[]={
 {L"autotime.release",L"earlier_ms",&trim,-30,60}, {L"autotime.maxping",L"max_ping",&maxPing,50,1000},
 {L"autotime.fresh",L"freshness_ms",&freshnessMs,60,200},
 {L"pumpfake.delay",L"pump_delay",&pumpDelay,60,300}, {L"pumpfake.range",L"pump_range",&pumpRange,4,16},
 {L"pumpfake.contest",L"pump_contest",&pumpContest,0,30},
 {L"shotassist.minimum",L"minimum_percent",&minRelease,1,100}, {L"shotassist.maximum",L"maximum_percent",&maxRelease,1,100},
 {L"shotassist.lead",L"overhold_lead_ms",&overholdLeadMs,0,120}, {L"ceiling.limit",L"contest_limit",&contestLimit,5,100}
};
struct Flag {const wchar_t* key; const wchar_t* file; std::atomic<bool>* value;};
inline Flag flags[]={
 {L"autotime.reviewer",L"reviewer_enabled",&reviewOn},
 {L"autotime",L"",&armed},{L"pumpfake",L"",&pumpOn},{L"shotassist",L"",&assist},{L"ceiling",L"",&ceilingOn},
 {L"pumpfake.visuals",L"pump_visuals",&pumpVisuals},{L"shotassist.hold",L"hold_early",&holdEarly},
 {L"shotassist.fall",L"protect_fall",&protectFall},{L"ceiling.closeout",L"closeout",&closeoutOn},
 {L"ball_arc",L"ball_arc",&ballArcOn},{L"ball_landing",L"ball_landing",&ballLandingOn},{L"rolling",L"rolling_intercept",&rollingOn}
};
inline std::wstring widen(const std::string& s){return std::wstring(s.begin(),s.end());}
inline const wchar_t* cards[]={L"guard",L"screens",L"lock",L"offball",L"steal",L"intercept",L"recovery",L"block",L"oop",L"rebound"};
inline double scaled(double v){return std::round(v*10000.0)/10000.0;}
inline json::Value number(double n){json::Value v;v.type=json::Value::Number;v.n=n;return v;}
inline json::Value boolean(bool b){json::Value v;v.type=json::Value::Bool;v.b=b;return v;}
inline json::Value object(){json::Value v;v.type=json::Value::Object;return v;}
inline json::Value string(const std::wstring& s){json::Value v;v.type=json::Value::String;v.s=s;return v;}
inline json::Value array(){json::Value v;v.type=json::Value::Array;return v;}
inline json::Value shotReport(const TimingRelease& shot){
 const wchar_t* names[]={L"Waiting for a shot",L"Auto Time",L"Held manual release",L"Maximum limit",L"Overhold protection",L"Closeout",L"Auto Time ceiling"};
 const auto reason=static_cast<unsigned>(shot.reason);auto out=object();
 out.o[L"count"]=number(shot.count);out.o[L"reason"]=string(reason<std::size(names)?names[reason]:L"Unknown");
 out.o[L"percent"]=number(shot.percent);out.o[L"leadMs"]=number(std::round(shot.lead*1000));
 out.o[L"adjustmentMs"]=number(shot.trim);out.o[L"pingMs"]=number(shot.ping);
 out.o[L"minimum"]=number(shot.minimum);out.o[L"maximum"]=number(shot.maximum);out.o[L"windowPing"]=number(shot.windowPing);
 out.o[L"rate"]=number(shot.rate);out.o[L"samples"]=number(static_cast<double>(shot.samples));out.o[L"spanMs"]=number(std::round(shot.span*1000));return out;
}
inline json::Value windowReport(){
 const auto w=currentTimingWindow();auto out=object();out.o[L"valid"]=boolean(w.bounds.valid);
 out.o[L"minimum"]=number(w.bounds.minimum);out.o[L"maximum"]=number(w.bounds.maximum);out.o[L"ping"]=number(w.ping);
 out.o[L"detail"]=string(w.bounds.valid?L"Captured for this shot":w.bounds.outOfRange?L"Ping is outside the calibrated timing range; automatic release paused":L"Waiting for fresh ping and a rising meter");return out;
}
inline json::Value reviewReport(){
 const auto v=reviewerView();auto out=object();
 out.o[L"enabled"]=boolean(reviewOn);out.o[L"stable"]=boolean(v.stable);
 out.o[L"shots"]=number(v.shots);out.o[L"feedbackCount"]=number(v.feedbackCount);out.o[L"perfect"]=number(v.perfect);out.o[L"overshoots"]=number(v.overshoots);
 out.o[L"gameFeedbackCount"]=number(v.gameFeedbackCount);out.o[L"gamePerfect"]=number(v.gamePerfect);out.o[L"gameShotId"]=number(v.gameShotId);out.o[L"gameGrade"]=string(widen(v.gameGrade));
 out.o[L"resultMeter"]=number(v.resultMeter);out.o[L"resultContest"]=number(v.resultContest);out.o[L"resultSent"]=number(v.resultSent);
 out.o[L"resultPhase"]=string(widen(v.resultPhase));
 out.o[L"pendingId"]=number(reviewOn&&armed?v.pendingId:0);
 out.o[L"adjustmentMs"]=number(v.adjustmentMs);out.o[L"guardMs"]=number(v.guardMs);
 out.o[L"windowShiftMs"]=number(v.windowShiftMs);
 out.o[L"ping"]=number(v.ping);out.o[L"jitter"]=number(v.jitter);
 out.o[L"reason"]=string(!reviewOn?L"Reviewer off":!armed?L"Enable Auto Time to review shots":widen(v.reason));
 out.o[L"last"]=string(widen(v.last));return out;
}
inline json::Value connection(const basketballConnection::Snapshot& s,bool workspace,const vantaRelease::Result& release){
 using namespace basketballConnection;
 const bool live=fresh(s,now()),place=live&&s.loaded&&targetGame(s);
 const bool player=place&&s.player&&s.characterReady;
 const bool gameReady=ready(s,now()),good=gameReady&&release.current;std::wstring detail;
 if(!live)detail=L"Connection data expired. Waiting for a fresh scan.";
 else if(!s.process)detail=L"Open Roblox and join Practical Basketball.";
 else if(!s.version)detail=connectionLabel();
 else if(!s.opened)detail=L"Waiting for readable Roblox game data.";
 else if(s.universeId&&s.universeId!=UniverseId)detail=L"Different game detected. Join Practical Basketball.";
 else if(!place)detail=L"Waiting for Practical Basketball to finish loading.";
 else if(!player)detail=L"Waiting for your player and a live Basketball character. Spawn into the game.";
 else if(!s.world)detail=L"Waiting for Basketball world data.";
 else if(!s.meter)detail=L"Waiting for a readable Basketball shot meter.";
 else if(!s.renderer)detail=L"Waiting for valid rendering data.";
 else if(!gameReady)detail=L"Confirming the connection remains stable.";
 else if(!release.current)detail=release.detail;
 else detail=L"Connected to Practical Basketball";
 auto result=object();result.o[L"type"]=string(L"basketballConnection");result.o[L"ready"]=boolean(good);
 result.o[L"workspaceOpen"]=boolean(workspace);result.o[L"detail"]=string(detail);
 result.o[L"placeId"]=string(std::to_wstring(s.placeId));result.o[L"universeId"]=string(std::to_wstring(s.universeId));result.o[L"checks"]=array();
 auto row=[&](const wchar_t* id,bool ok,const wchar_t* label,std::wstring note){auto v=object();v.o[L"id"]=string(id);v.o[L"ok"]=boolean(ok);v.o[L"result"]=string(ok?L"Verified":label);v.o[L"detail"]=string(note);result.o[L"checks"].a.push_back(v);};
 row(L"client",live&&s.process,L"Waiting",s.process?L"Roblox player detected":L"Start the Roblox player");
 row(L"version",live&&s.version&&s.opened,L"Not ready",s.version&&s.opened?L"Supported Roblox build":connectionLabel());
 row(L"place",gameReady,s.universeId&&s.universeId!=UniverseId?L"Wrong game":L"Loading",gameReady?L"Practical Basketball verified":detail);
 row(L"release",release.current,release.status.c_str(),release.detail);
 return result;
}
inline bool validKey(int key){return key==0||(key>=8&&key<=254&&key!=VK_ESCAPE&&key!=VK_F8&&key!=VK_LWIN&&key!=VK_RWIN)||key==VK_MBUTTON||key==VK_XBUTTON1||key==VK_XBUTTON2;}
inline std::wstring keyName(int key){
 if(!key)return L""; if(key>='A'&&key<='Z'||key>='0'&&key<='9')return std::wstring(1,wchar_t(key));
 switch(key){case VK_SPACE:return L"Space";case VK_SHIFT:return L"Shift";case VK_CONTROL:return L"Ctrl";case VK_MENU:return L"Alt";case VK_TAB:return L"Tab";case VK_RETURN:return L"Enter";case VK_MBUTTON:return L"Mouse3";case VK_XBUTTON1:return L"Mouse4";case VK_XBUTTON2:return L"Mouse5";}
 if(key>=VK_F1&&key<=VK_F24)return L"F"+std::to_wstring(key-VK_F1+1);
 switch(key){case VK_INSERT:return L"Insert";case VK_HOME:return L"Home";case VK_END:return L"End";case VK_PRIOR:return L"PageUp";case VK_NEXT:return L"PageDown";case VK_UP:return L"↑";case VK_DOWN:return L"↓";case VK_LEFT:return L"←";case VK_RIGHT:return L"→";}
 return L"VK"+std::to_wstring(key);
}
inline int keyCode(const json::Value& v){
 if(v.type!=json::Value::String)return -1;auto s=v.s;if(s.empty()||s==L"None")return 0;
 if(s.size()==1){wchar_t c=towupper(s[0]);if(c>='A'&&c<='Z'||c>='0'&&c<='9')return int(c);}
 for(int k=1;k<=254;k++)if(validKey(k)&&keyName(k)==s)return k;
 return -1;
}
inline bool set(const std::wstring& key,const json::Value& v){
 for(auto& b:flags)if(key==b.key){if(v.type!=json::Value::Bool)return false;*b.value=v.b;return true;}
 for(auto& b:numbers)if(key==b.key){
  if(v.type!=json::Value::Number||!std::isfinite(v.n)||v.n<b.low||v.n>b.high)return false;
  int n=(int)std::lround(v.n);
  if(b.value==&minRelease&&n>maxRelease||b.value==&maxRelease&&n<minRelease)return false;
  *b.value=n;return true;
 }
 if(key==L"autotime.shot"){
  if(v.type!=json::Value::Array||v.a.size()!=2)return false;
  int a=keyCode(v.a[0]),b=keyCode(v.a[1]);if(a<=0||b<0||!validKey(a)||!validKey(b)||a==b)return false;
  // Do not rebind an in-progress held shot.
  if(gameDown||physicalDown)return false;
  shootKey=a;shootKeyAlt=b;return true;
 }
 if(key.rfind(L"def_",0)==0){
  auto dot=key.find(L'.');std::wstring name=dot==std::wstring::npos?key.substr(4):key.substr(dot+1);
  for(int i=0;i<defense::Count;i++){
   auto& option=defense::options[i];if(name!=widen(option.key))continue;
   if(key.substr(0,dot)!=L"def_"+std::wstring(cards[option.card]))return false;
   int n=0;
   if(option.kind==defense::Toggle){if(v.type!=json::Value::Bool)return false;n=v.b?1:0;}
   else if(option.kind==defense::Key){if(v.type!=json::Value::Array||v.a.size()!=1)return false;n=keyCode(v.a[0]);if(n<=0||!validKey(n))return false;}
   else {if(v.type!=json::Value::Number||!std::isfinite(v.n))return false;double raw=v.n/option.scale;if(raw<option.lo-.001||raw>option.hi+.001)return false;n=(int)std::lround(raw);}
   if(n<option.lo||n>option.hi)return false;
   if(i==defense::Response)defense::applyResponsePreset(n);else option.value=n;
   return true;
  }
 }
 return false;
}
inline void stop(){
 armed=false;pumpOn=false;assist=false;ceilingOn=false;
 resetReviewer();
 publishTimingWindow({});
 for(auto id:{defense::Guard,defense::Offball,defense::Steal,defense::Intercept,defense::Block,defense::Oop,defense::Rebound,defense::Recovery,defense::RimBlock})defense::options[id].value=0;
 defense::pick=false;defense::selected=0;
}
inline json::Value keys(int a,int b=-1){auto v=array();v.a.push_back(string(keyName(a)));if(b>=0)v.a.push_back(string(keyName(b)));return v;}
inline json::Value snapshot(){
 auto result=object();
 auto row=[&](const std::wstring& key,json::Value v){auto dot=key.find(L'.');auto module=key.substr(0,dot);auto& m=result.o[module];m.type=json::Value::Object;m.o[L"rows"].type=json::Value::Object;if(dot==std::wstring::npos)m.o[L"on"]=v;else m.o[L"rows"].o[key.substr(dot+1)]=v;};
 for(auto& b:flags)row(b.key,boolean(b.value->load()));for(auto& b:numbers)row(b.key,number(b.value->load()));row(L"autotime.shot",keys(shootKey,shootKeyAlt));
 const wchar_t* main[]={L"guard",L"screens",L"lock",L"offball",L"steal",L"intercept",L"recovery",L"block",L"oop",L"rebound"};
 for(auto& o:defense::options){auto module=L"def_"+std::wstring(main[o.card]);auto name=widen(o.key);json::Value value;
  if(o.kind==defense::Toggle)value=boolean(o.value!=0);else if(o.kind==defense::Key)value=keys(o.value);else value=number(scaled(o.value*double(o.scale)));
  row(module+(name==main[o.card]?L"":L"."+name),value);
 }
 return result;
}
inline bool applyProfile(const json::Value& profile){
 if(profile.type!=json::Value::Object)return false;
 std::vector<int> oldNumbers,oldDefense;std::vector<bool> oldFlags;
 for(auto& b:numbers)oldNumbers.push_back(b.value->load());
 for(auto& b:flags)oldFlags.push_back(b.value->load());
 for(auto& o:defense::options)oldDefense.push_back(o.value.load());
 const int oldPrimary=shootKey,oldSecondary=shootKeyAlt;
 auto apply=[](const json::Value& p){
  for(auto& module:p.o){
   if(module.second.type!=json::Value::Object)return false;
   // Unknown UI-only sections are not engine commands.
   if(module.first==L"theme"||module.first==L"glass"||module.first==L"menukey"||module.first==L"about")continue;
   if(!set(module.first,module.second[L"on"]))return false;
   auto& rows=module.second[L"rows"];if(rows.type!=json::Value::Object)return false;
   if(module.first==L"shotassist"){
    auto& low=rows[L"minimum"];auto& high=rows[L"maximum"];
    if(low.type!=json::Value::Number||high.type!=json::Value::Number||low.n<1||high.n>100||low.n>high.n||!std::isfinite(low.n)||!std::isfinite(high.n))return false;
    minRelease=(int)low.n;maxRelease=(int)high.n;
   }
   // Apply response preset before its explicitly saved individual options.
   if(rows[L"response"].type!=json::Value::Null&&!set(module.first+L".response",rows[L"response"]))return false;
   for(auto& r:rows.o){if(r.first==L"response")continue;if(!set(module.first+L"."+r.first,r.second))return false;}
  }return true;
 };
 // Gate physical inputs for the whole transaction; restore previous values on rejection.
 bool active=sessionUnlocked.exchange(false);bool ok=apply(profile);
 if(!ok){for(size_t i=0;i<std::size(numbers);++i)*numbers[i].value=oldNumbers[i];for(size_t i=0;i<std::size(flags);++i)*flags[i].value=oldFlags[i];for(size_t i=0;i<std::size(defense::options);++i)defense::options[i].value=oldDefense[i];shootKey=oldPrimary;shootKeyAlt=oldSecondary;}
 sessionUnlocked=active;return ok;
}
inline json::Value readConfig(const std::filesystem::path& path){
 std::ifstream file(path,std::ios::binary);std::string s(65537,'\0');file.read(s.data(),s.size());s.resize((size_t)file.gcount());if(s.size()>65536)return object();
 try{auto result=json::parse(std::wstring(winrt::to_hstring(s)));return result.type==json::Value::Object?result:object();}catch(...){return object();}
}
inline void load(const std::filesystem::path& path){
 auto cfg=readConfig(path);
 for(auto& b:numbers){auto& v=cfg[b.file];if(v.type==json::Value::Number&&std::isfinite(v.n))*b.value=(int)std::clamp(v.n,double(b.low),double(b.high));}
 if(minRelease>maxRelease)maxRelease=minRelease.load();
 for(auto& b:flags)if(*b.file){auto& v=cfg[b.file];if(v.type==json::Value::Number)*b.value=v.n!=0;else if(v.type==json::Value::Bool)*b.value=v.b;}
 for(auto& o:defense::options){auto& v=cfg[L"def_"+widen(o.key)];if(v.type==json::Value::Number&&std::isfinite(v.n)){int n=(int)std::clamp(v.n,double(o.lo),double(o.hi));if(o.kind!=defense::Key||validKey(n))o.value=n;}}
 auto primary=cfg[L"shoot_key"],secondary=cfg[L"shoot_key_alt"];
 if(primary.type==json::Value::Number&&std::isfinite(primary.n)&&primary.n>0&&primary.n<=254&&validKey((int)primary.n))shootKey=(int)primary.n;
 if(secondary.type==json::Value::Number&&std::isfinite(secondary.n)&&secondary.n>=0&&secondary.n<=254&&validKey((int)secondary.n)&&(int)secondary.n!=shootKey)shootKeyAlt=(int)secondary.n;
}
inline bool save(const std::filesystem::path& path){
 auto cfg=readConfig(path);cfg.o[L"version"]=number(2);
 for(auto& b:numbers)cfg.o[b.file]=number(b.value->load());for(auto& b:flags)if(*b.file)cfg.o[b.file]=number(b.value->load()?1:0);
 for(auto& o:defense::options)cfg.o[L"def_"+widen(o.key)]=number(o.value.load());
 cfg.o[L"shoot_key"]=number(shootKey);cfg.o[L"shoot_key_alt"]=number(shootKeyAlt);
 auto tmp=path;tmp+=L".webui.tmp";auto wide=json::dump(cfg);std::string text=winrt::to_string(wide);
 {std::ofstream out(tmp,std::ios::binary|std::ios::trunc);out<<text;out.flush();if(!out)return false;}
 return MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
}
inline json::Value bootstrap(){
 auto data=object();data.o[L"modules"]=snapshot();auto schema=array();
 for(auto& o:defense::options){auto row=object();row.o[L"key"]=string(widen(o.key));row.o[L"label"]=string(o.name);row.o[L"hint"]=string(o.hint);row.o[L"card"]=number(o.card);row.o[L"kind"]=number(o.kind);row.o[L"min"]=number(scaled(o.lo*double(o.scale)));row.o[L"max"]=number(scaled(o.hi*double(o.scale)));row.o[L"step"]=number(scaled(o.step*double(o.scale)));row.o[L"advanced"]=boolean(o.advanced);schema.a.push_back(row);}
 data.o[L"schema"]=schema;return data;
}
}
