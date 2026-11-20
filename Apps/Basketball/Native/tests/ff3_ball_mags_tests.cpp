#define NOMINMAX
#include "../src/ff3_ball_edits.hpp"
#include "../src/ff3_collision_logic.hpp"
#include <iostream>
int main(){
 using namespace ff3Play;unsigned checks=0;auto check=[&](bool ok,const char* reason){++checks;if(!ok){std::cerr<<"FAIL "<<checks<<": "<<reason<<'\n';std::exit(1);}};
 auto ball=[](std::uint64_t id){return BallPart{{id,2000,3000+id,{.85,1.2,.85},true,4000+id},{.7,.9,.7}};};
 std::vector<std::string> sizeOrder;std::vector<BallPart> live{ball(1000)};BallEdits edits;unsigned writes=0,sizeWrites=0,meshWrites=0;bool readable=true,failMesh=false,failRestore=false;std::uint64_t badRead=0,badWrite=0;
 BallEdits::Read read=[&](const auto& seed,auto& out){if(!readable||seed.part.id==badRead)return false;for(const auto& p:live)if(p.part.id==seed.part.id){out=p;return true;}out={};return true;};
 BallEdits::Write write=[&](const auto& seed,auto field,Vec value){
  for(auto& p:live)if(p.part.id==seed.part.id&&p.part.primitive==seed.part.primitive){
   if(seed.part.id==badWrite)return false;
   if(field==BallEdits::Field::MeshScale){if(p.part.mesh!=seed.part.mesh||failMesh)return false;p.meshScale=value;++meshWrites;}
   else{if(failRestore&&sameSize(value,seed.part.size))return false;p.part.size=value;sizeOrder.push_back(sameSize(value,seed.part.size)?"original":"enlarged");++sizeWrites;}++writes;return true;
  }return false;
 };
 std::vector<std::string> order;bool failCollision=false;unsigned collisionWrites=0;
 BallEdits::CollisionWrite collisionWrite=[&](const BallPart& seed,bool enabled){if(failCollision)return false;for(auto& p:live)if(p.part.id==seed.part.id&&p.part.primitive==seed.part.primitive){p.canCollide=enabled;order.push_back(enabled?"collision-on":"collision-off");sizeOrder.push_back(enabled?"collision-on":"collision-off");++collisionWrites;return true;}return false;};
 const auto original=live[0];const Vec twenty{20,20,20};
 check(edits.apply(live,twenty,true,read,write),"one live ball is sufficient");
 check(sameSize(live[0].part.size,twenty),"ball geometry is exactly 20 studs on every axis");
 check(sameSize(live[0].meshScale,ballMeshScale(original,twenty)),"mesh scales from original dimensions");
 auto count=writes;check(edits.apply(live,twenty,true,read,write)&&writes==count,"stable frames do not rewrite or compound scaling");
 check(sameSize(edits.edits[0].original.part.size,original.part.size)&&sameSize(edits.edits[0].original.meshScale,original.meshScale),"original geometry and mesh survive repeated frames");
 check(edits.apply(live,{10,10,10},true,read,write)&&sameSize(live[0].meshScale,ballMeshScale(original,{10,10,10})),"slider changes recompute from originals");
 check(edits.apply(live,twenty,false,read,write)&&sameSize(live[0].meshScale,original.meshScale)&&sameSize(live[0].part.size,twenty),"disabling visual restores the mesh while retaining selected ball geometry");
 check(edits.apply(live,twenty,true,read,write),"visible football can be re-enabled");
 live[0].part.rig=9000;check(edits.apply(live,twenty,true,read,write)&&sameSize(edits.edits[0].original.part.size,original.part.size),"holding or handing off the same football retains originals");
 check(edits.restore(read,write,collisionWrite)&&sameSize(live[0].part.size,original.part.size)&&sameSize(live[0].meshScale,original.meshScale),"disable restores both fields after reparenting");
 live={original};check(edits.apply(live,twenty,true,read,write),"acquire for external size update");live[0].part.size={3,4,5};
 check(edits.restore(read,write,collisionWrite)&&sameSize(live[0].part.size,{3,4,5})&&sameSize(live[0].meshScale,original.meshScale),"server size update is preserved while our mesh is restored");
 live={original};edits.apply(live,twenty,true,read,write);live[0].meshScale={3,4,5};
 check(edits.restore(read,write,collisionWrite)&&sameSize(live[0].part.size,original.part.size)&&sameSize(live[0].meshScale,{3,4,5}),"server mesh update is preserved while our geometry is restored");
 live={original};failMesh=true;check(!edits.apply(live,twenty,true,read,write)&&sameSize(live[0].part.size,original.part.size),"failed mesh write rolls back the preceding size write");failMesh=false;check(edits.restore(read,write,collisionWrite)&&edits.edits.empty(),"partial failure leaves no outstanding writes");
 live={original};failMesh=failRestore=true;check(!edits.apply(live,twenty,true,read,write)&&!edits.edits.empty()&&edits.edits[0].sizeOwned,"failed rollback retains size ownership for retry");failMesh=failRestore=false;
 check(edits.restore(read,write,collisionWrite)&&sameSize(live[0].part.size,original.part.size),"rollback retry restores saved original");
 edits.apply(live,twenty,true,read,write);readable=false;check(!edits.restore(read,write,collisionWrite)&&!edits.edits.empty(),"transient read failure retains recovery data");readable=true;check(edits.restore(read,write,collisionWrite),"restoration resumes when reads recover");
 edits.apply(live,twenty,true,read,write);++live[0].part.primitive;count=writes;check(edits.restore(read,write,collisionWrite)&&writes==count,"reused primitive is never overwritten");
 live={original};edits.apply(live,twenty,true,read,write);++live[0].part.mesh;live[0].meshScale={2,3,4};
 check(edits.restore(read,write,collisionWrite)&&sameSize(live[0].part.size,original.part.size)&&sameSize(live[0].meshScale,{2,3,4}),"mesh replacement restores part geometry without writing the replacement mesh");
 live={original};edits.apply(live,twenty,true,read,write);check(edits.apply({},twenty,true,read,write)&&edits.edits.empty()&&sameSize(live[0].meshScale,original.meshScale),"disappearing from selection restores owned fields");
 auto hand=original;hand.part.ball=false;count=writes;check(!edits.apply({hand},twenty,true,read,write)&&writes==count,"catch parts are rejected before any writes");
 check(!edits.apply({original,original},twenty,true,read,write)&&writes==count,"duplicate selection is rejected atomically");
 check(!edits.apply({original},{51,20,20},true,read,write)&&writes==count,"invalid target size cannot reach the writer");
 auto invalid=original;invalid.meshScale={NAN,1,1};live={invalid};check(!edits.apply(live,twenty,true,read,write),"unreadable mesh scale cannot create a recovery record");
 live.clear();for(unsigned i=0;i<maxBallEdits;++i)live.push_back(ball(1000+i));check(edits.apply(live,twenty,true,read,write)&&edits.edits.size()==maxBallEdits,"all 64 discovered balls can be tracked independently");
 count=writes;auto tooMany=live;tooMany.push_back(ball(9999));check(!edits.apply(tooMany,twenty,true,read,write)&&writes==count,"discovery budget cannot overflow recovery capacity");check(edits.restore(read,write,collisionWrite)&&edits.edits.empty(),"all discovered ball records restore");
 live={original};edits.apply(live,twenty,true,read,write);live[0].part.size=original.part.size;live[0].meshScale=original.meshScale;
 check(edits.apply(live,twenty,true,read,write)&&sameSize(live[0].part.size,twenty)&&sameSize(live[0].meshScale,ballMeshScale(original,twenty)),"persistent mode reapplies a replicated reset without adopting it as the original");
 live.clear();check(edits.restore(read,write,collisionWrite)&&edits.edits.empty(),"destroyed ball releases its record");
 live={original};auto priorMeshWrites=meshWrites;check(edits.applyGeometry(live,twenty,read,write,collisionWrite)&&sameSize(live[0].meshScale,original.meshScale)&&meshWrites==priorMeshWrites,"external visual mode never expands the football mesh");
 check(edits.applyGeometry(live,{30,30,30},read,write,collisionWrite)&&sameSize(live[0].meshScale,original.meshScale)&&meshWrites==priorMeshWrites,"reach slider changes leave visible football scale unchanged");check(edits.restore(read,write,collisionWrite),"geometry-only disable restores the original size");
 edits.apply(live,twenty,true,read,write);check(edits.applyGeometry(live,twenty,read,write,collisionWrite)&&sameSize(live[0].meshScale,original.meshScale)&&sameSize(live[0].part.size,twenty),"upgrade from old visible scaling restores mesh and retains reach geometry");edits.restore(read,write,collisionWrite);
 live={original};order.clear();sizeOrder.clear();auto countCollision=collisionWrites;auto countSize=sizeWrites;
 check(edits.applyGeometry(live,twenty,read,write,collisionWrite)&&!live[0].canCollide&&edits.edits[0].collisionOwned,"enlarged ball has solid collision disabled");check(collisionWrites==countCollision+1&&sizeWrites==countSize+1&&sizeOrder.size()>=2&&sizeOrder[0]=="collision-off"&&sizeOrder[1]=="enlarged","collision write precedes the size write");
 check(edits.restore(read,write,collisionWrite)&&live[0].canCollide&&sameSize(live[0].part.size,original.part.size),"disable restores normal dimensions and original collision");check(sizeOrder.size()>=4&&sizeOrder[sizeOrder.size()-2]=="original"&&sizeOrder.back()=="collision-on","collision restoration occurs after normal size restoration");
 live={original};live[0].canCollide=false;countCollision=collisionWrites;check(edits.applyGeometry(live,twenty,read,write,collisionWrite)&&edits.restore(read,write,collisionWrite)&&!live[0].canCollide&&collisionWrites==countCollision,"naturally non-colliding football remains non-colliding after disable");
 live={original};failCollision=true;countSize=sizeWrites;check(!edits.applyGeometry(live,twenty,read,write,collisionWrite)&&sizeWrites==countSize&&sameSize(live[0].part.size,original.part.size),"collision failure blocks resizing entirely");failCollision=false;
 edits.applyGeometry(live,twenty,read,write,collisionWrite);failRestore=true;check(!edits.restore(read,write,collisionWrite)&&!live[0].canCollide&&edits.edits[0].collisionOwned,"failed size restoration cannot re-enable collision on an enlarged ball");failRestore=false;check(edits.restore(read,write,collisionWrite)&&live[0].canCollide,"collision returns after a successful restoration retry");
 live={original};edits.applyGeometry(live,twenty,read,write,collisionWrite);live[0].canCollide=true;check(edits.applyGeometry(live,twenty,read,write,collisionWrite)&&!live[0].canCollide,"replicated collision reset is cleared on the next frame");
 live[0].canCollide=true;failRestore=true;check(!edits.restore(read,write,collisionWrite)&&!live[0].canCollide,"rollback clears a collision reset before trying to shrink");failRestore=false;edits.restore(read,write,collisionWrite);
 live={original};edits.applyGeometry(live,twenty,read,write,collisionWrite);live[0].part.size={3,4,5};check(!edits.restore(read,write,collisionWrite)&&!live[0].canCollide&&sameSize(live[0].part.size,{3,4,5}),"external size changes are preserved without restoring solid collision prematurely");live[0].part.size=original.part.size;check(edits.restore(read,write,collisionWrite)&&live[0].canCollide,"pending collision restoration completes when normal geometry returns");
 live={original};live[0].part.size=twenty;live[0].canCollide=false;edits.edits.push_back({original,twenty,original.meshScale,false,true,false,true});check(edits.restore(read,write,collisionWrite)&&live[0].canCollide&&sameSize(live[0].part.size,original.part.size),"recovered collision journal restores size before collision");
 live={original};countSize=sizeWrites;
 check(!edits.applyGeometry(live,twenty,read,write,[](const BallPart&,bool){return true;})&&sizeWrites==countSize&&edits.edits.empty(),"successful callback without collision readback cannot enlarge a solid ball");
 {live={original};BallEdits::Write race=[&](const auto& seed,auto field,Vec value){bool ok=write(seed,field,value);if(field==BallEdits::Field::Size&&sameSize(value,twenty))live[0].canCollide=true;return ok;};
 check(!edits.applyGeometry(live,twenty,read,race,collisionWrite)&&sameSize(live[0].part.size,original.part.size)&&edits.edits.empty(),"collision reset during resizing rolls geometry back in the same update");}
 live={original};edits.applyGeometry(live,twenty,read,write,collisionWrite);live[0].part.size={3,4,5};live[0].canCollide=true;
 check(!edits.restore(read,write,collisionWrite)&&!live[0].canCollide&&!edits.edits.empty(),"external size and collision reset keeps collision disabled until normal dimensions return");live[0].part.size=original.part.size;check(edits.restore(read,write,collisionWrite)&&live[0].canCollide,"external reset recovery finishes after normal dimensions return");
 bool preserved=true;for(unsigned flags=0;flags<256;++flags)for(bool on:{false,true}){auto changed=collisionFlag(static_cast<unsigned char>(flags),8,on);preserved=preserved&&((changed&~8)==(flags&~8))&&bool(changed&8)==on;}check(preserved,"all flag patterns preserve CanTouch, CanQuery, Anchored and every unrelated bit");
 {Scene receiver;receiver.player=3;receiver.own=4;receiver.head={0,7,0};ff3qb::BallFlight flight;flight.id=9;flight.airborne=true;flight.at=10;flight.position={0,8,5};flight.velocity={0,-10,-10};flight.gravity=28;
 check(receivingBallCandidate(receiver,flight,0,10),"airborne pass with sufficient floor clearance can enlarge");flight.airborne=false;check(!receivingBallCandidate(receiver,flight,0,10),"stationary floor ball cannot become non-solid");flight.airborne=true;flight.position.y=1;check(!receivingBallCandidate(receiver,flight,0,10),"ball near ground keeps its normal geometry and collision");flight.position.y=8;flight.velocity.y=-50;check(!receivingBallCandidate(receiver,flight,0,10),"fast descending ball restores collision ahead of impact");flight.velocity.y=-10;check(!receivingBallCandidate(receiver,flight,NAN,10),"unknown field height withholds non-solid resizing");receiver.possessed=true;check(receivingBallCandidate(receiver,flight,0,10),"holding one ball does not disable enlargement on other airborne balls");receiver.possessed=false;flight.thrower=receiver.player;check(receivingBallCandidate(receiver,flight,0,10),"released practice throws stay eligible regardless of thrower identity");flight.thrower=0;check(receivingBallCandidate(receiver,flight,0,10),"unattributed practice throw can use motion and floor clearance");check(!receivingBallCandidate(receiver,flight,0,10.2),"stale motion cannot override the landing guard");
 }
 live={ball(1000),ball(1001),ball(1002)};badWrite=1001;check(!edits.applyGeometry(live,twenty,read,write,collisionWrite),"one unavailable ball reports a partial update");check(sameSize(live[0].part.size,twenty)&&sameSize(live[2].part.size,twenty)&&sameSize(live[1].part.size,original.part.size),"a failed ball cannot reset healthy balls");badWrite=0;check(edits.applyGeometry(live,twenty,read,write,collisionWrite)&&edits.edits.size()==3,"failed ball joins the others after recovery");badRead=1001;check(!edits.applyGeometry(live,twenty,read,write,collisionWrite)&&sameSize(live[0].part.size,twenty)&&sameSize(live[2].part.size,twenty),"transient read failure is isolated to that ball");badRead=0;check(edits.restore(read,write,collisionWrite)&&edits.edits.empty(),"all independently managed balls restore");
 {Scene r;r.head={0,7,0};ff3qb::BallFlight b;b.id=9;b.airborne=true;b.at=10;b.position={150,8,0};b.velocity={0,-10,0};b.gravity=28;check(receivingBallCandidate(r,b,0,10),"replicated distant balls are not dropped by a nearest-target restriction");}
 {
  // Exercise the same orchestration used by the worker with real edit logic and
  // a mock process containing both a football and two character catch parts.
  struct Editor {
   BallEdits& edits;const BallEdits::Read& read;const BallEdits::Write& write;const BallEdits::CollisionWrite& collisionWrite;
   unsigned applications=0,restorations=0;
   bool apply(const Scene&,const std::vector<BallPart>& balls){++applications;return edits.applyGeometry(balls,{10,10,10},read,write,collisionWrite);}
   bool restore(){++restorations;return edits.restore(read,write,collisionWrite);}
  } editor{edits,read,write,collisionWrite};
  auto left=ball(7000),right=ball(7001);left.part.ball=right.part.ball=false;left.part.size=right.part.size={1.4,1.65,1.4};
  live={original,left,right};std::vector<BallPart> selected{original};
  Scene scene;scene.valid=scene.focused=true;scene.menu=scene.typing=false;scene.pid=1;scene.model=2;scene.own=3;scene.at=10;
  check(updateBallMags(editor,scene,selected,true,10),"worker gate invokes ball sizing for focused fresh gameplay");
  check(sameSize(live[0].part.size,{10,10,10})&&sameSize(live[0].meshScale,original.meshScale),"worker enlarges football hitbox with normal visible mesh");
  check(sameSize(live[1].part.size,left.part.size)&&sameSize(live[2].part.size,right.part.size)&&edits.edits.size()==1&&edits.edits[0].original.part.ball,"worker cannot resize either hand catch zone");
  auto applications=editor.applications;
  check(updateBallMags(editor,scene,selected,false,10)&&editor.applications==applications&&edits.edits.empty()&&sameSize(live[0].part.size,original.part.size)&&live[0].canCollide,"disable or recovery block restores balls without new applications");
  for(int reason=0;reason<8;++reason){
   check(updateBallMags(editor,scene,selected,true,10),"acquire ball before pause");auto paused=scene;
   switch(reason){case 0:paused.focused=false;break;case 1:paused.menu=true;break;case 2:paused.typing=true;break;case 3:paused.valid=false;break;case 4:paused.at=9;break;case 5:paused.at=11;break;case 6:paused.pid=0;break;case 7:paused.velocity.x=NAN;break;}
   applications=editor.applications;
   check(updateBallMags(editor,paused,selected,true,10)&&editor.applications==applications&&edits.edits.empty()&&sameSize(live[0].part.size,original.part.size)&&live[0].canCollide,"pause restores original ball size and collision without applying");
  }
  badWrite=original.part.id;check(!updateBallMags(editor,scene,selected,true,10)&&edits.edits.empty()&&live[0].canCollide,"failed runtime update rolls back its partial ball modification");badWrite=0;
  check(sameSize(live[1].part.size,left.part.size)&&sameSize(live[2].part.size,right.part.size),"pause and rollback paths also leave catch zones unchanged");
 }
 {
  CollisionRules rules;rules.masks.fill(0xffffffffu);rules.names[0]="Default";rules.names[5]="NoPlayerCollide";rules.names[6]="Players";rules.masks[5]&=~(1u<<6);rules.masks[6]&=~(1u<<5);
  check(rules.valid()&&rules.choose(0,1u<<6,1)==5,"existing collision matrix selects non-player group while preserving floor and catch groups");
  check(rules.choose(5,1u<<6,1)==5,"a ball already in the correct group keeps that group");
  check(rules.choose(0,1,1)==-1&&rules.choose(32,1u<<6,1)==-1&&rules.choose(0,0,1)==-1,"overlapping floor/body groups and incomplete identities fail closed");
  auto missing=rules;missing.names[5].clear();check(missing.choose(0,1u<<6,1)==-1,"unnamed groups cannot be guessed or reserved in production");
  auto reset=rules;reset.masks[5]|=1u<<6;check(!reset.valid()&&reset.choose(0,1u<<6,1)==-1,"asymmetric or reset collision relationships cannot enable enlargement");
  auto blocked=rules;blocked.masks[5]&=~1u;blocked.masks[0]&=~(1u<<5);check(blocked.choose(0,1u<<6,1)==-1,"a group that loses floor contact is rejected");
  for(unsigned body=1;body<32;++body){unsigned target=body==31?30:31;CollisionRules each;each.masks.fill(0xffffffffu);each.names[0]="Default";each.names[body]="Players";each.names[target]="NoPlayerCollide";each.masks[target]&=~(1u<<body);each.masks[body]&=~(1u<<target);check(each.choose(0,1u<<body,1)==int(target),"collision selection works across the complete 32-group domain");}
  live={original};sizeOrder.clear();unsigned groupWrites=0;bool failGroupRestore=false;
  BallEdits::GroupWrite groupWrite=[&](const BallPart& seed,unsigned group){for(auto& b:live)if(sameBall(seed,b)){if(failGroupRestore&&group==seed.physicsGroup)return false;b.physicsGroup=group;sizeOrder.push_back(group==seed.physicsGroup?"group-original":"group-filtered");++groupWrites;return true;}return false;};
  BallEdits::GroupChoice choose=[&](const BallPart& b){return rules.choose(b.physicsGroup,1u<<6,1);};
  auto apply=[&](){return edits.applyFilteredGeometry(live,twenty,read,write,groupWrite,choose);};
  auto restoreFiltered=[&](){return edits.restore(read,write,collisionWrite,groupWrite);};
  check(apply()&&live[0].physicsGroup==5&&live[0].canCollide&&sameSize(live[0].part.size,twenty),"ground-capable Mags enlarges the ball with solid collision still enabled");
  check(sizeOrder.size()>=2&&sizeOrder[0]=="group-filtered"&&sizeOrder[1]=="enlarged"&&sameSize(live[0].meshScale,original.meshScale),"verified player filtering precedes geometry while visible mesh stays original");
  auto beforeGroup=groupWrites,beforeSize=sizeWrites;check(apply()&&groupWrites==beforeGroup&&sizeWrites==beforeSize,"stable filtered frames do not rewrite either field");
  check(restoreFiltered()&&live[0].physicsGroup==original.physicsGroup&&live[0].canCollide&&sameSize(live[0].part.size,original.part.size),"disable restores original size and group without disabling floor collision");
  check(sizeOrder[sizeOrder.size()-2]=="original"&&sizeOrder.back()=="group-original","original geometry returns before player collisions return");
  live={original};beforeSize=sizeWrites;check(!edits.applyFilteredGeometry(live,twenty,read,write,[](const BallPart&,unsigned){return true;},choose)&&sizeWrites==beforeSize&&edits.edits.empty(),"a group writer without actual physics readback cannot enlarge a ball");
  live={original};badWrite=original.part.id;check(!apply()&&live[0].physicsGroup==original.physicsGroup&&edits.edits.empty(),"failed size application rolls back the preceding group change");badWrite=0;
  live={original};check(apply(),"acquire filtered ball before restoration failure");failRestore=true;check(!restoreFiltered()&&live[0].physicsGroup==5&&live[0].canCollide&&edits.edits[0].groupOwned,"failed size restoration keeps player collisions filtered");failRestore=false;check(restoreFiltered()&&edits.edits.empty(),"size restoration retry then releases the group");
  live={original};apply();failGroupRestore=true;check(!restoreFiltered()&&sameSize(live[0].part.size,original.part.size)&&edits.edits[0].groupOwned,"failed group restoration retains a lease after geometry is normal");failGroupRestore=false;check(restoreFiltered()&&live[0].physicsGroup==original.physicsGroup,"group restoration resumes without further size changes");
  live={original};BallEdits::Write race=[&](const auto& seed,auto field,Vec value){bool ok=write(seed,field,value);if(field==BallEdits::Field::Size&&sameSize(value,twenty))live[0].physicsGroup=0;return ok;};
  check(!edits.applyFilteredGeometry(live,twenty,read,race,groupWrite,choose)&&sameSize(live[0].part.size,original.part.size)&&live[0].canCollide&&edits.edits.empty(),"group reset during enlargement restores geometry in the same update");
  live={original};beforeGroup=groupWrites;beforeSize=sizeWrites;check(!edits.applyFilteredGeometry(live,twenty,read,write,groupWrite,[](const BallPart&){return -1;})&&groupWrites==beforeGroup&&sizeWrites==beforeSize&&edits.edits.empty(),"missing compatible filter produces no process field writes");
  live={original};live[0].canCollide=false;check(apply()&&sameSize(live[0].part.size,twenty)&&!live[0].canCollide&&live[0].physicsGroup==5,"naturally non-solid balls can enlarge without changing their normal collision flag");check(restoreFiltered()&&!live[0].canCollide&&live[0].physicsGroup==0,"naturally non-solid ball restoration preserves its normal collision state");
  live={original};apply();live[0].physicsGroup=1;check(restoreFiltered()&&live[0].physicsGroup==1&&sameSize(live[0].part.size,original.part.size),"a foreign group change is preserved while owned geometry restores");
  struct PartialEditor {
   BallEdits& edits;const BallEdits::Read& read;const BallEdits::Write& write;const BallEdits::GroupWrite& group;const BallEdits::GroupChoice& choose;unsigned skipped=0;
   bool apply(const Scene&,const std::vector<BallPart>& parts){auto active=eligibleFilteredBalls(parts,read,choose,skipped);return edits.applyFilteredGeometry(active,{20,20,20},read,write,group,choose);}
   bool restore(){return edits.restore(read,write,{},group);}
  } worker{edits,read,write,groupWrite,choose};
  Scene frame;frame.valid=frame.focused=true;frame.typing=frame.menu=false;frame.pid=1;frame.model=2;frame.own=3;frame.at=10;
  live={ball(1000),ball(1001),ball(1002)};live[1].canCollide=false;live[1].physicsGroup=31;badWrite=1002;
  check(!updateBallMags(worker,frame,live,true,10,false),"partial worker update reports an unavailable individual ball");
  check(worker.skipped==1&&edits.edits.size()==1&&sameSize(live[0].part.size,twenty)&&live[0].physicsGroup==5&&live[0].canCollide,"one unverified group and one failed ball cannot roll back a healthy worker ball");
  check(sameSize(live[1].part.size,original.part.size)&&live[1].physicsGroup==31&&!live[1].canCollide&&sameSize(live[2].part.size,original.part.size)&&live[2].physicsGroup==0,"unsupported and failed balls retain their original state");
  badWrite=0;check(updateBallMags(worker,frame,live,true,10,false)&&edits.edits.size()==2&&sameSize(live[2].part.size,twenty),"a recovered ball joins healthy Mags while unverified groups remain skipped");
  check(updateBallMags(worker,frame,live,false,10,false)&&edits.edits.empty()&&live[0].physicsGroup==0&&live[2].physicsGroup==0&&sameSize(live[0].part.size,original.part.size)&&sameSize(live[2].part.size,original.part.size),"disable still restores every healthy record in partial-update mode");
  updateBallMags(worker,frame,live,true,10,false);frame.focused=false;check(updateBallMags(worker,frame,live,true,10,false)&&edits.edits.empty(),"focus loss always restores all balls even after mixed availability");
  frame.focused=true;live={ball(1000),ball(1001),ball(1002)};live[1].canCollide=false;
  check(updateBallMags(worker,frame,live,true,10,false)&&edits.edits.size()==3&&worker.skipped==0,"mixed naturally solid and non-solid balls all participate in Mags");
  check(live[0].canCollide&&!live[1].canCollide&&live[2].canCollide&&sameSize(live[1].part.size,twenty),"mixed batch keeps each ball's game-controlled collision flag intact");
  check(updateBallMags(worker,frame,live,false,10,false)&&edits.edits.empty()&&live[0].canCollide&&!live[1].canCollide&&live[2].canCollide,"mixed collision-state restoration preserves each original baseline");


 }
 std::cout<<checks<<" multi-ball sizing, fault isolation, collision and landing checks passed\n";
}
