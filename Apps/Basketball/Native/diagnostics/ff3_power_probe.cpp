#define NOMINMAX
#include "../src/game_module_ui.hpp"
#include "../src/ff3_input_policy.hpp"
#include "../src/ff3_gui_power.hpp"
#include <iostream>
#include <sstream>
void inspectUi(HANDLE h,addr root,int depth,std::ostringstream& out,int& budget){
 if(!root||depth<0||--budget<0)return;
 auto kind=defense::classOf(h,root),name=nm(h,root);
 if(name=="Frame0"&&kind=="Frame"&&rd<unsigned char>(h,root+Offsets::GuiObject::Visible)==1){
  std::vector<ff3Input::Button> buttons;int order=0;struct P{float x,y;};
  for(auto item:children(h,root)){
   ++order;auto value=ff3Input::powerIndex(nm(h,item));auto type=defense::classOf(h,item);
   if(!value||(type!="ImageButton"&&type!="TextButton")||rd<unsigned char>(h,item+Offsets::GuiObject::Visible)!=1)continue;
   auto p=rd<P>(h,item+Offsets::GuiBase2D::AbsolutePosition,{NAN,NAN}),s=rd<P>(h,item+Offsets::GuiBase2D::AbsoluteSize,{NAN,NAN});
   buttons.push_back({item,value,rd<int>(h,item+Offsets::GuiObject::ZIndex),order,{p.x,p.y},{s.x,s.y},rd<float>(h,item+Offsets::GuiBase2D::AbsoluteRotation,NAN)});
  }
  int selectable=0;for(int power=1;power<=95;++power){ff3Input::Point p;std::uint64_t instance=0;if(ff3Input::selectPower(buttons,power,32768,32768,p,instance))++selectable;}
  out<<"POWER_HIT_TEST buttons="<<buttons.size()<<" selectable="<<selectable<<"/95 (read-only); ";
 }
 const bool numeric=!name.empty()&&name.find_first_not_of("0123456789")==std::string::npos;
 if(!numeric||name=="1"||name=="2"||name=="50"||name=="73"||name=="90"||name=="94"||name=="95"||name=="100"){
  struct P{float x,y;};auto p=rd<P>(h,root+Offsets::GuiBase2D::AbsolutePosition,{NAN,NAN}),s=rd<P>(h,root+Offsets::GuiBase2D::AbsoluteSize,{NAN,NAN});
  out<<name<<':'<<kind<<" vis="<<int(rd<unsigned char>(h,root+Offsets::GuiObject::Visible,255))<<" xy="<<p.x<<','<<p.y<<" size="<<s.x<<','<<s.y<<" rot="<<rd<float>(h,root+Offsets::GuiBase2D::AbsoluteRotation,NAN);
  if(kind=="TextLabel"||kind=="TextButton"||kind=="TextBox")out<<" text=["<<defense::textAt(h,root+Offsets::GuiObject::Text)<<"] length="<<rd<addr>(h,root+Offsets::GuiObject::Text+16)<<" capacity="<<rd<addr>(h,root+Offsets::GuiObject::Text+24);
  if(kind=="NumberValue")out<<" value="<<rd<double>(h,root+Offsets::NumberValue::Value,NAN);
  if(kind=="IntValue")out<<" value="<<rd<std::int64_t>(h,root+Offsets::IntValue::Value);
  if(kind=="ImageButton"||kind=="TextButton")out<<" listeners(click/down/up/enter/leave/move)="<<rd<int>(h,root+Offsets::GuiButton::MouseButton1ClickConnectionCount,-1)<<'/'<<rd<int>(h,root+Offsets::GuiButton::MouseButton1DownConnectionCount,-1)<<'/'<<rd<int>(h,root+Offsets::GuiButton::MouseButton1UpConnectionCount,-1)<<'/'<<rd<int>(h,root+Offsets::GuiObject::MouseEnterConnectionCount,-1)<<'/'<<rd<int>(h,root+Offsets::GuiObject::MouseLeaveConnectionCount,-1)<<'/'<<rd<int>(h,root+Offsets::GuiObject::MouseMovedConnectionCount,-1);
  out<<"; ";
 }
 for(auto item:children(h,root))if(defense::classOf(h,item)!="LocalScript")inspectUi(h,item,depth-1,out,budget);
}
int main(int argc,char** argv){HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base,[](HANDLE candidate,addr module,DWORD id){auto s=inspectBasketball(candidate,module,id);auto kind=gameModules::identify(s);return kind&&kind->id==gameModules::Id::FootballFusion3?100:0;}))return 2;
 auto s=inspectBasketball(h,base,pid);auto module=gameModules::identify(s);if(!module||module->id!=gameModules::Id::FootballFusion3){CloseHandle(h);return 3;}
 auto players=child(h,s.model,"Players"),player=rd<addr>(h,players+Offsets::Player::LocalPlayer);std::string previous="initial";
 if(argc==3&&std::string(argv[1])=="--set-power"){
  std::cout<<"EXPERIMENTAL STORAGE DIAGNOSTIC: this does not invoke the GUI setter or verify rendering/throw power.\n";
  const auto digits=std::string(argv[2]);const int desired=ff3Input::powerIndex(digits);if(desired<1||desired>95){CloseHandle(h);return 4;}
  auto playerGui=child(h,player,"PlayerGui");std::vector<ff3Input::Wheel> wheels;int order=0;
  for(auto gui:children(h,playerGui)){++order;if(nm(h,gui)!="BallGui"||defense::classOf(h,gui)!="ScreenGui")continue;auto frame=child(h,gui,"Frame0"),display=child(h,frame,"Disp");wheels.push_back({gui,rd<int>(h,gui+Offsets::ScreenGui::DisplayOrder),order,rd<unsigned char>(h,gui+Offsets::GuiObject::ScreenGui_Enabled)==1,rd<unsigned char>(h,frame+Offsets::GuiObject::Visible)==1&&rd<unsigned char>(h,display+Offsets::GuiObject::Visible)==1});}
  auto gui=ff3Input::topWheel(wheels),frame=child(h,gui,"Frame0"),display=child(h,frame,"Disp");ff3GuiPower::SmallText text;
  if(!gui||rd<addr>(h,gui+Offsets::Instance::Parent)!=playerGui||rd<addr>(h,display+Offsets::Instance::Parent)!=frame||defense::classOf(h,display)!="TextLabel"||!ff3GuiPower::read(h,display+Offsets::GuiObject::Text,text)){CloseHandle(h);return 5;}
  const int before=ff3GuiPower::value(text);HANDLE writable=OpenProcess(PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION,FALSE,pid);
  const bool changed=writable&&ff3GuiPower::write(writable,display+Offsets::GuiObject::Text,before,desired);
  std::cout<<"DIRECT_GUI pid="<<pid<<" previous="<<before<<" desired="<<desired<<" written="<<changed<<" error="<<(changed?0:GetLastError())<<"; no cursor movement or throw input\n";
  if(writable)CloseHandle(writable);Sleep(200);std::cout<<"GUI_READBACK "<<defense::textAt(h,display+Offsets::GuiObject::Text)<<'\n';CloseHandle(h);return changed?0:6;
 }
 for(int i=0;i<(argc>1?5:150);i++){
  std::ostringstream out;auto own=rd<addr>(h,player+Offsets::Player::ModelInstance);
  out<<"character="<<bool(own)<<" gui="<<bool(child(h,player,"PlayerGui"))<<" ";
  for(auto item:children(h,own))if(nm(h,item)=="Football"||defense::classOf(h,item)=="Tool")out<<"EQUIPPED "<<nm(h,item)<<':'<<defense::classOf(h,item)<<" handle="<<bool(child(h,item,"Handle"))<<"; ";
  for(auto gui:children(h,child(h,player,"PlayerGui")))if(nm(h,gui)=="BallGui"){out<<"BALLGUI "<<gui<<" enabled="<<int(rd<unsigned char>(h,gui+Offsets::GuiObject::ScreenGui_Enabled,255))<<" ";int budget=250;inspectUi(h,gui,4,out,budget);}
  auto value=out.str();if(value!=previous){std::cout<<"t="<<i*.2<<' '<<value<<std::endl;previous=value;}Sleep(200);
 }CloseHandle(h);
}
