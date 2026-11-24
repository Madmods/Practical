#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
namespace ff3qb {
inline bool powerGridValid(int step,double anchor){return step==1||step==5&&std::isfinite(anchor)&&anchor>=1&&anchor<=95&&anchor==std::floor(anchor);}
inline bool powerReachable(double power,int step=1,double anchor=0){return powerGridValid(step,anchor)&&std::isfinite(power)&&power>=1&&power<=95&&std::abs(std::remainder(power-anchor,step))<.001;}
inline std::array<double,2> powerNeighbors(double power,int step=1,double anchor=0){
 if(!powerGridValid(step,anchor)||!std::isfinite(power))return {};
 power=std::clamp(power,1.,95.);const double units=(power-anchor)/step;
 std::array<double,2> result{anchor+std::floor(units)*step,anchor+std::ceil(units)*step};for(auto& value:result)if(!powerReachable(value,step,anchor))value=0;return result;
}
struct PowerKeyStep {bool valid=false;unsigned key=0;int expected=0;};
inline PowerKeyStep powerKeyStep(int current,int desired){
 if(!powerReachable(desired,5,current))return {};
 if(current==desired)return {true,0,current};
 const int direction=desired>current?1:-1;return {true,unsigned(direction>0?'R':'F'),current+direction*5};
}
class PowerKeyPress {
 std::function<bool(unsigned,bool)> send_;std::function<bool()> physical_;unsigned key_=0;bool owned_=false;
public:
 PowerKeyPress(std::function<bool(unsigned,bool)> send,std::function<bool()> physical):send_(std::move(send)),physical_(std::move(physical)){}
 bool begin(unsigned key){if(owned_||key!='R'&&key!='F'||physical_())return false;key_=key;return owned_=send_(key,true);}
 bool release(){if(!owned_)return true;if(physical_()){owned_=false;return true;}for(int i=0;i<2;++i)if(send_(key_,false)){owned_=false;return true;}return false;}
 ~PowerKeyPress(){release();}
 PowerKeyPress(const PowerKeyPress&)=delete;PowerKeyPress& operator=(const PowerKeyPress&)=delete;
};
enum class PowerStepResult {Waiting,Acknowledged,Unexpected,Timeout};
inline PowerStepResult powerStepResult(double previous,double expected,double observed,double elapsed){
 if(!std::isfinite(elapsed)||elapsed<0||elapsed>.25)return PowerStepResult::Timeout;
 if(observed==expected)return PowerStepResult::Acknowledged;
 return observed==previous?PowerStepResult::Waiting:PowerStepResult::Unexpected;
}
}
