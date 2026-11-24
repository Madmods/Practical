#define NOMINMAX
#include "../src/ff3_qb_runtime.hpp"
#include <iostream>
#include <iomanip>
// Independent read-only ball-flight measurement. No worker, hooks, writes or inputs.
int main(){HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 2;CloseHandle(h);h=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION|SYNCHRONIZE,FALSE,pid);if(!h)return 3;
 for(int i=0;i<6;++i){gameModules::publish(inspectBasketball(h,base,pid));Sleep(200);}basketballConnection::entered=true;basketballConnection::releaseCurrent=true;
 if(gameModules::router.open(now(),true,true)!=gameModules::Id::FootballFusion3)return 4;
 ff3Native::enabled=true;ff3Native::visualOnly=true;ff3Native::Reader reader;const double began=now();std::cout<<std::setprecision(12)<<std::unitbuf;
 std::cout<<"at,id,born,px,py,pz,gravity,fitted,fitAt,fitX,fitY,fitZ,vx,vy,vz,fitError\n";
 while(now()-began<90&&WaitForSingleObject(h,0)==WAIT_TIMEOUT){gameModules::publish(inspectBasketball(h,base,pid));const auto f=reader.read();
  for(const auto& ball:f.balls)if(ball.thrower&&(ball.thrower==f.localPlayer||ball.thrower==f.own)&&ball.born>=began){const auto& m=ball.model;
   std::cout<<now()<<','<<ball.id<<','<<ball.born<<','<<ball.position.x<<','<<ball.position.y<<','<<ball.position.z<<','<<ball.gravity<<','<<m.valid<<','<<m.at<<','<<m.origin.x<<','<<m.origin.y<<','<<m.origin.z<<','<<m.velocity.x<<','<<m.velocity.y<<','<<m.velocity.z<<','<<m.error<<'\n';
  }Sleep(8);
 }CloseHandle(h);return 0;}
