#pragma once
static void featureHeader(int id,const wchar_t*name,float height){
 R h=headers[id];bool on=id==0?armed.load():id==1?assist.load():id==2?ceilingOn.load():pumpOn.load();
 float enabled=uiToggle[id==0?0:id==1?1:id==2?4:6],hover=heat(h,inR({h.x,h.y,h.w-115,h.h},mx,my));
 panel({h.x,h.y,h.w,48+height},.15f+.75f*enabled);
 dc->PushAxisAlignedClip({h.x+1,h.y+1,h.x+h.w-1,h.y+48},D2D1_ANTIALIAS_MODE_ALIASED);
 fillR({h.x+1,h.y+1,h.w-2,50},10,C(visualTheme[0],.025f+.025f*hover+.025f*enabled));
 dc->PopAxisAlignedClip();
 float cy=h.y+24;
 fillR({h.x+15,h.y+9,30,30},9,C(visualTheme[0],.085f+.11f*enabled));
 strokeR({h.x+15,h.y+9,30,30},9,C(visualTheme[0],.17f+.13f*enabled),.8f);
 if(id==3){ring(h.x+30,cy,6,C(visualTheme[0]),1.1f);line(h.x+26,cy+2,h.x+31,cy-3,C(0xdfe8ff),1.2f);line(h.x+31,cy-3,h.x+31,cy+1,C(0xdfe8ff),1.2f);}
 else leafGlyph(id==0?0:id==1?1:4,h.x+30,cy,C(blend(visualTheme[0],0xffffff,.35f)));
 Tleft(name,F_TAB,h.x+57,cy,C(0xf2f5fc));
 R state{752,h.y+14,61,20};fillR(state,6,C(visualTheme[0],.045f+.055f*enabled));
 dot(state.x+10,cy,2.1f,C(on?visualTheme[0]:0x6f7c94));
 T(on?L"On":L"Off",F_SMALL,state.x+19,state.y,state.w-22,state.h,C(on?0xdce7ff:0x9aa9c2));
 float a=expansion[id],x=843;R disclosure{x-13,h.y+11,26,26};
 float ch=heat(disclosure,inR(disclosure,mx,my));
 fillR(disclosure,7,C(visualTheme[0],.025f+.065f*ch));
 line(x-3,cy-3+2*a,x+1-2*a,cy+2*a,C(0xb2c0d9),1.3f);
 line(x+1-2*a,cy+2*a,x-3+6*a,cy+3-4*a,C(0xb2c0d9),1.3f);
 R tog=id==0?autoSwitch:id==1?assistSwitch:id==2?ceilingSwitch:pumpSwitch;
 toggle(tog,enabled,inRpad(tog,7,mx,my));
 if(height>1){line(h.x+20,h.y+48,h.x+h.w-20,h.y+48,C(visualTheme[0],.12f));fillR({h.x+20,h.y+47.5f,38,1.5f},.75f,C(visualTheme[0],.24f+.36f*enabled));}
}
static void precisionTrack(int id){
 auto t=tracks[id];float x0=t.x0,x1=t.x1,y=t.y;
 bool hot=editing==id||inR({x0-9,y-14,x1-x0+18,28},mx,my);
 float h=heat({x0,y-10,x1-x0,20},hot),v=clampf(uiSlider[id],0,1),thumb=x0+v*(x1-x0);
 fillR({x0,y-3,x1-x0,6},3,C(visualTheme[2],.5f));
 strokeR({x0,y-3,x1-x0,6},3,C(visualTheme[0],.12f),.7f);
 if(thumb>x0){accentG->SetStartPoint({x0,y});accentG->SetEndPoint({x1,y});accentG->SetOpacity(fade*.92f);dc->FillRoundedRectangle(RR({x0,y-2,thumb-x0,4},2),accentG.Get());accentG->SetOpacity(1);}
 for(int i=0;i<=12;i++){float x=x0+(x1-x0)*i/12.f;line(x,y+8,x,y+(i%3==0?12:10),C(visualTheme[0],(x<=thumb?.25f:.11f)+.08f*h),.8f);}
 if(t.low<0&&t.high>0){float zero=x0+(0-t.low)*(x1-x0)/float(t.high-t.low);line(zero,y-6,zero,y+4,C(0xc3d0e8,.45f),1);}
 float hh=16+2*h;
 if(h>.01f)fillR({thumb-8,y-13,16,26},7,C(visualTheme[0],.1f*h));
 fillR({thumb-4,y-hh/2+1,8,hh},3,C(0x080d1a,.4f));
 fillR({thumb-4,y-hh/2,8,hh},3,C(0xeff4ff));
 strokeR({thumb-4,y-hh/2,8,hh},3,C(visualTheme[0],.8f),1);
 line(thumb,y-3,thumb,y+3,C(visualTheme[0],.85f),1.2f);
}
static void compactTrack(int id,const wchar_t*name,const wchar_t*unit){
 auto t=tracks[id];R row{leftEdge()+12,t.y-23,912-leftEdge(),46};
 float hover=heat(row,inR(row,mx,my));
 fillR(row,7,C(visualTheme[0],.012f+.03f*hover));
 T(name,F_LABEL,leftEdge()+24,t.y-14,200,28,C(0xdfe7f5));
 precisionTrack(id);
 R value{846,t.y-14,66,28};chip(value,7,inR(value,mx,my));
 line(value.x+8,value.y+1,value.x+value.w-8,value.y+1,C(visualTheme[0],.16f),.8f);
 std::wstring v=std::to_wstring(getValue(id))+unit;if(!id&&getValue(id)>0)v=L"+"+v;
 if(valueEditing==id){
  strokeR(value,7,C(visualTheme[0],.9f),1.5f);
  if(valueSelectAll)fillR({value.x+7,value.y+5,value.w-14,18},3,C(visualTheme[0],.22f));
  v=valueText;if(!valueSelectAll&&int(tsec*2)%2==0)v+=L"|";
 }
 T(v,F_CTRL,value.x,value.y,value.w,value.h,C(0xeaf0ff));
}
static void drawShotBinding(){
 R row{leftEdge()+12,384,912-leftEdge(),40};
 fillR(row,7,C(visualTheme[0],.025f));
 label(L"Shot input",rx0,391);
 T(L"or",F_CTRL,800,390,26,28,C(0x9eacc6));
 for(int slot=0;slot<2;slot++){
  R r=slot?shotBindAlt:shotBind;int key=slot?shootKeyAlt.load():shootKey.load();
  keyField(r,key?vkName(key):L"None",capturing==(slot?3:1),inR(r,mx,my));
 }
}
