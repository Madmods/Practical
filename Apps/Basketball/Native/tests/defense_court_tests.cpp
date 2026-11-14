#define NOMINMAX
#include "../src/engine.hpp"
#include <iostream>
#include <memory>

// Synthetic instance memory, read through the real bounded instance helpers and
// ReadProcessMemory against this test process. No game, hooks or input workers.
struct Instance {
 std::array<unsigned char,0x1000> memory{};std::array<addr,4> descriptor{};
 std::string name,kind;std::vector<std::pair<addr,addr>> children;std::array<addr,2> span{};
 Instance(std::string n,std::string k):name(n),kind(k){
  put(Offsets::Instance::NameContainer,reinterpret_cast<addr>(&name));
  descriptor[Offsets::Instance::ClassName/sizeof(addr)]=reinterpret_cast<addr>(&kind);
  put(Offsets::Instance::ClassDescriptor,reinterpret_cast<addr>(descriptor.data()));
  put(Offsets::Instance::ChildrenStart,reinterpret_cast<addr>(span.data()));
 }
 addr id()const{return reinterpret_cast<addr>(memory.data());}
 template<class T>void put(size_t offset,T value){std::memcpy(memory.data()+offset,&value,sizeof(value));}
 void append(Instance& c){children.push_back({c.id(),0});span={reinterpret_cast<addr>(children.data()),reinterpret_cast<addr>(children.data()+children.size())};c.put(Offsets::Instance::Parent,id());}
};
int main(){
 int count=0;auto check=[&](bool ok,const char* name){++count;if(!ok){std::cerr<<"FAIL: "<<name<<'\n';std::exit(1);}};
 using namespace defense;
 check(courtOpponent({10,1},{10,2}),"opponents on a shared-hoop court are enemies");
 check(!courtOpponent({10,1},{10,1}),"same court same team is friendly");
 check(!courtOpponent({10,1},{11,2}),"adjacent court is not an opponent");
 check(!courtOpponent({10,1},{10,2},true),"self rejected");
 for(CourtMember bad:{CourtMember{},CourtMember{0,1},CourtMember{10,0},CourtMember{10,3}}){check(!courtOpponent(bad,{10,2})&&!courtOpponent({10,1},bad),"unknown or malformed membership rejected");}
 CourtMembers duplicate;duplicate.add(1,{10,1});duplicate.add(1,{10,2});duplicate.add(1,{10,1});check(!duplicate.get(1).valid(),"duplicate stays ambiguous for whole snapshot");
 Instance ws("Workspace","Workspace"),map("Map","Model"),courts("Courts","Folder"),court("Court1","Model"),game("Game","Folder"),attributes("Attributes","Configuration"),players("Players","Folder"),one("Team1","Folder"),two("Team2","Folder"),alice("Alice","BoolValue"),bob("Bob","BoolValue"),ally("Ally","BoolValue"),ballValue("Basketball","ObjectValue"),ball("Basketball","Part");
 ws.append(map);map.append(courts);courts.append(court);court.append(game);game.append(attributes);attributes.append(players);players.append(one);players.append(two);one.append(alice);one.append(ally);two.append(bob);attributes.append(ballValue);
 alice.put<unsigned char>(Offsets::Misc::Value,1);bob.put<unsigned char>(Offsets::Misc::Value,1);ally.put<unsigned char>(Offsets::Misc::Value,1);ballValue.put(Offsets::Misc::Value,ball.id());
 const std::map<std::string,addr> lookup{{"Alice",1},{"Bob",2},{"Ally",3}};
 CourtRoster roster;HANDLE process=GetCurrentProcess();roster.scan(process,ws.id(),lookup);
 auto members=roster.read(process);check(roster.available&&members.players.size()==3,"real roster reader resolves named BoolValue members");
 check(courtOpponent(members.get(1),members.get(2))&&!courtOpponent(members.get(1),members.get(3)),"reader distinguishes opponents and allies");
 check(!members.get(4).valid(),"spectator has no court membership");
 check(roster.ball(process,members.get(1))==ball.id(),"court uses its assigned ball");
 check(!roster.ball(process,{}),"spectator cannot acquire another court ball");
 bob.put<unsigned char>(Offsets::Misc::Value,0);check(!roster.read(process).get(2).valid(),"inactive flag takes effect without rescan");
 bob.put<unsigned char>(Offsets::Misc::Value,2);check(!roster.read(process).get(2).valid(),"invalid boolean fails closed");bob.put<unsigned char>(Offsets::Misc::Value,1);
 bob.put<addr>(Offsets::Instance::Parent,0);check(!roster.read(process).get(2).valid(),"removed slot invalidates without rescan");bob.put(Offsets::Instance::Parent,two.id());
 bob.name="SomeoneElse";check(!roster.read(process).get(2).valid(),"renamed slot cannot keep cached identity");bob.name="Bob";
 game.put<addr>(Offsets::Instance::Parent,0);check(roster.read(process).players.empty()&&!roster.ball(process,members.get(1)),"removed court hierarchy invalidates roster and ball");game.put(Offsets::Instance::Parent,court.id());
 map.put<addr>(Offsets::Instance::Parent,0);check(roster.read(process).players.empty(),"unloaded Park map invalidates cached membership");map.put(Offsets::Instance::Parent,ws.id());
 ballValue.put<addr>(Offsets::Instance::Parent,0);check(!roster.ball(process,members.get(1)),"removed ball assignment rejected");ballValue.put(Offsets::Instance::Parent,attributes.id());
 Instance duplicateSlot("Alice","BoolValue");duplicateSlot.put<unsigned char>(Offsets::Misc::Value,1);two.append(duplicateSlot);roster.scan(process,ws.id(),lookup);check(!roster.read(process).get(1).valid(),"duplicate live roster slots fail closed");
 duplicateSlot.put<unsigned char>(Offsets::Misc::Value,0);check(roster.read(process).get(1).valid(),"clean membership recovers after duplicate clears");
 Instance otherWs("Workspace","Workspace");roster.scan(process,otherWs.id(),lookup);check(!roster.available&&roster.read(process).players.empty(),"non-Park map clears roster and keeps legacy fallback available");
 Instance loadingWs("Workspace","Workspace"),loadingMap("Map","Model"),loadingCourts("Courts","Folder");loadingWs.append(loadingMap);loadingMap.append(loadingCourts);roster.scan(process,loadingWs.id(),lookup);check(roster.available&&roster.read(process).players.empty(),"loading Park fails closed instead of using hoop fallback");
 check(!roster.practiceLayout(process),"empty loading court folder is not confirmed practice");
 Instance practiceCourt("Court1","Model"),halfOne("HalfCourt1","Model"),halfTwo("HalfCourt2","Model"),items("Items","Folder");
 loadingCourts.append(practiceCourt);practiceCourt.append(halfOne);practiceCourt.append(halfTwo);practiceCourt.append(items);
 roster.scan(process,loadingWs.id(),lookup);check(roster.practiceLayout(process),"observed geometry-only practice layout is recognized");
 check(practiceCourtContext(true,true,false,{},100),"opt-in practice context permits absent match metadata");
 check(!practiceCourtContext(false,true,false,{},100),"practice off preserves strict roster behavior");
 check(!practiceCourtContext(true,false,false,{},100),"unknown layout cannot bypass court filtering");
 check(!practiceCourtContext(true,true,true,{},100),"explicit match-state field prevents practice bypass");
 check(!practiceCourtContext(true,true,false,{10,1},100),"active match membership takes priority");
 check(!practiceCourtContext(true,true,false,{},0),"missing local goal prevents practice fallback");
 check(practiceCourtOpponent(true,practiceOpponent("AI1",false,true,true,true),100,100),"same-goal named practice AI remains an opponent");
 check(!practiceCourtOpponent(true,practiceOpponent("AI1",true,true,true,true),100,100),"human named like a bot never gets practice override");
 check(!practiceCourtOpponent(true,practiceOpponent("Dummy",false,true,true,true),100,100),"unrelated NPC is not a practice opponent");
 check(!practiceCourtOpponent(true,true,100,101),"another court goal cannot join the practice exception");
 halfOne.put<addr>(Offsets::Instance::Parent,0);check(!roster.practiceLayout(process),"stale practice hierarchy is rejected without a rescan");halfOne.put(Offsets::Instance::Parent,practiceCourt.id());
 Instance loadingGame("Game","Folder");practiceCourt.append(loadingGame);check(!roster.practiceLayout(process),"even an incomplete match Game disables practice override");
 // Reproduce the actual failure through Reader -> Planner, using this process's
 // synthetic instance memory. No game process or real input is involved.
 loadingGame.name="Unused";
 Instance root("Root","DataModel"),playerList("Players","Players"),local("Local","Player"),characters("Characters","Folder"),camera("Camera","Camera"),balls("Basketballs","Folder");
 root.append(loadingWs);root.append(playerList);playerList.append(local);loadingWs.append(characters);loadingWs.append(camera);loadingWs.append(balls);
 Matrix3 identity{{1,0,0,0,1,0,0,0,1}};camera.put(Offsets::Camera::Rotation,identity);loadingWs.put(Offsets::Workspace::CurrentCamera,camera.id());loadingWs.put<float>(Offsets::Workspace::ReadOnlyGravity,45);
 Instance own("Local","Model"),ai("AI1","Model"),ownRoot("HumanoidRootPart","Part"),aiRoot("HumanoidRootPart","Part"),ownHum("Humanoid","Humanoid"),aiHum("Humanoid","Humanoid"),ownPrim("","Primitive"),aiPrim("","Primitive"),ownAttrs("Attributes","Folder"),aiAttrs("Attributes","Folder"),ownGoal("Goal","ObjectValue"),aiGoal("Goal","ObjectValue"),rim("Rim","Part"),rimPrim("","Primitive");
 characters.append(own);characters.append(ai);own.append(ownRoot);own.append(ownHum);own.append(ownAttrs);ownAttrs.append(ownGoal);ai.append(aiRoot);ai.append(aiHum);ai.append(aiAttrs);aiAttrs.append(aiGoal);
 ownRoot.put(Offsets::BasePart::Primitive,ownPrim.id());aiRoot.put(Offsets::BasePart::Primitive,aiPrim.id());rim.put(Offsets::BasePart::Primitive,rimPrim.id());rimPrim.put(Offsets::Primitive::Position,Vec{0,10,15});ownGoal.put(Offsets::Misc::Value,rim.id());aiGoal.put(Offsets::Misc::Value,rim.id());
 ownPrim.put(Offsets::Primitive::Position,Vec{0,3,8});aiPrim.put(Offsets::Primitive::Position,Vec{0,3,0});ownPrim.put(Offsets::Primitive::Rotation,identity);aiPrim.put(Offsets::Primitive::Rotation,identity);
 for(auto hum:{&ownHum,&aiHum}){hum->put<float>(Offsets::Humanoid::Health,100);hum->put<float>(Offsets::Humanoid::Walkspeed,16);hum->put<float>(Offsets::Humanoid::HipHeight,2);hum->put<int>(Offsets::Humanoid::FloorMaterial,1);}
 ownPrim.put(Offsets::Primitive::Size,Vec{2,2,2});aiPrim.put(Offsets::Primitive::Size,Vec{2,2,2});
 Instance practiceBall("Ball","Part"),ballPrim("","Primitive"),ballAttrs("Attributes","Folder"),ownerValue("Character","ObjectValue");balls.append(practiceBall);practiceBall.append(ballAttrs);ballAttrs.append(ownerValue);practiceBall.put(Offsets::BasePart::Primitive,ballPrim.id());ballPrim.put(Offsets::Primitive::Position,Vec{0,3,1});ownerValue.put(Offsets::Misc::Value,ai.id());
 sceneRoot=root.id();sceneOwn=own.id();scenePlayer=local.id();gamePid=GetCurrentProcessId();options[PracticeAI].value=1;
 Reader reader;auto frame=reader.read(100);
 check(frame.scene.valid&&!frame.scene.playingKnown&&frame.scene.actors.size()==1&&frame.scene.actors[0].enemy,"reader retains practice AI despite Courts container");
 check(frame.scene.balls.size()==1&&frame.scene.balls[0].owner==ai.id(),"reader retains eligible AI possession");
 Config config;config.values[Guard]=1;config.values[Reaction]=0;Intent intent;intent.focused=intent.activation=true;Planner planner;
 auto plan=planner.step(config,frame.scene,intent,100);check(plan.target==ai.id()&&plan.moving,"real reader output can drive AI guarding plan");
 ownerValue.put<addr>(Offsets::Misc::Value,0);frame=reader.read(100.02);check(frame.scene.balls.size()==1&&frame.scene.balls[0].previousOwner==ai.id(),"continuous observed AI release remains scoped");
 ownerValue.put<addr>(Offsets::Misc::Value,12345);frame=reader.read(100.04);check(frame.scene.balls.empty(),"unrecognized owner is excluded");
 ownerValue.put<addr>(Offsets::Misc::Value,0);frame=reader.read(100.06);check(frame.scene.balls.empty(),"unknown owner cannot reuse earlier AI flight history");
 ownerValue.put(Offsets::Misc::Value,ai.id());reader.read(100.08);ownerValue.put<addr>(Offsets::Misc::Value,0);frame=reader.read(101);check(frame.scene.balls.empty(),"observation gap cannot invent a practice release");
 options[PracticeAI].value=0;frame=reader.read(101.02);check(frame.scene.actors.empty()&&frame.scene.balls.empty()&&frame.scene.playingKnown&&!frame.scene.playing,"practice switch off restores spectator exclusion immediately");
 options[PracticeAI].value=1;ownerValue.put(Offsets::Misc::Value,ai.id());frame=reader.read(101.04);check(frame.scene.actors.size()==1,"practice switch recovers without stale membership");
 loadingGame.name="Game";frame=reader.read(101.06);check(frame.scene.actors.empty()&&frame.scene.balls.empty()&&frame.scene.playingKnown&&!frame.scene.playing,"match layout appearing cancels practice override immediately");
 // Private-gym humans share the same hoop and no global Teams/rosters. Use
 // real Reader -> Planner fixture data, independent of the AI practice switch.
 loadingGame.name="Unused";ai.name="Friend";Instance friendPlayer("Friend","Player");playerList.append(friendPlayer);options[PracticeAI].value=0;
 frame=reader.read(102);check(frame.scene.actors.size()==1&&frame.scene.actors[0].enemy&&!frame.scene.playingKnown,"same-hoop human practice partner survives gym filtering with AI option off");
 check(frame.scene.balls.size()==1&&frame.scene.balls[0].owner==ai.id(),"gym partner possession is retained");
 planner={};plan=planner.step(config,frame.scene,intent,102);check(plan.target==ai.id()&&plan.moving,"human gym scene reaches guard planner");
 local.put<addr>(Offsets::Player::Team,42);friendPlayer.put<addr>(Offsets::Player::Team,42);frame=reader.read(102.02);
 check(frame.scene.actors.empty()&&frame.scene.balls.empty(),"known gym teammate is rejected immediately");
 local.put<addr>(Offsets::Player::Team,0);friendPlayer.put<addr>(Offsets::Player::Team,0);aiGoal.put<addr>(Offsets::Misc::Value,12345);frame=reader.read(102.04);
 check(frame.scene.actors.empty(),"different or missing explicit hoop cannot become a practice partner");aiGoal.put(Offsets::Misc::Value,rim.id());
 Instance friendInGame("InGame","BoolValue");aiAttrs.append(friendInGame);frame=reader.read(102.06);check(frame.scene.actors.empty(),"match-state presence prevents human practice override");friendInGame.name="Unused";
 frame=reader.read(102.08);check(frame.scene.actors.size()==1,"gym partner recovers after practice metadata returns");
 loadingGame.name="Game";frame=reader.read(102.10);check(frame.scene.actors.empty()&&frame.scene.playingKnown&&!frame.scene.playing,"appearing match layout immediately restores strict roster gate for humans");
 for(int bad=0;bad<6;bad++)check(!gymPracticePartner(bad!=0,bad!=1,bad==2,100,bad==3?101:100,bad>=4?9:0,bad==4?9:0),"gym policy rejects incomplete or friendly assignments");
 reader.close();gamePid=0;sceneRoot=sceneOwn=scenePlayer=0;
 std::cout<<count<<" court checks passed\n";
}
