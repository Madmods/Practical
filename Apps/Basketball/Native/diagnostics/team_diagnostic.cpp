#define NOMINMAX
#include "engine.hpp"
#include <iostream>
// Read-only, version-checked diagnostic. No worker threads or input hooks.
int main(){HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base)){std::cout<<"Supported process unavailable\n";return 1;}
 auto root=rd<addr>(h,rd<addr>(h,base+Offsets::FakeDataModel::Pointer)+Offsets::FakeDataModel::RealDataModel);
 auto players=child(h,root,"Players"),local=rd<addr>(h,players+Offsets::Player::LocalPlayer);
 auto own=path(h,root,{"Workspace","Characters",nm(h,local)});auto ownTeam=rd<addr>(h,local+Offsets::Player::Team);
 auto ownGoal=rd<addr>(h,path(h,own,{"Attributes","Goal"})+Offsets::Misc::Value);
 for(auto obj:{own,local}){std::cout<<"META "<<(obj==own?"own":"local")<<"\n";for(auto a:children(h,child(h,obj,"Attributes"))){auto cls=defense::classOf(h,a);std::cout<<nm(h,a)<<" "<<cls;if(cls=="BoolValue")std::cout<<"="<<(int)rd<unsigned char>(h,a+Offsets::Misc::Value);std::cout<<"\n";}}
 std::map<std::string,addr> members;for(auto p:children(h,players))members[nm(h,p)]=p;
 std::cout<<"local_character="<<bool(own)<<" local_team="<<bool(ownTeam)<<" local_goal="<<bool(ownGoal)<<"\n";
 int n=0;for(auto rig:children(h,path(h,root,{"Workspace","Characters"}))){if(rig==own||!child(h,rig,"Humanoid"))continue;
 auto found=members.find(nm(h,rig));auto p=found==members.end()?0:found->second;auto team=p?rd<addr>(h,p+Offsets::Player::Team):0;
 auto goal=rd<addr>(h,path(h,rig,{"Attributes","Goal"})+Offsets::Misc::Value);
 if(!p&&!goal)continue; // Empty placeholder rigs are irrelevant to eligibility.
 if(goal&&!p){std::cout<<"npc_model="<<nm(h,rig)<<"\n";for(auto a:children(h,child(h,rig,"Attributes"))){auto cls=defense::classOf(h,a);std::cout<<" attr="<<nm(h,a)<<" class="<<cls;if(cls=="BoolValue")std::cout<<" bool="<<(int)rd<unsigned char>(h,a+Offsets::Misc::Value);if(cls=="StringValue")std::cout<<" text="<<defense::textAt(h,a+Offsets::Misc::Value);std::cout<<"\n";}for(auto a:children(h,rig))std::cout<<" child="<<nm(h,a)<<" class="<<defense::classOf(h,a)<<"\n";}
 std::cout<<"actor="<<++n<<" player_match="<<bool(p)<<" team_known="<<bool(team)<<" same_team="<<(team&&team==ownTeam)<<" goal_known="<<bool(goal)<<" same_goal="<<(goal&&goal==ownGoal)<<"\n";
 }gamePid=pid;sceneRoot=root;sceneOwn=own;scenePlayer=local; defense::options[defense::PracticeAI].value=1;defense::Reader reader;auto frame=reader.read(10);int opponents=0;for(const auto& a:frame.scene.actors)if(a.enemy)++opponents;std::cout<<"reader_own="<<bool(frame.scene.own.id)<<" reader_actors="<<frame.scene.actors.size()<<" reader_opponents="<<opponents<<" reader_other="<<(frame.scene.actors.size()-opponents)<<"\n";
 std::cout<<"court_roster="<<reader.courtRoster.available<<" scanned_rigs="<<reader.rigs.size()<<" scene_valid="<<frame.scene.valid<<" scene_detail="<<frame.detail<<"\n";
 defense::Config config;config.values[defense::Guard]=1;config.values[defense::Reaction]=0;defense::Intent intent;intent.focused=intent.activation=true;defense::Planner planner;
 auto decision=planner.step(config,frame.scene,intent,frame.scene.at);
 std::cout<<"practice_layout="<<reader.courtRoster.practiceLayout(h)<<" balls="<<frame.scene.balls.size()<<" playing_known="<<frame.scene.playingKnown<<" playing="<<frame.scene.playing<<" planned_target="<<bool(decision.target)<<" planned_move="<<decision.moving<<" plan_reason="<<decision.reason<<" input_requests=0\n";
 for(auto c:children(h,reader.courtRoster.container)){std::cout<<"court_child="<<nm(h,c)<<" class="<<defense::classOf(h,c)<<" game="<<bool(child(h,c,"Game"))<<"\n";for(auto sub:children(h,c))std::cout<<" court_field="<<nm(h,sub)<<" class="<<defense::classOf(h,sub)<<"\n";}
 for(auto rig:children(h,path(h,root,{"Workspace","Characters"}))){auto name=nm(h,rig);if(name.compare(0,2,"AI")!=0)continue;auto hum=child(h,rig,"Humanoid"),prim=rd<addr>(h,child(h,rig,"HumanoidRootPart")+Offsets::BasePart::Primitive);auto pos=reader.position(prim);auto vel=rd<defense::Vec>(h,prim+Offsets::Primitive::AssemblyLinearVelocity);std::cout<<"practice_rig="<<name<<" health="<<rd<float>(h,hum+Offsets::Humanoid::Health,NAN)<<" primitive="<<bool(prim)<<" position="<<pos.x<<","<<pos.y<<","<<pos.z<<" distance="<<defense::distance(pos,frame.scene.own.p)<<" vertical_gap="<<fabs(pos.y-frame.scene.own.p.y)<<" velocity="<<vel.x<<","<<vel.y<<","<<vel.z<<"\n";}
 int observed=0,held=0,plans=0,moves=0;
 for(int tick=0;tick<60;tick++){
  auto sample=reader.read(now());auto plan=planner.step(config,sample.scene,intent,sample.scene.at);
  if(!sample.scene.actors.empty())++observed;
  for(const auto& a:sample.scene.actors)if(a.enemy&&defense::heldBall(sample.scene,a.id))++held;
  if(plan.target)++plans;if(plan.moving)++moves;Sleep(100);
 }
 std::cout<<"live_samples=60 partner_visible_samples="<<observed<<" partner_holding_samples="<<held<<" planned_target_samples="<<plans<<" planned_movement_samples="<<moves<<" input_requests=0\n";
 CloseHandle(h);return 0;}




