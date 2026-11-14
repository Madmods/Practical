#pragma once
static R footerResetRect(){return {672,580,122,32};}
static R footerProfilesRect(){return {805,580,131,32};}
static void drawWorkspaceFooter(){
 float l=leftEdge();
 line(l,575,936,575,C(visualTheme[0],.15f));
 line(l,575,l+46,575,C(visualTheme[0],.42f),1.2f);
 fillR({l,579,230,34},8,C(visualTheme[2],.22f));
 // Connection indicator: restrained when offline, brighter only when attached.
 float cx=l+16,cy=596;unsigned color=linked?0x72dda8:0x94a4c2;
 fillR({l+2,582,28,28},8,C(color,linked?.09f:.045f));
 strokeR({cx-7,cy-6,14,10},2,C(color,.8f),1.1f);
 line(cx,cy+4,cx,cy+7,C(color,.8f),1);
 line(cx-4,cy+7,cx+4,cy+7,C(color,.8f),1);
 dot(cx+8,cy-5,3.5f,C(visualTheme[2]));
 dot(cx+8,cy-5,2.1f,C(color));
 Tleft(connectionLabel(),F_FOOT,l+39,596,C(linked?0xe1efe9:0xb5c2d8));
 if(linked&&livePing){
  R latency{l+211,584,73,24};fillR(latency,6,C(visualTheme[0],.055f));
  for(int j=0;j<3;j++)fillR({latency.x+8+j*3,latency.y+16-j*3,1.7f,3.f+j*3},.7f,C(visualTheme[0],.65f));
  T(std::to_wstring(pingMs.load())+L" ms",F_CTRL,latency.x+22,latency.y,latency.w-25,latency.h,C(0xbccce5));
 }
 bool resetVisible=!defensePage(shootTab)&&shootTab!=5;
 if(resetVisible){
  R r=footerResetRect();float h=heat(r,inR(r,mx,my));bool pressed=inR(r,mx,my)&&(GetKeyState(VK_LBUTTON)&0x8000);
  line(r.x-16,585,r.x-16,607,C(visualTheme[0],.13f));
  fillR(r,8,C(visualTheme[0],.035f+.055f*h+(pressed?.04f:0)));
  strokeR(r,8,C(visualTheme[0],.14f+.16f*h),.9f);
  float x=r.x+17,y=r.y+16,angle=h*.45f;
  for(int j=0;j<15;j++){float a=-.5f+j*.32f+angle,b=a+.32f;line(x+5*cosf(a),y+5*sinf(a),x+5*cosf(b),y+5*sinf(b),C(0xabbcd8),1.1f);}
  float a=-.5f+angle,px=x+5*cosf(a),py=y+5*sinf(a);
  line(px,py,px-3.5f,py-.7f,C(0xabbcd8),1.1f);line(px,py,px+.4f,py-3.5f,C(0xabbcd8),1.1f);
  T(L"Reset panel",F_CTRL,r.x+30,r.y,r.w-37,r.h,C(blend(0xb6c5dd,0xf2f6ff,h)));
 }
 R r=footerProfilesRect();float h=heat(r,inR(r,mx,my));bool selected=shootTab==5,pressed=inR(r,mx,my)&&(GetKeyState(VK_LBUTTON)&0x8000);
 fillR({r.x,r.y+2,r.w,r.h},8,C(0x070c18,.15f));
 fillR(r,8,C(visualTheme[0],.13f+.065f*h+(selected?.06f:0)+(pressed?.04f:0)));
 strokeR(r,8,C(visualTheme[0],.29f+.2f*h),1);
 line(r.x+10,r.y+1,r.x+r.w-10,r.y+1,C(visualTheme[0],.3f+.16f*h),.8f);
 float x=r.x+17,y=r.y+16;
 strokeR({x-5,y-4,12,10},2,C(blend(visualTheme[0],0xffffff,.38f)),1.1f);
 line(x-7,y+3,x-7,y-7,C(visualTheme[0],.6f),1);
 line(x-7,y-7,x+3,y-7,C(visualTheme[0],.6f),1);
 T(L"Profiles",F_CTRL,r.x+32,r.y,65,r.h,C(0xe5eeff));
 float ax=r.x+r.w-17+2*h;line(ax-2,y-3,ax+1,y,C(0xbdcbea),1.2f);line(ax+1,y,ax-2,y+3,C(0xbdcbea),1.2f);
}
