#include "../src/ff3_gui_power.hpp"
#include <iostream>
int main(){
 unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok){std::cerr<<"Failed GUI power check "<<checks<<'\n';std::exit(1);}};
 std::string text="95";auto address=reinterpret_cast<std::uintptr_t>(&text);auto process=GetCurrentProcess();
 for(int next:{94,73,1,9,95}){int old=std::stoi(text);check(ff3GuiPower::write(process,address,old,next));check(text==std::to_string(next));check(text.capacity()==15);}
 check(!ff3GuiPower::write(process,address,94,73)&&text=="95");
 check(!ff3GuiPower::write(process,address,95,96)&&text=="95");
 check(!ff3GuiPower::write(process,address,95,0)&&text=="95");
 text="bad";check(!ff3GuiPower::write(process,address,95,73)&&text=="bad");
 text="95";text.reserve(40);auto pointer=text.data();auto capacity=text.capacity();check(!ff3GuiPower::write(process,address,95,73)&&text=="95"&&text.data()==pointer&&text.capacity()==capacity);
 check(!ff3GuiPower::write(process,0,95,73));
 std::cout<<checks<<" direct GUI storage checks passed in this test process; no game writes or inputs\n";
}
