#define NOMINMAX
#include <winsock2.h>
#include "basketball_ui_bridge.hpp"
#include "game_module_ui.hpp"
#include "pump_overlay.hpp"
#include "ff3_qb_overlay.hpp"
#include "webui/vanta_ui.hpp"
#include "network_inspector.hpp"
#include "ff3_packet_bridge.hpp"
#include <shellapi.h>

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
 const std::wstring args=GetCommandLineW();const bool test=args.find(L"--ui-test")!=std::wstring::npos;
 HANDLE single=CreateMutexW(nullptr,FALSE,test?L"Local\\VantaBasketballUITest":L"Local\\VantaNativeTiming");
 if(GetLastError()==ERROR_ALREADY_EXISTS){if(single)CloseHandle(single);CoUninitialize();return 0;}
 wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);
 std::filesystem::path folder=std::filesystem::path(exe).parent_path();
 if(!std::filesystem::exists(folder/L"bat-timing-settings.json")&&std::filesystem::exists(folder/L".."/L".."/L"bat-timing-settings.json"))folder=std::filesystem::weakly_canonical(folder/L".."/L"..");
 const auto config=folder/L"bat-timing-settings.json";
 const auto ff3Config=folder/L"ff3-qb-settings.json";
 const auto testFolder=std::filesystem::path(exe).parent_path();
 int testResult=1;
 basketballConnection::required=true;sessionUnlocked=false;if(!test)basketballUi::load(config);else basketballUi::stop();
 if(!test){ff3Native::load(ff3Config);ff3Native::logPath=folder/L"ff3-qb-events-v2.csv";}
 std::atomic<bool> authenticated{false};auto& workspaceEntered=basketballConnection::entered;std::atomic<int> testFixture{0};bool hooks=false;
 vanta::UiHost ui;networkInspector::Service inspector([&](const json::Value& value){ui.Post(json::dump(value));},!test);ff3Packets::Service qbPackets;vanta::AppInfo info;
 if(!test){ff3Native::frameSink=[&](const ff3Native::Frame& frame){qbPackets.update(frame);};ff3Native::packetStop=[&](){qbPackets.stop();};ff3Native::packetStatus=[&](){return qbPackets.status();};}
 info.title=L"Vanta";info.product=L"basketball";info.dataName=test?L"VantaBasketballUITest":L"VantaBasketball";
 info.version=info.latestVersion=vantaRelease::Version;info.robloxVersion=std::wstring(Offsets::ClientVersion.begin(),Offsets::ClientVersion.end());
 info.initialState=json::dump(basketballUi::bootstrap());info.debug=args.find(L"--debug")!=std::wstring::npos;
 info.testMode=test;info.skipAuth=false;info.user=L"";
 if(args.find(L"--no-blur")!=std::wstring::npos)info.glass=vanta::Glass::None;
 if(const auto at=args.find(L"--scale=");at!=std::wstring::npos)info.scale=_wtof(args.c_str()+at+8);
 vanta::UiCallbacks cb;
 auto publish=[&](){ui.Post(L"{\"type\":\"basketball\",\"modules\":"+json::dump(basketballUi::snapshot())+L"}");};
 auto error=[&](const wchar_t* text){ui.Post(L"{\"type\":\"basketballError\",\"error\":"+json::quote(text)+L"}");};
 auto persist=[&](){if(!test&&!basketballUi::save(config))error(L"Settings could not be saved. Check folder permissions.");};
 auto basketballActive=[&](){return authenticated&&workspaceEntered&&gameModules::router.active(now(),basketballConnection::releaseCurrent)==gameModules::Id::Basketball;};
 auto ff3Active=[&](){return authenticated&&ff3Native::admitted();};
 auto persistFf3=[&](){if(!test&&!ff3Native::save(ff3Config))error(L"Football settings could not be saved.");ui.Post(json::dump(ff3Native::report()));};
 cb.signIn=[&](const std::wstring& user,const std::wstring& pass,bool)->vanta::AuthResult{
  bool ok=user==L"1"&&pass==L"1";authenticated=ok;
  return ok?vanta::AuthResult{true}:vanta::AuthResult{false,L"Use the local preview login: 1 / 1.",L"pass"};
 };
 cb.createAccount=[](auto&,auto&,auto&,auto&)->vanta::AuthResult{return {false,L"Account creation is not enabled in this local preview.",L"user"};};
 cb.installUpdate=[](){return false;};
 cb.runCheck=[&](const std::wstring& id)->vanta::CheckResult{
  if(id==L"session")return authenticated?vanta::CheckResult{true,L"Local preview",L"Settings are stored on this PC"}:vanta::CheckResult{false,L"Sign in",L"Use 1 / 1"};
  auto report=gameModuleUi::connection(gameModules::router.get(),workspaceEntered,vantaRelease::get());
  for(auto& row:report[L"checks"].a)if(row[L"id"].str()==id)return {row[L"ok"].boolean(),row[L"result"].str(),row[L"detail"].str()};
  return {false,L"Unknown check"};
 };
 cb.featureChanged=[&](const std::wstring& key,bool on){
  if(ff3Active()){if(key==L"ff3_qb"){if(on&&!test&&!qbPackets.start(gameModules::router.get().pid)){const auto state=qbPackets.status();error(state.second.c_str());return;}ff3Native::set(key,basketballUi::boolean(on));persistFf3();}return;}
  if(!basketballActive())return;
  if(!basketballUi::set(key,basketballUi::boolean(on)))error(L"That feature is unavailable.");else persist();publish();
 };
 cb.settingChanged=[&](const std::wstring& key,const json::Value& value){
  if(!authenticated||!workspaceEntered||key.rfind(L"theme.",0)==0||key.rfind(L"glass.",0)==0)return;
  if(key==L"menukey.toggle"){
   if(value.type!=json::Value::Array||value.a.size()!=2){error(L"Choose one or two menu keys.");return;}
   int a=basketballUi::keyCode(value.a[0]),b=basketballUi::keyCode(value.a[1]);
   if(a<0||b<0||(!a&&!b)||(a&&a==b)||!ui.MenuKeys(a,b))error(L"Menu shortcut unavailable. You can still click the island to reopen.");
   return;
  }
  if(ff3Active()){if(key.rfind(L"ff3_qb.",0)==0){if(!ff3Native::set(key,value))error(L"Invalid Football setting.");else persistFf3();}return;}
  if(!basketballActive())return;
  if(!basketballUi::set(key,value))error(L"Invalid setting, conflicting release range, or a shot is still held.");else persist();publish();
 };
 cb.modulesChanged=[&](const json::Value& value){if(!basketballActive())return;if(!basketballUi::applyProfile(value))error(L"Profile rejected; previous settings restored.");else persist();publish();};
 cb.workspaceOpened=[&]()->bool{
  const auto selected=gameModules::router.open(now(),authenticated,basketballConnection::releaseCurrent);
  if(selected==gameModules::Id::None){error(L"Wait for the game and local release checks to pass.");return false;}
  sessionUnlocked=false;
  if(selected==gameModules::Id::Basketball){
   ff3Native::stop();ff3Native::uninstall();
   if(!test&&!hooks){hooks=installInput();if(!hooks){uninstallInput();gameModules::router.close();error(L"Input setup failed. Restart the app to retry.");return false;}}
  }else{
   if(hooks){uninstallInput();hooks=false;}
   ff3Native::stop();
   if(!test&&!ff3Native::install()){gameModules::router.close();error(L"Football input setup failed. Restart the app to retry.");return false;}
  }
  ui.MenuKeys(VK_INSERT);workspaceEntered=true;sessionUnlocked=!test&&selected==gameModules::Id::Basketball;
  auto opened=gameModuleUi::opened(selected);if(selected==gameModules::Id::FootballFusion3)opened.o[L"modules"]=ff3Native::modules();
  ui.Post(json::dump(opened));return true;
 };
 cb.signedOut=[&](){inspector.reset();authenticated=false;workspaceEntered=false;sessionUnlocked=false;gameModules::router.close();basketballUi::stop();ff3Native::stop();ff3Native::uninstall();ui.MenuKeys(0);if(hooks){uninstallInput();hooks=false;}publish();};
 cb.custom=[&](const std::wstring& type,const json::Value& msg){
  if(type==L"connectionNotice"){workspaceEntered=false;sessionUnlocked=false;gameModules::router.close();ff3Native::stop();ff3Native::uninstall();return;}
  if(test&&type==L"testConnection"){
   const auto fixture=msg[L"scenario"].str();testFixture=fixture==L"ready"?1:fixture==L"wrong"?2:fixture==L"meter"?3:fixture==L"outdated"?4:fixture==L"ff3"?5:0;
   // Invalidate immediately so even a click before the next UI status cannot pass.
   gameModules::publish({});
   ui.Post(L"{\"type\":\"reply\",\"rid\":"+std::to_wstring((int)msg[L"rid"].num())+L",\"ok\":true}");return;
  }
  if(test && type==L"testState"){
   ui.Post(L"{\"type\":\"reply\",\"rid\":"+std::to_wstring((int)msg[L"rid"].num())+L",\"ok\":true,\"authenticated\":"+(authenticated?L"true":L"false")+L",\"inputs\":"+((sessionUnlocked||hooks||ff3Native::mouse)?L"true":L"false")+L",\"ff3\":"+json::dump(ff3Native::modules())+L",\"modules\":"+json::dump(basketballUi::snapshot())+L"}");return;
  }
  if(test && type==L"testCapture"){
   const auto rid=(int)msg[L"rid"].num();const auto name=msg[L"name"].str();
   if(name==L"ff3passes"||name==L"network"||name==L"ff3"||name==L"shooting"||name==L"reviewer"||name==L"defense"||name==L"home"||name==L"login"||name==L"connection"||name==L"reconnect"||name==L"recovered"||name==L"quit")ui.Capture((testFolder/(L"webui-"+name+L".png")).wstring(),[&,rid](bool ok){ui.Post(L"{\"type\":\"reply\",\"rid\":"+std::to_wstring(rid)+L",\"ok\":"+(ok?L"true":L"false")+L"}");});return;
  }
  if(test&&type==L"testNetworkLog"){
   auto reply=networkInspector::object();reply.o[L"type"]=networkInspector::text(L"reply");reply.o[L"rid"]=msg[L"rid"];reply.o[L"ok"]=networkInspector::flag(false);
   try{
    const auto file=testFolder/L"network-captured-test.json";
    if(std::filesystem::exists(file)&&std::filesystem::file_size(file)<=65536){
     std::ifstream input(file,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(input)),{});auto root=json::parse(std::wstring(winrt::to_hstring(bytes)));json::Value calls;std::wstring reason;
     if(networkInspector::validateLog(root,calls,reason)){
      for(auto& call:calls.a)call.o[L"source"]=networkInspector::text(L"Captured traffic");root.o[L"calls"]=calls;root.o[L"type"]=networkInspector::text(L"networkInspector");root.o[L"active"]=networkInspector::flag(false);root.o[L"available"]=networkInspector::flag(false);reply.o[L"data"]=root;reply.o[L"ok"]=networkInspector::flag(true);
     }
    }
   }catch(...){}ui.Post(json::dump(reply));return;
  }
  if(test && (type==L"testDone"||type==L"testTimeout"||type==L"testJsError")){
   testResult=type==L"testDone"&&msg[L"ok"].boolean()?0:1;
   std::ofstream report(testFolder/L"webui-test.json");report<<winrt::to_string(json::dump(msg));report.close();
   if(testResult==0&&msg[L"awaitQuit"].boolean()){
    ui.Post(L"{\"type\":\"reply\",\"rid\":"+std::to_wstring((int)msg[L"rid"].num())+L",\"ok\":true}");return;
   }
   ui.Close();return;
  }
  if(type==L"networkInspector"){
   if(!authenticated||!workspaceEntered)return;
   const auto action=msg[L"action"].str();
   if(action==L"get")inspector.publish();
   else if(action==L"startCapture"){if(!test)inspector.startCapture();inspector.publish();}
   else if(action==L"stopCapture"){inspector.stopCapture();inspector.publish();}
   else if(action==L"refresh"){if(!test)inspector.refresh();else inspector.publish();}
   else if(action==L"clear")inspector.clear();
   else if(action==L"copy"){if(!networkInspector::copy(msg[L"text"].str()))error(L"Could not copy inspector text.");}
   else if(action==L"import"){
    const auto selected=networkInspector::chooseLog();if(selected.empty())return;
    try{
     if(std::filesystem::file_size(selected)>65536){error(L"Decoded logs must be at most 64 KiB.");return;}
     std::ifstream input(std::filesystem::path(selected),std::ios::binary);if(!input){error(L"Could not read the decoded log.");return;}
     std::string bytes((std::istreambuf_iterator<char>(input)),{});
     if(bytes.rfind("\xEF\xBB\xBF",0)==0)bytes.erase(0,3);
     inspector.import(json::parse(std::wstring(winrt::to_hstring(bytes))));
    }catch(...){error(L"Could not read this UTF-8 JSON log. Previous records were retained.");}
   }
   return;
  }
  if(type==L"basketballReady"){publish();return;}
  if(!basketballActive()||type!=L"bb")return;
  const auto action=msg[L"args"][L"name"].str();
  if(action.rfind(L"review",0)==0){
   bool ok=false;
   if(!gameDown&&sessionUnlocked&&basketballConnection::state.readyAt(now())){
    if(action==L"reviewReset"){resetReviewer();ok=true;}
    else {const auto& id=msg[L"args"][L"id"];
     if(id.type==json::Value::Number&&std::isfinite(id.n)&&id.n>0&&id.n<=UINT_MAX&&std::floor(id.n)==id.n){
      if(action==L"reviewEarly")ok=reviewerFeedback((unsigned)id.n,timingReview::Grade::Early);
      else if(action==L"reviewPerfect")ok=reviewerFeedback((unsigned)id.n,timingReview::Grade::Perfect);
      else if(action==L"reviewLate")ok=reviewerFeedback((unsigned)id.n,timingReview::Grade::Late);
     }
    }
   }
   ui.Post(L"{\"type\":\"reviewReply\",\"ok\":"+std::wstring(ok?L"true":L"false")+L",\"review\":"+json::dump(basketballUi::reviewReport())+L"}");return;
  }
  if(msg[L"op"].str()==L"stop"||action==L"stop"){basketballUi::stop();persist();publish();}
  else if(action==L"select")defense::pick=true;
  else if(action==L"clear"){defense::pick=false;defense::selected=0;}
 };
 if(!ui.Create(instance,info,cb)){ui.Shutdown();CloseHandle(single);CoUninitialize();return 1;}
 running=true;
 std::thread reader,geometry,visuals,defender,feedback,qb,qbVisual;
 if(!test)feedbackNative::logPath=folder/L"timing-review.csv";
 std::thread releaseWorker([&](){
  try{winrt::init_apartment(winrt::apartment_type::multi_threaded);
   const auto digest=vantaRelease::sha256(exe);
   while(running){vantaRelease::publish(vantaRelease::verify(testFolder/L"vanta-release.json",digest));for(int i=0;i<40&&running;++i)Sleep(50);}
   winrt::uninit_apartment();
  }catch(...){vantaRelease::publish({false,L"Unavailable",L"Could not verify the local Vanta release."});}
 });
 if(!test){reader=std::thread(engine);geometry=std::thread(contestWorker);visuals=std::thread(pumpVisualWorker);defender=std::thread(defense::worker);feedback=std::thread(shotFeedbackWorker);qb=std::thread([](){winrt::init_apartment(winrt::apartment_type::multi_threaded);ff3Native::worker();winrt::uninit_apartment();});qbVisual=std::thread(ff3Visual::worker);}
 std::thread telemetry([&](){while(running){
  if(test){ // Explicit synthetic fixtures only in isolated --ui-test; real policy/bridge.
   basketballConnection::Snapshot s;s.sampledAt=now();const int fixture=testFixture;
   if(fixture){s.pid=1;s.model=100;s.character=200;s.process=s.version=s.opened=s.loaded=s.player=s.characterReady=s.world=s.meter=s.renderer=true;s.placeId=basketballConnection::PlaceId;s.universeId=fixture==2?123:basketballConnection::UniverseId;if(fixture==3)s.meter=false;}
   if(fixture==5){s.placeId=gameModules::registry[1].rootPlace;s.universeId=gameModules::registry[1].universe;s.meter=false;s.character=0;s.characterReady=false;}
   gameModules::publish(s);
  }
  auto release=vantaRelease::get();if(test&&testFixture==4)release={false,L"Update needed",L"A newer local Vanta build is available."};
  basketballConnection::releaseCurrent=release.current;
  const auto connection=gameModules::router.get();if(!test)inspector.observe(connection.pid,connection.model,connection.universeId);const bool ready=gameModules::ready(connection,now())&&release.current;
  if(gameModules::router.active(now(),release.current)==gameModules::Id::None&&workspaceEntered.exchange(false)){sessionUnlocked=false;ff3Native::stop();}
  ui.Post(json::dump(gameModuleUi::connection(connection,workspaceEntered,release)));
  if(ready&&basketballConnection::targetGame(connection)&&livePing)ui.SetPing(pingMs);
  if(gameModules::router.active(now(),release.current)==gameModules::Id::FootballFusion3){ui.Post(json::dump(ff3Native::report()));const auto f=ff3Native::snapshot();if(f.ping>0&&now()-f.at<.3)ui.SetPing(int(std::lround(f.ping)));}
  if(gameModules::router.active(now(),release.current)!=gameModules::Id::Basketball){for(int i=0;i<10&&running;i++)Sleep(25);continue;}
  auto frame=defense::snapshot();
  auto reason=basketballUi::widen(frame.decision.reason);
  ui.Post(L"{\"type\":\"basketballLive\",\"reason\":"+json::quote(reason)+L",\"meter\":"+std::to_wstring(fillPct.load())+L",\"releases\":"+std::to_wstring(releaseCount.load())+L",\"shot\":"+json::dump(basketballUi::shotReport(lastTimingRelease()))+L",\"window\":"+json::dump(basketballUi::windowReport())+L",\"review\":"+json::dump(basketballUi::reviewReport())+L"}");
  for(int i=0;i<10&&running;i++)Sleep(25);
 }});
 const int result=ui.Run();inspector.stop();
 sessionUnlocked=false;ff3Native::stop();ff3Native::uninstall();running=false;
 if(qb.joinable())qb.join();if(qbVisual.joinable())qbVisual.join();
 if(reader.joinable())reader.join();if(geometry.joinable())geometry.join();if(defender.joinable())defender.join();if(visuals.joinable())visuals.join();if(feedback.joinable())feedback.join();
 telemetry.join();releaseWorker.join();if(hooks)uninstallInput();ui.Shutdown();
 CloseHandle(single);CoUninitialize();return test?testResult:result;
}

