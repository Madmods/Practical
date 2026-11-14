#pragma once
#include "launch_flow.hpp"
#include "launch_palette.hpp"
#include "launch_layout.hpp"
static LaunchPalette lp=launchPalette(visualTheme);
static LaunchFlow launchFlow;
static bool launchFreeze=false,runtimeStarted=false;
static std::wstring loginUser,loginPassword,launchError;
static int loginFocus=0;static bool loginSelect=false,loginShowPassword=false;
static constexpr float LaunchWidth=480,LaunchHeight=552;
static const R launchClose=launchLayout::close;
static const R launchUserBox=launchLayout::user,launchPassBox=launchLayout::password;
static const R launchSubmitBox=launchLayout::primary,launchContinueBox=launchLayout::primary;
static bool launching(){return launchFlow.stage!=LaunchFlow::Ready;}
static float shakePhase=0;static float launchLastPaint=0; // non-zero while an invalid-login shake is playing

static void launchText(const std::wstring&s,int f,R r,unsigned color){T(s,f,r.x,r.y,r.w,r.h,C(color));}
static float launchEase(float v){v=clampf(v,0,1);return v*v*(3-2*v);} // smoothstep
static float launchOutQuint(float v){v=clampf(v,0,1);float u=1-v;return 1-u*u*u*u*u;}

// Layered chevron logo. Three triangular facets picked up gradient tones.
static void launchMark(float x,float y,float size){
 static std::array<ComPtr<ID2D1PathGeometry>,3> facets;
 if(!facets[0]){ComPtr<ID2D1Factory> factory;dc->GetFactory(&factory);
  D2D1_POINT_2F points[3][3]={{{-.5f,-.35f},{-.08f,-.19f},{0,.45f}},{{-.08f,-.19f},{.18f,.1f},{0,.45f}},{{0,.45f},{.1f,-.15f},{.5f,-.35f}}};
  for(int n=0;n<3;n++){ComPtr<ID2D1GeometrySink> sink;
   if(SUCCEEDED(factory->CreatePathGeometry(&facets[n]))&&SUCCEEDED(facets[n]->Open(&sink))){
    sink->BeginFigure(points[n][0],D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(points[n][1]);sink->AddLine(points[n][2]);
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);sink->Close();}}}
 D2D1_MATRIX_3X2_F previous;dc->GetTransform(&previous);
 dc->SetTransform(D2D1::Matrix3x2F::Scale(size,size)*D2D1::Matrix3x2F::Translation(x,y)*previous);
 unsigned cols[]={lp.accentLight,lp.accentDeep,lp.accent};
 for(int n=0;n<3;n++)if(facets[n]){setc(C(cols[n]));dc->FillGeometry(facets[n].Get(),solid.Get());}
 dc->SetTransform(previous);
}

// Soft radial halo behind the logo medallion. No motion, no orbits —
// just a quiet accent lift so the mark reads against the dark surface.
static void launchHalo(float cx,float cy){
 if(glow){glow->SetCenter({cx,cy});glow->SetRadiusX(140);glow->SetRadiusY(140);
  glow->SetOpacity(.22f);dc->FillRectangle({cx-140,cy-140,cx+140,cy+140},glow.Get());
  glow->SetOpacity(1);}
}

static void launchGlyph(float x,float y,bool lock,unsigned color){
 if(lock){strokeR({x-6,y-1,12,10},2,C(color),1.1f);
  setc(C(color));dc->DrawEllipse(D2D1::Ellipse({x,y-3},4,5),solid.Get(),1.1f);
  fillR({x-5,y,10,8},1,C(lp.inset));dot(x,y+4,1.1f,C(color));}
 else{ring(x,y-4,3.1f,C(color),1.1f);strokeR({x-5,y+2,10,7},3,C(color),1.1f);}
}

static void launchSpinner(float x,float y,bool complete){
 if(complete){dot(x,y,9,C(0x8ec8b2,.12f));
  line(x-3,y,x-1,y+2,C(0xa4d6c1),1.35f);line(x-1,y+2,x+4,y-3,C(0xa4d6c1),1.35f);return;}
 ring(x,y,6,C(lp.line),1.1f);
 float a=reducedMotion?0:tsec*3.4f;
 for(int i=0;i<10;i++){float b=a+i*.11f;
  line(x+6*cosf(b),y+6*sinf(b),x+6*cosf(b+.11f),y+6*sinf(b+.11f),C(lp.accent,.22f+.075f*i),1.4f);}
}

// Primary call-to-action. Carries a sliding chevron that drifts on hover,
// an accent gradient fill, and the verifying/entering spinner-progress state.
static void launchButton(R r,const wchar_t* title,bool enabled){
 bool focus=loginFocus==2&&launchFlow.stage==LaunchFlow::Login;
 float h=heat(r,enabled&&(inR(r,mx,my)||focus));
 bool pressed=enabled&&inR(r,mx,my)&&(GetKeyState(VK_LBUTTON)&0x8000);
 float press=heat({r.x+2,r.y+2,r.w,r.h},pressed);r.y+=press*.6f;
 // Outer accent glow when hovered/focused.
 if(h>.001f){fillR({r.x-5,r.y-5,r.w+10,r.h+10},12,C(lp.accent,h*.08f));
  strokeR({r.x-3,r.y-3,r.w+6,r.h+6},10,C(lp.accent,h*.22f));}
 // Drop shadow + fill + gradient sheen.
 fillR({r.x,r.y+4,r.w,r.h},9,C(lp.shadow,.42f));
 fillR(r,9,C(enabled?blend(lp.accent,lp.accentLight,h*.28f):lp.raised));
 if(enabled&&launchGradient){launchGradient->SetStartPoint({r.x,r.y});
  launchGradient->SetEndPoint({r.x,r.y+r.h});
  launchGradient->SetOpacity(pressed?.5f:.88f);
  dc->FillRoundedRectangle(RR(r,9),launchGradient.Get());launchGradient->SetOpacity(1);}
 strokeR(r,9,C(enabled?lp.accentLight:lp.dim,enabled?.42f:.18f));
 // Top inner highlight line.
 line(r.x+10,r.y+1,r.x+r.w-10,r.y+1,C(lp.text,enabled?.26f:.025f));
 T(title,F_CTRL,r.x+36,r.y,r.w-72,r.h,C(enabled?lp.inset:lp.muted));
 bool busy=launchFlow.stage==LaunchFlow::Verifying||launchFlow.stage==LaunchFlow::Entering;
 if(busy){float dur=launchFlow.stage==LaunchFlow::Verifying?.45f:.5f;
  float p=clampf(launchFlow.age/dur,0,1);
  // Progress underline uses an accent gradient instead of a flat bar.
  R bar{r.x+10,r.y+r.h-4,(r.w-20)*p,2};
  if(accentG){accentG->SetStartPoint({bar.x,bar.y});accentG->SetEndPoint({bar.x+bar.w,bar.y});
   accentG->SetOpacity(.95f);dc->FillRoundedRectangle(RR(bar,1),accentG.Get());accentG->SetOpacity(1);}
  else fillR(bar,1,C(lp.accentLight,.85f));
  launchSpinner(r.x+r.w-24,r.y+r.h/2,false);return;}
 // Chevron that drifts right on hover.
 float cx=r.x+r.w-26+4.5f*h,cy=r.y+r.h/2;
 unsigned stroke=enabled?lp.inset:lp.dim;
 line(cx-6,cy,cx+5,cy,C(stroke),1.4f);
 line(cx+1,cy-5,cx+6,cy,C(stroke),1.4f);
 line(cx+6,cy,cx+1,cy+5,C(stroke),1.4f);
}

// Input field. Left accent dot lights up on focus; labeled with a lock or
// avatar glyph; eye button clips off the right side for password slots.
static void launchInput(R r,const std::wstring& value,int slot,const wchar_t* placeholder,bool password){
 bool focus=slot==loginFocus;
 float h=heat(r,inR(r,mx,my)||focus);
 float f=heat({r.x-1,r.y-1,r.w,r.h},focus);
 // Focus glow ring outside the field.
 if(f>.001f){strokeR({r.x-3,r.y-3,r.w+6,r.h+6},10,C(lp.accent,f*.28f));}
 // Field body.
 fillR({r.x,r.y+2,r.w,r.h},8,C(lp.shadow,.28f));
 fillR(r,8,C(lp.inset,std::min(1.f,windowAlpha+.2f)));
 strokeR(r,8,C(blend(lp.line,lp.accent,f),.65f+.35f*h),1);
 
 // Left accent pill — a focused-state indicator (muted when inactive).
 
 // Lock / user icon.
 launchGlyph(r.x+30,r.y+r.h/2,password,blend(lp.dim,lp.accentLight,.1f+.85f*f));
 
 // Text content, with visibility handling for the password slot.
 std::wstring text=password&&!loginShowPassword?std::wstring(value.size(),L'\x2022'):value;
 float x=r.x+54,width=r.w-(password?102:66),textWidth=0;
 ComPtr<IDWriteTextLayout> layout;
 if(!text.empty()&&SUCCEEDED(dw->CreateTextLayout(text.c_str(),(UINT)text.size(),
   fmt[F_LABEL].Get(),4096,40,&layout))){DWRITE_TEXT_METRICS m{};layout->GetMetrics(&m);
  textWidth=m.widthIncludingTrailingWhitespace;}
 dc->PushAxisAlignedClip({x,r.y+3,x+width,r.y+r.h-3},D2D1_ANTIALIAS_MODE_ALIASED);
 if(focus&&loginSelect&&!text.empty())
  fillR({x,r.y+11,std::min(width,textWidth+3),r.h-22},3,C(lp.accent,.26f));
 float shift=std::max(0.f,textWidth-width+5);
 if(text.empty())T(placeholder,F_LABEL,x,r.y,width,r.h,C(lp.muted,.85f));
 else{auto wrap=fmt[F_LABEL]->GetWordWrapping();
  fmt[F_LABEL]->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
  T(text,F_LABEL,x-shift,r.y,std::max(width,textWidth+8),r.h,C(lp.text));
  fmt[F_LABEL]->SetWordWrapping(wrap);}
 if(focus&&!loginSelect){float a=reducedMotion?1:.35f+.65f*(.5f+.5f*cosf(tsec*5));
  line(x+textWidth-shift,r.y+13,x+textWidth-shift,r.y+r.h-13,C(lp.accentLight,a),1.1f);}
 dc->PopAxisAlignedClip();
 // Eye toggle — keep the 42x44 hit area the tests rely on.
 if(password){R eye{r.x+r.w-42,r.y,42,r.h};
  float over=heat(eye,inR(eye,mx,my));
  fillR({eye.x+6,eye.y+8,30,eye.h-16},5,C(lp.body,.09f*over));
  float ex=r.x+r.w-21,y=r.y+r.h/2;
  setc(C(blend(lp.muted,lp.body,over)));
  dc->DrawEllipse(D2D1::Ellipse({ex,y},7,4),solid.Get(),1);
  dot(ex,y,1.7f,C(lp.muted));
  if(!loginShowPassword)line(ex-7,y+6,ex+7,y-6,C(lp.muted),1.1f);}
}

static void launchStatusRow(const wchar_t* name,const wchar_t* value,float y,bool ready,int index,float age){
 float a=reducedMotion?1:launchEase((age-index*.045f)/.25f);
 dc->PushLayer(D2D1::LayerParameters1(D2D1::InfiniteRect(),nullptr,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
  D2D1::Matrix3x2F::Identity(),a),nullptr);
 R row{40,y,400,28};
 float h=heat(row,inR(row,mx,my));
 fillR(row,6,C(lp.accent,h*.035f));
 launchSpinner(52,y+14,ready);
 launchText(name,F_LABEL,{72,y,216,28},lp.body);
 R pill{322,y+3,110,22};
 fillR(pill,5,C(ready?0x9fc5b8:lp.accent,.08f));
 strokeR(pill,5,C(ready?0x9fc5b8:lp.accent,.22f),1);
 T(value,F_CTRL,pill.x,pill.y,pill.w,pill.h,C(ready?0xbcddd0:lp.body));
 if(index<2)line(72,y+33,432,y+33,C(lp.body,.085f));
 dc->PopLayer();
}

static float launchLabelWidth(const wchar_t* name){
 ComPtr<IDWriteTextLayout> run;DWRITE_TEXT_METRICS m{};
 if(SUCCEEDED(dw->CreateTextLayout(name,(UINT)wcslen(name),fmt[F_TAB].Get(),200,28,&run)))run->GetMetrics(&m);
 return m.widthIncludingTrailingWhitespace;
}

// Stepper header: Account → Roblox with an animated underline that slides
// between the two label widths when the stage advances.
static void launchSteps(bool connected){
 // Two quiet milestones, with no competing card behind the page heading.
 for(int i=0;i<2;i++){
  float x=32.f+i*218.f;bool active=connected?(i==1):(i==0);
  unsigned color=active?lp.accentLight:lp.muted;
  T(i?L"02":L"01",F_SMALL,x,77,24,22,C(color));
  launchText(i?L"Connection":L"Account",F_TAB,{x+28,77,160,22},active?lp.text:lp.muted);
  fillR({x,107,198,2},1,C(active?lp.accent:lp.line,active?.85f:.5f));
  if(connected&&!i){line(x+175,85,x+179,89,C(0xa4d6c1),1.3f);
   line(x+179,89,x+185,82,C(0xa4d6c1),1.3f);}
 }
}

static void drawLaunchStage(LaunchFlow::Stage stage,float alpha,float yOffset,float age){
 if(alpha<=.001f)return;
 dc->PushLayer(D2D1::LayerParameters1(D2D1::InfiniteRect(),nullptr,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
  D2D1::Matrix3x2F::Identity(),alpha),nullptr);
 dc->SetTransform(D2D1::Matrix3x2F::Translation(0,yOffset));

 if(stage==LaunchFlow::Intro){
  // Presentation progress only: this never represents authentication work.
  const float cx=LaunchWidth/2,cy=220;
  float p=launchEase(age/2.35f);
  launchHalo(cx,cy);
  ring(cx,cy,88,C(lp.accent,.055f),1);
  ring(cx,cy,70,C(lp.accent,.1f),1);
  for(int i=0;i<48;i++){
   float a=i*6.2831853f/48-1.5707963f;
   bool lit=i<static_cast<int>(p*48);
   float inner=i%4==0?80.f:84.f;
   line(cx+inner*cosf(a),cy+inner*sinf(a),cx+89*cosf(a),cy+89*sinf(a),
    C(lit?lp.accentLight:lp.line,lit?.75f:.3f),i%4==0?1.5f:1.f);
  }
  float breathing=reducedMotion?0:.5f+.5f*sinf(tsec*1.8f);
  fillR({cx-43,cy-43,86,86},24,C(lp.accent,.04f+.025f*breathing));
  fillR({cx-35,cy-35,70,70},19,C(lp.raised));
  strokeR({cx-35,cy-35,70,70},19,C(lp.accent,.3f));
  line(cx-19,cy-34,cx+19,cy-34,C(lp.accentLight,.4f));
  launchMark(cx,cy,39);
  T(L"vanta",F_HERO,70,323,340,58,C(lp.text));
  T(L"A better place to begin.",F_CTRL,70,384,340,24,C(lp.body));
  R track{132,439,216,3};
  fillR(track,2,C(lp.line,.65f));
  fillR({track.x,track.y,track.w*p,track.h},2,C(lp.accent));
  T(p<.99f?L"Preparing your interface":L"Ready to sign in",F_CTRL,70,457,340,24,C(lp.muted));
 }else if(stage==LaunchFlow::Login||stage==LaunchFlow::Verifying){
  // ---- LOGIN ----
  // Horizontal shake on invalid, decaying over ~.4s.
  float shakeX=0;
  if(launchFlow.invalid&&shakePhase<.4f&&!reducedMotion){
   shakeX=cosf(shakePhase*48)*(1-shakePhase/.4f)*6;
   dc->SetTransform(D2D1::Matrix3x2F::Translation(shakeX,yOffset));
  }

  launchText(L"Welcome back.",F_TITLE,launchLayout::title,lp.text);
  launchText(L"Your workspace starts here.",F_LABEL,launchLayout::subtitle,lp.body);
  // Restrained, full-width fields provide a clear reading and keyboard order.
  launchText(L"Username",F_TAB,launchLayout::userLabel,lp.body);
  launchInput(launchUserBox,loginUser,0,L"Enter your username",false);
  launchText(L"Password",F_TAB,launchLayout::passwordLabel,lp.body);
  launchInput(launchPassBox,loginPassword,1,L"Enter your password",true);

  // Hint row: error, caps-lock warning, or default prompt.
  if(launchFlow.invalid){
   R hint=launchLayout::hint;
   fillR(hint,6,C(0xd87f8c,.11f));
   strokeR(hint,6,C(0xd87f8c,.26f),1);
   dot(hint.x+13,hint.y+hint.h/2,2.3f,C(0xf0b7c0));
   launchText(loginUser.empty()||loginPassword.empty()?L"Enter a username and password to continue.":L"Try again. Use 1 for both preview fields.",F_SMALL,
    {hint.x+26,hint.y,hint.w-32,hint.h},0xe9b4bc);
  }else if(loginFocus==1&&(GetKeyState(VK_CAPITAL)&1)){
   R hint=launchLayout::hint;
   dot(hint.x+13,hint.y+hint.h/2,2.3f,C(0xd9c488));
   launchText(L"Caps Lock is on",F_SMALL,{hint.x+26,hint.y,hint.w-32,hint.h},0xcdb982);
  }else{
   R hint=launchLayout::hint;
   launchText(L"Local preview. No credentials are sent.",F_SMALL,
    {hint.x,hint.y,hint.w,hint.h},lp.muted);
  }

  launchButton(launchSubmitBox,stage==LaunchFlow::Verifying?L"Checking preview…":L"Continue",stage==LaunchFlow::Login);

  // Divider + preview badge.
  line(32,478,448,478,C(lp.accent,.11f));
  dot(42,495,1.6f,C(lp.muted));
  launchText(L"Demo access  /  username & password",F_SMALL,{54,484,280,22},lp.muted);
  R pb=launchLayout::previewBadge;
  fillR(pb,5,C(lp.accent,.12f));
  strokeR(pb,5,C(lp.accent,.26f),1);
  T(L"1 / 1",F_CTRL,pb.x,pb.y,pb.w,pb.h,C(lp.text));

  if(shakeX!=0)dc->SetTransform(D2D1::Matrix3x2F::Translation(0,yOffset));

 }else{
  // ---- CONNECT ----
  auto state=connectionState.load();bool ready=linked,found=state!=ConnectionState::Waiting;

  launchText(ready?L"Ready to launch":L"Almost there.",F_TITLE,launchLayout::title,lp.text);
  launchText(ready?L"Your connection is ready. Open your workspace.":L"Waiting for the application connection.",
   F_LABEL,launchLayout::subtitle,lp.muted);

  // Client card.
  R app=launchLayout::client;
  fillR({app.x,app.y+4,app.w,app.h},10,C(lp.shadow,.26f));
  fillR(app,10,C(lp.raised,std::min(1.f,windowAlpha+.18f)));
  strokeR(app,10,C(lp.body,.18f));
  line(44,app.y+app.h-1,app.x+app.w-12,app.y+app.h-1,C(lp.body,.1f));
  // Roblox icon tile.
  fillR({48,220,40,40},9,C(lp.body,.085f));
  strokeR({48,220,40,40},9,C(lp.body,.14f));
  D2D1_MATRIX_3X2_F prev;dc->GetTransform(&prev);
  dc->SetTransform(D2D1::Matrix3x2F::Rotation(14,{68,240})*prev);
  fillR({58,230,20,20},2,C(lp.text));
  fillR({65,237,6,6},1,C(lp.raised));
  dc->SetTransform(prev);
  launchText(L"Application connection",F_TAB,{104,220,306,22},lp.text);
  launchText(ready?L"Client detected":connectionLabel(),F_SMALL,{104,244,328,22},ready?0x8ebba9:lp.muted);
  // Pulsing dot when we are still waiting.
  float pulse=ready||reducedMotion?1.f:.55f+.45f*(.5f+.5f*cosf(tsec*3));
  dot(428,228,3.2f,C(ready?0x92c9b3:lp.accent,pulse));
  if(!ready&&!reducedMotion)ring(428,228,5,C(lp.accent,.35f*pulse),1);

  // Diagnostic rows.
  fillR(launchLayout::checks,12,C(lp.raised,.28f));
  strokeR(launchLayout::checks,12,C(lp.accent,.1f));
  launchStatusRow(L"Client",found?L"Detected":L"Searching",300,found,0,age);
  launchStatusRow(L"Version",state==ConnectionState::VersionMismatch?L"Update required":
   state==ConnectionState::Loading||ready?L"Compatible":L"Pending",
   338,state==ConnectionState::Loading||ready,1,age);
  launchStatusRow(L"Session data",ready?L"Available":L"Waiting",376,ready,2,age);

  launchButton(launchContinueBox,stage==LaunchFlow::Entering?L"Launching…":
   ready?L"Open workspace":L"Waiting for connection…",ready&&stage!=LaunchFlow::Entering);
  if(!launchError.empty())launchText(launchError,F_SMALL,{32,480,416,26},0xe4a3a3);
 }
 dc->SetTransform(D2D1::Matrix3x2F::Identity());
 dc->PopLayer();
}

static void drawLaunch(){
 lp=launchPalette(visualTheme);
 // Matte shell with a restrained accent wash, shared by all launcher states.
 fillR({0,0,LaunchWidth,LaunchHeight},0,C(lp.base,windowAlpha));
 if(glow){glow->SetCenter({LaunchWidth,0});glow->SetRadiusX(420);glow->SetRadiusY(340);
  glow->SetOpacity(.11f);dc->FillRectangle({0,0,LaunchWidth,LaunchHeight},glow.Get());glow->SetOpacity(1);}
 strokeR({.5f,.5f,LaunchWidth-1,LaunchHeight-1},0,C(lp.body,.12f));
 line(24,57,LaunchWidth-24,57,C(lp.body,.1f));
 launchMark(34,29,22);
 launchText(L"V",F_TAB,{54,14,80,28},lp.text);
 R preview{333,19,78,21};
 fillR(preview,5,C(lp.accent,.08f));
 strokeR(preview,5,C(lp.accent,.2f));
 T(L"PREVIEW",F_CTRL,preview.x,preview.y,preview.w,preview.h,C(lp.accentLight));

 // Animated stepper (fades in on first transition out of Intro).
 if(launchFlow.stage!=LaunchFlow::Intro){
  float stripAlpha=reducedMotion||launchFlow.previous!=LaunchFlow::Intro?1:launchEase(launchFlow.age/.26f);
  dc->PushLayer(D2D1::LayerParameters1(D2D1::InfiniteRect(),nullptr,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
   D2D1::Matrix3x2F::Identity(),stripAlpha),nullptr);
  launchSteps(launchFlow.stage>=LaunchFlow::Connect);
  dc->PopLayer();
 }

 // Stage body (optionally crossfaded between previous and current).
 bool isAccount=launchFlow.stage==LaunchFlow::Login||launchFlow.stage==LaunchFlow::Verifying;
 bool wasAccount=launchFlow.previous==LaunchFlow::Login||launchFlow.previous==LaunchFlow::Verifying;
 bool samePage=launchFlow.stage==launchFlow.previous||(isAccount&&wasAccount)||launchFlow.stage==LaunchFlow::Entering;
 if(reducedMotion||samePage)drawLaunchStage(launchFlow.stage,1,0,launchFlow.age);
 else{
  float out=1-launchEase(launchFlow.age/.15f),in=launchOutQuint((launchFlow.age-.07f)/.26f);
  if(out>.001f)drawLaunchStage(launchFlow.previous,out,-5*(1-out),launchFlow.previousAge);
  drawLaunchStage(launchFlow.stage,in,8*(1-in),launchFlow.age);
 }

 // Footer reports the current page; connection details belong on step two.
 line(24,LaunchHeight-42,LaunchWidth-24,LaunchHeight-42,C(lp.body,.1f));
 bool account=launchFlow.stage<=LaunchFlow::Verifying;
 dot(33,LaunchHeight-21,2.5f,C(account?lp.accent:linked?0x9bc9b6:lp.muted));
 launchText(account?L"Local preview":linked?L"Connection ready":L"Waiting for connection",F_SMALL,
  {45,LaunchHeight-34,290,26},lp.muted);
 T(L"WINDOWS",F_SMALL,364,LaunchHeight-34,84,26,C(lp.muted));

 // Window close glyph (same hit zone as before).
 float h=heat(launchClose,inR(launchClose,mx,my));
 fillR(launchClose,6,C(0xcc7884,.18f*h));
 line(launchClose.x+10,launchClose.y+9,launchClose.x+launchClose.w-10,launchClose.y+launchClose.h-9,
  C(blend(lp.muted,0xf2c6cd,h)),1.2f);
 line(launchClose.x+launchClose.w-10,launchClose.y+9,launchClose.x+10,launchClose.y+launchClose.h-9,
  C(blend(lp.muted,0xf2c6cd,h)),1.2f);

 // Advance the invalid-shake decay while the login is on screen.
 float elapsed=clampf(tsec-launchLastPaint,0,.1f);launchLastPaint=tsec;
 if(launchFlow.invalid){shakePhase=std::min(1.f,shakePhase+elapsed);}else shakePhase=0;
}

static void submitLogin(){if(launchFlow.submit(loginUser,loginPassword)){loginPassword.clear();loginShowPassword=false;loginSelect=false;loginFocus=-1;}else shakePhase=0;}

// Bounded single-line paste. Never persist or log clipboard contents.
static void pasteLoginText(const wchar_t* value,size_t count){
 if(launchFlow.stage!=LaunchFlow::Login||loginFocus<0||loginFocus>1||!value)return;
 auto& field=loginFocus?loginPassword:loginUser;
 if(loginSelect)field.clear();loginSelect=false;
 for(size_t i=0;i<count&&value[i]&&field.size()<40;i++)
  if(value[i]>=32&&value[i]!=127)field+=value[i];
 launchFlow.invalid=false;shakePhase=0;
}

// Event handling is unchanged from the previous revision — the visual rewrite
// above preserves all hit rects (close, user, password + eye, submit, continue)
// so launcher_ui_tests and launcher_input_tests remain valid.
static bool launchEvent(HWND h,UINT msg,WPARAM w,LPARAM l,LRESULT& result){
 if(!launching())return false;result=0;
 switch(msg){
 case WM_SETCURSOR:{LPCWSTR cursor=IDC_ARROW;
  if(inR(launchClose,mx,my))cursor=IDC_HAND;
  else if(launchFlow.stage==LaunchFlow::Login){
   if(inR(launchUserBox,mx,my)||inR(launchPassBox,mx,my))cursor=IDC_IBEAM;
   if(inR({launchPassBox.x+launchPassBox.w-42,launchPassBox.y,42,44},mx,my))cursor=IDC_HAND;
   if(inR(launchSubmitBox,mx,my))cursor=IDC_HAND;}
  else if(launchFlow.stage==LaunchFlow::Connect&&linked&&inR(launchContinueBox,mx,my))cursor=IDC_HAND;
  SetCursor(LoadCursorW(nullptr,cursor));result=TRUE;return true;}
 case WM_MOUSEMOVE:mx=GET_X_LPARAM(l)/scale;my=GET_Y_LPARAM(l)/scale;return true;
 case WM_LBUTTONDOWN:{mx=GET_X_LPARAM(l)/scale;my=GET_Y_LPARAM(l)/scale;
  if(inR(launchClose,mx,my)){DestroyWindow(h);return true;}
  if(launchFlow.stage==LaunchFlow::Login){
   if(inR({launchPassBox.x+launchPassBox.w-42,launchPassBox.y,42,44},mx,my)){loginShowPassword=!loginShowPassword;return true;}
   loginFocus=inR(launchUserBox,mx,my)?0:inR(launchPassBox,mx,my)?1:-1;loginSelect=false;
   if(inR(launchSubmitBox,mx,my))submitLogin();
  }else if(launchFlow.stage==LaunchFlow::Connect&&inR(launchContinueBox,mx,my))launchFlow.proceed(linked);
  if(my<52&&!lockWindow){ReleaseCapture();SendMessageW(h,WM_NCLBUTTONDOWN,HTCAPTION,l);}return true;}
 case WM_KEYDOWN:
  if(launchFlow.stage==LaunchFlow::Login){
   if(w==VK_TAB){loginFocus=(loginFocus+((GetKeyState(VK_SHIFT)&0x8000)?2:1)+3)%3;loginSelect=false;}
   else if(w==VK_RETURN||(w==VK_SPACE&&loginFocus==2))submitLogin();
   else if(w=='V'&&(GetKeyState(VK_CONTROL)&0x8000)){if(h)SendMessageW(h,WM_PASTE,0,0);}
   else if(w==VK_ESCAPE){loginSelect=false;loginShowPassword=false;}
   else if(w=='A'&&(GetKeyState(VK_CONTROL)&0x8000))loginSelect=true;
  }else if(launchFlow.stage==LaunchFlow::Connect&&w==VK_RETURN)launchFlow.proceed(linked);
  return true;
 case WM_PASTE:
  if(launchFlow.stage==LaunchFlow::Login&&loginFocus>=0&&loginFocus<2&&OpenClipboard(h)){
   HANDLE data=GetClipboardData(CF_UNICODETEXT);
   if(data){const wchar_t* value=static_cast<const wchar_t*>(GlobalLock(data));
    if(value){pasteLoginText(value,std::min<size_t>(GlobalSize(data)/sizeof(wchar_t),4096));GlobalUnlock(data);}}
   CloseClipboard();
  }return true;
 case WM_CHAR:
  if(launchFlow.stage==LaunchFlow::Login&&loginFocus>=0&&loginFocus<2){auto& text=loginFocus?loginPassword:loginUser;
   if(w==8){if(loginSelect)text.clear();else if(!text.empty())text.pop_back();loginSelect=false;}
   else if(w>=32&&w!=127){if(loginSelect)text.clear();loginSelect=false;if(text.size()<40)text+=(wchar_t)w;}
   launchFlow.invalid=false;shakePhase=0;
  }return true;
 case WM_MOUSEWHEEL:case WM_LBUTTONUP:case WM_RBUTTONDOWN:case WM_XBUTTONDOWN:case WM_MBUTTONDOWN:return true;
 default:return false;
 }
}

