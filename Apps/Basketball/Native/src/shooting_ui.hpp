#pragma once
#include "pump_overlay.hpp"
#include "navigation_motion.hpp"
static NavigationMotion sidebarMotion;
static float drawer=0,drawerClock=0,linger=0;static bool railOpen=false;static int navGroup=0;static int themeDrag=0;static bool expanded[4]={true,true,false,false};static float expansion[4]={1,1,0,0};
static float railWidth(){return 60+(NavigationExpandedWidth-60)*drawer;}
static float leftEdge(){return railWidth()+18;}
static float savedScroll[2]={},savedScrollGoal[2]={};static bool scrollDragging=false,focusFeature=false;static float scrollGrab=0;
static int section=3,fromTab=0,selectionFeature=0,fromFeature=0,lastFeatures[11]={};static float entry=1;
static bool bench=false,captureDone=false,uiTest=false;static double benchStart=0;
static std::vector<double>frameTimes;static int qaTab=0;
static void go(int pg,int sub=3){page=0;section=3;}
static int navFrom=0;static float navBlend=1;
static void switchGroup(int n){if(n==navGroup)return;navFrom=navGroup;navGroup=n;navBlend=0;}
static void resetDefenseScroll();
static int contextOf(int tab){return tab<=1?0:tab==2?4:tab==4||tab>=7&&tab<=10?1:tab==5?3:2;}
static const wchar_t* contextTitle(int tab){static const wchar_t* names[]={L"Shooting",L"Defense",L"Settings",L"Configs",L"Visuals"};return names[contextOf(tab)];}
static bool treeOpen[5]={true,false,true,false,false};static float treeAnim[5]={1,0,1,0,0};
static void openTree(int ctx){treeOpen[ctx]=true;}
static void selectFeature(int tab,int feature){if(tab!=shootTab){resetDefenseScroll();if(shootTab<2){savedScroll[shootTab]=scroll;savedScrollGoal[shootTab]=scrollGoal;}scroll=tab<2?savedScroll[tab]:0;scrollGoal=tab<2?savedScrollGoal[tab]:0;}focusFeature=tab<2;openTree(contextOf(tab));switchGroup(tab==3||tab==5||tab==6?1:0);lastFeatures[tab]=feature;if(tab>=2){fromTab=shootTab;fromFeature=selectionFeature;shootTab=tab;selectionFeature=0;entry=0;return;}expanded[tab?feature+1:feature?3:0]=true;if(tab==shootTab&&feature==selectionFeature)return;fromTab=shootTab;fromFeature=selectionFeature;shootTab=tab;selectionFeature=feature;entry=0;}
static void selectTab(int n){selectFeature(n,lastFeatures[n]);}
static bool finding=false;static int matchPos=0;static std::wstring query;
struct Shortcut{const wchar_t*name,*group,*tags;int pg,sec,tab,offset;};
static const Shortcut indexList[]={
 {L"Appearance",L"Settings",L"ui settings theme colors palette opacity size scale tooltips animations",0,3,3,0},
 {L"Window",L"Settings",L"always on top compact minimize lock position",0,3,6,0},
 {L"Auto guard",L"Defense",L"guarding screens lock",0,3,4,0},
 {L"Off-ball guard",L"Defense",L"receiver matchup select deny",0,3,7,0},
 {L"Steals and interceptions",L"Defense",L"steal catch pass recovery",0,3,8,0},
 {L"Blocks and alley-oops",L"Defense",L"block dunk lob jump",0,3,9,0},
 {L"Auto rebound",L"Defense",L"rebound chase jump",0,3,10,0},
 {L"Configs",L"System",L"profiles save load",0,3,5,0},
 {L"Ball intercept",L"Visuals",L"rolling bouncing loose ball angle cutoff route arrival",0,3,2,0},
 {L"Ball arc",L"Visuals",L"flight trajectory landing prediction marker",0,3,2,0},
 {L"Auto time",L"Timing",L"automatic shooting",0,3,0,0},{L"Pump fake punish",L"Timing",L"bait jump opening reaction range visuals",0,3,0,1},{L"Release adjustment",L"Timing",L"offset milliseconds early late",0,3,0,0},
 {L"Maximum ping",L"Timing",L"latency limit",0,3,0,0},{L"Meter freshness",L"Timing",L"sample stale",0,3,0,0},{L"Shot input",L"Timing",L"keybind hold mouse",0,3,0,0},
 {L"Shot assist",L"Shot selection",L"release window percentage",0,3,1,0},{L"Hold early release",L"Shot selection",L"minimum",0,3,1,0},{L"Prevent overhold",L"Shot selection",L"falling minimum floor",0,3,1,0},{L"Overhold lead",L"Shot selection",L"earlier late delay",0,3,1,0},
 {L"Contest ceiling",L"Shot selection",L"maximum pressure",0,3,1,1},{L"Release before a closeout",L"Shot selection",L"contest threat",0,3,1,1}};
#include "finder.hpp"
static R leftCard,rightCard,autoSwitch,assistSwitch,ceilingSwitch,shotBind,shotBindAlt;
static R earlySwitch,fallSwitch,closeoutSwitch;static float rx0=102,rx1=500;static float rangeY=270;
static float uiToggle[11]={},uiSlider[8]={};static int editing=-1,rangeHandle=-1;static float rangeGrab=0;
struct Track{int id,tab;float x0,x1,y;int low,high,step;};
static Track tracks[]={{0,0,0,0,300,-30,60,1},{1,0,0,0,380,50,1000,25},{2,0,0,0,460,60,200,10},{3,1,0,0,306,5,100,5},{4,1,0,0,392,0,120,5},{5,0,0,0,0,4,16,1},{6,0,0,0,0,60,300,10},{7,0,0,0,0,0,30,5}};
static R headers[4],pumpSwitch,pumpVisualSwitch;
static void updateLayout(){float l=leftEdge(),w=936-l;headers[0]={l,164,w,48};headers[1]={l,164,w,48};headers[2]={l,224+226*expansion[1],w,48};headers[3]={l,224+232*expansion[0],w,48};pumpSwitch={874,headers[3].y+13,38,22};pumpVisualSwitch={874,headers[3].y+215,38,22};
 leftCard={l,164,w,48+232*expansion[0]};rightCard={l,164,w,48+226*expansion[1]};
 autoSwitch={874,177,38,22};assistSwitch=autoSwitch;ceilingSwitch={874,headers[2].y+13,38,22};
 rx0=l+24;rx1=912;rangeY=270;shotBind={714,390,86,28};shotBindAlt={826,390,86,28};earlySwitch={874,294,38,22};fallSwitch={874,339,38,22};closeoutSwitch={874,headers[2].y+131,38,22};
 tracks[0]={0,0,l+232,824,244,-30,60,1};tracks[1]={1,0,l+232,824,296,50,1000,25};tracks[2]={2,0,l+232,824,348,60,200,10};tracks[3]={3,1,l+232,824,headers[2].y+86,5,100,5};tracks[4]={4,1,l+232,824,392,0,120,5};tracks[5]={5,0,l+232,824,headers[3].y+80,4,16,1};tracks[6]={6,0,l+232,824,headers[3].y+128,60,300,10};tracks[7]={7,0,l+232,824,headers[3].y+176,0,30,5};}
static float contentBottom(bool target=false){if(shootTab>1)return 550;auto a=[&](int id){return target?(expanded[id]?1.f:0.f):expansion[id];};return shootTab?272+226*a(1)+126*a(2):272+232*a(0)+228*a(3);}
static float maxScroll(){return std::max(0.f,contentBottom()-550);}
static R scrollbarThumb(){float height=404,extent=height+maxScroll(),thumb=std::max(42.f,height*height/extent);return {943,158+(height-thumb)*(maxScroll()>0?scroll/maxScroll():0),5,thumb};}
static void dragScroll(float y){R r=scrollbarThumb();float travel=404-r.h;scroll=scrollGoal=travel>0?clampf((y-158-scrollGrab)/travel,0,1)*maxScroll():0;}
static void drawScrollbar(){if(shootTab>1||maxScroll()<.5f)return;R r=scrollbarThumb();fillR({943,158,5,404},2.5f,C(visualTheme[0],.055f));fillR(r,2.5f,C(visualTheme[0],scrollDragging||inRpad(r,4,mx,my)?.8f:.38f));}
static int getValue(int id){switch(id){case 0:return trim;case 1:return maxPing;case 2:return freshnessMs;case 4:return overholdLeadMs;case 5:return pumpRange;case 6:return pumpDelay;case 7:return pumpContest;default:return contestLimit;}}
static void setValue(int id,int v){switch(id){case 0:trim=v;break;case 1:maxPing=v;break;case 2:freshnessMs=v;break;case 4:overholdLeadMs=v;break;case 5:pumpRange=v;break;case 6:pumpDelay=v;break;case 7:pumpContest=v;break;default:contestLimit=v;}}
static int valueEditing=-1;static std::wstring valueText;static bool valueSelectAll=false;
static void finishValue(bool commit){if(valueEditing<0)return;if(commit){try{auto t=tracks[valueEditing];int value=std::stoi(valueText);value=std::clamp(value,t.low,t.high);value=t.low+(int)std::lround((value-t.low)/float(t.step))*t.step;setValue(valueEditing,std::clamp(value,t.low,t.high));save();}catch(...){}}valueEditing=-1;valueSelectAll=false;}
static void editTrack(int id,float x){auto t=tracks[id];int v=t.low+(int)std::lround(clampf((x-t.x0)/(t.x1-t.x0),0,1)*(t.high-t.low)/t.step)*t.step;setValue(id,std::clamp(v,t.low,t.high));}
static int rangeAt(float x){return std::clamp((int)lround(1+99*(x-rx0)/(rx1-rx0)),1,100);}
static void editRange(float x){int v=rangeAt(x);if(rangeHandle==0)minRelease=std::min(v,maxRelease.load());if(rangeHandle==1)maxRelease=std::max(v,minRelease.load());if(rangeHandle==2){int span=maxRelease-minRelease,a=std::clamp(rangeAt(x-rangeGrab),1,100-span);minRelease=a;maxRelease=a+span;}}
static void label(const std::wstring&s,float x,float y,float width=330){T(s,F_LABEL,x,y,width,25,C(0xf4f6ff));}
static void hintText(const std::wstring&s,float x,float y,float width=400,float h=25){T(s,F_SMALL,x,y,width,h,C(0xcbd5ed));}
static void drawTrack(int id,const wchar_t*name,const wchar_t*unit){auto t=tracks[id];label(name,t.x0,t.y-40,t.x1-t.x0-85);std::wstring v=std::to_wstring(getValue(id))+unit;if(!id&&getValue(id)>0)v=L"+"+v;T(v,F_CTRL,t.x1-84,t.y-40,84,25,C(0xe8edff));slider(t.x0,t.x1,t.y,uiSlider[id],uiSlider[id],editing==id);}
static void divider(R c,float y){line(c.x+22,y,c.x+c.w-22,y,C(0xbfc7fa,.16f));}
static void toggleRow(const wchar_t*title,const wchar_t*desc,R r,float anim){label(title,rx0,r.y,rx1-rx0-57);hintText(desc,rx0,r.y+28,rx1-rx0,33);toggle(r,anim,inRpad(r,5,mx,my));}
static std::wstring engineStatus(){if(!linked)return connectionLabel();if(statusCode==0&&linked)return meterFound?L"Ready for a new shot":L"Waiting for supported meter";static const wchar_t*names[]={L"Waiting for Roblox",L"Ready for a new shot",L"Waiting for meter",L"Manual release",L"Holding early release",L"Release sent",L"Ping above your limit",L"Meter sample is stale",L"Tracking rising meter",L"Tracking falling meter",L"Shot blocked by ceiling",L"Meter ended / hold cleared",L"Input release failed"};return names[std::clamp(statusCode.load(),0,12)];}
static const double sessionStart=now();
static void logo(){auto facet=[&](D2D1_POINT_2F a,D2D1_POINT_2F b,D2D1_POINT_2F c,unsigned rgb){ComPtr<ID2D1Factory>f;dc->GetFactory(&f);ComPtr<ID2D1PathGeometry>p;ComPtr<ID2D1GeometrySink>s;if(SUCCEEDED(f->CreatePathGeometry(&p))&&SUCCEEDED(p->Open(&s))){s->BeginFigure(a,D2D1_FIGURE_BEGIN_FILLED);s->AddLine(b);s->AddLine(c);s->EndFigure(D2D1_FIGURE_END_CLOSED);s->Close();setc(C(rgb));dc->FillGeometry(p.Get(),solid.Get());}};
 facet({23,13},{35,17},{37,35},blend(visualTheme[0],0xffffff,.3f));facet({35,17},{37,35},{41,25},blend(visualTheme[0],0x101522,.25f));facet({37,35},{40,19},{52,13},visualTheme[0]);Tleft(L"V",F_LOGO,65,25,C(0xf6f8ff));line(96,13,96,35,C(0xbac7e8,.3f));Tleft(contextTitle(shootTab),F_HSUB,113,25,C(0xd1daef));}
static void workspaceGlyph(float x,float y){for(float a:{-7.f,1.f})for(float b:{-7.f,1.f})strokeR({x+a,y+b,6,6},1.2f,C(0xd8e3fa),1.2f);}
static void systemGlyph(float x,float y){for(int i=0;i<3;i++){float yy=y-5+i*5.f,k=i==1?3.f:-3.f;line(x-8,yy,x+k-2,yy,C(0xd8e3fa),1.2f);line(x+k+2,yy,x+8,yy,C(0xd8e3fa),1.2f);ring(x+k,yy,1.7f,C(0xd8e3fa),1.2f);}}
struct TreeRow {const wchar_t* name;float y;int tab;bool parent;float opacity=1,clipBottom=562;bool last=false;};
static std::vector<TreeRow> treeRows(int group){
 std::vector<TreeRow> rows;float y=110;
 auto section=[&](const wchar_t* label,int tab,std::initializer_list<std::pair<const wchar_t*,int>> children){int ctx=contextOf(tab);float a=treeAnim[ctx],top=y+40,bottom=top+children.size()*32*a;rows.push_back({label,y,tab,true});int n=0;for(auto child:children){if(a>.001f)rows.push_back({child.first,top+32*n,child.second,false,a,bottom,n+1==(int)children.size()});n++;}y=bottom+10;};
 if(group){section(L"Settings",3,{{L"Appearance",3},{L"Window",6}});section(L"Configs",5,{{L"Profiles",5}});}
 else{section(L"Shooting",0,{{L"Timing",0},{L"Shot selection",1}});section(L"Defense",4,{{L"On-ball",4},{L"Off-ball",7},{L"Steals",8},{L"Blocks",9},{L"Rebounds",10}});section(L"Visuals",2,{{L"Court",2}});}
 return rows;
}
static void navIcon(int tab,float x,float y,D2D1_COLOR_F color){
 if(tab==0){ring(x,y,5.5f,color,1.2f);dot(x,y,1.5f,color);line(x-8,y,x-5,y,color);line(x+5,y,x+8,y,color);}
 else if(tab==4){line(x-6,y-5,x,y-7,color,1.2f);line(x,y-7,x+6,y-5,color,1.2f);line(x+6,y-5,x+5,y+3,color,1.2f);line(x+5,y+3,x,y+7,color,1.2f);line(x,y+7,x-5,y+3,color,1.2f);line(x-5,y+3,x-6,y-5,color,1.2f);}
 else if(tab==2){setc(color);dc->DrawEllipse(D2D1::Ellipse({x,y},7,4.5f),solid.Get(),1.2f);dot(x,y,2,color);}
 else if(tab==3){for(int i=0;i<3;i++){float yy=y-5+i*5;line(x-6,yy,x+6,yy,color,1);dot(x+(i==1?2:-2),yy,1.7f,color);}}
 else{strokeR({x-6,y-5,12,11},2,color,1.2f);line(x-3,y-8,x+4,y-8,color,1.2f);}
}
static int sidebarGroupAt(float x,float y){
 int group=navigationHeaderGroup(drawer,x,y);if(group>=0)return group;
 if(drawer<.2f&&x>=0&&x<60){if(fabs(y-87)<18)return 0;if(fabs(y-129)<18)return 1;}
 return -1;
}
#include "navigation_tree.hpp"
#include "navigation_rail.hpp"
static void drawSidebar(){float rw=railWidth();fillR({0,47,rw,H-47},0,ink(blend(railSurface(),0x111520,.3f),railAlpha));line(rw,47,rw,H,C(visualTheme[0],.16f));dc->PushAxisAlignedClip({0,47,rw,H},D2D1_ANTIALIAS_MODE_ALIASED);
 float q=1-std::pow(1-navBlend,3.f);
 float iconFade=1-clampf(drawer/.35f,0,1),labelFade=navigationLabelOpacity(drawer);
 drawCompactRail(iconFade,navFrom+(navGroup-navFrom)*q);
 if(labelFade>.001f){dc->SetTransform(D2D1::Matrix3x2F::Translation(navigationContentOffset(drawer),0));fade=labelFade;constexpr float half=(NavigationExpandedWidth-28)/2;fillR({12,64,NavigationExpandedWidth-24,34},6,C(visualTheme[2],.45f));strokeR({12,64,NavigationExpandedWidth-24,34},6,C(visualTheme[0],.1f));float sx=14+(navFrom+(navGroup-navFrom)*q)*half;fillR({sx,66,half,30},4,C(visualTheme[0],.14f));
  auto wrap=fmt[F_CTRL]->GetWordWrapping();fmt[F_CTRL]->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
  T(L"Workspace",F_CTRL,14,66,half,30,C(navGroup==0?0xf0f4fc:0x9ba9c0));T(L"System",F_CTRL,14+half,66,half,30,C(navGroup==1?0xf0f4fc:0x9ba9c0));fmt[F_CTRL]->SetWordWrapping(wrap);fade=1;
  dc->PushAxisAlignedClip({10,106,NavigationExpandedWidth-8,526},D2D1_ANTIALIAS_MODE_ALIASED);if(navBlend<1)drawTree(navFrom,1-q,-5*q);drawTree(navGroup,q,5*(1-q));dc->PopAxisAlignedClip();dc->SetTransform(D2D1::Matrix3x2F::Identity());}
 if(labelFade>.001f){fade=labelFade;R pin{12,532,NavigationExpandedWidth-24,26};float h=heat(pin,inR(pin,mx,my));fillR(pin,5,C(visualTheme[0],pinned?.13f:.035f+.05f*h));Tleft(pinned?L"Navigation pinned":L"Keep navigation open",F_SMALL,23,545,C(pinned?0xe4ecff:0x96a7c1));float x=NavigationExpandedWidth-29;line(x-3,539,x+3,539,C(visualTheme[0]),1.3f);line(x-2,539,x-2,544,C(visualTheme[0]));line(x+2,539,x+2,544,C(visualTheme[0]));line(x-4,545,x+4,545,C(visualTheme[0]));line(x,545,x,551,C(visualTheme[0]));fade=1;}
 line(14,575,rw-14,575,C(visualTheme[0],.13f));if(labelFade>.001f){dc->SetTransform(D2D1::Matrix3x2F::Translation(navigationContentOffset(drawer),0));fade=labelFade;unsigned e=(unsigned)(now()-sessionStart);wchar_t b[24];swprintf_s(b,L"%02u:%02u:%02u",e/3600,e/60%60,e%60);Tleft(L"Session",F_SMALL,18,596,C(0x8f9db5));T(b,F_SMALL,NavigationExpandedWidth-89,581,76,30,C(0xc1ccdf));fade=1;dc->SetTransform(D2D1::Matrix3x2F::Identity());}drawRailSession(iconFade);dc->PopAxisAlignedClip();}
static std::vector<std::pair<int,const wchar_t*>> contextTabs(){switch(contextOf(shootTab)){case 1:return {{4,L"On-ball"},{7,L"Off-ball"},{8,L"Steals"},{9,L"Blocks"},{10,L"Rebounds"}};case 4:return {{2,L"Court"}};case 2:return {{3,L"Appearance"},{6,L"Window"}};case 3:return {{5,L"Profiles"}};default:return {{0,L"Timing"},{1,L"Shot selection"}};}}
static float contextTabWidth(int tab){return tab==0?112.f:tab==1?152.f:tab==3?112.f:86.f;}
static float tabIndicatorX=4,tabIndicatorW=86;
#include "shooting_components.hpp"
#include "preferences.hpp"
#include "ui_settings.hpp"
#include "profiles.hpp"
#include "defense_ui.hpp"
#include "workspace_footer.hpp"
#include "launch_ui.hpp"
static void drawContents(int tab,int feature){float l=leftEdge();
 if(tab==3){drawUISettings();return;}if(tab==6){drawWindowSettings();return;}if(tab==5){drawProfiles();return;}if(defensePage(tab)){drawDefense();return;}
 if(tab==2){panel({l,164,936-l,120},ballArcOn?.7f:0);navIcon(2,l+28,188,C(visualTheme[0]));Tleft(L"Ball arc",F_TAB,l+54,188,C(0xf6f8ff));toggle({874,177,38,22},uiToggle[8],inRpad({874,177,38,22},5,mx,my));line(l+20,212,916,212,C(visualTheme[0],.16f));label(L"Landing marker",l+24,238);toggle({874,240,38,22},uiToggle[9],inRpad({874,240,38,22},5,mx,my));
 panel({l,300,936-l,60},rollingOn?.7f:0);navIcon(2,l+28,330,C(visualTheme[0]));Tleft(L"Ball intercept",F_TAB,l+54,330,C(0xf6f8ff));toggle({874,319,38,22},uiToggle[10],inRpad({874,319,38,22},5,mx,my));return;}

 if(!tab){featureHeader(0,L"Auto time",232*expansion[0]);dc->PushAxisAlignedClip({l,212,936,212+232*expansion[0]},D2D1_ANTIALIAS_MODE_ALIASED);compactTrack(0,L"Release adjustment",L" ms");compactTrack(1,L"Maximum ping",L" ms");compactTrack(2,L"Meter freshness",L" ms");drawShotBinding();dc->PopAxisAlignedClip();
 featureHeader(3,L"Pump fake punish",228*expansion[3]);dc->PushAxisAlignedClip({l,headers[3].y+48,936,headers[3].y+48+228*expansion[3]},D2D1_ANTIALIAS_MODE_ALIASED);compactTrack(5,L"Defender range",L" st");compactTrack(6,L"Reaction delay",L" ms");compactTrack(7,L"Other defender limit",L"%");label(L"Court visuals",rx0,pumpVisualSwitch.y);toggle(pumpVisualSwitch,uiToggle[7],inRpad(pumpVisualSwitch,5,mx,my));dc->PopAxisAlignedClip();
 if(pumpOn)T(pumpLabel(pumpStatus.load()),F_SMALL,l+250,headers[3].y,330,48,C(0xcddbf3));}
 else{
 featureHeader(1,L"Shot assist",226*expansion[1]);dc->PushAxisAlignedClip({l,212,936,212+226*expansion[1]},D2D1_ANTIALIAS_MODE_ALIASED);
 label(L"Release window",rx0,228);T(std::to_wstring(minRelease.load())+L" – "+std::to_wstring(maxRelease.load())+L"%",F_CTRL,806,226,106,28,C(0xeaf0ff));rangeSlider(rx0,rx1,rangeY,aRelMin,aRelMax,rangeHandle==0,rangeHandle==1,rangeHandle==2);
 label(L"Hold early release",rx0,294);toggle(earlySwitch,uiToggle[2],inRpad(earlySwitch,5,mx,my));label(L"Prevent overhold",rx0,339);toggle(fallSwitch,uiToggle[3],inRpad(fallSwitch,5,mx,my));compactTrack(4,L"Overhold lead",L" ms");dc->PopAxisAlignedClip();
 featureHeader(2,L"Contest ceiling",126*expansion[2]);dc->PushAxisAlignedClip({l,headers[2].y+48,936,headers[2].y+48+126*expansion[2]},D2D1_ANTIALIAS_MODE_ALIASED);compactTrack(3,L"Maximum estimate",L"%");label(L"Release before closeout",rx0,closeoutSwitch.y);toggle(closeoutSwitch,uiToggle[5],inRpad(closeoutSwitch,5,mx,my));dc->PopAxisAlignedClip();}
}
static std::wstring hoverHint;static float hintClock=0;static float hintX=0,hintY=0;
static std::wstring hintTitle;
static void drawHints(){
 if(!showTooltips){hoverHint.clear();hintTitle.clear();return;}
 std::wstring next,title;
 auto name=[&](const wchar_t* text,float x,float cy,int font,const wchar_t* description){
  static std::map<std::pair<std::wstring,int>,float> widths;auto key=std::make_pair(std::wstring(text),font);auto it=widths.find(key);
  if(it==widths.end()){ComPtr<IDWriteTextLayout> layout;DWRITE_TEXT_METRICS metrics{};float width=160;if(SUCCEEDED(dw->CreateTextLayout(text,(UINT32)wcslen(text),fmt[font].Get(),700,32,&layout))&&SUCCEEDED(layout->GetMetrics(&metrics)))width=metrics.width;it=widths.emplace(key,width).first;}
  if(shootTab<2)cy-=scroll;
  if(cy>=158&&cy<=562&&inR({x,cy-11,it->second,22},mx,my)){title=text;next=description;}
 };
 if(!capturing&&!finding&&editing<0&&valueEditing<0&&rangeHandle<0&&!themeDrag&&entry>.99f){
  float l=leftEdge();
  if(defensePage(shootTab)&&defenseDrag<0){for(auto h:defenseHints)name(h.name,h.x,h.y,h.font,h.hint);}
  else if(shootTab==3){auto a=appearanceCard(),b=interfaceCard();float x=b.x+20;
   name(L"Color palette",a.x+20,188,F_TAB,L"Apply coordinated accent, card and background colors. Changes blend smoothly across the interface.");
   name(L"Window opacity",x,238,F_LABEL,L"Adjust the main panel transparency while keeping its text readable.");
   name(L"Sidebar opacity",x,303,F_LABEL,L"Adjust the navigation background independently of the main panel.");
   name(L"Interface size",x,366,F_LABEL,L"Resize the whole interface. Sizes are limited to fit your monitor.");
   name(L"Tooltips",x,465,F_LABEL,L"Show explanations when you hover over a feature name.");
   name(L"Reduced motion",x,511,F_LABEL,L"Remove navigation, slider and palette transition animations.");
  }else if(shootTab==6){
   name(L"Always on top",l+4,194,F_LABEL,L"Keep the Vanta window above other windows. Court overlays are unchanged.");
   name(L"Compact bar when minimized",l+4,258,F_LABEL,L"Show the small restore bar at the top of your monitor while minimized.");
   name(L"Lock window position",l+4,322,F_LABEL,L"Disable title-bar dragging. Center window remains available.");
  }else if(shootTab==5){name(L"Saved profiles",l+24,190,F_TAB,L"Save or load settings and appearance. Loading does not enable shooting features.");
  }else if(shootTab==2){
   name(L"Ball arc",l+54,188,F_TAB,L"Predict the nearest unheld airborne ball from measured motion. Future rim and board collisions are not predicted.");
   name(L"Landing marker",l+24,250,F_LABEL,L"Show estimated court contact position and time, assuming a flat court.");
   name(L"Ball intercept",l+54,330,F_TAB,L"Track the closest rolling or bouncing loose ball. The blue line leads to an estimated reachable meeting point.");
  }else if(shootTab==0){
   name(L"Auto time",headers[0].x+57,headers[0].y+24,F_TAB,L"Automatically release using the measured meter trend and timing adjustment.");
   name(L"Pump fake punish",headers[3].x+57,headers[3].y+24,F_TAB,L"Tap the shot key to fake. After a nearby jump, shoot on descent if you keep the ball and stay still. Requires Auto time.");
   if(expansion[0]>.95f){
    name(L"Release adjustment",rx0,244,F_LABEL,L"Positive values release earlier; negative values release later.");
    name(L"Maximum ping",rx0,296,F_LABEL,L"Skip Auto time when measured ping exceeds this limit. Manual input still works.");
    name(L"Meter freshness",rx0,348,F_LABEL,L"Ignore meter samples older than this limit.");
    name(L"Shot input",rx0,403,F_LABEL,L"Either binding starts timing independently. The input you press is the one released. Click a key to change it; side mouse buttons work too.");
   }
   if(expansion[3]>.95f){
    name(L"Defender range",rx0,tracks[5].y,F_LABEL,L"Watch a defender within this distance when you pump fake.");
    name(L"Reaction delay",rx0,tracks[6].y,F_LABEL,L"Minimum delay after a confirmed jump. Also waits for the defender to descend.");
    name(L"Other defender limit",rx0,tracks[7].y,F_LABEL,L"Cancel when another defender's estimated contest exceeds this limit.");
    name(L"Court visuals",rx0,pumpVisualSwitch.y+12,F_LABEL,L"Highlight the targeted defender and detected jumps.");
   }
  }else if(shootTab==1){
   name(L"Shot assist",headers[1].x+57,headers[1].y+24,F_TAB,L"Constrain release to your percentage window. Manual release inside the window remains yours.");
   name(L"Contest ceiling",headers[2].x+57,headers[2].y+24,F_TAB,L"Block new shots above an estimated contest limit. This is not server contest feedback.");
   if(expansion[1]>.95f){
    name(L"Release window",rx0,240,F_LABEL,L"Drag either end to set your minimum and maximum, or drag the band to move both.");
    name(L"Hold early release",rx0,306,F_LABEL,L"Hold a manual early release until the measured minimum is reached.");
    name(L"Prevent overhold",rx0,351,F_LABEL,L"Predict release arrival to protect the falling floor. May send before the visible minimum to allow for input delay.");
    name(L"Overhold lead",rx0,392,F_LABEL,L"Extra anticipation for falling-meter protection. Does not change manual early-release hold.");
   }
   if(expansion[2]>.95f){name(L"Maximum estimate",rx0,tracks[3].y,F_LABEL,L"Block new shots above this estimated contest percentage.");name(L"Release before closeout",rx0,closeoutSwitch.y+12,F_LABEL,L"Release under sustained defensive pressure within a fresh rising 90-100% meter, respecting Shot assist.");}
  }
 }
 if(next!=hoverHint||title!=hintTitle){hoverHint=next;hintTitle=title;hintClock=tsec;hintX=mx;hintY=my;}if(next.empty()||tsec-hintClock<.4f)return;
 float a=clampf((tsec-hintClock-.4f)/.12f,0,1),x=clampf(hintX-30,72,522),y=hintY>450?hintY-96:hintY+24;fade=a;
 fillR({x,y,414,86},8,C(blend(visualTheme[2],0x080e19,.55f),.99f));strokeR({x,y,414,86},8,C(visualTheme[0],.5f));T(title,F_TAB,x+14,y+8,386,23,C(0xf5f8ff));T(next,F_SMALL,x+14,y+33,386,44,C(0xd9e5f7));fade=1;
}
static void captureFrame(){if(!bench||captureDone||!wcsstr(GetCommandLineW(),L"--capture")||now()-benchStart<.8)return;
 auto size=target->GetPixelSize();ComPtr<ID2D1Bitmap1>b;auto pr=D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_CPU_READ|D2D1_BITMAP_OPTIONS_CANNOT_DRAW,target->GetPixelFormat());if(FAILED(dc->CreateBitmap(size,nullptr,0,&pr,&b))||FAILED(b->CopyFromBitmap(nullptr,target.Get(),nullptr)))return;D2D1_MAPPED_RECT map{};if(FAILED(b->Map(D2D1_MAP_OPTIONS_READ,&map)))return;
 BITMAPFILEHEADER fh{};BITMAPINFOHEADER ih{};ih.biSize=sizeof ih;ih.biWidth=size.width;ih.biHeight=-int(size.height);ih.biPlanes=1;ih.biBitCount=32;fh.bfType=0x4d42;fh.bfOffBits=sizeof fh+sizeof ih;fh.bfSize=fh.bfOffBits+size.width*size.height*4;std::ofstream o(cfg.parent_path()/L"Native"/L"build"/(defensePage(qaTab)?L"defense-qa.bmp":qaTab?L"shot-selection-qa.bmp":L"timing-qa.bmp"),std::ios::binary);o.write((char*)&fh,sizeof fh);o.write((char*)&ih,sizeof ih);for(UINT y=0;y<size.height;y++)o.write((char*)map.bits+y*map.pitch,size.width*4);b->Unmap();captureDone=true;
}
static void render(){if(!dc)return;updateLayout();if(!target){ComPtr<IDXGISurface>surf;if(FAILED(swap->GetBuffer(0,IID_PPV_ARGS(&surf))))return;auto bp=D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET|D2D1_BITMAP_OPTIONS_CANNOT_DRAW,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED),scale*96,scale*96);if(FAILED(dc->CreateBitmapFromDxgiSurface(surf.Get(),&bp,&target)))return;}
 dc->SetTarget(target.Get());dc->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);dc->BeginDraw();dc->Clear(D2D1::ColorF(0,0,0,0));if(!launching()){fillR({0,0,W,47},0,C(visualTheme[2],windowAlpha));fillR({railWidth(),47,W-railWidth(),H-47},0,C(visualTheme[2],windowAlpha));
 if(glow){glow->SetCenter({780,65});glow->SetRadiusX(500);glow->SetRadiusY(420);glow->SetOpacity(.18f);dc->FillRectangle({railWidth(),0,W,H},glow.Get());glow->SetOpacity(1);}
 logo();if(inR(minBtn,mx,my))fillR(minBtn,6,C(visualTheme[0],.2f));if(inR(closeBtn,mx,my))fillR(closeBtn,6,C(0xff6868,.2f));line(880,23,892,23,C(0xe9eeff),1.5);line(922,17,932,27,C(0xe9eeff),1.5);line(932,17,922,27,C(0xe9eeff),1.5);line(20,47,940,47,C(0xc4cdef,.22f));
 float l=leftEdge();line(l,98,936,98,C(visualTheme[0],.13f));float tx=4;auto tabs=contextTabs();
 float tabSpan=0;for(auto item:tabs)tabSpan+=contextTabWidth(item.first)+4;fillR({l,59,tabSpan+4,35},9,C(visualTheme[2],.28f));strokeR({l,59,tabSpan+4,35},9,C(visualTheme[0],.09f),.8f);
 fillR({l+tabIndicatorX,63,tabIndicatorW,27},6,C(visualTheme[0],.17f));strokeR({l+tabIndicatorX,63,tabIndicatorW,27},6,C(visualTheme[0],.17f),.8f);
 for(auto item:tabs){float tw=contextTabWidth(item.first);R tab{l+tx,62,tw,31};if(item.first!=shootTab){float h=heat(tab,inR(tab,mx,my));if(h>.01f)fillR({tab.x,64,tw,26},5,C(visualTheme[0],.055f*h));}if(contextOf(shootTab)==0){leafGlyph(item.first,l+tx+18,77,C(shootTab==item.first?visualTheme[0]:0x8d9cb8));T(item.second,F_CTRL,l+tx+30,62,tw-38,29,C(shootTab==item.first?0xf2f6ff:0xaebcd3));}else T(item.second,F_CTRL,l+tx,62,tw,29,C(shootTab==item.first?0xf2f6ff:0xaebcd3));tx+=tw+4;}
 fillR({l+tabIndicatorX+12,96,tabIndicatorW-24,2},1,C(visualTheme[0]));finderBox={746,64,181,28};drawFinderButton();
 Tleft(contextTitle(shootTab),F_TITLE,l,128,C(0xf7f9ff));
 if(shootTab<=2){int active=shootTab==0?int(armed.load())+int(pumpOn.load()):shootTab==1?int(assist.load())+int(ceilingOn.load()):int(ballArcOn.load())+int(rollingOn.load());R badge{814,116,122,24};fillR(badge,7,C(visualTheme[0],.06f));strokeR(badge,7,C(visualTheme[0],.12f),.8f);for(int j=0;j<2;j++)fillR({badge.x+10+j*7,badge.y+8,4,8},1.5f,C(j<active?visualTheme[0]:0x59647e,j<active?.9f:.35f));T(std::to_wstring(active)+L" / 2 active",F_SMALL,badge.x+31,badge.y,badge.w-34,badge.h,C(0xb8c6de));}
 dc->PushAxisAlignedClip({l-1,158,940,562},D2D1_ANTIALIAS_MODE_ALIASED);
 float physicalMouseY=my;if(shootTab<2)my=(my>=158&&my<=562)?my+scroll:-1000;
 if(entry<1){float op=1-std::pow(1-entry,3.f);dc->PushLayer(D2D1::LayerParameters1(D2D1::InfiniteRect(),nullptr,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,D2D1::Matrix3x2F::Identity(),op),nullptr);dc->SetTransform(D2D1::Matrix3x2F::Translation(0,(1-op)*10-(shootTab<2?scroll:0)));drawContents(shootTab,selectionFeature);dc->SetTransform(D2D1::Matrix3x2F::Identity());dc->PopLayer();}else{dc->SetTransform(D2D1::Matrix3x2F::Translation(0,-(shootTab<2?scroll:0)));drawContents(shootTab,selectionFeature);dc->SetTransform(D2D1::Matrix3x2F::Identity());}my=physicalMouseY;dc->PopAxisAlignedClip();drawScrollbar();
 drawWorkspaceFooter();
 drawSidebar();drawRailHandle();drawRailHint();drawFinder();drawHints();drawToast();drawBind();}if(launching())drawLaunch();auto hr=dc->EndDraw();dc->SetTarget(nullptr);if(SUCCEEDED(hr)){captureFrame();hr=swap->Present(1,0);}if(hr==DXGI_ERROR_DEVICE_REMOVED||hr==DXGI_ERROR_DEVICE_RESET){dc.Reset();swap.Reset();comp.Reset();compTarget.Reset();visual.Reset();d3d.Reset();createDevice();}}
static void tick(float dt){tsec+=dt;if(launching()){if(!launchFreeze)launchFlow.tick(dt,linked,reducedMotion);tickTheme(dt);if(!launching())entry=0;return;}tickDefense(dt);if(bench&&wcsstr(GetCommandLineW(),L"--nav-bench")){double phase=now()-benchStart;if(phase<.2||phase>2.6){mx=930;my=560;}else if(phase<.85){mx=28;my=87;}else if(phase<1.65){mx=180;my=81;}else{mx=60;my=81;}}for(int i=0;i<5;i++)treeAnim[i]=approach(treeAnim[i],treeOpen[i]?1.f:0.f,22,dt);tickTree(dt);tickRail(dt);tickTheme(dt);{float tx=4;for(auto item:contextTabs()){float tw=contextTabWidth(item.first);if(item.first==shootTab){if(contextOf(fromTab)!=contextOf(shootTab)){tabIndicatorX=tx;tabIndicatorW=tw;}else{tabIndicatorX=approach(tabIndicatorX,tx,24,dt);tabIndicatorW=approach(tabIndicatorW,tw,24,dt);}}tx+=tw+4;}}navBlend=reducedMotion?1:std::min(1.f,navBlend+dt/.10f);entry=reducedMotion?1.f:std::min(1.f,entry+dt/.22f);subAnim=approach(subAnim,(float)shootTab,24,dt);bindAnim=capturing?approach(bindAnim,1,24,dt):0;
 {int group=sidebarGroupAt(mx,my);if(group>=0)switchGroup(group);}
 for(int i=0;i<4;i++)expansion[i]=approach(expansion[i],expanded[i]?1.f:0.f,24,dt);windowAlpha=approach(windowAlpha,windowOpacity*.01f,28,dt);railAlpha=approach(railAlpha,railOpacity*.01f,28,dt);
 if(railSuppressHover&&(mx<0||mx>railWidth()+16||my<47||my>H))railSuppressHover=false;bool handleHot=inRpad(railHandle(),4,mx,my);
 bool hover=!railSuppressHover&&!(drawer<.05f&&handleHot)&&mx>=0&&mx<railWidth()&&my>=47&&my<H&&!capturing&&!finding&&editing<0&&valueEditing<0&&rangeHandle<0&&themeDrag==0;if(scrollDragging||defenseDrag>=0||defenseThumbDrag)hover=false;if(hover)linger=.07f;else linger=std::max(0.f,linger-dt);railOpen=pinned||hover||linger>0;if(editing<0&&rangeHandle<0&&themeDrag==0&&!scrollDragging&&defenseDrag<0&&!defenseThumbDrag)drawer=sidebarMotion.update(railOpen,dt,reducedMotion);drawerClock=drawer;if(bench&&!wcsstr(GetCommandLineW(),L"--nav-bench"))drawer=drawerClock=wcsstr(GetCommandLineW(),L"--drawer-mid")?.5f:wcsstr(GetCommandLineW(),L"--sidebar")?1:0;updateLayout();
 if(shootTab<2){if(focusFeature){float first=shootTab?226*(expanded[1]?1.f:0.f):232*(expanded[0]?1.f:0.f);scrollGoal=selectionFeature?std::min(60+first,std::max(0.f,contentBottom(true)-550)):0;if(std::abs(contentBottom()-contentBottom(true))<.1f)focusFeature=false;}
  if(!focusFeature)scrollGoal=clampf(scrollGoal,0,maxScroll());if(editing<0&&rangeHandle<0&&valueEditing<0&&!scrollDragging)scroll=approach(scroll,std::min(scrollGoal,maxScroll()),22,dt);scroll=clampf(scroll,0,maxScroll());}
 bool vals[]={armed.load(),assist.load(),holdEarly.load(),protectFall.load(),ceilingOn.load(),closeoutOn.load(),pumpOn.load(),pumpVisuals.load(),ballArcOn.load(),ballLandingOn.load(),rollingOn.load()};for(int i=0;i<11;i++)uiToggle[i]=approach(uiToggle[i],vals[i]?1.f:0.f,22,dt);for(auto t:tracks)uiSlider[t.id]=approach(uiSlider[t.id],(getValue(t.id)-t.low)/float(t.high-t.low),25,dt);aRelMin=approach(aRelMin,(minRelease-1)/99.f,25,dt);aRelMax=approach(aRelMax,(maxRelease-1)/99.f,25,dt);searchFocus=approach(searchFocus,finding?1:0,25,dt);searchOpen=finding&&typed()?approach(searchOpen,1,28,dt):0;selectionY=approach(selectionY,(float)(matchPos-firstMatch()),24,dt);}
static LPCWSTR controlCursor(){
 if(capturing)return IDC_ARROW;
 if(inRpad(railHandle(),4,mx,my))return IDC_HAND;
 if(inR(finderBox,mx,my))return finding?IDC_IBEAM:IDC_HAND;
 if(defensePage(shootTab)){for(auto h:defenseHits)if(inR(h.box,mx,my))return h.action==4?IDC_SIZEWE:IDC_HAND;}
 if(my<47)return inR(closeBtn,mx,my)||inR(minBtn,mx,my)?IDC_HAND:IDC_ARROW;
 if(mx<railWidth()||inR(footerProfilesRect(),mx,my)||(!defensePage(shootTab)&&shootTab!=5&&inR(footerResetRect(),mx,my))||my>=61&&my<=98)return IDC_HAND;
 if(shootTab<2&&my>=158&&my<=562){float cy=my+scroll;
  for(int n=0;n<2;n++){int id=shootTab?n+1:n?3:0;if(inR(headers[id],mx,cy))return IDC_HAND;}
  for(auto t:tracks)if(t.tab==shootTab&&expansion[shootTab?(t.id==4?1:2):(t.id>=5?3:0)]>.95f){if(inR({846,t.y-14,66,28},mx,cy))return IDC_IBEAM;if(inR({t.x0-9,t.y-14,t.x1-t.x0+18,28},mx,cy))return IDC_SIZEWE;}
  if(shootTab==1&&expansion[1]>.95f&&inR({rx0-10,rangeY-14,rx1-rx0+20,28},mx,cy))return IDC_SIZEWE;
  if(!shootTab&&(expansion[0]>.95f&&(inR(shotBind,mx,cy)||inR(shotBindAlt,mx,cy))||expansion[3]>.95f&&inR(pumpVisualSwitch,mx,cy)))return IDC_HAND;
  if(shootTab==1&&(expansion[1]>.95f&&(inR(earlySwitch,mx,cy)||inR(fallSwitch,mx,cy))||expansion[2]>.95f&&inR(closeoutSwitch,mx,cy)))return IDC_HAND;
 }
 if(shootTab==3){for(int i=0;i<(int)std::size(presets);i++)if(inR(uiPalette(i),mx,my))return IDC_HAND;for(int i=0;i<4;i++)if(inR(uiSize(i),mx,my))return IDC_HAND;}
 if(shootTab==5&&inR({leftEdge()+24,438,936-leftEdge()-280,34},mx,my))return IDC_IBEAM;
 return IDC_ARROW;
}
#include "compact.hpp"
LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){LRESULT launchResult=0;if(launchEvent(h,m,w,l,launchResult))return launchResult;switch(m){
 case WM_SETCURSOR:if(LOWORD(l)==HTCLIENT){SetCursor(LoadCursorW(nullptr,controlCursor()));return TRUE;}break;
 case WM_APP+23:if(uiTest)return (int)lround(scroll*1000);break;
 case WM_APP+24:if(uiTest)return (expanded[0]?1:0)|(expanded[1]?2:0)|(expanded[2]?4:0)|(expanded[3]?8:0);break;
 case WM_APP+25:if(uiTest)return (int)lround((tracks[std::clamp((int)w,0,7)].y-scroll)*1000);break;
 case WM_APP+26:if(uiTest)return (int)lround((headers[std::clamp((int)w,0,3)].y-scroll)*1000);break;
 case WM_APP+18:if(uiTest)return (int)hoverHint.size();break;
 case WM_APP+17:if(uiTest)return (int)(drawer*1000)+(shootTab==3?10000:0)+navGroup*20000;break;

 case WM_APP+20:if(uiTest)return visualTheme[0];break;
 case WM_APP+22:if(uiTest)return (int)(treeAnim[std::clamp((int)w,0,4)]*1000);break;
 case WM_APP+21:if(uiTest)return contextOf(shootTab);break;
 case WM_APP+19:if(uiTest)return uiScale+(showTooltips?1000:0)+(reducedMotion?2000:0)+shootTab*10000;break;
 case WM_SIZE:if(w==SIZE_MINIMIZED)miniShow(h);else if(compact){KillTimer(compact,1);ShowWindow(compact,SW_HIDE);}return 0;
 case WM_MOUSELEAVE:mx=my=-100;return 0;
 case WM_MOUSEMOVE:{TRACKMOUSEEVENT ev{sizeof ev,TME_LEAVE,h,0};if(!uiTest)TrackMouseEvent(&ev);mx=GET_X_LPARAM(l)/scale;my=GET_Y_LPARAM(l)/scale;{int group=sidebarGroupAt(mx,my);if(group>=0)switchGroup(group);}if(defenseDrag>=0)defenseEdit(mx);if(defenseThumbDrag)defenseDragScroll(my);if(scrollDragging)dragScroll(my);if(editing>=0)editTrack(editing,mx);if(rangeHandle>=0)editRange(mx);if(themeDrag){if(shootTab==3)editUIOpacity(mx);else (themeDrag==1?windowOpacity:railOpacity)=std::clamp((int)round((mx-470)*100/182),0,100);}return 0;}
 case WM_LBUTTONDOWN:{float x=GET_X_LPARAM(l)/scale,y=GET_Y_LPARAM(l)/scale;mx=x;my=y;if(y<158||x<railWidth())profileEditing=false;
  if(capturing){float by=215+6*(1-bindAnim);if(inR({269,by+123,141,32},x,y))assignKey(capturing>=100?defense::options[capturing-100].initial:capturing==1?'E':capturing==3?VK_SPACE:'G');else if(inR({585,by+123,103,32},x,y)||!inR({249,by,462,172},x,y))capturing=0;return 0;}

  if(valueEditing>=0){auto t=tracks[valueEditing];if(y>=158&&y<=562&&inR({846,t.y-14,66,28},x,y+(shootTab<2?scroll:0)))return 0;finishValue(true);}
  if(inR(finderBox,x,y)){if(!finding)openFinder();return 0;}if(finding){if(finderClick(x,y))return 0;finding=false;}
  if(inRpad(closeBtn,4,x,y)){DestroyWindow(h);return 0;}if(inRpad(minBtn,4,x,y)){ShowWindow(h,SW_MINIMIZE);return 0;}
  if(inRpad(railHandle(),4,x,y)){bool close=drawer>.5f||pinned;pinned=!close;railSuppressHover=close;railOpen=!close;linger=0;saveTheme();return 0;}
  if(x<railWidth()&&y>47){if(drawer>.78f&&inR({12,532,NavigationExpandedWidth-24,26},x,y)){pinned=!pinned;saveTheme();announce(pinned?L"Navigation pinned":L"Navigation unpinned");return 0;}int group=sidebarGroupAt(x,y);if(group>=0)switchGroup(group);else if(drawer>=.78f){for(auto row:treeRows(navGroup))if((row.parent||row.opacity>.15f)&&y>=row.y&&y<=row.y+(row.parent?36:31)&&y<=row.clipBottom){if(row.parent&&(x>=NavigationExpandedWidth-43||contextOf(row.tab)==contextOf(shootTab)))treeOpen[contextOf(row.tab)]=!treeOpen[contextOf(row.tab)];else selectTab(row.tab);break;}}railOpen=true;linger=.07f;return 0;}

  {float tx=leftEdge()+4;for(auto item:contextTabs()){float tw=contextTabWidth(item.first);if(inR({tx,61,tw,33},x,y)){selectTab(item.first);return 0;}tx+=tw+4;}}
  if(inR(footerProfilesRect(),x,y)){selectTab(5);return 0;}
  if(inR(footerResetRect(),x,y)){if(!defensePage(shootTab)&&shootTab!=5)announce(L"Panel defaults restored");if(shootTab==3){resetUISettings();return 0;}if(shootTab==6){alwaysOnTop=false;compactBar=true;lockWindow=false;applyWindowOptions();saveTheme();return 0;}if(shootTab>=4)return 0;if(shootTab==2){ballArcOn=true;ballLandingOn=true;rollingOn=true;}else if(!shootTab){if(selectionFeature){pumpRange=12;pumpDelay=120;pumpContest=10;pumpVisuals=true;}else{trim=40;maxPing=1000;freshnessMs=120;}}else if(!selectionFeature){minRelease=80;maxRelease=100;holdEarly=true;protectFall=true;overholdLeadMs=40;}else{contestLimit=30;closeoutOn=false;}save();return 0;}
  if(shootTab==6&&y>=158){clickWindowSettings(x,y);return 0;}if(shootTab==5&&y>=158){clickProfiles(x,y);return 0;}if(defensePage(shootTab)&&y>=158){clickDefense(x,y);return 0;}
  if(shootTab==3&&y>=158){clickUISettings(x,y);return 0;}
  if(shootTab==2){if(inRpad({874,319,38,22},7,x,y)){rollingOn=!rollingOn;save();return 0;}if(inRpad({874,177,38,22},7,x,y)){ballArcOn=!ballArcOn;save();return 0;}if(inRpad({874,240,38,22},7,x,y)){ballLandingOn=!ballLandingOn;save();return 0;}if(y>=158)return 0;}
  if(shootTab<2&&y>=47){if(y<158||y>562)return 0;if(maxScroll()>.5f&&x>=939&&x<=952){R thumb=scrollbarThumb();scrollGrab=inRpad(thumb,4,x,y)?y-thumb.y:thumb.h/2;scrollDragging=true;focusFeature=false;dragScroll(y);SetCapture(h);return 0;}if(x>936)return 0;y+=scroll;}
  for(int i=0;i<(shootTab==2?0:2);i++){int id=shootTab?i+1:i==0?0:3;R t=id==0?autoSwitch:id==1?assistSwitch:id==2?ceilingSwitch:pumpSwitch;if(inRpad(t,7,x,y)){if(id==0)armed=!armed;else if(id==1)assist=!assist;else if(id==2)ceilingOn=!ceilingOn;else pumpOn=!pumpOn;bool enabled=id==0?armed.load():id==1?assist.load():id==2?ceilingOn.load():pumpOn.load();if(enabled){expanded[id]=true;selectionFeature=lastFeatures[shootTab]=id==3||id==2?1:0;}return 0;}if(inR(headers[id],x,y)){expanded[id]=!expanded[id];selectionFeature=lastFeatures[shootTab]=id==3||id==2?1:0;return 0;}}
  if(!shootTab&&expansion[3]>.95f&&inRpad(pumpVisualSwitch,5,x,y)){pumpVisuals=!pumpVisuals;save();return 0;}
  if(!shootTab&&expansion[0]>.95f&&(inR(shotBind,x,y)||inR(shotBindAlt,x,y))){if(!gameDown&&!physicalDown){capturing=inR(shotBind,x,y)?1:3;bindHint.clear();}return 0;}
  if(shootTab==1&&expansion[1]>.95f){if(inRpad(earlySwitch,5,x,y)){holdEarly=!holdEarly;save();return 0;}if(inRpad(fallSwitch,5,x,y)){protectFall=!protectFall;save();return 0;}
   if(inR({rx0-10,rangeY-14,rx1-rx0+20,28},x,y)){float a=rx0+(minRelease-1)*(rx1-rx0)/99,b=rx0+(maxRelease-1)*(rx1-rx0)/99;if(fabs(x-a)<=9&&fabs(x-a)<=fabs(x-b))rangeHandle=0;else if(fabs(x-b)<=9)rangeHandle=1;else if(x>a&&x<b){rangeHandle=2;rangeGrab=x-a;}else rangeHandle=x<a?0:1;editRange(x);SetCapture(h);return 0;}}
  if(shootTab==1&&expansion[2]>.95f&&inRpad(closeoutSwitch,5,x,y)){closeoutOn=!closeoutOn;save();return 0;}
  for(auto t:tracks)if(t.tab==shootTab&&expansion[shootTab?(t.id==4?1:2):(t.id>=5?3:0)]>.95f&&inR({846,t.y-14,66,28},x,y)){valueEditing=t.id;valueText=std::to_wstring(getValue(t.id));valueSelectAll=true;return 0;}
  for(auto t:tracks)if(t.tab==shootTab&&expansion[shootTab?(t.id==4?1:2):(t.id>=5?3:0)]>.95f&&inR({t.x0-9,t.y-14,t.x1-t.x0+18,28},x,y)){editing=t.id;editTrack(editing,x);SetCapture(h);return 0;}
  if(y<46&&!lockWindow){ReleaseCapture();SendMessageW(h,WM_NCLBUTTONDOWN,HTCAPTION,l);}return 0;}
 case WM_LBUTTONUP:case WM_CAPTURECHANGED:if(defenseDrag>=0)save();defenseDrag=-1;defenseThumbDrag=false;scrollDragging=false;if(themeDrag){themeDrag=0;saveTheme();}if(editing>=0||rangeHandle>=0){save();editing=rangeHandle=-1;}if(m==WM_LBUTTONUP)ReleaseCapture();return 0;
 case WM_KILLFOCUS:finishValue(true);finding=false;capturing=0;profileEditing=false;return 0;
 case WM_MOUSEWHEEL:if(defensePage(shootTab)&&!finding&&!capturing&&mx>=leftEdge()&&my>=158&&my<=562){defenseScrollGoal=clampf(defenseScrollGoal-GET_WHEEL_DELTA_WPARAM(w)/120.f*64,0,defenseMaxScroll());return 0;}if(shootTab<2&&!finding&&!capturing&&editing<0&&rangeHandle<0&&valueEditing<0&&!scrollDragging&&mx>=leftEdge()&&my>=158&&my<=562){focusFeature=false;scrollGoal=clampf(scrollGoal-GET_WHEEL_DELTA_WPARAM(w)/120.f*64,0,maxScroll());return 0;}if(shootTab==5&&!finding){profileOffset=std::clamp(profileOffset-GET_WHEEL_DELTA_WPARAM(w)/120,0,std::max(0,(int)profileNames.size()-5));return 0;}if(finding&&typed())matchPos=std::clamp(matchPos-GET_WHEEL_DELTA_WPARAM(w)/120,0,std::max(0,(int)matches().size()-1));return 0;
 case WM_XBUTTONDOWN:if(capturing){assignKey(GET_XBUTTON_WPARAM(w)==XBUTTON1?VK_XBUTTON1:VK_XBUTTON2);return TRUE;}break;
 case WM_MBUTTONDOWN:if(capturing){assignKey(VK_MBUTTON);return 0;}break;
 case WM_SYSKEYDOWN:if(!capturing)break;[[fallthrough]];
 case WM_KEYDOWN:if(valueEditing>=0){if(w==VK_RETURN)finishValue(true);else if(w==VK_ESCAPE)finishValue(false);else if(w=='A'&&(GetKeyState(VK_CONTROL)&0x8000))valueSelectAll=true;else if(w==VK_UP||w==VK_DOWN){auto t=tracks[valueEditing];int v=getValue(t.id);try{v=std::stoi(valueText);}catch(...){}valueText=std::to_wstring(std::clamp(v+(w==VK_UP?t.step:-t.step),t.low,t.high));valueSelectAll=true;}return 0;}if(profileEditing&&!finding){if(w==VK_ESCAPE)profileEditing=false;if(w==VK_RETURN){saveProfile();profileEditing=false;}return 0;}if(capturing){if(!(l&(1LL<<30)))assignKey((int)w);return 0;}if(w=='K'&&(GetKeyState(VK_CONTROL)&0x8000)){openFinder();return 0;}
  if(finding){auto v=matches();if(w==VK_ESCAPE)finding=false;else if(w==VK_DOWN&&matchPos+1<(int)v.size())matchPos++;else if(w==VK_UP&&matchPos>0)matchPos--;else if(w==VK_RETURN&&!v.empty())jump(v[matchPos]);return 0;}if(w==VK_ESCAPE){ShowWindow(h,SW_MINIMIZE);return 0;}return 0;
 case WM_CHAR:if(valueEditing>=0){if(w==8){if(valueSelectAll)valueText.clear();else if(!valueText.empty())valueText.pop_back();valueSelectAll=false;}else if((w>='0'&&w<='9')||w=='-'){if(valueSelectAll)valueText.clear();valueSelectAll=false;if(valueText.size()<6&&(w!='-'||valueText.empty()))valueText+=(wchar_t)w;}return 0;}if(profileEditing&&!finding){if(w==8){if(!profileName.empty())profileName.pop_back();}else if(profileName.size()<32&&((w>='a'&&w<='z')||(w>='A'&&w<='Z')||(w>='0'&&w<='9')||w==' '||w=='-'||w=='_'))profileName+=(wchar_t)w;return 0;}if(finding){if(w==8){if(!query.empty())query.pop_back();}else if(w>=32&&w!=127&&query.size()<64)query+=(wchar_t)w;matchPos=0;return 0;}break;
 case WM_DESTROY:if(compact){DestroyWindow(compact);compact=nullptr;}running=false;PostQuitMessage(0);return 0;
 }return DefWindowProcW(h,m,w,l);}
int WINAPI wWinMain(HINSTANCE ins,HINSTANCE,LPWSTR,int){CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 uiTest=wcsstr(GetCommandLineW(),L"--ui-test")!=nullptr;bench=wcsstr(GetCommandLineW(),L"--frame-test")!=nullptr;qaTab=wcsstr(GetCommandLineW(),L"--selection")?1:0;if(bench){shootTab=qaTab;if(wcsstr(GetCommandLineW(),L"--visuals"))shootTab=qaTab=2;if(wcsstr(GetCommandLineW(),L"--settings"))shootTab=qaTab=3;if(wcsstr(GetCommandLineW(),L"--pump")){shootTab=qaTab=0;selectionFeature=1;expanded[0]=false;expanded[3]=true;expansion[0]=0;expansion[3]=1;}if(wcsstr(GetCommandLineW(),L"--contest")){selectionFeature=1;expanded[1]=false;expanded[2]=true;expansion[1]=0;expansion[2]=1;}}if(wcsstr(GetCommandLineW(),L"--settings")){shootTab=3;navGroup=1;}if(wcsstr(GetCommandLineW(),L"--window")){shootTab=6;navGroup=1;}if(wcsstr(GetCommandLineW(),L"--configs")){shootTab=5;navGroup=1;}if(wcsstr(GetCommandLineW(),L"--defense"))shootTab=4;if(wcsstr(GetCommandLineW(),L"--offball"))shootTab=7;if(wcsstr(GetCommandLineW(),L"--steals"))shootTab=8;if(wcsstr(GetCommandLineW(),L"--blocks"))shootTab=9;if(wcsstr(GetCommandLineW(),L"--rebounds"))shootTab=10;if(bench)qaTab=shootTab;openTree(contextOf(shootTab));for(int i=0;i<5;i++)treeAnim[i]=treeOpen[i]?1.f:0.f;updateLayout();
 HANDLE one=CreateMutexW(nullptr,FALSE,(bench||uiTest)?L"Local\\VantaFrameTest":L"Local\\VantaNativeTiming");if(GetLastError()==ERROR_ALREADY_EXISTS)return 0;
 wchar_t exe[MAX_PATH];GetModuleFileNameW(0,exe,MAX_PATH);cfg=fs::path(exe).parent_path()/L"bat-timing-settings.json";
 sessionUnlocked=false;loadGameplay(cfg);loadAppearance(cfg.parent_path()/L"vanta-theme.txt");railAlpha=railOpacity*.01f;std::copy(theme,theme+3,visualTheme);
 if(uiTest){int palette=wcsstr(GetCommandLineW(),L"--palette-rose")?7:wcsstr(GetCommandLineW(),L"--palette-slate")?2:wcsstr(GetCommandLineW(),L"--palette-cobalt")?1:-1;if(palette>=0){std::copy(presets[palette],presets[palette]+3,theme);std::copy(theme,theme+3,visualTheme);}}
 if(bench||uiTest){launchFlow.move(LaunchFlow::Ready);if(wcsstr(GetCommandLineW(),L"--launch-login")){launchFlow.move(LaunchFlow::Login);launchFreeze=true;launchFlow.age=1;}if(wcsstr(GetCommandLineW(),L"--launch-connect")){launchFlow.move(LaunchFlow::Connect);launchFreeze=true;launchFlow.age=1;linked=true;connectionState=ConnectionState::Connected;}if(wcsstr(GetCommandLineW(),L"--launch-waiting")){launchFlow.move(LaunchFlow::Connect);launchFreeze=true;launchFlow.age=1;linked=false;connectionState=ConnectionState::Waiting;}if(wcsstr(GetCommandLineW(),L"--launch-intro")){launchFlow.move(LaunchFlow::Intro);launchFreeze=true;launchFlow.age=.75f;}}

 if(uiTest&&wcsstr(GetCommandLineW(),L"--launch-resize")){launchFlow.move(LaunchFlow::Entering);linked=true;connectionState=ConnectionState::Connected;}
 aRelMin=(minRelease-1)/99.f;aRelMax=(maxRelease-1)/99.f;for(auto t:tracks)uiSlider[t.id]=(getValue(t.id)-t.low)/float(t.high-t.low);uiToggle[2]=holdEarly?1:0;uiToggle[3]=protectFall?1:0;uiToggle[5]=closeoutOn?1:0;uiToggle[7]=pumpVisuals?1:0;uiToggle[8]=ballArcOn?1:0;uiToggle[9]=ballLandingOn?1:0;uiToggle[10]=rollingOn?1:0;subAnim=(float)shootTab;
 WNDCLASSW wc{};wc.hCursor=LoadCursor(0,IDC_ARROW);wc.lpfnWndProc=proc;wc.hInstance=ins;wc.lpszClassName=L"VantaGlass";RegisterClassW(&wc);
 scale=std::min(GetDpiForSystem()/96.f*(uiScale*.01f),std::min((GetSystemMetrics(SM_CXSCREEN)-28)/W,(GetSystemMetrics(SM_CYSCREEN)-64)/H));int pw=(int)lround((launching()?LaunchWidth:W)*scale),ph=(int)lround((launching()?LaunchHeight:H)*scale);
 wnd=CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP,wc.lpszClassName,L"Vanta",WS_POPUP,(GetSystemMetrics(SM_CXSCREEN)-pw)/2,(GetSystemMetrics(SM_CYSCREEN)-ph)/2,pw,ph,0,0,ins,0);BOOL dark=TRUE;DwmSetWindowAttribute(wnd,DWMWA_USE_IMMERSIVE_DARK_MODE,&dark,sizeof dark);int corner=DWMWCP_ROUND;DwmSetWindowAttribute(wnd,DWMWA_WINDOW_CORNER_PREFERENCE,&corner,sizeof corner);
 if(HMODULE u=GetModuleHandleW(L"user32.dll"))if(auto set=(SetWCA)GetProcAddress(u,"SetWindowCompositionAttribute")){ACCENT_POLICY ap{ACCENT_ENABLE_ACRYLICBLURBEHIND,0,0x402e221a,0};WINCOMPATTRDATA d{19,&ap,sizeof ap};set(wnd,&d);}
 applyWindowOptions();if(!createDevice())return 1;if(uiTest)SetWindowPos(wnd,nullptr,-10000,-10000,0,0,SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER);ShowWindow(wnd,uiTest?SW_SHOWNOACTIVATE:SW_SHOW);std::thread worker,geometryWorker,visualWorker,defenseWorker;if(!bench&&!uiTest)worker=std::thread(engine);
 MSG msg{};double previous=now();benchStart=previous;while(running){int processed=0;while(processed++<64&&PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT){running=false;break;}TranslateMessage(&msg);DispatchMessageW(&msg);}if(!running)break;
 if(IsIconic(wnd)){MsgWaitForMultipleObjectsEx(0,nullptr,100,QS_ALLINPUT,MWMO_INPUTAVAILABLE);previous=now();if(GetAsyncKeyState(VK_F8)&0x8000)DestroyWindow(wnd);continue;}
 DWORD wait=MsgWaitForMultipleObjectsEx(uiTest?0:1,uiTest?nullptr:&frameReady,uiTest?16:250,QS_ALLINPUT,MWMO_INPUTAVAILABLE);if(wait!=WAIT_OBJECT_0&&!(uiTest&&wait==WAIT_TIMEOUT))continue;if(GetAsyncKeyState(VK_F8)&0x8000){DestroyWindow(wnd);continue;}double t=now();if(bench&&t-benchStart>.6)frameTimes.push_back((t-previous)*1000);tick((float)std::min(.08,t-previous));previous=t;
 if(uiTest&&!runtimeStarted&&!launching()&&wcsstr(GetCommandLineW(),L"--launch-resize")){applyUISize();runtimeStarted=true;}
 if(!runtimeStarted&&!launching()&&!bench&&!uiTest){
  if(installInput()){applyUISize();entry=0;sessionUnlocked=true;runtimeStarted=true;geometryWorker=std::thread(contestWorker);visualWorker=std::thread(pumpVisualWorker);defenseWorker=std::thread(defense::worker);}
  else{uninstallInput();launchError=L"Input setup failed. Please try Continue again.";launchFlow.move(LaunchFlow::Connect);}
 }
 render();
 if(bench&&t-benchStart>3){std::sort(frameTimes.begin(),frameTimes.end());if(!frameTimes.empty())std::ofstream(cfg.parent_path()/L"frame-test.txt")<<"median_ms "<<frameTimes[frameTimes.size()/2]<<" p95_ms "<<frameTimes[frameTimes.size()*95/100];DestroyWindow(wnd);}}
 if(defenseWorker.joinable())defenseWorker.join();if(visualWorker.joinable())visualWorker.join();if(worker.joinable())worker.join();if(geometryWorker.joinable())geometryWorker.join();uninstallInput();if(frameReady)CloseHandle(frameReady);CloseHandle(one);CoUninitialize();return 0;}
