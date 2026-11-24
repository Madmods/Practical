#define NOMINMAX
#include "../src/ff3_qb_runtime.hpp"
#include <iostream>
#include <iomanip>
// Bounded host for the production worker; never sends a throw. Match real
// click plans to independent observations of the user's released football.
int main(){
 winrt::init_apartment();HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 2;
 for(int i=0;i<6;++i){gameModules::publish(inspectBasketball(h,base,pid));Sleep(200);}
 basketballConnection::entered=true;basketballConnection::releaseCurrent=true;
 if(gameModules::router.open(now(),true,true)!=gameModules::Id::FootballFusion3)return 3;
 ff3Native::managedDefaults();ff3Native::enabled=true;ff3Native::packetMode=false;ff3Native::visualOnly=false;ff3Native::cursorOnly=false;
 ff3Native::logPath=L"Apps/Basketball/Native/build/aim-verification-events.csv";
 if(!ff3Native::install())return 4;
 std::thread worker([](){winrt::init_apartment(winrt::apartment_type::multi_threaded);ff3Native::worker();winrt::uninit_apartment();});
 std::cout<<std::setprecision(12)<<std::unitbuf;std::string previous;auto deadline=now()+180;ULONGLONG click=0;double release=0;ff3Native::Frame plan;
 while(now()<deadline){
  gameModules::publish(inspectBasketball(h,base,pid));auto f=ff3Native::snapshot();
  if(f.reason!=previous){std::cout<<"STATE "<<f.reason<<" held="<<f.held<<" power="<<f.gamePower<<" at="<<now()<<'\n';previous=f.reason;}
  if(ff3Native::physicalClickAt!=click){click=ff3Native::physicalClickAt;std::lock_guard<std::mutex> lock(ff3Native::mutex);plan=ff3Native::requested;release=now();
   std::cout<<"CLICK at="<<release<<" ready="<<plan.ready<<" target="<<plan.name<<" velocity="<<plan.solution.velocity.x<<','<<plan.solution.velocity.y<<','<<plan.solution.velocity.z<<" gravity="<<plan.gravity<<" angle="<<plan.solution.angle<<" power="<<plan.power<<'\n';}
  if(release)for(const auto& ball:f.balls)if(ball.born>=release-.1&&(ball.id==plan.heldBall||ball.thrower==plan.localPlayer||ball.thrower==plan.own)){
   std::cout<<"BALL elapsed="<<now()-release<<" id="<<ball.id<<" born="<<ball.born<<" p="<<ball.position.x<<','<<ball.position.y<<','<<ball.position.z<<" v="<<ball.velocity.x<<','<<ball.velocity.y<<','<<ball.velocity.z<<" fitted="<<ball.model.valid;
   if(ball.model.valid)std::cout<<" fit="<<ball.model.velocity.x<<','<<ball.model.velocity.y<<','<<ball.model.velocity.z<<" fitAt="<<ball.model.at<<" error="<<ball.model.error;
   std::cout<<'\n';
  }
  Sleep(10);
 }
 running=false;ff3Native::inputCancelled=true;worker.join();ff3Native::uninstall();CloseHandle(h);return release?0:5;
}


