#pragma once
// Deadline-driven pacing with no queued render jobs and no busy-spin.
struct PumpFrameClock {
 HANDLE timer=nullptr;bool highResolution=false;
 PumpFrameClock(){timer=CreateWaitableTimerExW(nullptr,nullptr,0x2,TIMER_ALL_ACCESS);highResolution=timer!=nullptr;if(!timer)timer=CreateWaitableTimerW(nullptr,FALSE,nullptr);}
 ~PumpFrameClock(){if(timer)CloseHandle(timer);}
 void waitUntil(double deadline){
  double remaining=deadline-now();if(remaining<=0)return;
  if(!timer){MsgWaitForMultipleObjectsEx(0,nullptr,(DWORD)std::max(1.,ceil(remaining*1000)),QS_ALLINPUT,MWMO_INPUTAVAILABLE);return;}
  LARGE_INTEGER due{};due.QuadPart=-(LONGLONG)std::max(1.,remaining*1e7);
  if(!SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE))return;
  MsgWaitForMultipleObjectsEx(1,&timer,INFINITE,QS_ALLINPUT,MWMO_INPUTAVAILABLE);
 }
};
