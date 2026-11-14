#pragma once
#include <windows.h>
// A per-worker timer; does not change the system timer period or busy-spin.
// Older Windows/creation failures retain the ordinary scheduler fallback.
class PreciseWait {
 HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_MODIFY_STATE|SYNCHRONIZE);
public:
 PreciseWait()=default;
 PreciseWait(const PreciseWait&)=delete;
 PreciseWait& operator=(const PreciseWait&)=delete;
 ~PreciseWait(){if(timer)CloseHandle(timer);}
 bool highResolution()const{return timer!=nullptr;}
 void milliseconds(DWORD duration){
  LARGE_INTEGER due{};due.QuadPart=-static_cast<LONGLONG>(duration?duration:1)*10000;
  if(timer&&SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE)&&WaitForSingleObject(timer,duration+100)==WAIT_OBJECT_0)return;
  Sleep(duration);
 }
};
