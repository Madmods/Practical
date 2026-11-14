#pragma once
static float railHover[2]={},railFooterHover=0,railTipAlpha=0,railHandleHover=0;
static bool railSuppressHover=false;
static constexpr float railMidY=311;
static R railHandle(){return {railWidth()-13,railMidY-23,26,46};}
static int railTip=-1;static float railTipSince=0;
static void tickRail(float dt){
 int hit=drawer<.7f?sidebarGroupAt(mx,my):-1;
 for(int n=0;n<2;n++)railHover[n]=approach(railHover[n],hit==n?1.f:0.f,20,dt);
 bool footer=mx>=10&&mx<=50&&my>=578&&my<=614;
 railFooterHover=approach(railFooterHover,footer?1.f:0.f,20,dt);
 railHandleHover=approach(railHandleHover,inRpad(railHandle(),4,mx,my)?1.f:0.f,18,dt);
 int tip=footer?2:hit;
 if(tip!=railTip){railTip=tip;railTipSince=tsec;}
 railTipAlpha=approach(railTipAlpha,showTooltips&&tip>=0&&tsec-railTipSince>.08f&&!capturing&&!finding?1.f:0.f,24,dt);
}
static void railGlyph(int group,float x,float y,D2D1_COLOR_F c,float hover){
 if(group==0){
  for(int row=0;row<2;row++)for(int col=0;col<2;col++){
   float xx=x-8.5f+col*10,yy=y-8.5f+row*10;
   if(row==0&&col==1){fillR({xx,yy,7,7},2,C(visualTheme[0],.32f+.22f*hover));}
   strokeR({xx,yy,7,7},2,c,1.25f);
  }
 }else{
  for(int j=0;j<3;j++){float yy=y-6+j*6,k=(j==1?3.f:-3.f)+(j==1?-1:1)*hover;
   line(x-8,yy,x+k-2.4f,yy,c,1.25f);line(x+k+2.4f,yy,x+8,yy,c,1.25f);
   fillR({x+k-2.2f,yy-2.2f,4.4f,4.4f},1.5f,C(visualTheme[2],.9f));
   strokeR({x+k-2.2f,yy-2.2f,4.4f,4.4f},1.5f,c,1.15f);
  }
 }
}
static void drawCompactRail(float alpha,float progress){
 if(alpha<.001f)return;fade=alpha;
 // A recessed dock gives the two destinations one shared, deliberate container.
 fillR({7,64,46,85},12,C(0x060b16,.16f));
 fillR({7,62,46,87},12,C(visualTheme[2],.52f));
 strokeR({7,62,46,87},12,C(visualTheme[0],.16f),.9f);
 line(19,63,41,63,C(visualTheme[0],.17f),.8f);
 float y=70+42*progress;
 fillR({10,y+2,40,34},9,C(0x080d1a,.18f));
 fillR({10,y,40,34},9,C(visualTheme[0],.27f));
 strokeR({10,y,40,34},9,C(visualTheme[0],.46f),1);
 strokeR({12,y+2,36,30},7,C(0xe3ebff,.055f),.7f);
 line(18,y+1,42,y+1,C(visualTheme[0],.55f),1);
 // Edge marker follows the active destination rather than duplicating the icon.
 fillR({1,y+6,2.5f,22},1.25f,C(visualTheme[0],.85f));
 fillR({3.5f,y+10,1,14},.5f,C(visualTheme[0],.16f));
 for(int n=0;n<2;n++){
  float h=railHover[n];if(h>.01f&&n!=navGroup)fillR({11,70+42.f*n,38,34},8,C(visualTheme[0],.085f*h));
  railGlyph(n,30,87+42.f*n,C(n==navGroup?blend(visualTheme[0],0xffffff,.55f):blend(0x94a4c1,0xe0e9fc,h)),h);
 }
 fade=1;
}
static void drawRailHandle(){
 R r=railHandle();float h=railHandleHover;
 // The handle straddles the divider, centered in the working area.
 // It remains present in both states, with a genuine open/close click target.
 line(railWidth(),railMidY-63,railWidth(),r.y-5,C(visualTheme[0],.25f+.12f*h),1.2f);
 line(railWidth(),r.y+r.h+5,railWidth(),railMidY+63,C(visualTheme[0],.25f+.12f*h),1.2f);
 fillR({r.x-3,r.y-3,r.w+6,r.h+6},13,C(visualTheme[0],.045f+.045f*h));
 fillR({r.x,r.y+2,r.w,r.h},10,C(0x050a15,.3f));
 fillR(r,10,C(blend(visualTheme[2],visualTheme[0],.26f+.08f*h),.98f));
 strokeR(r,10,C(visualTheme[0],.58f+.22f*h),1.1f);
 line(r.x+8,r.y+1,r.x+r.w-8,r.y+1,C(0xe9efff,.24f+.12f*h),1);
 float cx=r.x+r.w*.5f,cy=railMidY;
 // Morph the chevron rather than swapping glyphs during expansion.
 float direction=1-2*drawer;
 line(cx-2.8f*direction,cy-5.5f,cx+3.2f*direction,cy,C(blend(visualTheme[0],0xffffff,.85f)),1.9f);
 line(cx+3.2f*direction,cy,cx-2.8f*direction,cy+5.5f,C(blend(visualTheme[0],0xffffff,.85f)),1.9f);
}
static void drawRailSession(float alpha){
 if(alpha<.001f)return;fade=alpha;float h=railFooterHover;
 fillR({11,579,38,34},9,C(visualTheme[0],.035f+.07f*h));
 strokeR({11,579,38,34},9,C(visualTheme[0],.10f+.12f*h),.8f);
 ring(30,596,7.5f,C(visualTheme[0],.2f),1.1f);
 // The perimeter advances with the current minute of the actual session.
 float elapsed=(float)fmod(now()-sessionStart,60.0),angle=elapsed/60.f*6.2831853f;
 for(int j=0;j<24;j++){float a=j*6.2831853f/24-1.5707963f;if(j*6.2831853f/24>angle)break;
  float b=a+.17f;line(30+7.5f*cosf(a),596+7.5f*sinf(a),30+7.5f*cosf(b),596+7.5f*sinf(b),C(visualTheme[0],.85f),1.3f);}
 line(30,596,30,592,C(0xc9d5ec),1.1f);line(30,596,33,597.5f,C(0xc9d5ec),1.1f);
 dot(44,582,3.2f,C(visualTheme[2]));dot(44,582,1.9f,C(linked?0x72dda8:0x91a1ba));
 fade=1;
}
static void drawRailHint(){
 float alpha=railTipAlpha*(1-clampf(drawer/.8f,0,1));
 if(alpha<.01f||railTip<0)return;
 fade=alpha;float x=railWidth()+10,y=railTip==2?552.f:railTip==0?70.f:112.f;
 R b{x,y,178,48};fillR({x,y+3,178,48},8,C(0x060b15,.18f));
 fillR(b,8,C(visualTheme[2],.98f));strokeR(b,8,C(visualTheme[0],.25f),.8f);
 Tleft(railTip==0?L"Workspace":railTip==1?L"System":connectionLabel(),F_TAB,x+12,y+15,C(0xecf2ff));
 std::wstring sub=railTip==0?L"Features and court visuals":railTip==1?L"Appearance and profiles":L"Session  ";
 if(railTip==2){unsigned e=(unsigned)(now()-sessionStart);wchar_t text[24];swprintf_s(text,L"%02u:%02u:%02u",e/3600,e/60%60,e%60);sub+=text;}
 Tleft(sub,F_SMALL,x+12,y+34,C(0x9faec9));fade=1;
}
