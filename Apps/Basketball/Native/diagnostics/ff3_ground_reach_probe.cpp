#define NOMINMAX
#include "../src/ff3_play_runtime.hpp"
#include <iostream>
// Read-only ground/catch discovery. Catch resizing has been removed.
int main(int argc,char** argv){
 if(argc>1){std::cerr<<"Read-only probe: catch resizing is removed; --apply is unsupported.\n";return 7;}
 winrt::init_apartment();HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 2;
 for(int i=0;i<5;++i){gameModules::publish(inspectBasketball(h,base,pid));Sleep(200);}basketballConnection::entered=true;basketballConnection::releaseCurrent=true;
 if(gameModules::router.open(now(),true,true)!=gameModules::Id::FootballFusion3){CloseHandle(h);return 3;}
 ff3Play::magsOn=true;ff3Play::Reader reader;ff3Play::Scene scene;for(int i=0;i<4;++i){scene=reader.read({});Sleep(100);}
 auto hands=ff3Native::catchParts(h,scene.own);using namespace basketballUi;auto report=object();report.o[L"readOnly"]=basketballUi::boolean(true);report.o[L"pid"]=number(pid);report.o[L"valid"]=basketballUi::boolean(scene.valid);report.o[L"groundKnown"]=basketballUi::boolean(std::isfinite(scene.groundY));report.o[L"groundBalls"]=number(double(reader.groundParts.size()));report.o[L"airborneBalls"]=number(double(reader.magParts.size()));auto catchParts=array();
 for(auto id:{hands.first,hands.second}){auto p=rd<addr>(h,id+Offsets::BasePart::Primitive);auto flags=rd<unsigned char>(h,p+Offsets::Primitive::Flags,255);auto row=object();row.o[L"found"]=basketballUi::boolean(id&&p);row.o[L"nonSolid"]=basketballUi::boolean(ff3Play::nonSolidCatchFlags(flags));row.o[L"flags"]=number(flags);catchParts.a.push_back(row);}report.o[L"catchParts"]=catchParts;
 int result=scene.valid?0:4;
 std::cout<<winrt::to_string(json::dump(report))<<std::endl;CloseHandle(h);return result;
}
