#define NOMINMAX
#include "../src/game_module_ui.hpp"
#include <iostream>
#include <sstream>
void inspectUi(HANDLE h,addr root,int depth,std::ostringstream& out,int& budget){
 if(!root||depth<0||--budget<0)return;
 auto kind=defense::classOf(h,root);out<<nm(h,root)<<':'<<kind<<" vis="<<int(rd<unsigned char>(h,root+Offsets::GuiObject::Visible,255));
 if(kind=="TextLabel"||kind=="TextButton"||kind=="TextBox")out<<" text=["<<defense::textAt(h,root+Offsets::GuiObject::Text)<<']';out<<"; ";
 for(auto item:children(h,root))if(defense::classOf(h,item)!="LocalScript")inspectUi(h,item,depth-1,out,budget);
}
int main(){HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 2;
 auto s=inspectBasketball(h,base,pid);auto module=gameModules::identify(s);if(!module||module->id!=gameModules::Id::FootballFusion3){CloseHandle(h);return 3;}
 auto players=child(h,s.model,"Players"),player=rd<addr>(h,players+Offsets::Player::LocalPlayer);std::string previous="initial";
 for(int i=0;i<150;i++){
  std::ostringstream out;auto own=rd<addr>(h,player+Offsets::Player::ModelInstance);
  out<<"character="<<bool(own)<<" gui="<<bool(child(h,player,"PlayerGui"))<<" ";
  for(auto item:children(h,own))if(nm(h,item)=="Football"||defense::classOf(h,item)=="Tool")out<<"EQUIPPED "<<nm(h,item)<<':'<<defense::classOf(h,item)<<" handle="<<bool(child(h,item,"Handle"))<<"; ";
  for(auto gui:children(h,child(h,player,"PlayerGui")))if(nm(h,gui)=="BallGui"){out<<"BALLGUI "<<gui<<" enabled="<<int(rd<unsigned char>(h,gui+Offsets::GuiObject::ScreenGui_Enabled,255))<<" ";int budget=100;inspectUi(h,gui,4,out,budget);}
  auto value=out.str();if(value!=previous){std::cout<<"t="<<i*.2<<' '<<value<<std::endl;previous=value;}Sleep(200);
 }CloseHandle(h);
}
