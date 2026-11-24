#pragma once
#include "ff3_mouse_aim.hpp"
#include <functional>
namespace ff3Input {
struct CameraPanStep {int dx=0,dy=0;bool valid=false,done=false;};
inline CameraPanStep cameraPanStep(ff3qb::Screen target,int width,int height){
 if(!target.valid||!std::isfinite(target.x)||!std::isfinite(target.y)||width<100||height<100)return {};
 if(ff3qb::aimInViewport(target,width,height,48))return {0,0,true,true};
 return {int(std::lround(std::clamp((target.x-width*.5)*.22,-64.,64.))),int(std::lround(std::clamp((target.y-height*.5)*.22,-64.,64.))),true,false};
}
// Own only the synthetic right-button edge. A real right-button press takes
// over ownership; its matching release remains the user's responsibility.
class CameraPanButton {
 std::function<bool(bool)> sender;std::function<bool()> physical;bool down=false;
public:
 CameraPanButton(std::function<bool(bool)> send,std::function<bool()> held):sender(std::move(send)),physical(std::move(held)){if(!physical())down=sender(true);}
 ~CameraPanButton(){if(down&&!physical()&&!sender(false))sender(false);}
 bool active()const{return down&&!physical();}
 CameraPanButton(const CameraPanButton&)=delete;CameraPanButton& operator=(const CameraPanButton&)=delete;
};
}
