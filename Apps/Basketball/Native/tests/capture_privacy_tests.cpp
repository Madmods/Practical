#define NOMINMAX
#include "../src/capture_privacy.hpp"
#include <iostream>
int main(){
 unsigned checks=0;auto check=[&](bool value){++checks;if(!value){std::cerr<<"Capture privacy check "<<checks<<" failed; Windows error "<<GetLastError()<<'\n';std::exit(1);}};
 WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=DefWindowProcW;wc.lpszClassName=L"VantaCaptureTest";RegisterClassW(&wc);
 auto create=[&](DWORD extended=0){auto window=CreateWindowExW(extended,wc.lpszClassName,L"",WS_POPUP,0,0,32,32,nullptr,nullptr,wc.hInstance,nullptr);check(window!=nullptr);return window;};
 auto affinity=[](HWND window){DWORD value=~DWORD(0);GetWindowDisplayAffinity(window,&value);return value;};
 HWND untouched=create(),panel=create(),overlay=create(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW);
 check(!capturePrivacy::attach(nullptr));
 check(capturePrivacy::attach(panel));check(capturePrivacy::attach(overlay));
 auto state=capturePrivacy::status();check(state.requested&&state.windows==2&&state.failed==0);
 check(affinity(panel)==capturePrivacy::Exclude&&affinity(overlay)==capturePrivacy::Exclude);
 check(affinity(untouched)==WDA_NONE); // Do not modify unrelated windows.
 state=capturePrivacy::set(false);check(!state.requested&&state.windows==2&&state.failed==0);
 check(affinity(panel)==WDA_NONE&&affinity(overlay)==WDA_NONE);
 HWND late=create(WS_EX_LAYERED);check(capturePrivacy::attach(late));check(affinity(late)==WDA_NONE);
 state=capturePrivacy::set(true);check(state.requested&&state.windows==3&&state.failed==0);
 check(affinity(late)==capturePrivacy::Exclude&&affinity(untouched)==WDA_NONE);
 capturePrivacy::detach(overlay);DestroyWindow(overlay);state=capturePrivacy::status();check(state.windows==2&&state.failed==0);
 for(auto window:{panel,late}){capturePrivacy::detach(window);DestroyWindow(window);}DestroyWindow(untouched);
 check(capturePrivacy::status().windows==0);
 std::cout<<checks<<" Windows capture-affinity lifecycle checks passed; recorder output not tested\n";
}
