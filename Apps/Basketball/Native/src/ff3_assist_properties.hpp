#pragma once
#include "ff3_assist_settings.hpp"
namespace ff3Assist {
enum class PropertyKind {JumpPower=1,JumpHeight=2,PartSize=3,CollisionGroup=4,LegacyWalkSpeed=5,PlatformStand=6,EvaluateStateMachine=7,HeadSize=8,HeadCollision=9,RootAnchored=10};
inline bool headProperty(PropertyKind k){return k==PropertyKind::HeadSize||k==PropertyKind::HeadCollision;}
inline bool flagProperty(PropertyKind k){return k==PropertyKind::HeadCollision||k==PropertyKind::RootAnchored;}
struct Property {PropertyKind kind=PropertyKind::JumpPower;addr instance=0,parent=0,primitive=0;std::vector<unsigned char> value;};
inline size_t propertySize(PropertyKind kind){return (kind==PropertyKind::PartSize||kind==PropertyKind::HeadSize)?12:(kind==PropertyKind::CollisionGroup||kind==PropertyKind::PlatformStand||kind==PropertyKind::EvaluateStateMachine||flagProperty(kind))?1:4;}
inline addr propertyOffset(PropertyKind kind){switch(kind){case PropertyKind::JumpPower:return Offsets::Humanoid::JumpPower;case PropertyKind::JumpHeight:return Offsets::Humanoid::JumpHeight;case PropertyKind::EvaluateStateMachine:return Offsets::Humanoid::EvaluateStateMachine;case PropertyKind::PlatformStand:return Offsets::Humanoid::PlatformStand;case PropertyKind::LegacyWalkSpeed:return Offsets::Humanoid::WalkSpeed;case PropertyKind::PartSize:case PropertyKind::HeadSize:return Offsets::Primitive::Size;case PropertyKind::HeadCollision:case PropertyKind::RootAnchored:return Offsets::Primitive::Flags;default:return ff3Play::physicsGroupOffset;}}
template<class T> inline std::vector<unsigned char> bytes(T value){const auto* begin=reinterpret_cast<const unsigned char*>(&value);return {begin,begin+sizeof value};}
inline bool validPropertyValue(PropertyKind kind,const std::vector<unsigned char>& value){
 if(value.size()!=propertySize(kind))return false;if(kind==PropertyKind::CollisionGroup)return value[0]<32;if(kind==PropertyKind::PlatformStand||kind==PropertyKind::EvaluateStateMachine||flagProperty(kind))return value[0]<=1;
 if(kind==PropertyKind::PartSize||kind==PropertyKind::HeadSize){shooting::Vec v;memcpy(&v,value.data(),sizeof v);return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&std::min({v.x,v.y,v.z})>.1&&std::max({v.x,v.y,v.z})<=(kind==PropertyKind::HeadSize?20:30);}
 float f;memcpy(&f,value.data(),sizeof f);return std::isfinite(f)&&f>=0&&f<=(kind==PropertyKind::JumpPower?150:60);
}
inline bool sameProperty(const Property& a,const Property& b){return a.kind==b.kind&&a.instance==b.instance&&a.parent==b.parent&&a.primitive==b.primitive;}
struct PropertyLease {Property property;std::vector<unsigned char> original,previous;};
struct Properties {
 HANDLE h=nullptr;DWORD pid=0;addr own=0;std::wstring stamp;std::vector<PropertyLease> leases;bool journalOwned=false;
 ~Properties(){restore();if(h)CloseHandle(h);}
 bool identity(const Property& p)const{
  if(!h||WaitForSingleObject(h,0)!=WAIT_TIMEOUT||(!headProperty(p.kind)&&p.parent!=own)||!p.instance||rd<addr>(h,p.instance+Offsets::Instance::Parent)!=p.parent)return false;
  if(p.kind==PropertyKind::JumpPower||p.kind==PropertyKind::JumpHeight||(p.kind==PropertyKind::LegacyWalkSpeed||p.kind==PropertyKind::PlatformStand||p.kind==PropertyKind::EvaluateStateMachine))return defense::classOf(h,p.instance)=="Humanoid";
  if(headProperty(p.kind)&&(p.parent==own||nm(h,p.instance)!="Head"||!child(h,p.parent,"Humanoid")))return false;
  if(p.kind==PropertyKind::RootAnchored&&nm(h,p.instance)!="HumanoidRootPart")return false;
  if(!defense::partClass(defense::classOf(h,p.instance))||!p.primitive||rd<addr>(h,p.instance+Offsets::BasePart::Primitive)!=p.primitive)return false;
  auto flags=rd<unsigned char>(h,p.primitive+Offsets::Primitive::Flags,255);if(flags==255||p.kind!=PropertyKind::RootAnchored&&!headProperty(p.kind)&&(flags&Offsets::PrimitiveFlags::Anchored))return false;
  if(p.kind==PropertyKind::PartSize&&(flags&Offsets::PrimitiveFlags::CanCollide))return false;
  return true;
 }
 bool retired(const Property& p)const{
  if(h&&WaitForSingleObject(h,0)==WAIT_OBJECT_0)return true;if(!h)return false;addr parent=0;SIZE_T count=0;
  if(!ReadProcessMemory(h,reinterpret_cast<void*>(p.instance+Offsets::Instance::Parent),&parent,sizeof parent,&count)||count!=sizeof parent){MEMORY_BASIC_INFORMATION info{};return VirtualQueryEx(h,reinterpret_cast<void*>(p.instance),&info,sizeof info)==sizeof info&&(info.State==MEM_FREE||info.State==MEM_RESERVE);}if(parent!=p.parent)return true;
  if(p.kind==PropertyKind::PartSize||p.kind==PropertyKind::CollisionGroup||headProperty(p.kind)||p.kind==PropertyKind::RootAnchored){addr primitive=0;if(ReadProcessMemory(h,reinterpret_cast<void*>(p.instance+Offsets::BasePart::Primitive),&primitive,sizeof primitive,&count)&&count==sizeof primitive&&primitive!=p.primitive)return true;}return false;
 }
 addr address(const Property& p)const{return (p.kind==PropertyKind::JumpPower||p.kind==PropertyKind::JumpHeight||(p.kind==PropertyKind::LegacyWalkSpeed||p.kind==PropertyKind::PlatformStand||p.kind==PropertyKind::EvaluateStateMachine)?p.instance:p.primitive)+propertyOffset(p.kind);}
 float baseline(const Property& p,float fallback)const{for(const auto& lease:leases)if(sameProperty(p,lease.property)&&lease.original.size()==4){float v;memcpy(&v,lease.original.data(),4);return v;}return fallback;}
 Vec baselineSize(const Property& p,Vec fallback)const{for(const auto& lease:leases)if(sameProperty(p,lease.property)&&lease.original.size()==12){shooting::Vec v;memcpy(&v,lease.original.data(),12);return {v.x,v.y,v.z};}return fallback;}
 bool read(const Property& p,std::vector<unsigned char>& value)const{if(!identity(p))return false;value.resize(propertySize(p.kind));SIZE_T count=0;if(!ReadProcessMemory(h,reinterpret_cast<void*>(address(p)),value.data(),value.size(),&count)||count!=value.size())return false;if(flagProperty(p.kind)){const unsigned mask=p.kind==PropertyKind::RootAnchored?Offsets::PrimitiveFlags::Anchored:Offsets::PrimitiveFlags::CanCollide;value[0]=(value[0]&mask)?1:0;}return validPropertyValue(p.kind,value);}
 bool write(const Property& p,const std::vector<unsigned char>& value){if(!identity(p)||!validPropertyValue(p.kind,value))return false;auto native=value;if(flagProperty(p.kind)){const auto flags=rd<unsigned char>(h,address(p),255);if(flags==255)return false;const unsigned mask=p.kind==PropertyKind::RootAnchored?Offsets::PrimitiveFlags::Anchored:Offsets::PrimitiveFlags::CanCollide;native[0]=value[0]?flags|mask:flags&~mask;}
  SIZE_T count=0;if(!WriteProcessMemory(h,reinterpret_cast<void*>(address(p)),native.data(),native.size(),&count)||count!=native.size())return false;std::vector<unsigned char> actual;return read(p,actual)&&actual==value;
 }

 bool journal(){
  if(recoveryFile.empty())return true;using namespace basketballUi;auto out=object();out.o[L"version"]=number(1);out.o[L"pid"]=number(pid);out.o[L"created"]=string(stamp);out.o[L"own"]=number(double(own));auto rows=array();
  auto encoded=[](const std::vector<unsigned char>& value){auto a=basketballUi::array();for(auto v:value)a.a.push_back(basketballUi::number(v));return a;};
  for(const auto& lease:leases){auto p=object();p.o[L"kind"]=number(int(lease.property.kind));p.o[L"instance"]=number(double(lease.property.instance));p.o[L"parent"]=number(double(lease.property.parent));p.o[L"primitive"]=number(double(lease.property.primitive));p.o[L"original"]=encoded(lease.original);p.o[L"applied"]=encoded(lease.property.value);p.o[L"previous"]=encoded(lease.previous);rows.a.push_back(p);}out.o[L"properties"]=rows;
  auto temp=recoveryFile;temp+=L".tmp";{std::ofstream file(temp,std::ios::binary);file<<winrt::to_string(json::dump(out));file.flush();if(!file)return false;}if(!MoveFileExW(temp.c_str(),recoveryFile.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return false;journalOwned=true;return true;
 }
 void retireJournal(){if(leases.empty()&&journalOwned&&!recoveryFile.empty()){std::error_code error;std::filesystem::remove(recoveryFile,error);journalOwned=false;}}
 bool restore(){
  bool ok=true,changed=false;for(auto it=leases.begin();it!=leases.end();){std::vector<unsigned char> actual;if(!identity(it->property)){if(retired(it->property)){it=leases.erase(it);changed=true;}else{ok=false;++it;}continue;}if(!read(it->property,actual)){ok=false;++it;continue;}if(actual!=it->property.value&&actual!=it->previous||write(it->property,it->original)){it=leases.erase(it);changed=true;}else{ok=false;++it;}}
  if(leases.empty())retireJournal();else if(changed&&journalOwned)journal();return ok;
 }
 bool bind(const ff3Play::Scene& s){if(pid==s.pid&&own==s.own&&h)return true;if(!restore())return false;if(h)CloseHandle(h);h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,s.pid);pid=s.pid;own=s.own;stamp=h?ff3Play::LegacyMags::created(h):L"";return h&&!stamp.empty();}
 bool apply(const ff3Play::Scene& s,const std::vector<Property>& wanted){
  if(wanted.size()>64||!bind(s))return false;bool ok=true,changed=false;
  for(auto it=leases.begin();it!=leases.end();){if(std::any_of(wanted.begin(),wanted.end(),[&](const auto& p){return sameProperty(p,it->property); })){++it;continue;}std::vector<unsigned char> actual;if(!identity(it->property)){if(retired(it->property)){it=leases.erase(it);changed=true;}else{ok=false;++it;}continue;}if(!read(it->property,actual)){ok=false;++it;continue;}if(actual!=it->property.value&&actual!=it->previous||write(it->property,it->original)){it=leases.erase(it);changed=true;}else{ok=false;++it;}}
  for(const auto& p:wanted){if(p.kind==PropertyKind::LegacyWalkSpeed||!validPropertyValue(p.kind,p.value)){ok=false;continue;}auto it=std::find_if(leases.begin(),leases.end(),[&](const auto& lease){return sameProperty(p,lease.property);});std::vector<unsigned char> current;if(!read(p,current)){ok=false;continue;}
   if(it!=leases.end()&&current==p.value)continue;
   if(it==leases.end()){if(current==p.value)continue;leases.push_back({p,current,current});it=leases.end()-1;}
   else{if(current!=it->property.value&&current!=it->previous)it->original=current;it->previous=current;it->property.value=p.value;}
   if(!journal()||!write(p,p.value))ok=false;else it->previous.clear();changed=true;
  }if(leases.empty())retireJournal();else if(changed&&journalOwned)journal();return ok;
 }
 static bool recover(){
  if(recoveryFile.empty()||!std::filesystem::exists(recoveryFile))return true;
  try{auto data=basketballUi::readConfig(recoveryFile);if(data[L"version"].num()!=1||data[L"properties"].type!=json::Value::Array||data[L"properties"].a.size()>64)return false;
   auto number=[](const json::Value& value)->addr{if(value.type!=json::Value::Number||!std::isfinite(value.n)||value.n<65536||value.n>9007199254740991.||std::floor(value.n)!=value.n)return 0;return addr(value.n);};
   const auto process=data[L"pid"].num();if(process<=0||process>MAXDWORD||process!=std::floor(process))return false;Properties recovery;recovery.pid=DWORD(process);recovery.own=number(data[L"own"]);recovery.stamp=data[L"created"].str();if(!recovery.own||recovery.stamp.empty())return false;
   recovery.h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,recovery.pid);
   if(!recovery.h){if(GetLastError()==ERROR_INVALID_PARAMETER){std::filesystem::remove(recoveryFile);return true;}return false;}
   if(ff3Play::LegacyMags::created(recovery.h)!=recovery.stamp||WaitForSingleObject(recovery.h,0)!=WAIT_TIMEOUT){std::filesystem::remove(recoveryFile);return true;}
   auto decode=[](const json::Value& value){std::vector<unsigned char> bytes;if(value.type!=json::Value::Array||value.a.size()>12)return bytes;for(auto n:value.a){if(n.type!=json::Value::Number||n.n<0||n.n>255||n.n!=std::floor(n.n))return std::vector<unsigned char>{};bytes.push_back(static_cast<unsigned char>(n.n));}return bytes;};
   std::vector<PropertyLease> parsed;for(const auto& row:data[L"properties"].a){const auto kind=row[L"kind"].num();if(kind<1||kind>10||kind!=std::floor(kind))return false;Property p{PropertyKind(int(kind)),number(row[L"instance"]),number(row[L"parent"]),number(row[L"primitive"]),decode(row[L"applied"])};auto original=decode(row[L"original"]),previous=decode(row[L"previous"]);if(!p.instance||(!headProperty(p.kind)&&p.parent!=recovery.own)||!validPropertyValue(p.kind,p.value)||!validPropertyValue(p.kind,original)||!previous.empty()&&!validPropertyValue(p.kind,previous))return false;parsed.push_back({p,original,previous});}recovery.leases=std::move(parsed);
   recovery.journalOwned=true;return recovery.restore();
  }catch(...){return false;}
 }
};
}
