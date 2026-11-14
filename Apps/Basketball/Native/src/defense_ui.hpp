#pragma once
static bool defensePage(int tab){return tab==4||tab>=7&&tab<=10;}
static std::vector<int> defenseCards(){switch(shootTab){case 7:return {3,2};case 8:return {4,5,6};case 9:return {7,8};case 10:return {9};default:return {0,1,2};}}
static const wchar_t* defenseTitles[]={L"Auto guard",L"Screen avoidance",L"Target lock",L"Off-ball guard",L"Auto steal",L"Pass interception",L"Loose-ball marker",L"Auto block",L"Anti alley-oop",L"Auto rebound"};
static bool defenseOpen[10]={true,false,false,true,true,false,false,true,false,true},defenseMore[10]={};
static float defenseExpansion[10]={1,0,0,1,1,0,0,1,0,1},defenseAnim[defense::Count]={};
static float defenseScroll=0,defenseScrollGoal=0,defenseBottom=550;static int defenseDrag=-1;static float defenseX0=0,defenseX1=0;static bool defenseThumbDrag=false;static float defenseGrab=0;
struct DefenseHit{R box;int option=-1,card=-1,action=0;};static std::vector<DefenseHit> defenseHits;
struct DefenseHint{const wchar_t*name,*hint;float x,y;int font;};static std::vector<DefenseHint> defenseHints;
static float defenseMaxScroll(){return std::max(0.f,defenseBottom-550);}
static void resetDefenseScroll(){defenseScroll=defenseScrollGoal=0;defenseDrag=-1;defenseThumbDrag=false;}
static R defenseThumb(){float h=404,range=defenseMaxScroll(),thumb=std::max(40.f,h*h/(h+range));return {943,158+(h-thumb)*(range>0?defenseScroll/range:0),5,thumb};}
static void defenseEdit(float x){if(defenseDrag<0)return;auto&o=defense::options[defenseDrag];int v=o.lo+(int)lround(clampf((x-defenseX0)/(defenseX1-defenseX0),0,1)*(o.hi-o.lo)/o.step)*o.step;o.value=std::clamp(v,o.lo,o.hi);}
static void defenseDragScroll(float y){auto r=defenseThumb();defenseScroll=defenseScrollGoal=clampf((y-158-defenseGrab)/(404-r.h),0,1)*defenseMaxScroll();}
static std::wstring defenseValue(const defense::Option&o){wchar_t b[64];if(o.scale<1)swprintf_s(b,L"%.2f st",o.value.load()*o.scale);else if(std::string(o.key).find("delay")!=std::string::npos||std::string(o.key).find("cooldown")!=std::string::npos||std::string(o.key).find("reaction")!=std::string::npos||std::string(o.key).find("window")!=std::string::npos||std::string(o.key).find("lead")!=std::string::npos)swprintf_s(b,L"%d ms",o.value.load());else if(&o==&defense::options[defense::Softness])swprintf_s(b,L"%d%%",o.value.load());else swprintf_s(b,L"%d st",o.value.load());return b;}
static void drawDefense(){using namespace defense;float l=leftEdge(),y=220;defenseHits.clear();defenseHints.clear();auto live=snapshot();std::wstring reason(live.decision.reason.begin(),live.decision.reason.end());
 dc->SetTransform(D2D1::Matrix3x2F::Translation(0,-defenseScroll));
 fillR({l,164,936-l,42},7,C(visualTheme[0],.055f));dot(l+16,185,3.2f,C(live.scene.valid?0x72dda8:0xa9b7d0));Tleft(reason,F_SMALL,l+30,185,C(0xc4d0e5));
 auto hit=[&](R box,int opt,int card,int action){box.y-=defenseScroll;if(box.y>=158&&box.y+box.h<=562)defenseHits.push_back({box,opt,card,action});};
 auto hint=[&](int id,float x,float cy,int font){defenseHints.push_back({options[id].name,options[id].hint,x,cy-defenseScroll,font});};
 for(int card:defenseCards()){int main=-1,rows=0;bool more=false;std::vector<int> ids;for(int i=0;i<Count;i++)if(options[i].card==card){if(main<0){main=i;continue;}if(options[i].advanced)more=true;if(!options[i].advanced||defenseMore[card])ids.push_back(i);}if(card==0){ids={Response,Gap,Reaction,AntiPost};more=true;if(defenseMore[card])for(int id:{Activation,Coverage,Route,PracticeAI})ids.push_back(id);}if(card==7){ids={Response,BlockDelay,BlockRange};more=true;if(defenseMore[card])for(int id:{JumpKey,JumpCooldown})ids.push_back(id);}rows=(int)ids.size();bool select=card==2||card==3;float full=rows*44.f+(select?60:0)+(more?38:0)+12,body=full*defenseExpansion[card];R header{l,y,936-l,48};panel({l,y,936-l,48+body},on((Id)main)?.65f:0);
  fillR({l+16,y+12,24,24},6,C(visualTheme[0],.12f));navIcon(4,l+28,y+24,C(visualTheme[0]));Tleft(defenseTitles[card],F_TAB,l+54,y+24,C(0xf0f4fc));float a=defenseExpansion[card];line(841,y+21+2*a,845-2*a,y+24+2*a,C(0x9dabc2));line(845-2*a,y+24+2*a,841+6*a,y+27-2*a,C(0x9dabc2));R tog{874,y+13,38,22};toggle(tog,defenseAnim[main],inR(tog,mx,my+defenseScroll));hit(tog,main,card,1);hit({l,y,792-l,48},-1,card,2);hit({824,y,40,48},-1,card,2);hint(main,l+54,y+24,F_TAB);
  if(body>.5f){line(l+20,y+48,916,y+48,C(visualTheme[0],.12f));dc->PushAxisAlignedClip({l,y+48,936,y+48+body},D2D1_ANTIALIAS_MODE_ALIASED);float row=y+56;
   for(int id:ids){auto&o=options[id];float cy=row+18;Tleft(o.name,F_LABEL,l+24,cy,C(0xdce5f4));hint(id,l+24,cy,F_LABEL);R ctrl{874,cy-11,38,22};bool visible=row+36<=y+48+body;
    if(o.kind==Toggle){toggle(ctrl,defenseAnim[id],inR(ctrl,mx,my+defenseScroll));if(visible)hit(ctrl,id,card,1);}
    else if(o.kind==Key){ctrl={826,cy-14,86,28};keyField(ctrl,vkName(o.value.load()),capturing==100+id,inR(ctrl,mx,my+defenseScroll));if(visible)hit(ctrl,id,card,3);}
    else if(o.kind==Choice){const wchar_t* names[]={L"Legit",L"Balanced",L"Blatant"};for(int n=0;n<3;n++){R pill{624+n*96.f,cy-14,92,28};fillR(pill,5,C(visualTheme[0],o.value==n?.22f:.05f));T(names[n],F_CTRL,pill.x,pill.y,pill.w,pill.h,C(o.value==n?0xf5f7ff:0xa9b8cf));if(visible)hit(pill,id,n,8);}}
    else{float x0=std::max(l+230,520.f),x1=818;slider(x0,x1,cy,defenseAnim[id],defenseAnim[id],defenseDrag==id);R val{834,cy-14,78,28};chip(val,6,false);T(defenseValue(o),F_CTRL,val.x,val.y,val.w,val.h,C(0xe9f0ff));if(visible)hit({x0-8,cy-14,x1-x0+16,28},id,card,4);}
    row+=44;
   }
   if(select){std::wstring who=live.selectedName.empty()?L"No matchup selected":std::wstring(live.selectedName.begin(),live.selectedName.end());Tleft(who,F_SMALL,l+24,row+20,C(0xaabbd3));R choose{688,row+4,142,30},clear{840,row+4,72,30};keycap(choose,pick?L"Cancel selection":L"Select matchup",inR(choose,mx,my+defenseScroll));keycap(clear,L"Clear",inR(clear,mx,my+defenseScroll));if(row+34<=y+48+body){hit(choose,-1,card,5);hit(clear,-1,card,6);}row+=60;}
   if(more){R moreBox{l+20,row,220,30};Tleft(defenseMore[card]?L"Fewer settings":card==0?L"Controls & visuals":L"More settings",F_SMALL,l+24,row+15,C(visualTheme[0]));if(row+30<=y+48+body)hit(moreBox,-1,card,7);}
   dc->PopAxisAlignedClip();
  }y+=60+body;
 }
 defenseBottom=y+6;std::wstring detail(live.detail.begin(),live.detail.end());Tleft(detail,F_SMALL,l+4,y,C(0x97a8c2));dc->SetTransform(D2D1::Matrix3x2F::Identity());
 if(defenseMaxScroll()>.5f){auto r=defenseThumb();fillR({943,158,5,404},2.5f,C(visualTheme[0],.05f));fillR(r,2.5f,C(visualTheme[0],.45f));}
}
static void clickDefense(float x,float y){if(y<158||y>562)return;if(x>=939&&defenseMaxScroll()>.5f){auto r=defenseThumb();defenseGrab=inRpad(r,4,x,y)?y-r.y:r.h/2;defenseThumbDrag=true;defenseDragScroll(y);SetCapture(wnd);return;}
 for(auto h:defenseHits)if(inRpad(h.box,h.action==1?4:0,x,y)){switch(h.action){case 1:{auto&o=defense::options[h.option];o.value=!o.value.load();if(o.value)defenseOpen[h.card]=true;save();break;}case 2:defenseOpen[h.card]=!defenseOpen[h.card];break;case 3:capturing=100+h.option;bindHint.clear();break;case 4:defenseDrag=h.option;defenseX0=h.box.x+8;defenseX1=h.box.x+h.box.w-8;defenseEdit(x);SetCapture(wnd);break;case 5:defense::pick=!defense::pick.load();break;case 6:defense::selected=0;defense::pick=false;break;case 7:defenseMore[h.card]=!defenseMore[h.card];break;case 8:defense::applyResponsePreset(h.card);save();break;}return;}}
static void tickDefense(float dt){for(int i=0;i<10;i++)defenseExpansion[i]=approach(defenseExpansion[i],defenseOpen[i]?1.f:0.f,24,dt);for(int i=0;i<defense::Count;i++){auto&o=defense::options[i];float v=o.kind==defense::Number?(o.value.load()-o.lo)/(float)(o.hi-o.lo):o.value.load()?1.f:0.f;defenseAnim[i]=approach(defenseAnim[i],v,25,dt);}defenseScrollGoal=clampf(defenseScrollGoal,0,defenseMaxScroll());if(!defenseThumbDrag)defenseScroll=approach(defenseScroll,defenseScrollGoal,22,dt);}



