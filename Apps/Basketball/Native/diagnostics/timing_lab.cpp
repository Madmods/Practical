#define NOMINMAX
#include "basketball_ui_bridge.hpp"
#include <iostream>
static BOOL WINAPI cancelLab(DWORD event){if(event==CTRL_C_EVENT||event==CTRL_BREAK_EVENT||event==CTRL_CLOSE_EVENT){running=false;return TRUE;}return FALSE;}
struct PracticeMove {
 bool keys[4]{};const int codes[4]={'W','S','D','A'};
 ~PracticeMove(){release();}
 void release(){for(int i=0;i<4;i++)if(keys[i]){nativeShotInput(codes[i],false);keys[i]=false;}}
 void toward(const defense::Scene& scene,shooting::Vec target){
  if(!gameFocused()||!scene.valid){release();return;}
  auto dir=defense::unit(defense::flat(target-scene.own.p));const float f=defense::dot(dir,scene.cameraForward),r=defense::dot(dir,scene.cameraRight);float axes[4]={f,-f,r,-r};
  for(int i=0;i<4;i++){const bool want=defense::movementAxis(axes[i],keys[i]);if(want!=keys[i]&&nativeShotInput(codes[i],want))keys[i]=want;}
 }
};
// Explicit, bounded practice-shot integration test. Uses the production engine
// and normal keyboard events; never writes to game memory or saved settings.
// Run with Basketball closed and the supported practice court in the foreground.
int main(int argc,char** argv){
 if((argc!=4&&argc!=5)||std::string(argv[1])!="--shoot"||(argc==5&&std::string(argv[4])!="--collect")){std::cerr<<"Usage: timing-lab --shoot <count:1..30> <seconds:10..600> [--collect]\n";return 2;}
 const int count=atoi(argv[2]),seconds=atoi(argv[3]);if(count<1||count>30||seconds<10||seconds>600)return 2;
 HANDLE single=CreateMutexW(nullptr,FALSE,L"Local\\VantaNativeTiming");if(!single||GetLastError()==ERROR_ALREADY_EXISTS){std::cerr<<"Close Basketball before running this test.\n";if(single)CloseHandle(single);return 3;}
 HANDLE cancel=CreateEventW(nullptr,TRUE,FALSE,L"Local\\VantaTimingLabStop");if(cancel)ResetEvent(cancel);
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 SetConsoleCtrlHandler(cancelLab,TRUE);
 wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);const auto folder=std::filesystem::path(exe).parent_path();
 basketballUi::load(folder/L".."/L".."/L"bat-timing-settings.json");
 armed=true;assist=false;ceilingOn=false;pumpOn=false;reviewOn=true;sessionUnlocked=true;
 basketballConnection::required=true;basketballConnection::entered=true;basketballConnection::releaseCurrent=true;
 feedbackNative::logPath=folder/L"timing-lab-results.csv";
 std::ofstream trace(folder/L"timing-lab-trace.csv");trace<<"seconds,press,ping_ms,meter,slope,minimum,maximum,window_rate,window_ping,status,game_grade,text_alpha,feedback_meter\n";
 std::thread engineThread(engine),feedbackThread(shotFeedbackWorker);
 PreciseWait traceWait;
 HANDLE h=nullptr;DWORD attached=0;feedbackNative::Reader reader;
 defense::Reader sceneReader;defense::Scene scene;PracticeMove movement;shooting::Vec spot{};bool spotKnown=false;double sceneReadAt=-1,settledAt=-1;
 const double began=now();double next=began+4,pressed=-1,lastTrace=-1,heartbeat=-1;int attempted=0;unsigned resultSeen=0;bool down=false;addr initialOwner=0;double oldMeter=-1;
 std::cout<<"Bounded practice test started; focus game, hold a ball. Escape stops. Saved settings are read-only.\n"<<std::flush;
 while(running&&now()-began<seconds){
  const double t=now();if(GetAsyncKeyState(VK_ESCAPE)&0x8000){std::cout<<"Escape: stopping.\n";break;}
  if(cancel&&WaitForSingleObject(cancel,0)==WAIT_OBJECT_0)break;
  auto pid=gamePid.load();if(pid!=attached){if(h)CloseHandle(h);h=pid?OpenProcess(PROCESS_VM_READ|SYNCHRONIZE,FALSE,pid):nullptr;attached=pid;reader={};}
  const auto own=sceneOwn.load();auto f=h?reader.read(h,own,t):shotFeedback::Frame{};auto w=currentTimingWindow();auto view=reviewerView();
  if(t-heartbeat>=5){std::cout<<"State: foreground/ready="<<gameFocused()<<", connection="<<basketballConnection::state.readyAt(t)<<", feedback="<<f.valid<<", scene="<<scene.valid<<", playing="<<scene.playing<<", actors="<<scene.actors.size()<<", balls="<<scene.balls.size()<<", ping="<<pingMs<<"\n"<<std::flush;heartbeat=t;}
  if(t-lastTrace>.02||f.meter!=oldMeter){trace<<t-began<<','<<pressSerial<<','<<pingMs<<','<<progress<<','<<meterSlope<<','<<w.bounds.minimum<<','<<w.bounds.maximum<<','<<w.rate<<','<<w.ping<<','<<statusCode<<','<<int(f.grade)<<','<<f.alpha<<','<<f.meter<<'\n';lastTrace=t;oldMeter=f.meter;}
  if(view.gameShotId&&view.gameShotId!=resultSeen){resultSeen=view.gameShotId;std::cout<<"Result "<<resultSeen<<": "<<view.gameGrade<<", sent "<<view.resultSent<<"%, feedback meter "<<view.resultMeter<<"%, ping "<<pingMs<<"ms; "<<view.reason<<'\n'<<std::flush;}
  if(down&&(!gameFocused()||!h||own!=initialOwner||t-pressed>3||!gameDown)){
   sendRelease();gameDown=false;manualDone=true;down=false;next=t+4;
   std::cout<<"Shot "<<attempted<<" ended: "<<lastTimingRelease().percent<<"% at "<<lastTimingRelease().ping<<"ms.\n"<<std::flush;
  }
  if(attempted>=count&&!down&&t>=next)break;
  if(argc==5&&!down&&t>=next&&gameFocused()){
   if(t-sceneReadAt>.04){scene=sceneReader.read(t).scene;sceneReadAt=t;
    // The production defense reader intentionally filters balls to an assigned
    // match court. This solo practice test instead reads loose practice balls.
    if(scene.valid&&!scene.playing&&scene.actors.empty()){
     scene.balls.clear();for(auto b:children(h,path(h,sceneRoot.load(),{"Workspace","Basketballs"}))){
      if(!defense::partClass(defense::classOf(h,b)))continue;auto primitive=rd<addr>(h,b+Offsets::BasePart::Primitive);if(!primitive)continue;
      defense::Ball ball;ball.id=b;ball.p=rd<shooting::Vec>(h,primitive+Offsets::Primitive::Position,{NAN,NAN,NAN});ball.v=rd<shooting::Vec>(h,primitive+Offsets::Primitive::AssemblyLinearVelocity,{NAN,NAN,NAN});
      ball.owner=rd<addr>(h,path(h,b,{"Attributes","Character"})+Offsets::Misc::Value);
      if(defense::finite(ball.p)&&defense::finite(ball.v)&&defense::distance(ball.p,scene.own.p)<60)scene.balls.push_back(ball);
     }
    }
   }
   // Collection is deliberately limited to a solo practice scene. It is not
   // part of the product, never runs during a match, and does not target players.
   const bool solo=scene.valid&&!scene.playing&&scene.actors.empty()&&t-scene.at<.15;
   if(!solo){movement.release();settledAt=-1;}
   else {
    if(!spotKnown){spot=scene.own.p;spotKnown=true;}
    const auto ball=defense::heldBall(scene,scene.own.id);
    if(ball){
     if(defense::distance(spot,scene.own.p)>1){movement.toward(scene,spot);settledAt=-1;}
     else {movement.release();if(settledAt<0)settledAt=t;}
    }else{
     settledAt=-1;const defense::Ball* nearest=nullptr;float gap=45;
     for(const auto& b:scene.balls)if(!b.owner&&std::abs(b.p.y-scene.own.p.y)<5&&defense::length(b.v)<4){float d=defense::distance(b.p,scene.own.p);if(d<gap){gap=d;nearest=&b;}}
     if(nearest)movement.toward(scene,nearest->p);else movement.release();
    }
   }
  }else movement.release();
  if(argc==5&&(settledAt<0||t-settledAt<.5)){traceWait.milliseconds(5);continue;}
  if(!down&&attempted<count&&t>=next&&gameFocused()&&livePing&&f.valid&&f.alpha>=.98&&f.meter<.01){
   bool possession=false;for(auto b:children(h,path(h,sceneRoot.load(),{"Workspace","Basketballs"}))){auto value=path(h,b,{"Attributes","Character"});if(value&&rd<addr>(h,value+Offsets::Misc::Value)==own){possession=true;break;}}
   if(possession){activeShotKey=shootKey.load();if(sendShotInput(true)){initialOwner=own;manualDone=false;pendingUp=false;blockDown=false;activeMeter=false;gameDown=true;pressed=t;down=true;++pressSerial;++attempted;std::cout<<"Shot "<<attempted<<" started at "<<pingMs<<"ms, trim "<<trim<<"ms.\n"<<std::flush;}}
  }
  traceWait.milliseconds(5);
 }
 movement.release();if(down)sendRelease();gameDown=false;manualDone=true;running=false;engineThread.join();feedbackThread.join();if(h)CloseHandle(h);if(cancel)CloseHandle(cancel);CloseHandle(single);CoUninitialize();
 std::cout<<"Completed "<<attempted<<" attempts, "<<reviewerView().gameFeedbackCount<<" correlated game results.\n";
 return attempted==count?0:4;
}
