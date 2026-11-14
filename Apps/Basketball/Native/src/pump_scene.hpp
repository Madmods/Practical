#pragma once
// Only named instances and the version-pinned fields in build-offsets.hpp are read.
inline void updatePump(HANDLE h,addr root,addr own,addr player,addr visualEngine){
 static pump::Logic logic;static addr previousOwn=0;static std::map<addr,pump::GroundPlane> floors;static std::map<addr,std::pair<double,shooting::Vec>> history;
 double t=now();if(own!=previousOwn){logic={};history.clear();floors.clear();previousOwn=own;}
 shooting::Vec p,pv;if(!positionOf(h,own,p,pv)){heldBall=0;sceneAt=-10;pumpStatus=pump::Unavailable;return;}
 auto ownPrevious=history.find(own);if(ownPrevious!=history.end()){double dt=t-ownPrevious->second.first;if(dt>.005&&dt<.25){auto measured=(p-ownPrevious->second.second)*(float)(1/dt);if(shooting::length(measured)>shooting::length(pv))pv=measured;}}history[own]={t,p};
 auto ws=child(h,root,"Workspace");addr ball=0;for(auto b:children(h,child(h,ws,"Basketballs"))){auto val=path(h,b,{"Attributes","Character"});if(val&&rd<addr>(h,val+Offsets::Misc::Value)==own){ball=b;break;}}
 heldBall=ball;sceneAt=t;
 std::map<std::string,addr> teams;for(auto a:children(h,child(h,root,"Players")))teams[nm(h,a)]=rd<addr>(h,a+Offsets::Player::Team);auto team=rd<addr>(h,player+Offsets::Player::Team);
 struct Rig{addr model;shooting::Vec p,v;float gap;};std::vector<Rig> rigs;addr locked=(logic.waiting||logic.committed)?logic.locked:tapSerial!=logic.consumed?tapTarget.load():0;Rig target{};float closest=(float)pumpRange.load();
 for(auto model:children(h,child(h,ws,"Characters"))){if(model==own)continue;auto name=nm(h,model);if(name=="InvisCharacter")continue;auto it=teams.find(name);if(team&&it!=teams.end()&&it->second==team)continue;auto hum=child(h,model,"Humanoid");if(!hum||rd<float>(h,hum+Offsets::Humanoid::Health)<=0)continue;
  shooting::Vec pos,vel;if(!positionOf(h,model,pos,vel)||std::abs(pos.y-p.y)>9)continue;float gap=shooting::length(pos-p);if(gap>35)continue;
  auto prev=history.find(model);if(prev!=history.end()){double dt=t-prev->second.first;if(dt>.005&&dt<.25){auto measured=(pos-prev->second.second)*(float)(1/dt);if(std::abs(measured.y)<70)vel.y=measured.y;if(shooting::length(vel)<.5&&shooting::length(measured)<60){vel.x=measured.x;vel.z=measured.z;}}}history[model]={t,pos};
  Rig r{model,pos,vel,gap};rigs.push_back(r);if(locked?model==locked:gap<=closest){target=r;closest=gap;}
 }
 for(auto i=history.begin();i!=history.end();)if(t-i->second.first>3)i=history.erase(i);else ++i;
 nearTarget=target.model;nearHeight=target.p.y;nearVertical=target.v.y;
 std::vector<shooting::Defender> others;for(auto&r:rigs)if(r.model!=target.model)others.push_back({r.p,r.v,16});double pressure=std::max(shooting::estimate(p,pv,others,0),shooting::estimate(p,pv,others,.35));
 pump::Observation o;o.time=t;o.tapAt=tapAt;o.ball=ball;o.tapBall=tapBall;o.target=target.model;o.focused=gameFocused();o.timing=armed;o.stationary=shooting::length(pv)<1.2&&std::abs(pv.y)<2;o.fresh=true;o.distance=target.model?target.gap:999;o.height=target.p.y;o.verticalSpeed=target.v.y;o.otherContest=pressure;o.tap=tapSerial;o.tapTarget=tapTarget;o.tapHeight=tapHeight;o.tapGrounded=tapGrounded;
 auto d=logic.step({pumpOn.load(),(double)pumpRange.load(),pumpDelay.load()*.001,(double)pumpContest.load()},o);pumpStatus=d.state;
 // Bounded local trace for validating this new trigger; no unrelated input is logged.
 static std::wstring logPath;static int lastState=-1,logged=0;static double lastLog=-1;
 if(logPath.empty()){wchar_t exe[MAX_PATH];GetModuleFileNameW(nullptr,exe,MAX_PATH);logPath=exe;logPath.resize(logPath.find_last_of(L"\\")+1);logPath+=L"pump-fake-trace.csv";std::ofstream(logPath.c_str())<<"time,state,ball,target,gap,height,vertical,other_contest,stationary,tap,shot_request\n";}
 if(logged<2000&&(d.state!=lastState||(logic.waiting&&t-lastLog>.1)||d.shoot)){std::ofstream file(logPath.c_str(),std::ios::app);file<<t<<","<<(int)d.state<<","<<bool(ball)<<","<<bool(target.model)<<","<<o.distance<<","<<o.height<<","<<o.verticalSpeed<<","<<pressure<<","<<o.stationary<<","<<o.tap<<","<<d.shoot<<"\n";lastLog=t;lastState=d.state;++logged;}
 if(d.shoot){pumpRequestBall=ball;pumpRequestAt=t;++pumpRequest;}
 // Publish identities only. The renderer samples the camera and transforms together,
 // independently of this slower possession/defender scan.
 PumpVisual visual;visual.at=now();visual.state=d.state;visual.gap=target.gap;visual.jumpConfirmed=logic.waiting&&logic.jumpAt>=0;visual.shotSent=now()-pumpSentAt.load()<.7&&target.model&&target.model==pumpSentTarget.load();
 visual.ownPrimitive=rd<addr>(h,child(h,own,"HumanoidRootPart")+Offsets::BasePart::Primitive);
 if(target.model)visual.targetPrimitive=rd<addr>(h,child(h,target.model,"HumanoidRootPart")+Offsets::BasePart::Primitive);
 // Humanoid standing clearance accounts for differently scaled R15 characters.
 auto floorFor=[&](addr model,addr primitive,shooting::Vec position,float vertical,float fallback){
  auto hum=child(h,model,"Humanoid");float hip=rd<float>(h,hum+Offsets::Humanoid::HipHeight,NAN);auto size=rd<shooting::Vec>(h,primitive+Offsets::Primitive::Size,{NAN,NAN,NAN});
  if(!std::isfinite(hip)||hip<.1f||hip>10||!std::isfinite(size.y)||size.y<.1f||size.y>10)return fallback;
  return floors[model].update(position.y-hip-size.y*.5f,vertical,t,fallback)+.025f;
 };
 visual.ownGround=floorFor(own,visual.ownPrimitive,p,pv.y,NAN);
 if(target.model)visual.targetGround=floorFor(target.model,visual.targetPrimitive,target.p,target.v.y,visual.ownGround);
 for(auto it=floors.begin();it!=floors.end();)if(it->first!=own&&!history.count(it->first))it=floors.erase(it);else ++it;
 visual.valid=visual.ownPrimitive!=0&&std::isfinite(visual.ownGround);
 {std::lock_guard<std::mutex>lock(pumpVisualMutex);pumpVisual=std::move(visual);}
}
