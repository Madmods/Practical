#define NOMINMAX
#include "engine.hpp"
#include "launch_flow.hpp"
#include <iostream>
#include <cstdlib>
static int checks=0;static std::vector<std::pair<int,bool>> sent;
static void check(bool ok,const char* why){++checks;if(!ok){std::cerr<<"FAIL: "<<why<<"\n";std::exit(1);}}
static bool record(int key,bool down){sent.push_back({key,down});return true;}
int main(){
 LaunchFlow flow;
 check(!flow.proceed(true),"cannot skip sign-in");flow.tick(1.2f,false,false);check(flow.stage==LaunchFlow::Intro,"intro stays visible during loading");flow.tick(1.41f,false,false);
 check(flow.stage==LaunchFlow::Login,"intro ends at login");
 check(!flow.submit(L"1",L"wrong")&&flow.invalid,"invalid credentials rejected");
 check(flow.submit(L"1",L"1"),"preview credentials accepted");
 check(!flow.proceed(true),"verification must complete before continuing");
 flow.tick(.5f,false,false);check(flow.stage==LaunchFlow::Connect,"login advances to connection");
 check(!flow.proceed(false),"cannot continue without Roblox data");
 check(flow.proceed(true),"connected client can continue");flow.tick(.25f,false,false);
 check(flow.stage==LaunchFlow::Connect,"connection loss during transition returns to connection screen");
 flow.proceed(true);flow.tick(.51f,true,false);check(flow.stage==LaunchFlow::Ready,"workspace opens after connection");
 check(!flow.submit(L"1",L"1"),"workspace cannot restart sign-in accidentally");
 LaunchFlow reduced;reduced.tick(.11f,false,true);check(reduced.stage==LaunchFlow::Login,"reduced motion skips extended intro");
 ShotBindingRouter router;
 check(router.observe('E',true,true)==ShotBindingRouter::Start,"E starts a shot");
 check(router.observe(VK_SPACE,true,true)==ShotBindingRouter::Ignore,"Space cannot take over a held E shot");
 check(router.observe(VK_SPACE,false,true)==ShotBindingRouter::Ignore,"unrelated key-up cannot end E shot");
 check(router.observe('E',true,true)==ShotBindingRouter::Repeat,"E repeat remains same shot");
 check(router.observe('E',false,true)==ShotBindingRouter::Release,"E release finishes its shot");
 check(router.observe(VK_SPACE,true,true)==ShotBindingRouter::Start,"Space starts next shot");
 check(router.observe('E',true,true)==ShotBindingRouter::Ignore,"E cannot take over Space shot");
 check(router.observe(VK_SPACE,false,true)==ShotBindingRouter::Release,"Space release finishes its shot");
 check(router.observe('E',true,true)==ShotBindingRouter::Ignore,"key already held cannot start on autorepeat");
 router.observe('E',false,true);
 check(router.observe('E',true,false)==ShotBindingRouter::Ignore,"input while locked passes through");
 check(router.observe('E',true,true)==ShotBindingRouter::Ignore,"unlocking while held cannot start shot");
 router.observe('E',false,true);
 check(router.observe('E',true,true)==ShotBindingRouter::Start,"fresh press after unlock works");
 shotSender=record;
 activeShotKey=VK_SPACE;sendRelease();
 check(sent.back()==std::make_pair((int)VK_SPACE,false),"automatic release targets Space shot, not primary E");
 activeShotKey='E';sendRelease();check(sent.back()==std::make_pair((int)'E',false),"automatic release targets E shot");
 shootKey='Q';activeShotKey=VK_SPACE;sendRelease();check(sent.back().first==VK_SPACE,"current shot release remains frozen across binding changes");
 check(isShotKey('Q')&&isShotKey(VK_SPACE)&&!isShotKey('E'),"both current bindings recognized");
 shootKeyAlt=VK_XBUTTON1;check(isShotKey(VK_XBUTTON1)&&!isShotKey(VK_SPACE),"secondary supports mouse side button");
 std::cout<<checks<<" launcher and multi-input checks passed (mock input only)\n";
}
