#pragma once
#include <windows.h>
#include <atomic>
#include <mutex>

// A request to supported Windows capture APIs, not a guarantee about every
// recorder. Only mark our own top-level windows; never touch the game window.
namespace capturePrivacy {
inline constexpr DWORD Exclude=0x00000011; // WDA_EXCLUDEFROMCAPTURE
inline constexpr wchar_t Property[]=L"Vanta.CapturePrivacy";
inline std::atomic<bool> enabled{true};
inline std::mutex mutex;
inline bool applyLocked(HWND window){
 const DWORD desired=enabled?Exclude:WDA_NONE;
 DWORD actual=0;
 return SetWindowDisplayAffinity(window,desired)&&GetWindowDisplayAffinity(window,&actual)&&actual==desired;
}
inline bool attach(HWND window){
 std::lock_guard<std::mutex> lock(mutex);
 if(!window||!SetPropW(window,Property,reinterpret_cast<HANDLE>(1)))return false;
 return applyLocked(window);
}
inline void detach(HWND window){if(window)RemovePropW(window,Property);}
inline bool owned(HWND window){DWORD pid=0;GetWindowThreadProcessId(window,&pid);return pid==GetCurrentProcessId()&&GetPropW(window,Property);}
struct State {bool requested=true;unsigned windows=0,failed=0;};
inline BOOL CALLBACK inspect(HWND window,LPARAM context){
 if(owned(window)){
  auto& state=*reinterpret_cast<State*>(context);++state.windows;DWORD actual=0;
  if(!GetWindowDisplayAffinity(window,&actual)||actual!=(state.requested?Exclude:WDA_NONE))++state.failed;
 }
 return TRUE;
}
inline State statusLocked(){State state;state.requested=enabled;EnumWindows(inspect,reinterpret_cast<LPARAM>(&state));return state;}
inline State status(){std::lock_guard<std::mutex> lock(mutex);return statusLocked();}
inline BOOL CALLBACK update(HWND window,LPARAM){if(owned(window))applyLocked(window);return TRUE;}
inline State set(bool value){std::lock_guard<std::mutex> lock(mutex);enabled=value;EnumWindows(update,0);return statusLocked();}
}
