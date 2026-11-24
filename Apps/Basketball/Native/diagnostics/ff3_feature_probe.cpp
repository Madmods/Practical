#define NOMINMAX
#include "../src/game_module_ui.hpp"
#include "../src/ff3_assist_runtime.hpp"
#include <iostream>
using namespace basketballUi;
void inspectGui(HANDLE h,addr node,int depth,int& budget,const std::string& prefix,json::Value& out){
 if(!node||depth<0||budget--<=0)return;const auto name=nm(h,node),kind=defense::classOf(h,node);
 if(name.find("Chat")!=std::string::npos||kind=="LocalScript"||kind=="ModuleScript"||kind=="Script")return;
 const auto full=prefix+name;auto row=object();row.o[L"path"]=string(widen(full));row.o[L"class"]=string(widen(kind));
 if(kind=="ScreenGui")row.o[L"enabled"]=basketballUi::boolean(rd<unsigned char>(h,node+Offsets::GuiObject::ScreenGui_Enabled,255)==1);
 if(kind=="Frame"||kind=="TextLabel"||kind=="TextButton"||kind=="ImageLabel"||kind=="ImageButton"){
  struct P{float x,y;};const auto p=rd<P>(h,node+Offsets::GuiBase2D::AbsolutePosition,{NAN,NAN}),s=rd<P>(h,node+Offsets::GuiBase2D::AbsoluteSize,{NAN,NAN});
  row.o[L"visible"]=basketballUi::boolean(rd<unsigned char>(h,node+Offsets::GuiObject::Visible,255)==1);
  if(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(s.x)&&std::isfinite(s.y)){auto rect=array();for(double v:{double(p.x),double(p.y),double(s.x),double(s.y)})rect.a.push_back(number(v));row.o[L"rect"]=rect;}
  if(kind=="TextLabel"||kind=="TextButton")row.o[L"text"]=string(widen(defense::textAt(h,node+Offsets::GuiObject::Text)));
 }
 if(kind=="NumberValue")row.o[L"value"]=number(rd<double>(h,node+Offsets::Misc::Value,NAN));
 if(kind=="IntValue")row.o[L"value"]=number(double(rd<long long>(h,node+Offsets::Misc::Value)));
 if(kind=="StringValue")row.o[L"value"]=string(widen(defense::textAt(h,node+Offsets::Misc::Value)));
 if(kind=="ObjectValue")row.o[L"valueName"]=string(widen(nm(h,rd<addr>(h,node+Offsets::Misc::Value))));
 out.a.push_back(row);for(auto c:children(h,node))inspectGui(h,c,depth-1,budget,full+"/",out);
}
int main(int argc,char** argv){
 HANDLE h=nullptr;DWORD pid=0;addr base=0;
 if(!connectRoblox(h,pid,base,[](HANDLE process,addr module,DWORD id){auto s=inspectBasketball(process,module,id);auto game=gameModules::identify(s);if(!game||game->id!=gameModules::Id::FootballFusion3)return 0;DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);return id==foreground?200:100;})){std::cout<<"{\"available\":false,\"reason\":\"No readable FF3 client\"}";return 2;}
 auto session=inspectBasketball(h,base,pid);auto game=gameModules::identify(session);if(!game||game->id!=gameModules::Id::FootballFusion3){CloseHandle(h);return 3;}
 auto players=child(h,session.model,"Players"),player=rd<addr>(h,players+Offsets::Player::LocalPlayer),own=rd<addr>(h,player+Offsets::Player::ModelInstance),ws=child(h,session.model,"Workspace");
 if(argc>1&&std::string(argv[1])=="--catch-pose"){
  const double started=now(),duration=argc>2?std::clamp(std::atoi(argv[2]),1,120):1;std::cout<<"at,measured,handsOut,leftX,leftY,leftZ,rightX,rightY,rightZ\n";
  while(now()-started<duration){ff3Play::Scene scene;scene.own=rd<addr>(h,player+Offsets::Player::ModelInstance);scene.head=ff3Native::position(h,child(h,scene.own,"Head"));const auto pose=ff3Assist::observedCatchPose(h,scene);std::cout<<now()-started<<','<<pose.measured<<','<<pose.handsOut;for(auto v:{pose.left,pose.right})std::cout<<','<<v.x<<','<<v.y<<','<<v.z;std::cout<<'\n';Sleep(25);}CloseHandle(h);return 0;
 }
 if(argc>1&&std::string(argv[1])=="--catch-contact"){
  ff3Play::Scene scene;scene.own=own;scene.head=ff3Native::position(h,child(h,own,"Head"));scene.rootPosition=ff3Native::position(h,child(h,own,"HumanoidRootPart"));const auto reach=ff3Assist::catchContact(h,scene);const auto parts=ff3Native::catchParts(h,own);
  auto vector=[](ff3qb::Vec v){auto out=array();for(double n:{v.x,v.y,v.z})out.a.push_back(number(n));return out;};auto report=object();report.o[L"readOnly"]=basketballUi::boolean(true);report.o[L"measured"]=basketballUi::boolean(ff3Assist::validContactReach(reach));report.o[L"radius"]=number(reach.radius);report.o[L"headOffset"]=vector(reach.offset);report.o[L"headToRoot"]=vector(scene.head-scene.rootPosition);auto hands=array();for(auto part:{parts.first,parts.second})if(part){auto hand=object();hand.o[L"size"]=vector(ff3Native::partSize(h,part));hand.o[L"headOffset"]=vector(ff3Native::position(h,part)-scene.head);hands.a.push_back(hand);}report.o[L"catchParts"]=hands;std::cout<<winrt::to_string(json::dump(report));CloseHandle(h);return 0;
 }
 if(argc>1&&std::string(argv[1])=="--ball-census"){
  const auto head=ff3Native::position(h,child(h,own,"Head"));ff3Native::Reader reader;ff3Native::Frame frame;frame.pid=pid;frame.at=now();reader.collectBalls(h,ws,frame,true);
  auto report=object(),found=array();report.o[L"runtimeDiscovered"]=number(double(reader.flightParts.size()));report.o[L"airborneTracked"]=number(double(frame.balls.size()));report.o[L"looseObserved"]=number(double(frame.looseBalls.size()));
  struct Entry{addr node;int depth;std::string path;};std::vector<Entry> queue{{ws,0,"Workspace"}};unsigned visited=0;
  for(size_t index=0;index<queue.size()&&visited<32768;++index)for(auto item:children(h,queue[index].node)){
   if(++visited>32768)break;const auto type=defense::classOf(h,item),name=nm(h,item),full=queue[index].path+"/"+name;
   const bool part=defense::partClass(type),gravity=part&&path(h,item,{"Configuration","Gravity"});
   if(part&&(name=="Football"||(name=="Handle"&&gravity))){auto p=ff3Native::position(h,item),v=ff3Native::velocity(h,item);auto primitive=rd<addr>(h,item+Offsets::BasePart::Primitive);auto row=object();row.o[L"path"]=string(widen(full));row.o[L"class"]=string(widen(type));row.o[L"depth"]=number(queue[index].depth+1);row.o[L"gravityMetadata"]=basketballUi::boolean(gravity);row.o[L"anchored"]=basketballUi::boolean(rd<unsigned char>(h,primitive+Offsets::Primitive::Flags,255)&Offsets::PrimitiveFlags::Anchored);row.o[L"runtimeFound"]=basketballUi::boolean(std::find(reader.flightParts.begin(),reader.flightParts.end(),item)!=reader.flightParts.end());row.o[L"speed"]=number(ff3qb::length(v));row.o[L"distance"]=number(ff3qb::length(p-head));row.o[L"y"]=number(p.y);row.o[L"parent"]=string(widen(defense::classOf(h,rd<addr>(h,item+Offsets::Instance::Parent))));found.a.push_back(row);}
   if(queue[index].depth<15&&(type=="Folder"||type=="Model"||type=="WorldModel")&&!child(h,item,"Humanoid"))queue.push_back({item,queue[index].depth+1,full});
  }
  report.o[L"visited"]=number(visited);report.o[L"truncated"]=basketballUi::boolean(visited>=32768);report.o[L"candidates"]=found;std::cout<<winrt::to_string(json::dump(report));CloseHandle(h);return 0;
 }
 if(argc>1&&std::string(argv[1])=="--catch-window"){
  const double started=now(),duration=argc>2?std::clamp(std::atoi(argv[2]),1,30):8;double ground=NAN;ff3Native::Reader reader;
  while(now()-started<duration){ff3Play::Scene scene;scene.pid=pid;scene.model=session.model;scene.player=player;scene.own=rd<addr>(h,player+Offsets::Player::ModelInstance);const auto human=child(h,scene.own,"Humanoid"),rootPart=child(h,scene.own,"HumanoidRootPart");scene.head=ff3Native::position(h,child(h,scene.own,"Head"));scene.rootPosition=ff3Native::position(h,rootPart);scene.velocity=ff3Native::velocity(h,rootPart);scene.at=now();scene.worldGravity=rd<float>(h,ws+Offsets::Workspace::ReadOnlyGravity,NAN);scene.speed=rd<float>(h,human+Offsets::Humanoid::WalkSpeed,NAN);scene.jumpVelocity=rd<float>(h,human+Offsets::Humanoid::JumpPower,0);scene.naturalShape=scene.shape=ff3Play::receivingShape(h,scene.own);scene.cameraForward={0,0,-1};scene.cameraRight={1,0,0};
   const auto state=rd<addr>(h,human+Offsets::Humanoid::HumanoidState);const int id=rd<int>(h,state+Offsets::Humanoid::HumanoidStateID,-1);const auto platform=rd<unsigned char>(h,human+Offsets::Humanoid::PlatformStand,255);scene.grounded=id==8&&platform==0&&std::abs(scene.velocity.y)<3;
   if(scene.grounded){auto foot=child(h,scene.own,"LeftFoot");if(!foot)foot=child(h,scene.own,"Left Leg");const auto p=ff3Native::position(h,foot),size=ff3Native::partSize(h,foot);if(ff3qb::finite(p)&&ff3qb::finite(size))ground=p.y-size.y*.5;}
   scene.groundY=ground;scene.standingHeadHeight=scene.head.y-ground;scene.standingRootHeight=scene.rootPosition.y-ground;scene.valid=ff3qb::finite(scene.head)&&std::isfinite(ground);scene.focused=true;scene.typing=scene.menu=false;scene.possessed=ff3Native::Reader::possessedBall(h,scene.own)!=0;
   ff3Native::Frame frame;frame.pid=pid;frame.at=scene.at;frame.head=scene.head;frame.groundY=ground;reader.collectBalls(h,ws,frame,true);scene.balls=std::move(frame.balls);scene.looseBalls=std::move(frame.looseBalls);const auto contact=ff3Assist::plannedCatchContact(h,scene);auto display=ff3Assist::guideObservations(scene);const auto plan=ff3Assist::planCatchGuide(display,{},scene.at,0,nullptr,nullptr,nullptr,false,false,&contact);
   auto vec=[](ff3qb::Vec value){auto result=array();for(double n:{value.x,value.y,value.z})result.a.push_back(number(n));return result;};auto result=object();result.o[L"readOnly"]=basketballUi::boolean(true);result.o[L"at"]=number(scene.at);result.o[L"ownedBall"]=basketballUi::boolean(scene.possessed);result.o[L"grounded"]=basketballUi::boolean(scene.grounded);result.o[L"ballCount"]=number(double(scene.looseBalls.size()));result.o[L"valid"]=basketballUi::boolean(plan.valid);result.o[L"action"]=number(plan.action);result.o[L"ball"]=number(double(plan.ball));result.o[L"eta"]=number(plan.contactAt-scene.at);result.o[L"zone"]=vec(plan.zoneCenter);result.o[L"contact"]=vec(plan.contact);result.o[L"head"]=vec(scene.head);result.o[L"poseOffset"]=vec(contact.offset);result.o[L"anticipatedPose"]=basketballUi::boolean(contact.anticipated);std::cout<<winrt::to_string(json::dump(result))<<'\n';Sleep(40);
  }CloseHandle(h);return 0;
 }
 if(argc>1&&std::string(argv[1])=="--motiontrace"){
  const double start=now(),duration=argc>2?std::clamp(std::atoi(argv[2]),1,120):45;
  std::cout<<"at,E,G,S,Space,state,platformStand,jumpPower,walkSpeed,x,y,z,vx,vy,vz,uptime,W,A,D,focused,evaluateStateMachine,bodyVelocityCount,bodyVx,bodyVy,bodyVz,maxForceX,maxForceY,maxForceZ,linearVelocityCount,torsoX,torsoY,torsoZ\n";
  while(now()-start<duration){auto rig=rd<addr>(h,player+Offsets::Player::ModelInstance),human=child(h,rig,"Humanoid"),root=child(h,rig,"HumanoidRootPart"),state=rd<addr>(h,human+Offsets::Humanoid::HumanoidState);auto p=ff3Native::position(h,root),v=ff3Native::velocity(h,root);
   DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);const bool focused=foreground==pid;
   std::cout<<now()-start;for(int key:{int('E'),int('G'),int('S'),int(VK_SPACE)})std::cout<<','<<bool(focused&&(GetAsyncKeyState(key)&0x8000));
   unsigned bodies=0,linears=0;ff3qb::Vec bv{NAN,NAN,NAN},force{NAN,NAN,NAN};for(auto part:children(h,rig)){if(!defense::partClass(defense::classOf(h,part)))continue;for(auto controller:children(h,part)){const auto kind=defense::classOf(h,controller);if(kind=="BodyVelocity"){++bodies;bv=ff3Native::vector(rd<shooting::Vec>(h,controller+Offsets::BodyVelocity::Velocity,{NAN,NAN,NAN}));force=ff3Native::vector(rd<shooting::Vec>(h,controller+Offsets::BodyVelocity::MaxForce,{NAN,NAN,NAN}));}if(kind=="LinearVelocity")++linears;}}
   const auto torso=ff3Native::position(h,child(h,rig,"Torso"));
   std::cout<<','<<rd<int>(h,state+Offsets::Humanoid::HumanoidStateID,-1)<<','<<int(rd<unsigned char>(h,human+Offsets::Humanoid::PlatformStand,255))<<','<<rd<float>(h,human+Offsets::Humanoid::JumpPower,NAN)<<','<<rd<float>(h,human+Offsets::Humanoid::WalkSpeed,NAN)<<','<<p.x<<','<<p.y<<','<<p.z<<','<<v.x<<','<<v.y<<','<<v.z<<','<<now()<<','<<bool(focused&&(GetAsyncKeyState('W')&0x8000))<<','<<bool(focused&&(GetAsyncKeyState('A')&0x8000))<<','<<bool(focused&&(GetAsyncKeyState('D')&0x8000))<<','<<focused<<','<<int(rd<unsigned char>(h,human+Offsets::Humanoid::EvaluateStateMachine,255))<<','<<bodies<<','<<bv.x<<','<<bv.y<<','<<bv.z<<','<<force.x<<','<<force.y<<','<<force.z<<','<<linears<<','<<torso.x<<','<<torso.y<<','<<torso.z<<'\n';Sleep(16);
  }CloseHandle(h);return 0;
 }
 if(argc>1&&std::string(argv[1])=="--kicktrace"){
  auto gui=child(h,player,"PlayerGui");const double start=now(),duration=argc>2?std::clamp(std::atoi(argv[2]),1,60):30;std::cout<<"at,cursorX,cursorY,cursorW,cursorH,meterW,meterH,arrow1,arrow2,arrow1Y,arrow2Y,enabled,layerEnabled,adornee\n";
  while(now()-start<duration){auto surface=child(h,gui,"KickerGui"),frame=child(h,surface,"Frame"),cursor=child(h,frame,"Cursor"),meter=child(h,frame,"Meter"),a=child(h,frame,"Arrow1"),b=child(h,frame,"Arrow2");struct P{float x,y;};auto p=rd<P>(h,cursor+Offsets::GuiBase2D::AbsolutePosition,{NAN,NAN}),z=rd<P>(h,cursor+Offsets::GuiBase2D::AbsoluteSize,{NAN,NAN}),size=rd<P>(h,meter+Offsets::GuiBase2D::AbsoluteSize,{NAN,NAN}),ap=rd<P>(h,a+Offsets::GuiBase2D::AbsolutePosition,{NAN,NAN}),bp=rd<P>(h,b+Offsets::GuiBase2D::AbsolutePosition,{NAN,NAN});
   std::cout<<now()-start<<','<<p.x<<','<<p.y<<','<<z.x<<','<<z.y<<','<<size.x<<','<<size.y<<','<<int(rd<unsigned char>(h,a+Offsets::GuiObject::Visible,255))<<','<<int(rd<unsigned char>(h,b+Offsets::GuiObject::Visible,255))<<','<<ap.y<<','<<bp.y<<','<<int(rd<unsigned char>(h,surface+Offsets::GuiObject::ScreenGui_Enabled,255))<<','<<int(rd<unsigned char>(h,surface+Offsets::LayerCollector::Enabled,255))<<','<<rd<addr>(h,surface+Offsets::Misc::Adornee)<<'\n';Sleep(16);
  }CloseHandle(h);return 0;
 }
 auto report=object();report.o[L"available"]=basketballUi::boolean(true);report.o[L"pid"]=number(pid);report.o[L"versionCompatible"]=basketballUi::boolean(session.version);report.o[L"place"]=number(double(session.placeId));report.o[L"universe"]=number(double(session.universeId));
 auto capabilities=object();auto humanoid=child(h,own,"Humanoid"),body=child(h,own,"HumanoidRootPart"),head=child(h,own,"Head");auto shape=ff3Play::receivingShape(h,own);auto hands=ff3Native::catchParts(h,own);
 capabilities.o[L"catchPartsPresent"]=basketballUi::boolean(hands.first&&hands.second);capabilities.o[L"measuredCatchShape"]=basketballUi::boolean(shape.measured);capabilities.o[L"jumpMode"]=number(rd<unsigned char>(h,humanoid+Offsets::Humanoid::UseJumpPower,255));capabilities.o[L"jumpPower"]=number(rd<float>(h,humanoid+Offsets::Humanoid::JumpPower,NAN));capabilities.o[L"jumpHeight"]=number(rd<float>(h,humanoid+Offsets::Humanoid::JumpHeight,NAN));capabilities.o[L"floorMaterial"]=number(rd<int>(h,humanoid+Offsets::Humanoid::FloorMaterial,-1));
 auto state=rd<addr>(h,humanoid+Offsets::Humanoid::HumanoidState);capabilities.o[L"humanoidState"]=number(rd<int>(h,state+Offsets::Humanoid::HumanoidStateID,-1));capabilities.o[L"evaluateStateMachine"]=number(rd<unsigned char>(h,humanoid+Offsets::Humanoid::EvaluateStateMachine,255));ff3Play::Scene scene;scene.pid=pid;scene.model=session.model;scene.own=own;scene.player=player;scene.head=ff3Native::position(h,head);scene.velocity=ff3Native::velocity(h,body);auto actors=ff3Assist::actors(h,scene,ws,ff3Native::isPracticeRange(session.placeId));
 capabilities.o[L"opponents"]=number(double(std::count_if(actors.begin(),actors.end(),[](const auto& a){return a.opponent;})));capabilities.o[L"blockHitboxes"]=number(double(ff3Assist::hitboxProperties(h,scene,"block",8).size()));capabilities.o[L"swatHitboxes"]=number(double(ff3Assist::hitboxProperties(h,scene,"swat",8).size()));capabilities.o[L"contactFilterParts"]=number(double(ff3Assist::contactProperties(h,scene,ws,actors).size()));capabilities.o[L"ownedCaptainVerified"]=basketballUi::boolean(ff3Assist::ownedCaptain(h,player));capabilities.o[L"jobIdentityReadable"]=basketballUi::boolean(ff3Assist::validJob(defense::textAt(h,session.model+Offsets::DataModel::JobId)));
 HANDLE writable=OpenProcess(PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,pid);capabilities.o[L"writeHandleAvailable"]=basketballUi::boolean(writable!=nullptr);if(writable)CloseHandle(writable);report.o[L"capabilities"]=capabilities;
 auto gui=array();int budget=2000;inspectGui(h,child(h,player,"PlayerGui"),7,budget,"",gui);report.o[L"gui"]=gui;
 auto values=array();for(auto o:children(h,ws)){auto type=defense::classOf(h,o);if(type=="StringValue"||type=="IntValue"||type=="NumberValue"||type=="ObjectValue"){budget=100;inspectGui(h,o,1,budget,"Workspace/",values);}}report.o[L"values"]=values;
 auto parts=array();budget=400;inspectGui(h,own,3,budget,"Character/",parts);report.o[L"character"]=parts;
 std::cout<<winrt::to_string(json::dump(report));CloseHandle(h);
}
