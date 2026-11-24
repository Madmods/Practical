#pragma once
#include "ff3_play_runtime.hpp"
#include "ff3_assist_logic.hpp"
#include "ff3_assist_catalog.hpp"
namespace ff3Assist {
struct FeatureConfig {
 bool on=false;std::map<std::wstring,json::Value> rows;
 double n(const wchar_t* key,double fallback=0)const{auto i=rows.find(key);return i==rows.end()?fallback:i->second.num(fallback);}
 bool b(const wchar_t* key,bool fallback=false)const{auto i=rows.find(key);return i==rows.end()?fallback:i->second.boolean(fallback);}
 int key(const wchar_t* key=L"input")const{auto i=rows.find(key);return i!=rows.end()&&i->second.a.size()==1?basketballUi::keyCode(i->second.a.front()):0;}
};
using Configuration=std::map<std::wstring,FeatureConfig>;
inline std::mutex settingsMutex;inline std::filesystem::path configPath,recoveryFile;
inline std::atomic<bool> active{false},visualActive{false},navigationActive{false};inline std::atomic<int> travelRequest{0};
inline json::Value catalog(){static const auto definitions=json::parse(CatalogJson);return definitions;}
inline Configuration defaults(){Configuration out;for(const auto& f:catalog().a){auto& target=out[f[L"id"].str()];for(const auto& row:f[L"rows"].a){const auto type=row[L"type"].str();if(type==L"switch"||type==L"slider"||type==L"choice")target.rows[row[L"id"].str()]=row[L"value"];else if(type==L"keybind")target.rows[row[L"id"].str()]=row[L"keys"];}}return out;}
inline Configuration& settings(){static Configuration values=defaults();return values;}
inline std::map<std::wstring,std::string> statuses;
inline void activity(){active=false;visualActive=false;for(const auto& pair:settings())if(pair.second.on){active=true;if(pair.first==L"ff3_intercept"||pair.first==L"ff3_pocket"||pair.first==L"ff3_kick"&&pair.second.b(L"visual"))visualActive=true;}}
inline Configuration configuration(){std::lock_guard<std::mutex> lock(settingsMutex);return settings();}
inline std::pair<FeatureConfig,FeatureConfig> visualConfiguration(){std::lock_guard<std::mutex> lock(settingsMutex);return {settings().at(L"ff3_intercept"),settings().at(L"ff3_pocket")};}
inline FeatureConfig feature(const Configuration& config,const wchar_t* id){auto it=config.find(id);return it==config.end()?FeatureConfig{}:it->second;}
inline void stop(){std::lock_guard<std::mutex> lock(settingsMutex);for(auto& pair:settings())pair.second.on=false;active=visualActive=navigationActive=false;travelRequest=0;}
inline bool set(const std::wstring& key,const json::Value& value){
 const auto separator=key.find(L'.');const auto id=key.substr(0,separator);std::lock_guard<std::mutex> lock(settingsMutex);auto found=settings().find(id);if(found==settings().end())return false;
 if(separator==std::wstring::npos){if(value.type!=json::Value::Bool)return false;if(value.b&&(id==L"ff3_kick"||id==L"ff3_kickwalk"))return false;found->second.on=value.b;statuses[id]=value.b?"Waiting for live FF3 data":"Off";activity();return true;}
 const auto name=key.substr(separator+1);for(const auto& module:catalog().a)if(module[L"id"].str()==id)for(const auto& row:module[L"rows"].a)if(row[L"id"].str()==name){
  auto type=row[L"type"].str();if(type==L"switch"){if(value.type!=json::Value::Bool)return false;}
  else if(type==L"slider"){double step=row[L"step"].num(1);if(value.type!=json::Value::Number||!std::isfinite(value.n)||value.n<row[L"min"].num()||value.n>row[L"max"].num()||std::abs((value.n-row[L"min"].num())/step-std::round((value.n-row[L"min"].num())/step))>1e-5)return false;}
  else if(type==L"choice"){if(value.type!=json::Value::Number||!std::isfinite(value.n))return false;bool listed=false;for(const auto& option:row[L"options"].a)if(option.a.size()==2&&option.a[0].type==json::Value::Number&&option.a[0].n==value.n)listed=true;if(!listed)return false;}
  else if(type==L"keybind"){if(value.type!=json::Value::Array||value.a.size()>1)return false;if(!value.a.empty()){int code=basketballUi::keyCode(value.a.front());if(code<=0||!basketballUi::validKey(code)||code==VK_INSERT||code==VK_RETURN||code==VK_ESCAPE||code==VK_LWIN||code==VK_RWIN)return false;}}
  else return false;found->second.rows[name]=value;activity();return true;
 }return false;
}
inline json::Value modules(){using namespace basketballUi;auto out=object();std::lock_guard<std::mutex> lock(settingsMutex);for(const auto& pair:settings()){auto module=object();module.o[L"on"]=basketballUi::boolean(pair.second.on);module.o[L"rows"]=object();module.o[L"rows"].o=pair.second.rows;out.o[pair.first]=module;}return out;}
inline void status(const wchar_t* id,std::string text){std::lock_guard<std::mutex> lock(settingsMutex);statuses[id]=text.substr(0,240);}
inline json::Value statusReport(){auto out=basketballUi::object();std::lock_guard<std::mutex> lock(settingsMutex);for(const auto& pair:settings()){const auto it=statuses.find(pair.first);out.o[pair.first]=basketballUi::string(basketballUi::widen(pair.first==L"ff3_travel"?(it==statuses.end()?"Choose Rejoin or Find server":it->second):!pair.second.on?"Off":it==statuses.end()?"Waiting for live FF3 data":it->second));}return out;}
inline bool save(){if(configPath.empty())return true;auto temp=configPath;temp+=L".tmp";{std::ofstream out(temp,std::ios::binary);out<<winrt::to_string(json::dump(modules()));out.flush();if(!out)return false;}return MoveFileExW(temp.c_str(),configPath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;}
inline void load(){auto data=basketballUi::readConfig(configPath);{std::lock_guard<std::mutex> lock(settingsMutex);settings()=defaults();}for(const auto& pair:data.o)for(const auto& row:pair.second[L"rows"].o)set(pair.first+L"."+row.first,row.second);stop();}
inline bool requestTravel(const std::wstring& kind,int minimum=0){if(!ff3Native::admitted())return false;const int action=kind==L"rejoin"?1:kind==L"hop"?2:0;if(!action)return false;int empty=0;if(!travelRequest.compare_exchange_strong(empty,action|(std::clamp(minimum,0,100)<<4)))return false;status(L"ff3_travel",action==1?"Preparing rejoin":"Finding another public server");return true;}
}
