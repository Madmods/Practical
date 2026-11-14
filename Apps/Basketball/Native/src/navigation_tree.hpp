#pragma once
// Visual treatment only. Rows and hit testing share the layout in treeRows().
static float treeSelectionY[2]={-1,-1},treeSelectionAlpha[2]={};
static float treeHover[11]={},treeParentHover[5]={};
static void leafGlyph(int tab,float x,float y,D2D1_COLOR_F c){
 if(tab==0){ring(x,y,4.5f,c,1.1f);line(x,y,x,y-3,c);line(x,y,x+2,y+1,c);}
 else if(tab==1){line(x-5,y-3,x+5,y-3,c);line(x-5,y+3,x+5,y+3,c);dot(x-1,y-3,1.5f,c);dot(x+2,y+3,1.5f,c);}
 else if(tab==4){ring(x,y,4,c,1.1f);line(x-7,y,x-4,y,c);line(x+4,y,x+7,y,c);line(x,y-7,x,y-4,c);dot(x,y,1.2f,c);}
 else if(tab==7){ring(x-2,y-2,2.1f,c,1.1f);line(x-6,y+5,x-5,y+1,c);line(x-5,y+1,x+1,y+1,c);line(x+1,y+1,x+2,y+5,c);line(x+4,y-3,x+6,y-1,c);line(x+6,y-1,x+4,y+1,c);}
 else if(tab==8){line(x-6,y+3,x+4,y-3,c,1.2f);line(x,y-4,x+5,y-4,c,1.2f);line(x+5,y-4,x+5,y+1,c,1.2f);}
 else if(tab==9){line(x-5,y+5,x-5,y-5,c,1.2f);line(x-5,y-5,x+5,y-5,c,1.2f);line(x+5,y-5,x+5,y+5,c,1.2f);line(x-2,y+2,x+3,y-3,c,1.2f);}
 else if(tab==10){setc(c);dc->DrawEllipse(D2D1::Ellipse({x,y-3},6,2),solid.Get(),1.1f);line(x-5,y-2,x-2,y+5,c);line(x+5,y-2,x+2,y+5,c);line(x-2,y+5,x+2,y+5,c);}
 else navIcon(tab,x,y,c);
}
static void tickTree(float dt){
 for(int group=0;group<2;group++){
  bool found=false;
  for(auto r:treeRows(group)){
   if(r.parent){int ctx=contextOf(r.tab);bool over=group==navGroup&&mx>=12&&mx<NavigationExpandedWidth-12&&my>=r.y&&my<r.y+36;
    treeParentHover[ctx]=approach(treeParentHover[ctx],over?1.f:0.f,18,dt);
   }else{bool over=group==navGroup&&mx>=43&&mx<NavigationExpandedWidth-14&&my>=r.y&&my<r.y+30&&my<r.clipBottom;
    treeHover[r.tab]=approach(treeHover[r.tab],over?1.f:0.f,20,dt);
    if(r.tab==shootTab&&r.opacity>.02f){found=true;if(treeSelectionY[group]<0||contextOf(fromTab)!=contextOf(shootTab))treeSelectionY[group]=r.y;else treeSelectionY[group]=approach(treeSelectionY[group],r.y,22,dt);}
   }
  }treeSelectionAlpha[group]=approach(treeSelectionAlpha[group],found?1.f:0.f,22,dt);
 }
}
static void drawTree(int group,float alpha,float offset){
 float rw=NavigationExpandedWidth,base=navigationLabelOpacity(drawer)*alpha;if(base<.001f)return;
 auto rows=treeRows(group);
 for(auto r:rows){
  float y=r.y+offset;bool selected=!r.parent&&r.tab==shootTab,active=contextOf(r.tab)==contextOf(shootTab);fade=base*r.opacity;
  if(r.parent){
   int ctx=contextOf(r.tab);float hover=treeParentHover[ctx],open=treeAnim[ctx];
   // Quiet group surface with a separate icon well and a consistent disclosure.
   R box{12,y,rw-24,36};
   fillR(box,8,C(visualTheme[0],(active?.09f:.012f)+hover*.045f));
   if(active)strokeR(box,8,C(visualTheme[0],.15f),.8f);
   fillR({20,y+7,22,22},6,C(visualTheme[0],active?.16f:.055f));
   navIcon(r.tab,31,y+18,C(active?blend(visualTheme[0],0xffffff,.30f):0xa8b6cd));
   Tleft(r.name,F_TAB,51,y+18,C(active?0xf1f5ff:0xb7c4db));
   int count=ctx==1?5:ctx==0||ctx==2?2:1;
   R badge{rw-65,y+10,19,17};fillR(badge,5,C(visualTheme[0],.07f+.04f*open));
   T(std::to_wstring(count),F_CTRL,badge.x,badge.y,badge.w,badge.h,C(0xa3b2d0));
   float cx=rw-28,cy=y+18,angle=open*1.5707963f;
   auto rotate=[&](float x,float yy){return D2D1::Point2F(cx+x*cosf(angle)-yy*sinf(angle),cy+x*sinf(angle)+yy*cosf(angle));};
   auto a=rotate(-2.2f,-3.5f),b=rotate(1.3f,0),c=rotate(-2.2f,3.5f);
   line(a.x,a.y,b.x,b.y,C(active?0xc3d0ea:0x8c9bb6),1.25f);line(b.x,b.y,c.x,c.y,C(active?0xc3d0ea:0x8c9bb6),1.25f);
  }else{
   if(y>=r.clipBottom+offset)continue;
   dc->PushAxisAlignedClip({18,y,rw-10,std::min(y+32,r.clipBottom+offset)},D2D1_ANTIALIAS_MODE_ALIASED);
   float hover=treeHover[r.tab];
   line(31,y,31,std::min(y+(r.last?16:32),r.clipBottom+offset),C(visualTheme[0],.16f),1);
   line(31,y+16,39,y+16,C(visualTheme[0],selected?.6f:.15f),1);
   if(hover>.001f)fillR({44,y+1,rw-58,30},6,C(visualTheme[0],.055f*hover));
   // One sliding selection surface, clipped into every row, avoids snapping.
   float sy=treeSelectionY[group]+offset;
   if(treeSelectionAlpha[group]>.01f){
    float old=fade;fade*=treeSelectionAlpha[group];
    fillR({43,sy+1,rw-57,30},7,C(visualTheme[0],.155f));
    strokeR({43,sy+1,rw-57,30},7,C(visualTheme[0],.24f),.8f);
    line(51,sy+2,rw-23,sy+2,C(visualTheme[0],.25f),.8f);
    fade=old;
   }
   if(selected){dot(31,y+16,3.8f,C(visualTheme[0],.14f));dot(31,y+16,1.7f,C(visualTheme[0]));}
   leafGlyph(r.tab,59,y+16,C(selected?blend(visualTheme[0],0xffffff,.35f):blend(0x8191af,visualTheme[0],hover*.5f)));
   Tleft(r.name,F_SMALL,75,y+16,C(selected?0xf4f7ff:blend(0xa3b2cc,0xe6edfa,hover)));
   if(selected){ring(rw-28,y+16,2.4f,C(visualTheme[0],.9f),1.1f);dot(rw-28,y+16,.8f,C(0xe4ebff));}
   dc->PopAxisAlignedClip();
  }
 }
 fade=1;
}
