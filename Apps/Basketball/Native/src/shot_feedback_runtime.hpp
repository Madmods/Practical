#pragma once
#include <filesystem>
#include <iomanip>
#include <sstream>
// Version-gated local-character UI properties, confirmed by the read-only
// feedback diagnostic. Never scan other players or use general screen text.
namespace feedbackNative {
inline constexpr addr Text=0xa30,TextTransparency=0xb0c;
struct Reader {
 addr owner=0,hrp=0,board=0,timing=0,contest=0,meter=0,bat=0,fill=0;
 double scanAt=-10;
 shotFeedback::Frame read(HANDLE h,addr own,double t){
  if(own!=owner||t-scanAt>1){owner=own;scanAt=t;hrp=child(h,own,"HumanoidRootPart");board=child(h,hrp,"ShotFeedback");timing=child(h,board,"ReleaseTiming");contest=child(h,board,"Contest");bat=child(h,hrp,"BatMeter");fill=child(h,bat,"Meter_Fill");meter=child(h,fill,"FillGradient");}
  auto parent=[&](addr a,addr p){return a&&p&&rd<addr>(h,a+Offsets::Instance::Parent)==p;};
  if(!parent(hrp,own)||!parent(board,hrp)||!parent(timing,board)||!parent(contest,board)||!parent(bat,hrp)||!parent(fill,bat)||!parent(meter,fill)||defense::classOf(h,board)!="BillboardGui"||defense::classOf(h,timing)!="TextLabel"||defense::classOf(h,contest)!="TextLabel"||defense::classOf(h,meter)!="UIGradient")return {};
  const auto label=defense::textAt(h,timing+Text),pressure=defense::textAt(h,contest+Text);
  auto frame=shotFeedback::Frame{own,board,true,rd<float>(h,timing+TextTransparency,NAN),-rd<float>(h,meter+0x12c,NAN),shotFeedback::parseContest(pressure),shotFeedback::parseGrade(label)};
  if(!parent(hrp,own)||!parent(board,hrp)||!parent(timing,board)||!parent(contest,board)||!parent(bat,hrp)||!parent(fill,bat)||!parent(meter,fill))return {};
  return frame;
 }
};
inline std::filesystem::path logPath;
inline std::deque<std::string> logLines;
inline void log(const TimingRelease& shot,const shotFeedback::Result& r,const timingReview::View& view){
 if(logPath.empty())return;
 std::ostringstream row;row<<std::fixed<<std::setprecision(4)<<shot.count<<','<<shot.sent<<','<<shot.ping<<','<<shot.windowPing<<','<<view.resultSent<<','<<shot.minimum<<','<<shot.maximum<<','<<shot.rate<<','<<view.gameGrade<<','<<r.meter*100<<','<<r.contest<<','<<view.adjustmentMs<<','<<view.guardMs<<','<<view.resultPhase<<','<<view.windowShiftMs;
 logLines.push_back(row.str());while(logLines.size()>200)logLines.pop_front();
 std::ofstream file(logPath);if(!file)return;
 file<<"shot,monotonic_sent,ping_ms,window_ping_ms,sent_percent,minimum,maximum,meter_rate,game_grade,meter_at_feedback_percent,contest_percent,learned_adjustment_ms,learned_guard_ms,meter_direction,learned_window_shift_ms\n";
 for(const auto& line:logLines)file<<line<<'\n';
}
}
inline void shotFeedbackWorker(){
 HANDLE h=nullptr;DWORD attached=0;feedbackNative::Reader reader;shotFeedback::Tracker tracker;
 while(running){
  const auto pid=gamePid.load();
  if(pid!=attached){if(h)CloseHandle(h);h=pid?OpenProcess(PROCESS_VM_READ|SYNCHRONIZE,FALSE,pid):nullptr;attached=pid;reader={};tracker.reset();}
  if(!h||!sessionUnlocked||!reviewOn||WaitForSingleObject(h,0)!=WAIT_TIMEOUT){tracker.reset();Sleep(20);continue;}
  const auto shot=lastTimingRelease();const auto t=now();
  if(basketballConnection::required&&!basketballConnection::state.readyAt(t)){tracker.reset();Sleep(20);continue;}
  const auto frame=reader.read(h,sceneOwn.load(),t);
  if(frame.owner!=sceneOwn.load()){tracker.reset();Sleep(10);continue;}
  const auto result=tracker.observe(frame,{shot.count,shot.press,shot.owner,shot.started,shot.sent},pressSerial.load(),t);
  if(result.id){timingReview::View view;bool accepted=false;
   {std::lock_guard<std::mutex> lock(reviewMutex);accepted=reviewer.gameResult(result);view=reviewer.view(t);}
   if(accepted)feedbackNative::log(shot,result,view);
  }
  Sleep(10);
 }
 if(h)CloseHandle(h);
}
