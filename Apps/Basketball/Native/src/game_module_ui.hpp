#pragma once
#include "basketball_ui_bridge.hpp"
namespace gameModuleUi {
inline json::Value connection(const gameModules::Snapshot& s,bool workspace,const vantaRelease::Result& release){
 using namespace basketballUi;
 const auto* m=gameModules::identify(s);
 if(m&&m->id==gameModules::Id::Basketball){auto out=basketballUi::connection(s,workspace,release);out.o[L"moduleId"]=string(m->key);out.o[L"gameName"]=string(m->name);return out;}
 const bool live=basketballConnection::fresh(s,now()),ready=gameModules::ready(s,now());
 std::wstring detail;
 if(!live)detail=L"Connection data expired. Waiting for a fresh scan.";
 else if(!s.process)detail=L"Open Roblox and join Practical Basketball or Football Fusion 3.";
 else if(!s.version)detail=connectionLabel();
 else if(!s.opened)detail=L"Waiting for readable Roblox game data.";
 else if(!m)detail=s.universeId?L"Unsupported game. Join Practical Basketball or Football Fusion 3.":L"Identifying the current game.";
 else if(!gameModules::compatible(s))detail=L"Waiting for Football Fusion 3 and your player to load.";
 else if(!ready)detail=L"Confirming the connection remains stable.";
 else if(!release.current)detail=release.detail;
 else detail=L"Connected to Football Fusion 3";
 auto out=object();out.o[L"type"]=string(L"basketballConnection");out.o[L"ready"]=basketballUi::boolean(ready&&release.current);out.o[L"workspaceOpen"]=basketballUi::boolean(workspace);
 out.o[L"detail"]=string(detail);out.o[L"moduleId"]=string(m?m->key:L"");out.o[L"gameName"]=string(m?m->name:L"Supported game");
 out.o[L"placeId"]=string(std::to_wstring(s.placeId));out.o[L"universeId"]=string(std::to_wstring(s.universeId));out.o[L"checks"]=array();
 auto row=[&](const wchar_t* id,bool ok,const wchar_t* status,const std::wstring& note){auto v=object();v.o[L"id"]=string(id);v.o[L"ok"]=basketballUi::boolean(ok);v.o[L"result"]=string(ok?L"Verified":status);v.o[L"detail"]=string(note);out.o[L"checks"].a.push_back(v);};
 row(L"client",live&&s.process,L"Waiting",L"Find the running Roblox player");
 row(L"version",live&&s.version&&s.opened,L"Not ready",s.version&&s.opened?L"Supported Roblox build":connectionLabel());
 row(L"place",ready,!m&&s.universeId?L"Wrong game":L"Loading",detail);
 row(L"release",release.current,release.status.c_str(),release.detail);return out;
}
inline json::Value opened(gameModules::Id id){
 using namespace basketballUi;auto out=object();out.o[L"type"]=string(L"gameModule");
 for(const auto& m:gameModules::registry)if(m.id==id){out.o[L"moduleId"]=string(m.key);out.o[L"gameName"]=string(m.name);out.o[L"gameplay"]=basketballUi::boolean(m.gameplay);}
 if(id==gameModules::Id::Basketball)out.o[L"modules"]=snapshot();return out;
}
}
