#define NOMINMAX
#include "engine.hpp"
#include <iostream>
#include <cstdlib>
using namespace defense;
static std::vector<std::pair<int,bool>> sent;static int checks=0;
static bool record(int k,bool down){sent.push_back({k,down});return true;}
static void check(bool ok,const char*why){++checks;if(!ok){std::cerr<<"FAIL: "<<why<<"\n";exit(1);}}
static bool saw(int k,bool down){return std::find(sent.begin(),sent.end(),std::make_pair(k,down))!=sent.end();}
int main(){inputSender=record;Config c;Scene s;s.valid=true;s.at=10;s.cameraForward={0,0,-1};s.cameraRight={1,0,0};Intent i;i.focused=true;Decision d;d.moving=true;d.direction={0,0,-1};d.sprint=d.stance=true;Inputs inputs;
 inputs.apply(c,s,i,d,10);check(saw('W',true)&&saw(VK_LSHIFT,true)&&saw('G',true),"movement, sprint and stance down");sent.clear();inputs.apply(c,s,i,d,10.02);check(sent.empty(),"held inputs are not spammed");
 inputs.apply(c,s,i,{},10.03);check(saw('W',false)&&saw(VK_LSHIFT,false)&&saw('G',false),"disengagement releases all owned keys");
 sent.clear();physicalEdge('W',true);d.direction={0,0,1};inputs.apply(c,s,i,d,10.04);check(!saw('S',true),"never fights physical W with S");inputs.release();check(!saw('W',false),"physical W is preserved");physicalEdge('W',false);
 sent.clear();d={};d.jump=true;inputs.apply(c,s,i,d,10.05);check(saw(VK_SPACE,true),"jump pulse starts");sent.clear();inputs.apply(c,s,i,{},10.08);check(!saw(VK_SPACE,false),"pulse lasts long enough for game input");s.at=10.13;inputs.apply(c,s,i,{},10.13);check(saw(VK_SPACE,false),"jump pulse ends");
 sent.clear();physicalEdge('R',true);d={};d.steal=true;inputs.apply(c,s,i,d,10.14);check(sent.size()==2&&sent[0]==std::make_pair((int)'R',false)&&sent[1]==std::make_pair((int)'R',true),"held interception key retriggers catch at opportunity");physicalEdge('R',false);
 sent.clear();d={};d.moving=true;d.direction={1,0,0};inputs.apply(c,s,i,d,10.15);check(saw('D',true),"camera-relative direction mapped");i.focused=false;inputs.apply(c,s,i,d,10.16);check(saw('D',false),"focus loss releases owned movement");i.focused=true;
 sent.clear();pick=true;inputs.apply(c,s,i,d,10.17);check(sent.empty(),"selection mode pauses motion");pick=false;i.chat=true;inputs.apply(c,s,i,d,10.18);check(sent.empty(),"chat pauses motion");i.chat=false;inputs.apply(c,s,i,d,10.5);check(sent.empty(),"stale scene cannot inject");
 s.at=11;d={};d.moving=true;d.direction={.4f,0,-.9165f};sent.clear();inputs.apply(c,s,i,d,11);check(saw('D',true),"lateral axis engages above upper threshold");
 sent.clear();d.direction={.3f,0,-.9539f};inputs.apply(c,s,i,d,11.02);check(sent.empty(),"retention band prevents diagonal key chatter");
 d.direction={.2f,0,-.9798f};inputs.apply(c,s,i,d,11.04);check(saw('D',false),"lateral axis releases below lower threshold");
 sent.clear();d.direction={-.4f,0,-.9165f};inputs.apply(c,s,i,d,11.06);check(saw('A',true)&&!inputs.owned['D'],"direction reversal never owns both lateral keys");
 sent.clear();inputs.apply(c,s,i,{},11.08);check(saw('W',false)&&saw('A',false),"settling releases movement without delayed key retention");
 sent.clear();s.at=12;d={};d.moving=d.retreat=true;d.basketAxis={0,0,1};d.direction=unit(Vec{.6f,0,1});inputs.apply(c,s,i,d,12);
 check(saw('S',true)&&!inputs.owned['A']&&inputs.owned['D'],"script steering retains the lateral correction toward the basket line");
 sent.clear();i.focused=false;inputs.apply(c,s,i,d,12.02);check(saw('S',false),"retreat releases immediately on focus loss");
 // End-to-end planner -> input adapter: WASD must not suppress a valid block.
 inputs.release();sent.clear();s={};s.valid=s.grounded=true;s.at=20;s.own.id=1;s.own.p={0,3,0};s.own.facing={0,0,-1};
 Actor shooter;shooter.id=2;shooter.enemy=true;shooter.p={0,3,-4};shooter.shotMeterActive=shooter.shotMeterAdvancing=true;shooter.shotMeterFill=.45f;shooter.shotMeterAge=.3;s.actors.push_back(shooter);
 Ball charging;charging.id=3;charging.owner=2;charging.p={0,6.5f,-3};charging.heldRiseSpeed=4;s.balls.push_back(charging);
 c.values[Block]=1;i={};i.focused=i.manual=true;physicalEdge('W',true);Planner planner;
 auto block=planner.step(c,s,i,20);check(block.jump,"moving defender planner requests late-charge block");
 inputs.apply(c,s,i,block,20);check(saw(VK_SPACE,true),"late-charge block reaches jump input adapter");
 sent.clear();i.focused=false;inputs.apply(c,s,i,{},20.02);check(saw(VK_SPACE,false)&&!saw('W',false),"focus loss releases jump without releasing physical movement");physicalEdge('W',false);
 // Planner -> ordinary input path: post crowding backs away, then releases on key-up.
 inputs.release();sent.clear();s={};s.valid=s.grounded=true;s.at=30;s.own.id=1;s.own.p={0,3,4.25f};
 s.cameraForward={0,0,-1};s.cameraRight={1,0,0};Actor post;post.id=2;post.enemy=post.goalValid=true;post.p={0,3,0};post.goal={0,10,15};post.facing={0,0,-1};s.actors.push_back(post);
 Ball held;held.id=3;held.owner=2;held.p={0,3,-1};s.balls.push_back(held);
 c=Config{};c.values[Guard]=1;c.values[Reaction]=0;i={};i.focused=i.activation=true;planner={};
 for(int n=0;n<12;n++){s.at+=.02;planner.step(c,s,i,s.at);}
 s.own.p.z=3;s.at+=.02;auto backdown=planner.step(c,s,i,s.at);inputs.apply(c,s,i,backdown,s.at);
 check(saw('S',true)&&!saw('W',true)&&!saw(VK_LSHIFT,true)&&!saw(VK_SPACE,true),"anti-post retreats through normal input without sprint/jump");
 sent.clear();i.activation=false;s.at+=.02;inputs.apply(c,s,i,planner.step(c,s,i,s.at),s.at);
 check(saw('S',false)&&saw('G',false),"anti-post releases movement and stance with activation");
 inputs.release();sent.clear();planner={};s.at=40;s.own.p={0,3,8};s.actors[0].p={0,3,0};s.actors[0].goal={0,10,30};s.actors[0].v={};s.actors[0].facing={0,0,1};c.values[Reaction]=500;i.activation=true;
 for(int n=0;n<4;n++){s.at+=.02;planner.step(c,s,i,s.at);}
 s.at+=.02;s.actors[0].p.z=-.16f;s.actors[0].v={0,0,-8};s.balls[0].p=s.actors[0].p+Vec{0,0,-1};
 auto fade=planner.step(c,s,i,s.at);inputs.apply(c,s,i,fade,s.at);
 check(fade.reason=="Pressuring sudden backward cut"&&saw('W',true)&&!saw('S',true)&&!saw(VK_SPACE,true),"backward cut reaches forward movement without waiting or jumping");
 sent.clear();i.activation=false;s.at+=.02;inputs.apply(c,s,i,planner.step(c,s,i,s.at),s.at);
 check(saw('W',false)&&saw('G',false),"fade pursuit releases immediately when activation ends");
 std::cout<<checks<<" input-ownership checks passed (mock input only)\n";
}

