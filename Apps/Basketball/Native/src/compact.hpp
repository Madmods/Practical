#pragma once
#pragma comment(lib,"gdi32.lib")
static HWND compact=nullptr;
static COLORREF nativeColor(unsigned c){return RGB(c>>16,(c>>8)&255,c&255);}
static LRESULT CALLBACK compactProc(HWND h,UINT msg,WPARAM w,LPARAM l){
 if(msg==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
 if(msg==WM_LBUTTONUP){ShowWindow(h,SW_HIDE);ShowWindow(wnd,SW_RESTORE);SetForegroundWindow(wnd);return 0;}
 if(msg==WM_SETCURSOR){SetCursor(LoadCursor(nullptr,IDC_HAND));return TRUE;}
 if(msg==WM_TIMER){InvalidateRect(h,nullptr,FALSE);return 0;}
 if(msg==WM_ERASEBKGND)return 1;
 if(msg==WM_PAINT){PAINTSTRUCT ps;HDC screen=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);auto dc0=CreateCompatibleDC(screen);auto bmp=CreateCompatibleBitmap(screen,r.right,r.bottom);auto old=SelectObject(dc0,bmp);
 HBRUSH surface=CreateSolidBrush(nativeColor(visualTheme[2]));FillRect(dc0,&r,surface);DeleteObject(surface);
 auto font=CreateFontW(-r.bottom*37/100,0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");auto oldFont=SelectObject(dc0,font);
 SetBkMode(dc0,TRANSPARENT);RECT t=r;t.left=r.bottom/2;SetTextColor(dc0,RGB(244,247,255));DrawTextW(dc0,L"vanta",-1,&t,DT_SINGLELINE|DT_VCENTER);
 t.left=r.right*21/100;std::wstring status=(armed?L"Armed":L"Standby")+std::wstring(L"   /   ")+std::to_wstring(pingMs.load())+L" ms";if(fillPct>0)status+=L"   /   "+std::to_wstring(fillPct.load())+L"%";SetTextColor(dc0,RGB(211,222,239));DrawTextW(dc0,status.c_str(),-1,&t,DT_SINGLELINE|DT_VCENTER);
 t.right=r.right-r.bottom/2;SetTextColor(dc0,RGB(239,244,255));DrawTextW(dc0,L"Open  +",-1,&t,DT_SINGLELINE|DT_VCENTER|DT_RIGHT);
 RECT strip{0,r.bottom-2,r.right,r.bottom};auto accent=CreateSolidBrush(nativeColor(visualTheme[0]));FillRect(dc0,&strip,accent);DeleteObject(accent);
 BitBlt(screen,0,0,r.right,r.bottom,dc0,0,0,SRCCOPY);SelectObject(dc0,oldFont);DeleteObject(font);SelectObject(dc0,old);DeleteObject(bmp);DeleteDC(dc0);EndPaint(h,&ps);return 0;}
 return DefWindowProcW(h,msg,w,l);}
static void miniShow(HWND main){if(!compactBar)return;
 MONITORINFO mi{sizeof mi};GetMonitorInfoW(MonitorFromWindow(main,MONITOR_DEFAULTTONEAREST),&mi);
 if(!compact){WNDCLASSW c{};c.lpfnWndProc=compactProc;c.hInstance=GetModuleHandleW(nullptr);c.lpszClassName=L"VantaCompactHUD";RegisterClassW(&c);
 compact=CreateWindowExW(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW|WS_EX_TOPMOST,c.lpszClassName,L"Vanta mini",WS_POPUP,0,0,1,1,nullptr,nullptr,c.hInstance,nullptr);}
 int dpi=GetDpiForWindow(main),ww=MulDiv(320,dpi,96),hh=MulDiv(36,dpi,96);
 SetWindowRgn(compact,CreateRoundRectRgn(0,0,ww+1,hh+1,hh/2,hh/2),FALSE);
 SetWindowPos(compact,HWND_TOPMOST,(mi.rcWork.left+mi.rcWork.right-ww)/2,mi.rcWork.top+8,ww,hh,SWP_NOACTIVATE|SWP_SHOWWINDOW);
 SetTimer(compact,1,120,nullptr);}
