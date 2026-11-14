// Vanta shooting panel. Renderer primitives live here; shooting_ui.hpp owns
// the two-page UI, engine.hpp owns input and sampling, shooting_logic.hpp owns decisions.
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <sstream>
#pragma comment(lib,"comdlg32.lib")
#include "engine.hpp"
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <dcomp.h>
#include <dwmapi.h>
#include <wrl/client.h>
#include <windowsx.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <cmath>
#include <chrono>
#include <algorithm>

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d2d1.lib")
#pragma comment(lib,"dwrite.lib")
#pragma comment(lib,"dcomp.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dwmapi.lib")

using Microsoft::WRL::ComPtr;
namespace fs=std::filesystem;
EXTERN_C const GUID CLSID_D2D1Shadow={0xC67EA361,0x1863,0x4E69,{0x89,0xDB,0x69,0x5D,0x3E,0x9A,0x5B,0x6B}};

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif
enum ACCENT_STATE{ACCENT_ENABLE_ACRYLICBLURBEHIND=4};
struct ACCENT_POLICY{int AccentState;int AccentFlags;unsigned GradientColor;int AnimationId;};
struct WINCOMPATTRDATA{int Attrib;void*pvData;size_t cbData;};
typedef BOOL(WINAPI*SetWCA)(HWND,WINCOMPATTRDATA*);

// --------------------------------- layout ------------------------------------
struct R{float x,y,w,h;};
static const float W=960,H=622;
static R cardS   ={72,151,429,241};   // Shooting
static R cardG   ={514,151,432,241};  // Guarding
static R cardShot={72,404,429,152};    // Shot selection
static R cardMove={514,404,432,152};   // Movement
// shooting rows (relative to card top)
static const float SH_AUTO=62,SH_RELL=101,SH_SLIDE=140,SH_PING=180,SH_HOLD=217;
// guarding rows
static const float GU_AUTO=64,GU_HOLD=111,GU_STANDL=160,GU_SLIDE=190;
// shot selection rows
static const float SS_OUT=64,SS_CEILL=100,SS_SLIDE=124;
// movement rows
static const float MV_DRIB=64,MV_COMBO=108;

static int glass=2;static int page=0;static bool pinned=false;static std::wstring notice; static float iR(R c){return c.x+c.w-21;}
static R toggleAt(R c,float ry){return{iR(c)-46,c.y+ry-13,46,26};}
static R keyAt(R c,float ry){return{iR(c)-32,c.y+ry-15,32,30};}
static R dropAt(R c,float ry,float w){return{iR(c)-w,c.y+ry-14,w,28};}
static R stepPlus(R c,float ry){return{iR(c)-30,c.y+ry-13,30,26};}
static R stepMinus(R c,float ry){return{iR(c)-146,c.y+ry-13,36,26};}
static const R minBtn={873,11,24,24},closeBtn={915,11,24,24};
static const R tabPill={72,56,162,33};

// --------------------------------- state -------------------------------------
static fs::path cfg;
static HWND wnd;
static float scale=1.f,mx=-1,my=-1,tsec=0;
static bool slideDrag=false; static int dragSlider=-1;   // 0 release,1 standoff,2 ceiling
// local control state (not in the external engine)
static bool autoGuard=false,contestOutline=true,dribbleGlide=false;
static float standOff=5.0f,contestCeiling=62.f;
// animation
static float aTog[4]={0,0,0,0},aRel=0,aStand=0,aCeil=0;
// release offset as a draggable min..max window (ms); the engine's trim = midpoint
static int relMin=20,relMax=40; static float aRelMin=.55f,aRelMax=.77f; static int dragRel=-1; // 0 left,1 right,2 band
static float bandGrab=0;
// keybinds (shootKey lives in engine.hpp; guard key is local UI)
static int guardKey='G'; static int capturing=0;           // 0 none, 1 shoot, 2 guard
// dropdown popups
static int pingMode=0,comboMode=0,openDrop=0,dropShown=0; static R dropAnchor{}; static float aDrop=0;
static const wchar_t* pingOpts[]={L"Auto",L"Manual",L"Off"};
static const wchar_t* comboOpts[]={L"Default",L"Aggressive",L"Safe",L"Custom"};

static int uiScale=90;static bool showTooltips=true,reducedMotion=false,alwaysOnTop=false,compactBar=true,lockWindow=false;
static float approach(float c,float t,float rate,float dt){return reducedMotion?t:c+(t-c)*(1.f-std::exp(-rate*dt));}
static float clampf(float v,float a,float b){return v<a?a:v>b?b:v;}
static bool inR(R r,float x,float y){return x>=r.x&&x<=r.x+r.w&&y>=r.y&&y<=r.y+r.h;}
static bool inRpad(R r,float p,float x,float y){return x>=r.x-p&&x<=r.x+r.w+p&&y>=r.y-p&&y<=r.y+r.h+p;}

static void save(){std::ofstream output(cfg);output<<"{\"version\":2,\"earlier_ms\":"<<trim.load()<<",\"shoot_key\":"<<shootKey.load()<<",\"shoot_key_alt\":"<<shootKeyAlt.load()<<",\"minimum_percent\":"<<minRelease.load()<<",\"maximum_percent\":"<<maxRelease.load()<<",\"max_ping\":"<<maxPing.load()<<",\"freshness_ms\":"<<freshnessMs.load()<<",\"hold_early\":"<<int(holdEarly.load())<<",\"protect_fall\":"<<int(protectFall.load())<<",\"contest_limit\":"<<contestLimit.load()<<",\"closeout\":"<<int(closeoutOn.load())<<",\"overhold_lead_ms\":"<<overholdLeadMs.load()<<",\"pump_range\":"<<pumpRange.load()<<",\"pump_delay\":"<<pumpDelay.load()<<",\"pump_contest\":"<<pumpContest.load()<<",\"pump_visuals\":"<<int(pumpVisuals.load())<<",\"ball_arc\":"<<int(ballArcOn.load())<<",\"ball_landing\":"<<int(ballLandingOn.load())<<",\"rolling_intercept\":"<<int(rollingOn.load());defense::writeSettings(output);output<<"}\n";}
static void change(int v){trim=std::clamp(v,-30,60);save();}
// --------------------------------- D3D / D2D ---------------------------------
static ComPtr<ID3D11Device> d3d;
static ComPtr<IDXGISwapChain1> swap;
static ComPtr<ID2D1DeviceContext> dc;
static ComPtr<IDCompositionDevice> comp; static ComPtr<IDCompositionTarget> compTarget; static ComPtr<IDCompositionVisual> visual;
static ComPtr<IDWriteFactory> dw;
static ComPtr<ID2D1SolidColorBrush> solid;
static ComPtr<ID2D1LinearGradientBrush> sheenG,accentG,surfaceG,launchGradient;
static ComPtr<ID2D1RadialGradientBrush> glow;
static ComPtr<ID2D1Bitmap1> target;
struct Shadow{ComPtr<ID2D1Effect> fx;float dy;}; static Shadow shad[4];

enum Fmt{F_LOGO,F_HSUB,F_TAB,F_TITLE,F_CARDT,F_LABEL,F_VAL,F_CTRL,F_SMALL,F_FOOT,F_HERO,F_LAUNCH_HEAD,F_COUNT};
static ComPtr<IDWriteTextFormat> fmt[F_COUNT];

static unsigned theme[]={0x899aff,0x292e43,0x191e2f};
static const unsigned presets[][3]={
 {0x899aff,0x292e43,0x191e2f}, // Moonlight: original default
 {0x829fff,0x303c53,0x1c2638}, // Cobalt
 {0xa7b6cc,0x343d4b,0x212935}, // Slate
 {0x89c5e8,0x304452,0x1c2c38}, // Ice
 {0xa29ded,0x37394e,0x242538}, // Indigo
 {0xb5a0d5,0x403a4d,0x2a2635}, // Amethyst
 {0xc8a5c9,0x443b49,0x2c2632}, // Orchid
 {0xd6a0af,0x493b43,0x30262d}, // Rose
 {0xd4aa8c,0x463e38,0x2e2925}, // Copper
 {0xcbbd9f,0x434139,0x2c2b25}, // Sand
 {0xa6beaa,0x39463f,0x252e29}, // Sage
 {0x8dbfbd,0x334648,0x202e31}, // Teal
 {0xb6b9c2,0x393b42,0x25262c}  // Graphite
};
static const wchar_t* paletteNames[]={L"Moonlight",L"Cobalt",L"Slate",L"Ice",L"Indigo",L"Amethyst",L"Orchid",L"Rose",L"Copper",L"Sand",L"Sage",L"Teal",L"Graphite"};
static unsigned visualTheme[]={0x899aff,0x292e43,0x191e2f},themeFrom[]={0x899aff,0x292e43,0x191e2f};static float themeProgress=1;
static float leftEdge();
static R paletteRect(int i){return {leftEdge()+22+(902-leftEdge())*.25f*(i%4),203.f+43*(i/4),(902-leftEdge())*.25f-12,34};}
static int shootTab=0,settingsTab=0;static float scroll=0,scrollGoal=0,subAnim=0;
static int railOpacity=65;static float railAlpha=.65f;static int dragOpacity=0,windowOpacity=48;static float windowAlpha=.48f;
static void saveTheme(){std::ofstream o(cfg.parent_path()/L"vanta-theme.txt");for(auto v:theme)o<<v<<" ";o<<railOpacity<<" "<<windowOpacity<<" "<<uiScale<<" "<<showTooltips<<" "<<reducedMotion<<" "<<alwaysOnTop<<" "<<compactBar<<" "<<lockWindow<<" "<<pinned;}
static float fade=1.f;static D2D1_COLOR_F C(unsigned rgb,float a=1.f){if(rgb==0x6a8dff||rgb==0x90a9ff||rgb==0x6f9bff||rgb==0x91a4ff)rgb=visualTheme[0];else if(rgb==0x385680)rgb=visualTheme[1];else if(rgb==0x2c456e)rgb=visualTheme[2];else if(rgb==0x53699a)rgb=visualTheme[0];return D2D1::ColorF(((rgb>>16)&255)/255.f,((rgb>>8)&255)/255.f,(rgb&255)/255.f,a*fade);}
static unsigned blend(unsigned a,unsigned b,float t){unsigned out=0;for(int shift:{0,8,16})out|=(unsigned)std::lround(((a>>shift)&255)*(1-t)+((b>>shift)&255)*t)<<shift;return out;}
static D2D1_COLOR_F ink(unsigned c,float a=1){return D2D1::ColorF(c,a*fade);}
static unsigned railSurface(){return blend(blend(visualTheme[2],visualTheme[1],.55f),visualTheme[0],.13f);}
static unsigned textOn(unsigned bg){auto f=[](unsigned v){float x=v/255.f;return x<=.04045f?x/12.92f:std::pow((x+.055f)/1.055f,2.4f);};return .2126f*f(bg>>16)+.7152f*f((bg>>8)&255)+.0722f*f(bg&255)>.179f?0x0b1018:0xf4f6fa;}
static bool showUser=false;static float userAnim=0;
static D2D1_ROUNDED_RECT RR(R r,float rad){return D2D1::RoundedRect(D2D1::RectF(r.x,r.y,r.x+r.w,r.y+r.h),rad,rad);}

static ComPtr<IDWriteTextFormat> mk(float s,DWRITE_FONT_WEIGHT w,DWRITE_TEXT_ALIGNMENT a=DWRITE_TEXT_ALIGNMENT_LEADING){
 ComPtr<IDWriteTextFormat> f; dw->CreateTextFormat(L"Segoe UI",nullptr,w,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,s,L"en-us",&f);
 if(f){f->SetTextAlignment(a);f->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);} return f;}
static ComPtr<ID2D1LinearGradientBrush> mkLin(D2D1_COLOR_F a,D2D1_COLOR_F b){
 D2D1_GRADIENT_STOP s[2]={{0,a},{1,b}}; ComPtr<ID2D1GradientStopCollection> g; dc->CreateGradientStopCollection(s,2,&g);
 ComPtr<ID2D1LinearGradientBrush> br; if(g)dc->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0,0),D2D1::Point2F(0,1)),g.Get(),&br); return br;}
static void refreshThemeBrushes(){float prior=fade;fade=1;launchGradient=mkLin(C(blend(visualTheme[0],0xffffff,.25f),.18f),C(blend(visualTheme[0],visualTheme[2],.5f),.14f));surfaceG=mkLin(C(blend(visualTheme[1],visualTheme[0],.055f),.47f),C(visualTheme[2],.25f));auto c=C(visualTheme[0]);accentG=mkLin(c,D2D1::ColorF(.72f*c.r+.28f,.72f*c.g+.28f,.72f*c.b+.28f));D2D1_GRADIENT_STOP stops[2]={{0,C(visualTheme[0],.4f)},{1,C(visualTheme[0],0)}};ComPtr<ID2D1GradientStopCollection> collection;if(SUCCEEDED(dc->CreateGradientStopCollection(stops,2,&collection))){glow.Reset();dc->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties({0,0},{0,0},1,1),collection.Get(),&glow);}fade=prior;}
static void themeChanged(){std::copy(visualTheme,visualTheme+3,themeFrom);themeProgress=0;saveTheme();}
static void tickTheme(float dt){if(themeProgress>=1)return;themeProgress=reducedMotion?1.f:std::min(1.f,themeProgress+dt/.26f);float t=themeProgress*themeProgress*(3-2*themeProgress);for(int i=0;i<3;i++)visualTheme[i]=blend(themeFrom[i],theme[i],t);if(dc)refreshThemeBrushes();}


static Shadow mkShadow(R r,float rad,float blur,D2D1_COLOR_F c,float dy){
 Shadow sh; sh.dy=dy; ComPtr<ID2D1CommandList> list; if(FAILED(dc->CreateCommandList(&list)))return sh;
 ComPtr<ID2D1Image> prev; dc->GetTarget(&prev); dc->SetTarget(list.Get()); dc->BeginDraw();
 solid->SetColor(C(0,1)); dc->FillRoundedRectangle(RR(r,rad),solid.Get()); dc->EndDraw(); list->Close(); dc->SetTarget(prev.Get());
 dc->CreateEffect(CLSID_D2D1Shadow,&sh.fx);
 if(sh.fx){sh.fx->SetInput(0,list.Get());sh.fx->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION,blur);sh.fx->SetValue(D2D1_SHADOW_PROP_COLOR,c);} return sh;}

static HANDLE frameReady=nullptr;
static bool createDevice(){
 if(frameReady){CloseHandle(frameReady);frameReady=nullptr;}target.Reset();
 RECT client{};GetClientRect(wnd,&client);
 UINT px=std::max(1L,client.right-client.left),py=std::max(1L,client.bottom-client.top);
 D3D_FEATURE_LEVEL fl;
 if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&d3d,&fl,nullptr)))
  if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&d3d,&fl,nullptr)))return false;
 ComPtr<IDXGIDevice> dxgi; d3d.As(&dxgi);
 ComPtr<IDXGIAdapter> ad; dxgi->GetAdapter(&ad); ComPtr<IDXGIFactory2> fac; ad->GetParent(IID_PPV_ARGS(&fac));
 DXGI_SWAP_CHAIN_DESC1 sd{}; sd.Width=px; sd.Height=py; sd.Format=DXGI_FORMAT_B8G8R8A8_UNORM; sd.SampleDesc.Count=1;
 sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.BufferCount=2;sd.Flags=DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT; sd.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; sd.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
 if(FAILED(fac->CreateSwapChainForComposition(d3d.Get(),&sd,nullptr,&swap)))return false;
 ComPtr<IDXGISwapChain2> sc2;if(FAILED(swap.As(&sc2))||FAILED(sc2->SetMaximumFrameLatency(1)))return false;frameReady=sc2->GetFrameLatencyWaitableObject();if(!frameReady)return false;
 ComPtr<ID2D1Factory1> d2f; D2D1_FACTORY_OPTIONS fo{}; D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,fo,d2f.GetAddressOf());
 ComPtr<ID2D1Device> dev; d2f->CreateDevice(dxgi.Get(),&dev); dev->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,&dc); dc->SetDpi(scale*96.f,scale*96.f);
 if(FAILED(DCompositionCreateDevice(dxgi.Get(),IID_PPV_ARGS(&comp))))return false;
 comp->CreateTargetForHwnd(wnd,TRUE,&compTarget); comp->CreateVisual(&visual); visual->SetContent(swap.Get()); compTarget->SetRoot(visual.Get()); comp->Commit();
 DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),(IUnknown**)dw.GetAddressOf());
 fmt[F_LAUNCH_HEAD]=mk(33,DWRITE_FONT_WEIGHT_SEMI_BOLD);
 fmt[F_HERO]=mk(48,DWRITE_FONT_WEIGHT_SEMI_BOLD,DWRITE_TEXT_ALIGNMENT_CENTER);
 fmt[F_LOGO]=mk(23,DWRITE_FONT_WEIGHT_BOLD); fmt[F_HSUB]=mk(14.5f,DWRITE_FONT_WEIGHT_NORMAL); fmt[F_TAB]=mk(13.5f,DWRITE_FONT_WEIGHT_SEMI_BOLD);
 fmt[F_TITLE]=mk(27,DWRITE_FONT_WEIGHT_SEMI_BOLD); fmt[F_CARDT]=mk(19,DWRITE_FONT_WEIGHT_NORMAL); fmt[F_LABEL]=mk(15,DWRITE_FONT_WEIGHT_NORMAL);
 fmt[F_VAL]=mk(16,DWRITE_FONT_WEIGHT_SEMI_BOLD,DWRITE_TEXT_ALIGNMENT_CENTER); fmt[F_CTRL]=mk(13.5f,DWRITE_FONT_WEIGHT_SEMI_BOLD,DWRITE_TEXT_ALIGNMENT_CENTER);
 fmt[F_SMALL]=mk(12.5f,DWRITE_FONT_WEIGHT_NORMAL); fmt[F_FOOT]=mk(13.5f,DWRITE_FONT_WEIGHT_NORMAL);
 dc->CreateSolidColorBrush(C(0xffffff),&solid);
 
 sheenG=mkLin(C(0xb3b9e6,0.16f),C(0x141a2b,.15f));
 refreshThemeBrushes();
 return true;
}

// ------------------------------- primitives ----------------------------------
static void setc(D2D1_COLOR_F c){solid->SetColor(c);solid->SetOpacity(1);}
static void fillR(R r,float rad,D2D1_COLOR_F c){setc(c);dc->FillRoundedRectangle(RR(r,rad),solid.Get());solid->SetOpacity(1);}
static void strokeR(R r,float rad,D2D1_COLOR_F c,float w=1){setc(c);dc->DrawRoundedRectangle(RR(r,rad),solid.Get(),w);solid->SetOpacity(1);}
static void line(float ax,float ay,float bx,float by,D2D1_COLOR_F c,float w=1){setc(c);dc->DrawLine(D2D1::Point2F(ax,ay),D2D1::Point2F(bx,by),solid.Get(),w);solid->SetOpacity(1);}
static void dot(float x,float y,float r,D2D1_COLOR_F c){setc(c);dc->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x,y),r,r),solid.Get());solid->SetOpacity(1);}
static void ring(float x,float y,float r,D2D1_COLOR_F c,float w){setc(c);dc->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x,y),r,r),solid.Get(),w);solid->SetOpacity(1);}
// text in a rect, vertically centered
#include <map>
#include <tuple>
static void T(const std::wstring&s,int f,float x,float y,float w,float h,D2D1_COLOR_F c){
 if(!fmt[f])return;using Key=std::tuple<std::wstring,int,int,int,float,float>;static std::map<Key,ComPtr<IDWriteTextLayout>> cache;
 Key key{s,f,(int)fmt[f]->GetTextAlignment(),(int)fmt[f]->GetWordWrapping(),w,h};if(cache.size()>500)cache.clear();auto& run=cache[key];
 if(!run&&FAILED(dw->CreateTextLayout(s.c_str(),(UINT)s.size(),fmt[f].Get(),w,h,&run)))return;
 setc(c);dc->DrawTextLayout({x,y},run.Get(),solid.Get());}

static void Tleft(const std::wstring&s,int f,float x,float cy,D2D1_COLOR_F c){T(s,f,x,cy-16,400,32,c);}

// frosted-glass card
static void card(R r){
 fillR(r,12,C(0x385680,std::min(1.f,windowAlpha*1.13f)));
 if(sheenG){sheenG->SetStartPoint(D2D1::Point2F(r.x,r.y));sheenG->SetEndPoint(D2D1::Point2F(r.x+r.w*.7f,r.y+r.h));
  setc(C(0,0));dc->FillRoundedRectangle(RR(r,12),sheenG.Get());}
 strokeR(r,12,C(0xcad6ff,0.30f),1.2f);
}

// Shared control motion and surfaces.
static float heat(R r,bool over){struct Hot {float amount=0,at=0;};static std::map<std::pair<int,int>,Hot> vals;auto&v=vals[{int(r.x),int(r.y)}];float dt=clampf(tsec-v.at,0,.06f);v.at=tsec;v.amount=approach(v.amount,over?1:0,20,dt);return v.amount;}
static void panel(R r,float active=0){
 fillR({r.x,r.y+2,r.w,r.h},11,C(0x05070d,.10f));
 if(surfaceG){surfaceG->SetStartPoint({r.x,r.y});surfaceG->SetEndPoint({r.x,r.y+r.h});surfaceG->SetOpacity(fade);dc->FillRoundedRectangle(RR(r,10),surfaceG.Get());surfaceG->SetOpacity(1);}
 strokeR(r,10,C(visualTheme[0],.16f+.16f*active));line(r.x+12,r.y+.5f,r.x+r.w-12,r.y+.5f,C(0xffffff,.045f));
}
static void toggle(R t,float a,bool hover){
 float light=heat(t,hover),pressed=hover&&(GetKeyState(VK_LBUTTON)&0x8000)?1.f:0.f;
 auto off=C(blend(visualTheme[1],0x090c14,.24f),.95f),on=C(visualTheme[0]);
 auto color=D2D1::ColorF(off.r+(on.r-off.r)*a,off.g+(on.g-off.g)*a,off.b+(on.b-off.b)*a,fade*(.9f+.1f*a));
 fillR(t,t.h/2,color);strokeR(t,t.h/2,C(0xffffff,.10f+.08f*light));
 float ky=t.y+t.h/2,kx=t.x+t.h/2+a*(t.w-t.h),kr=t.h/2-4;
 dot(kx,ky+1,kr+.5f,C(0x060910,.22f));fillR({kx-kr-pressed,ky-kr,2*kr+pressed*2,2*kr},kr,C(0xf8faff));
 if(light>.01f)ring(kx,ky,kr+3,C(0xffffff,.12f*light),1);
}
// control chip (key cap / dropdown / stepper button)
static void chip(R r,float rad,bool hover){
 float h=heat(r,hover),pressed=hover&&(GetKeyState(VK_LBUTTON)&0x8000)?1.f:0.f;
 fillR(r,rad,C(blend(visualTheme[2],visualTheme[0],.07f+.06f*h),.72f+.18f*h));strokeR(r,rad,C(visualTheme[0],.20f+.18f*h));
 if(pressed)fillR(r,rad,C(visualTheme[0],.08f));
}
static void keycap(R r,const std::wstring&s,bool hover){chip(r,7,hover);T(s,F_CTRL,r.x,r.y,r.w,r.h,C(0xe2e8f5));}
static void dropdown(R r,const std::wstring&s,bool hover,bool open=false){chip(r,8,hover||open);
 fmt[F_CTRL]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
 T(s,F_CTRL,r.x+14,r.y,r.w-30,r.h,C(0xe0e6f3));
 fmt[F_CTRL]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
 float cx=r.x+r.w-16,cy=r.y+r.h/2;
 if(open){line(cx-4,cy+2,cx,cy-2,C(0xbcccea),1.7f);line(cx+4,cy+2,cx,cy-2,C(0xbcccea),1.7f);}
 else{line(cx-4,cy-2,cx,cy+2,C(0xa7b2cc),1.7f);line(cx+4,cy-2,cx,cy+2,C(0xa7b2cc),1.7f);}}

// simple slider with round thumb
static void slider(float x0,float x1,float y,float t,float trail,bool hover){
 float tx=x0+trail*(x1-x0),h=heat({x0,y-10,x1-x0,20},hover);fillR({x0,y-2,x1-x0,4},2,C(visualTheme[0],.15f));
 accentG->SetStartPoint({x0,y});accentG->SetEndPoint({x1,y});accentG->SetOpacity(fade);dc->FillRoundedRectangle(RR({x0,y-2,tx-x0,4},2),accentG.Get());accentG->SetOpacity(1);
 if(h>.01f)dot(tx,y,10,C(visualTheme[0],.10f*h));dot(tx,y+1,6,C(0x080c14,.5f));dot(tx,y,6,C(0xf0f4ff));dot(tx,y,2.3f,C(visualTheme[0],.9f));
}
// release slider: tick marks + highlighted range box + bright thumb line
static void releaseSlider(float x0,float x1,float y,float t,bool hover){
 int N=19; float bw=(x1-x0)*0.30f, bx=clampf(x0+t*(x1-x0)-bw*0.6f,x0,x1-bw);
 line(x0,y,x1,y,C(0x333c54,0.9f),2);
 // range box
 R box={bx,y-7,bw,17};
 accentG->SetStartPoint(D2D1::Point2F(bx,y));accentG->SetEndPoint(D2D1::Point2F(bx+bw,y));
 setc(C(0x95a6f3,0.38f)); dc->FillRoundedRectangle(RR(box,1),solid.Get()); solid->SetOpacity(1);
 strokeR(box,1,C(0x7ea2ff,0.45f),1);
 // ticks
 for(int i=0;i<N;i++){float tx=x0+(x1-x0)*i/(N-1); bool inb=tx>=bx&&tx<=bx+bw;
  line(tx,y-6,tx,y+6,inb?C(0x9db8ff,0.75f):C(0xffffff,0.16f),1);}
 // thumb
 float px=x0+t*(x1-x0); line(px,y-13,px,y+13,C(0xeaf0ff,0.95f),2.4f); dot(px,y-13,2.2f,C(0xeaf0ff));
}
// dual-handle range slider: drag either end, or the band to move both
static void rangeSlider(float x0,float x1,float y,float a,float b,bool hovA,bool hovB,bool hovBand){
 float span=x1-x0,ax=x0+a*span,bx=x0+b*span;fillR({x0,y-2,span,4},2,C(visualTheme[0],.15f));
 for(int i=0;i<=10;i++){float tx=x0+span*i/10.f;line(tx,y+7,tx,y+10,C(visualTheme[0],.17f));}
 fillR({ax,y-6,bx-ax,12},3,C(visualTheme[0],.1f));accentG->SetStartPoint({ax,y});accentG->SetEndPoint({bx,y});accentG->SetOpacity(fade);dc->FillRoundedRectangle(RR({ax,y-2,bx-ax,4},2),accentG.Get());accentG->SetOpacity(1);
 auto handle=[&](float x,bool hover){float h=heat({x,y-12,6,24},hover),height=16+4*h;fillR({x-4,y-height/2,8,height},3,C(visualTheme[0]));strokeR({x-4,y-height/2,8,height},3,C(0xffffff,.4f));line(x,y-height/2+4,x,y+height/2-4,C(0xffffff,.9f),1.5f);};handle(ax,hovA||hovBand);handle(bx,hovB||hovBand);
}
static float toastAt=-20;static std::wstring toastMessage;
static void announce(const std::wstring& message){toastMessage=message;toastAt=tsec;}
static void drawToast(){float age=tsec-toastAt;if(age<0||age>2.8f||toastMessage.empty())return;float a=reducedMotion?1:std::min(clampf(age/.16f,0,1),clampf((2.8f-age)/.25f,0,1));fade=a;R r{W/2-158,105-6*(1-a),316,38};fillR({r.x,r.y+3,r.w,r.h},9,C(0x040812,.25f));fillR(r,9,C(visualTheme[2],.98f));strokeR(r,9,C(visualTheme[0],.45f));line(r.x+14,r.y+19,r.x+18,r.y+23,C(0x9edcc1),1.5f);line(r.x+18,r.y+23,r.x+25,r.y+15,C(0x9edcc1),1.5f);T(toastMessage,F_TAB,r.x+36,r.y,r.w-46,r.h,C(0xe9effc));fade=1;}
// VK -> short label
static std::wstring vkName(int vk){
 if((vk>='A'&&vk<='Z')||(vk>='0'&&vk<='9'))return std::wstring(1,(wchar_t)vk);
 switch(vk){case VK_XBUTTON1:return L"Mouse 4";case VK_XBUTTON2:return L"Mouse 5";case VK_MBUTTON:return L"Middle";case VK_SPACE:return L"Space";case VK_SHIFT:case VK_LSHIFT:case VK_RSHIFT:return L"Shift";
  case VK_CONTROL:case VK_LCONTROL:case VK_RCONTROL:return L"Ctrl";case VK_MENU:return L"Alt";
  case VK_TAB:return L"Tab";case VK_RETURN:return L"Enter";case VK_BACK:return L"Bksp";case VK_CAPITAL:return L"Caps";}
 if(vk>=VK_F1&&vk<=VK_F12)return L"F"+std::to_wstring(vk-VK_F1+1);
 wchar_t label[64]{};UINT scan=MapVirtualKeyW(vk,MAPVK_VK_TO_VSC_EX);if(scan&&GetKeyNameTextW((scan&255)<<16|((scan&0xff00)?1<<24:0),label,64))return label;
 wchar_t b[8];swprintf(b,8,L"0x%02X",vk);return b;}
// keybind field with a pulsing "listening" state
static void keyField(R r,const std::wstring&s,bool listening,bool hover){
 if(listening){float p=.5f+.5f*std::sin(tsec*6.5f);
  fillR({r.x-3,r.y-3,r.w+6,r.h+6},9,C(0x6a8dff,.16f+.26f*p));
  fillR(r,7,C(0x2b3566,.92f));strokeR(r,7,C(0x8aa6ff,.5f+.4f*p),1.4f);T(L"Press…",F_CTRL,r.x,r.y,r.w,r.h,C(0xdbe4ff));return;}
 chip(r,7,hover);T(s,F_CTRL,r.x,r.y,r.w,r.h,C(0xe2e8f5));}
static float bindAnim=0;static std::wstring bindHint;
static void assignKey(int vk){if(!capturing)return;if(vk==VK_ESCAPE){capturing=0;return;}
 if(vk==VK_F8||vk==VK_LWIN||vk==VK_RWIN){bindHint=L"Reserved by the app or Windows. Choose another key.";return;}
 if(capturing>=100&&capturing<100+defense::Count){if(vk==VK_LBUTTON||vk==VK_RBUTTON||vk==VK_RETURN||vk==VK_OEM_2||vk==shootKey.load()){bindHint=L"Choose a key distinct from shooting, chat and primary clicks.";return;}defense::options[capturing-100].value=vk;}else if(capturing==1||capturing==3){if(vk==(capturing==1?shootKeyAlt.load():shootKey.load())){bindHint=L"That input is already in the other slot.";return;}if(capturing==1)shootKey=vk;else shootKeyAlt=vk;}else guardKey=vk;save();announce(L"Input set to "+vkName(vk));capturing=0;}
static void drawBind(){if(!capturing)return;
 fillR({0,0,W,H},10,C(0x060b15,.60f*bindAnim));float y=215+6*(1-bindAnim);fade=bindAnim;
 R b{249,y,462,172};fillR({b.x,b.y+5,b.w,b.h},13,C(0x03060d,.4f));fillR(b,12,C(visualTheme[2],.99f));strokeR(b,12,C(visualTheme[0],.48f));
 fillR({269,y+20,44,44},10,C(visualTheme[0],.16f));strokeR({280,y+32,22,16},3,C(visualTheme[0]),1.5f);for(int n=0;n<3;n++)line(284+n*6,y+37,286+n*6,y+37,C(0xe8eeff),1.5f);line(285,y+43,297,y+43,C(0xe8eeff));
 Tleft(L"Choose your input",F_CARDT,327,y+30,C(0xf4f7ff));Tleft(L"Listening for a key or mouse button",F_SMALL,327,y+54,C(0xa8b9d4));
 T(bindHint.empty()?L"Mouse side buttons supported. Escape cancels.":bindHint,F_SMALL,269,y+78,420,30,C(bindHint.empty()?0xc0cce0:0xf0b8aa));
 line(269,y+113,691,y+113,C(visualTheme[0],.15f));keycap({269,y+123,141,32},L"Restore default",inR({269,y+123,141,32},mx,my));keycap({585,y+123,103,32},L"Cancel",inR({585,y+123,103,32},mx,my));fade=1;}
// dropdown options popup (drawn as a top overlay)
static void drawPopup(){
 if(dropShown==0||aDrop<.01f)return;
 const wchar_t** opts=dropShown==1?pingOpts:comboOpts; int n=dropShown==1?3:4; int sel=dropShown==1?pingMode:comboMode;
 float ih=31,full=n*ih+8,h=full*(0.4f+0.6f*aDrop);
 R box={dropAnchor.x,dropAnchor.y+dropAnchor.h+7,dropAnchor.w,full};
 fade=aDrop;
 fillR({box.x+2,box.y+5,box.w,h},10,C(0x03050b,.5f));
 fillR({box.x,box.y,box.w,h},10,C(0x20273f,.995f));strokeR({box.x,box.y,box.w,h},10,C(0x9aa8d6,.5f));
 dc->PushAxisAlignedClip({box.x,box.y,box.x+box.w,box.y+h},D2D1_ANTIALIAS_MODE_ALIASED);
 fmt[F_CTRL]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
 for(int i=0;i<n;i++){R it={box.x+5,box.y+4+i*ih,box.w-10,ih-2};bool hov=inR(it,mx,my);
  if(i==sel)fillR(it,6,C(0x6a8dff,.24f));else if(hov)fillR(it,6,C(0xffffff,.07f));
  T(opts[i],F_CTRL,it.x+11,it.y,it.w-24,it.h,C(i==sel?0xeaefff:0xc4cde3));
  if(i==sel){float cx=it.x+it.w-14,cy=it.y+it.h/2;line(cx-6,cy,cx-2,cy+4,C(0x9fc0ff),1.9f);line(cx-2,cy+4,cx+5,cy-5,C(0x9fc0ff),1.9f);}}
 fmt[F_CTRL]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
 dc->PopAxisAlignedClip();fade=1;}

// ------------------------------- card icons ----------------------------------
static void icoTarget(float x,float y,D2D1_COLOR_F c){ring(x,y,9,c,2);ring(x,y,4.5f,c,2);dot(x,y,1.6f,c);}
static void icoShield(float x,float y,D2D1_COLOR_F c){ComPtr<ID2D1PathGeometry> p;ID2D1Factory*f;dc->GetFactory(&f);f->CreatePathGeometry(&p);
 if(p){ComPtr<ID2D1GeometrySink> s;p->Open(&s);s->BeginFigure(D2D1::Point2F(x,y-10),D2D1_FIGURE_BEGIN_HOLLOW);
  s->AddLine(D2D1::Point2F(x+9,y-6));s->AddLine(D2D1::Point2F(x+9,y+2));s->AddBezier(D2D1::BezierSegment(D2D1::Point2F(x+9,y+8),D2D1::Point2F(x+4,y+11),D2D1::Point2F(x,y+12)));
  s->AddBezier(D2D1::BezierSegment(D2D1::Point2F(x-4,y+11),D2D1::Point2F(x-9,y+8),D2D1::Point2F(x-9,y+2)));s->AddLine(D2D1::Point2F(x-9,y-6));s->EndFigure(D2D1_FIGURE_END_CLOSED);s->Close();}
  setc(c);dc->DrawGeometry(p.Get(),solid.Get(),2);solid->SetOpacity(1);f->Release();}
static void icoCross(float x,float y,D2D1_COLOR_F c){ring(x,y,9,c,2);line(x-13,y,x-9,y,c,2);line(x+9,y,x+13,y,c,2);line(x,y-13,x,y-9,c,2);line(x,y+9,x,y+13,c,2);}
static void icoPad(float x,float y,D2D1_COLOR_F c){R b={x-11,y-6,22,13};strokeR(b,6,c,2);line(x-6,y,x-2,y,c,2);line(x-4,y-2,x-4,y+2,c,2);dot(x+4,y-1,1.4f,c);dot(x+6,y+2,1.4f,c);}

// ------------------------------- render --------------------------------------


#include "shooting_ui.hpp"
