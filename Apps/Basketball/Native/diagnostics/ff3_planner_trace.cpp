#define NOMINMAX
#include "../src/ff3_qb_runtime.hpp"
#include <iostream>
#include <iomanip>
// Read-only planner trace: no worker, hooks, target writes or synthetic inputs.
int main(){HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 2;CloseHandle(h);h=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION|SYNCHRONIZE,FALSE,pid);if(!h)return 3;
 for(int i=0;i<6;++i){gameModules::publish(inspectBasketball(h,base,pid));Sleep(200);}basketballConnection::entered=true;basketballConnection::releaseCurrent=true;
 if(gameModules::router.open(now(),true,true)!=gameModules::Id::FootballFusion3)return 4;
 ff3Native::load(L"Apps/Basketball/ff3-qb-settings.json");ff3Native::managedDefaults();ff3Native::enabled=true;ff3Native::Reader reader;double began=now();std::cout<<std::setprecision(12)<<std::unitbuf;
 std::cout<<"at,readMs,focused,held,ready,target,actualPower,plannedPower,angle,flight,minimum,maximum,fitX,fitY,fitZ,worldX,worldY,worldZ,reason\n";
 while(now()-began<60&&WaitForSingleObject(h,0)==WAIT_TIMEOUT){gameModules::publish(inspectBasketball(h,base,pid));double start=now();auto f=reader.read();
 std::cout<<now()<<','<<(now()-start)*1000<<','<<ff3Native::focused(pid)<<','<<f.held<<','<<f.ready<<','<<f.target<<','<<f.gamePower<<','<<f.power<<','<<f.solution.angle<<','<<f.solution.time<<','<<f.minimumAngle<<','<<f.maximumAngle<<','<<f.fitOffset.x<<','<<f.fitOffset.y<<','<<f.fitOffset.z<<','<<f.selectedPosition.x<<','<<f.selectedPosition.y<<','<<f.selectedPosition.z<<','<<std::quoted(f.reason)<<'\n';Sleep(8);}
 CloseHandle(h);return 0;}
