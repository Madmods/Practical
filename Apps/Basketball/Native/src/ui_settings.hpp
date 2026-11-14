#pragma once
static R appearanceCard(){float l=leftEdge();return {l,164,(936-l)*.53f,384};}
static R interfaceCard(){auto a=appearanceCard();return {a.x+a.w+14,164,936-a.x-a.w-14,384};}
static R uiPalette(int i){auto a=appearanceCard();if(!i)return {a.x+20,224,a.w-40,48};int j=i-1;float cell=(a.w-40)/3;return {a.x+20+cell*(j%3),284.f+57*(j/3),cell-8,48};}

static R uiSize(int i){auto a=interfaceCard();float cell=(a.w-40)/4;return {a.x+20+cell*i,390,cell-6,29};}
static void applyUISize(){
 MONITORINFO monitor{sizeof monitor};GetMonitorInfoW(MonitorFromWindow(wnd,MONITOR_DEFAULTTONEAREST),&monitor);
 RECT old{};GetWindowRect(wnd,&old);float previousScale=scale;
 scale=std::min(GetDpiForWindow(wnd)/96.f*(uiScale*.01f),std::min((monitor.rcWork.right-monitor.rcWork.left-28)/W,(monitor.rcWork.bottom-monitor.rcWork.top-28)/H));
 int width=(int)lround(W*scale),height=(int)lround(H*scale);
 dc->SetTarget(nullptr);target.Reset();
 if(FAILED(swap->ResizeBuffers(0,width,height,DXGI_FORMAT_UNKNOWN,DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT))){scale=previousScale;return;}
 dc->SetDpi(scale*96,scale*96);
 int x=uiTest?-10000:std::clamp((old.left+old.right-width)/2,monitor.rcWork.left,monitor.rcWork.right-width);
 int y=uiTest?-10000:std::clamp((old.top+old.bottom-height)/2,monitor.rcWork.top,monitor.rcWork.bottom-height);
 SetWindowPos(wnd,nullptr,x,y,width,height,SWP_NOACTIVATE|SWP_NOZORDER);saveTheme();
}
static void editUIOpacity(float x){auto a=interfaceCard();int v=(int)lround(clampf((x-a.x-20)/(a.w-40),0,1)*100);(themeDrag==1?windowOpacity:railOpacity)=v;}
static void drawUISettings(){
 auto a=appearanceCard(),b=interfaceCard();
 for(auto r:{a,b}){panel(r);line(r.x+20,210,r.x+r.w-20,210,C(visualTheme[0],.16f));}
 Tleft(L"Color palette",F_TAB,a.x+20,188,C(0xf6f8ff));Tleft(L"Interface",F_TAB,b.x+20,188,C(0xf6f8ff));
 for(int i=0;i<(int)std::size(presets);i++){auto r=uiPalette(i);bool selected=std::equal(theme,theme+3,presets[i]);
  float h=heat(r,inR(r,mx,my));fillR(r,7,ink(presets[i][2],.65f+.22f*h));strokeR(r,7,ink(selected?visualTheme[0]:presets[i][0],selected?.9f:.22f+.22f*h),selected?1.5f:1.f);if(selected)fillR({r.x+8,r.y+r.h-3,r.w-16,2},1,C(visualTheme[0]));
  if(!i){for(int j=0;j<3;j++)fillR({r.x+12+j*16,r.y+15,12,18},3,ink(presets[i][j]));Tleft(L"Moonlight",F_TAB,r.x+70,r.y+24,C(0xf1f5ff));T(L"Original",F_SMALL,r.x+r.w-68,r.y,62,r.h,C(0xbfcbe3));}
  else{for(int j=0;j<3;j++)fillR({r.x+8+j*(r.w-16)/3,r.y+8,(r.w-16)/3-3,7},2,ink(presets[i][j]));T(paletteNames[i],F_SMALL,r.x+8,r.y+22,r.w-16,22,C(0xf1f5ff));}
 }
 float x=b.x+20,end=b.x+b.w-20;
 for(int i=0;i<2;i++){float y=260+i*65.f;int value=i?railOpacity:windowOpacity;
  Tleft(i?L"Sidebar opacity":L"Window opacity",F_LABEL,x,y-22,C(0xeaf0ff));T(std::to_wstring(value)+L"%",F_CTRL,end-46,y-36,46,27,C(0xeaf0ff));slider(x,end,y,value*.01f,i?railAlpha:windowAlpha,themeDrag==i+1);
 }
 Tleft(L"Interface size",F_LABEL,x,366,C(0xeaf0ff));for(int i=0;i<4;i++){auto r=uiSize(i);int percent=80+i*10;fillR(r,5,C(visualTheme[0],uiScale==percent?.32f:.08f));strokeR(r,5,C(visualTheme[0],uiScale==percent?.7f:.2f));T(std::to_wstring(percent)+L"%",F_CTRL,r.x,r.y,r.w,r.h,C(0xf2f6ff));}
 line(x,439,end,439,C(visualTheme[0],.16f));Tleft(L"Tooltips",F_LABEL,x,465,C(0xeaf0ff));toggle({end-38,454,38,22},showTooltips?1:0,inR({end-38,454,38,22},mx,my));
 Tleft(L"Reduced motion",F_LABEL,x,511,C(0xeaf0ff));toggle({end-38,500,38,22},reducedMotion?1:0,inR({end-38,500,38,22},mx,my));
}
static void resetUISettings(){std::copy(presets[0],presets[0]+3,theme);railOpacity=65;windowOpacity=48;showTooltips=true;reducedMotion=false;uiScale=90;themeChanged();applyUISize();}
static void clickUISettings(float x,float y){
 for(int i=0;i<(int)std::size(presets);i++)if(inR(uiPalette(i),x,y)){std::copy(presets[i],presets[i]+3,theme);themeChanged();return;}

 for(int i=0;i<4;i++)if(inR(uiSize(i),x,y)){uiScale=80+i*10;applyUISize();return;}
 auto r=interfaceCard();float end=r.x+r.w-20;
 for(int i=0;i<2;i++)if(inR({r.x+12,246+i*65.f,r.w-24,28},x,y)){themeDrag=i+1;editUIOpacity(x);SetCapture(wnd);return;}
 if(inR({r.x+20,447,r.w-40,35},x,y)){showTooltips=!showTooltips;saveTheme();return;}
 if(inR({r.x+20,493,r.w-40,35},x,y)){reducedMotion=!reducedMotion;saveTheme();}
}
static void applyWindowOptions(){SetWindowPos(wnd,alwaysOnTop?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);}
static void drawWindowSettings(){float l=leftEdge();
 const wchar_t* names[]={L"Always on top",L"Compact bar when minimized",L"Lock window position"};bool values[]={alwaysOnTop,compactBar,lockWindow};
 for(int i=0;i<3;i++){float y=194+64.f*i;Tleft(names[i],F_LABEL,l+4,y,C(0xeaf0ff));toggle({874,y-11,38,22},values[i]?1:0,inR({874,y-11,38,22},mx,my));line(l,y+32,936,y+32,C(visualTheme[0],.13f));}
 keycap({l+4,382,166,32},L"Center window",inR({l+4,382,166,32},mx,my));
}
static void clickWindowSettings(float x,float y){float l=leftEdge();for(int i=0;i<3;i++)if(inR({l,175+64.f*i,936-l,38},x,y)){bool& setting=i==0?alwaysOnTop:i==1?compactBar:lockWindow;setting=!setting;applyWindowOptions();saveTheme();return;}
 if(inR({l+4,382,166,32},x,y)){MONITORINFO m{sizeof m};GetMonitorInfoW(MonitorFromWindow(wnd,MONITOR_DEFAULTTONEAREST),&m);RECT r;GetWindowRect(wnd,&r);if(!uiTest)SetWindowPos(wnd,nullptr,(m.rcWork.left+m.rcWork.right-(r.right-r.left))/2,(m.rcWork.top+m.rcWork.bottom-(r.bottom-r.top))/2,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);}
}
static void drawGuardingPage(){float l=leftEdge();R r{l,164,936-l,174};panel(r);icoShield(l+32,194,C(visualTheme[0]));Tleft(L"Defense",F_TAB,l+58,194,C(0xf6f8ff));line(l+20,220,916,220,C(visualTheme[0],.16f));Tleft(L"Native controls are not connected yet",F_LABEL,l+24,252,C(0xeaf0ff));T(L"This section is ready for guarding features as we add them.",F_SMALL,l+24,272,840-l,36,C(0xc0cce3));}
