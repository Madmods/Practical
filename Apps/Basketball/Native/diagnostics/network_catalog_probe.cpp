#define NOMINMAX
#include "../src/game_module_ui.hpp"
#include <shobjidl.h>
#include <wrl.h>
#include "../src/network_inspector.hpp"
#include <iostream>
int main(){winrt::init_apartment();HANDLE process=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(process,pid,base))return 2;
 for(int i=0;i<7;++i){gameModules::publish(inspectBasketball(process,base,pid));Sleep(120);}
 std::atomic<bool> finished{false};json::Value result;std::mutex lock;
 networkInspector::Service inspector([&](const json::Value& report){if(!report[L"busy"].boolean()){std::lock_guard<std::mutex> guard(lock);result=report;finished=true;}});
 inspector.refresh();double started=now();while(!finished&&now()-started<6){gameModules::publish(inspectBasketball(process,base,pid));Sleep(100);}
 inspector.stop();CloseHandle(process);if(!finished)return 3;
 std::cout<<winrt::to_string(json::dump(result))<<'\n';winrt::uninit_apartment();return 0;
}
