#define NOMINMAX
#include "../src/basketball_ui_bridge.hpp"
#include <iostream>
int main(){
 winrt::init_apartment(winrt::apartment_type::single_threaded);
 using namespace basketballUi;int passed=0,failed=0;
 auto check=[&](bool ok,const char* name){if(ok)++passed;else{++failed;std::cerr<<"FAIL "<<name<<'\n';}};
 sessionUnlocked=false;
 const auto report=shotReport({3,shooting::Release::Maximum,81,30,37,.086,2.5,.13,20});
 check(report[L"reason"].str()==L"Maximum limit"&&report[L"percent"].num()==81&&report[L"leadMs"].num()==86,"release cause and send timing remain distinct from game feedback");
 const auto capped=shotReport({4,shooting::Release::AutoMaximum,84,30,32,.081,2.5,.13,20});
 check(capped[L"reason"].str()==L"Auto Time ceiling"&&capped[L"percent"].num()==84,"ceiling reports actual overshoot without clamping diagnostics");
 publishTimingWindow({shooting::pingWindow(80,2.5,true),80,2.5});const auto win=windowReport();
 check(win[L"valid"].boolean()&&win[L"minimum"].num()==64.5&&win[L"maximum"].num()==68.5&&win[L"ping"].num()==80,"captured ping-adjusted window reaches the UI");
 publishTimingWindow({shooting::pingWindow(1000,4,true),1000,4});
 check(!windowReport()[L"valid"].boolean(),"impossible window is not presented as valid");publishTimingWindow({});
 const double reviewAt=now();for(int i=4;i>=0;--i)reviewer.ping(reviewAt-i*.5,30);
 const auto tuning=reviewer.prepare(reviewAt,2.5,trim.load(),1);
 reviewer.observe({7,reviewAt,.78,2.5,30,true,false,tuning});armed=true;
 check(!reviewerFeedback(7,timingReview::Grade::Perfect),"review feedback requires an authenticated input session");
 sessionUnlocked=true;gameDown=true;
 check(!reviewerFeedback(7,timingReview::Grade::Perfect),"held shot prevents feedback adjustment");gameDown=false;
 reviewOn=false;check(!reviewerFeedback(7,timingReview::Grade::Perfect),"disabled reviewer rejects feedback");reviewOn=true;
 check(reviewerFeedback(7,timingReview::Grade::Perfect),"eligible explicit feedback reaches native reviewer");
 check(!reviewerFeedback(7,timingReview::Grade::Perfect),"duplicate native feedback rejected");
 const auto review=reviewReport();
 check(review[L"perfect"].num()==1&&review[L"feedbackCount"].num()==1&&review[L"pendingId"].num()==0,"manual results and pending state exposed accurately");
 check(set(L"autotime.reviewer",basketballUi::boolean(false))&&!reviewOn,"reviewer setting reaches native state");
 check(!set(L"autotime.reviewer",number(1)),"reviewer toggle rejects numeric coercion");reviewOn=true;
 publishTimingWindow({shooting::pingWindow(80,2.5,true),80,2.5});stop();sessionUnlocked=false;
 check(!windowReport()[L"valid"].boolean(),"stop/logout clears the prior shot window");
 check(reviewerView().shots==0,"stop/logout clears reviewer learning");
 // Exact place, complete data, stable identity and fresh observations are required.
 basketballConnection::Snapshot good;good.pid=1;good.model=100;good.character=200;good.placeId=basketballConnection::PlaceId;good.universeId=basketballConnection::UniverseId;
 good.process=good.version=good.opened=good.loaded=good.player=good.characterReady=good.world=good.meter=good.renderer=true;
 basketballConnection::Store store;good.sampledAt=10;store.publish(good);
 check(!store.readyAt(10),"one sample cannot unlock Continue");
 good.sampledAt=10.5;store.publish(good);check(!store.readyAt(10.5),"stable interval required");
 good.sampledAt=10.75;store.publish(good);check(store.readyAt(10.75),"exact game and all data unlock after stability");
 check(!store.readyAt(12.1),"stale connection fails closed");
 check(!store.readyAt(9),"future timestamp fails closed");
 for(auto member:{&basketballConnection::Snapshot::process,&basketballConnection::Snapshot::version,&basketballConnection::Snapshot::opened,&basketballConnection::Snapshot::loaded,&basketballConnection::Snapshot::player,&basketballConnection::Snapshot::characterReady,&basketballConnection::Snapshot::world,&basketballConnection::Snapshot::meter,&basketballConnection::Snapshot::renderer}){
  auto invalid=store.get();invalid.*member=false;check(!basketballConnection::ready(invalid,10.75),"each required dependency gates connection");
 }
 auto other=store.get();other.universeId++;check(!basketballConnection::ready(other,10.75),"other experience rejected even with root place ID");
 other=store.get();other.placeId=137269396533485ULL;check(basketballConnection::ready(other,10.75),"court inside the same Practical Basketball experience accepted");
 other.placeId=0;check(!basketballConnection::ready(other,10.75),"unknown place rejected");
 for(int field=0;field<3;++field){auto changed=good;changed.sampledAt+=.25;if(field==0)changed.pid++;if(field==1)changed.model++;if(field==2)changed.character++;
  store.publish(changed);check(!store.readyAt(changed.sampledAt),"process teleport or respawn restarts stability");store.publish(good);}
 good.sampledAt=20;store.publish(good);check(!store.readyAt(20),"observation gap restarts stability");
 auto offline=good;offline.process=false;offline.sampledAt=20.1;store.publish(offline);check(!store.readyAt(20.1),"disconnect invalidates immediately");
 good.sampledAt=20.2;store.publish(good);check(!store.readyAt(20.2),"reconnect requires new stable interval");
 check(json::parse(L"{\"a\":1}")[L"a"].n==1,"valid JSON");
 for(auto input:{L"{\"a\":}",L"{\"a\":1}junk",L"[1,]",L"{a:1}",L"{\"a\":NaN}",L"{\"a\":1e999}"})check(json::parse(input).type==json::Value::Null,"malformed JSON rejected");
 check(json::parse(std::wstring(40,L'[')+L"0"+std::wstring(40,L']')).type==json::Value::Null,"depth bounded");
 check(json::parse(std::wstring(65537,L' ')).type==json::Value::Null,"length bounded");
 check(!set(L"unknown",basketballUi::boolean(true)),"unknown setting rejected");
 check(!set(L"autotime",number(1)),"toggle type validation");
 check(!set(L"autotime.release",number(INFINITY)),"nonfinite rejected");
 check(!set(L"autotime.release",number(999)),"range enforced");
 check(set(L"autotime.release",number(37))&&trim==37,"integer binding");
 check(set(L"def_guard.gap",number(3.25))&&defense::get(defense::Gap)==325,"scaled stand-off");
 check(!set(L"def_steal.gap",number(3)),"module ownership validated");
 check(set(L"def_guard.response",number(0))&&defense::get(defense::Reaction)==80,"response preset dependent values");
 check(set(L"autotime.shot",keys('Q',VK_XBUTTON1))&&shootKey=='Q'&&shootKeyAlt==VK_XBUTTON1,"two shot keys and mouse binding");
 check(!set(L"autotime.shot",keys('E','E')),"duplicate keys rejected");
 check(!set(L"autotime.shot",keys(VK_F8,0)),"exit key reserved");
 check(set(L"def_block.jump_key",keys(VK_SPACE)),"single defense key");
 gameDown=true;check(!set(L"autotime.shot",keys('E',0)),"held shot cannot be rebound");gameDown=false;
 for(int k:{int('E'),VK_SPACE,VK_F2,VK_INSERT,VK_UP,VK_XBUTTON2})check(keyCode(string(keyName(k)))==k,"key round trip");
 auto profile=snapshot();profile.o[L"shotassist"].o[L"rows"].o[L"minimum"]=number(10);profile.o[L"shotassist"].o[L"rows"].o[L"maximum"]=number(20);
 check(applyProfile(profile)&&minRelease==10&&maxRelease==20,"release range updated together");
 const auto before=json::dump(snapshot());
 profile.o[L"autotime"].o[L"rows"].o[L"release"]=number(42);profile.o[L"shotassist"].o[L"rows"].o[L"maximum"]=number(0);
 sessionUnlocked=true;check(!applyProfile(profile)&&json::dump(snapshot())==before&&sessionUnlocked,"rollback and gate restored");
 gameDown=true;profile=snapshot();profile.o[L"autotime"].o[L"on"]=basketballUi::boolean(true);
 check(!applyProfile(profile)&&json::dump(snapshot())==before,"rollback with held key restores all flags");gameDown=false;sessionUnlocked=false;
 auto schema=bootstrap();check(schema[L"schema"].a.size()==defense::Count,"every defense option exposed");
 auto current=snapshot();for(auto& o:defense::options){const auto module=L"def_"+std::wstring(cards[o.card]);auto key=widen(o.key);const auto& val=key==cards[o.card]?current[module][L"on"]:current[module][L"rows"][key];check(val.type!=json::Value::Null,"option exists in native snapshot");}
 const auto path=std::filesystem::temp_directory_path()/(L"vanta-ui-test-"+std::to_wstring(GetCurrentProcessId())+L".json");
 {std::ofstream f(path);f<<"{\"unknown\":\"unicode \\u2603\",\"earlier_ms\":1e200,\"def_gap\":-1e200}";}
 load(path);check(trim==60&&defense::get(defense::Gap)==200,"large legacy numbers clamp without overflow");
 trim=23;defense::options[defense::Gap].value=425;
 check(save(path),"atomic settings save");trim=1;defense::options[defense::Gap].value=200;load(path);
 check(trim==23&&defense::get(defense::Gap)==425,"legacy config round trip");
 check(defense::on(defense::AntiPost),"anti-post defaults on for existing settings");
 defense::options[defense::AntiPost].value=0;check(save(path),"anti-post preference saves");
 defense::options[defense::AntiPost].value=1;load(path);check(!defense::on(defense::AntiPost),"anti-post off preference restores");
 defense::options[defense::AntiPost].value=1;
 reviewOn=false;check(save(path),"reviewer preference saves");reviewOn=true;load(path);
 check(!reviewOn,"reviewer preference restores independently of learned adjustments");reviewOn=true;
 check(readConfig(path)[L"unknown"].s==L"unicode \u2603","unknown Unicode config preserved");
 std::filesystem::remove(path);
 const auto releasePath=std::filesystem::temp_directory_path()/(L"vanta-release-test-"+std::to_wstring(GetCurrentProcessId()));
 {std::ofstream f(releasePath,std::ios::binary);f<<"abc";}
 const auto digest=vantaRelease::sha256(releasePath);
 check(digest==L"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","Windows SHA256 matches known vector");
 std::filesystem::remove(releasePath);
 check(!vantaRelease::verify(releasePath,digest).current,"missing release catalog fails closed");
 auto catalog=object();catalog.o[L"version"]=string(vantaRelease::Version);catalog.o[L"sha256"]=string(digest);
 auto writeCatalog=[&](){std::ofstream f(releasePath,std::ios::binary);f<<winrt::to_string(json::dump(catalog));};
 writeCatalog();check(vantaRelease::verify(releasePath,digest).current,"matching local release accepted");
 check(!vantaRelease::verify(releasePath,L"").current,"unreadable executable fails closed");
 check(!vantaRelease::verify(releasePath,std::wstring(64,L'0')).current,"changed executable rejected");
 catalog.o[L"version"]=string(L"0.0.0");writeCatalog();check(!vantaRelease::verify(releasePath,digest).current,"different release version rejected");
 catalog.o[L"version"]=string(vantaRelease::Version);catalog.o[L"sha256"]=string(std::wstring(64,L'z'));writeCatalog();check(!vantaRelease::verify(releasePath,digest).current,"invalid checksum rejected");
 {std::ofstream f(releasePath);f<<"{broken";}check(!vantaRelease::verify(releasePath,digest).current,"malformed catalog rejected");
 {std::ofstream f(releasePath);f<<std::string(16385,' ');}check(!vantaRelease::verify(releasePath,digest).current,"oversized catalog rejected");
 std::filesystem::remove(releasePath);
 good.sampledAt=now();good.stableSince=good.sampledAt-1;
 auto readyPacket=connection(good,true,{true,L"Up to date",L"Local release verified"});
 check(readyPacket[L"ready"].boolean()&&readyPacket[L"checks"].a.size()==4,"ready packet has exactly four checks");
 const wchar_t* ids[]={L"client",L"version",L"place",L"release"};
 for(int i=0;i<4;i++)check(readyPacket[L"checks"].a[i][L"id"].str()==ids[i],"connection check order");
 check(!connection(good,true,{false,L"Update needed",L"Install current build"})[L"ready"].boolean(),"release status gates ready packet");
 armed=true;pumpOn=true;assist=true;ceilingOn=true;defense::options[defense::Guard].value=1;defense::options[defense::Block].value=1;
 stop();check(!armed&&!pumpOn&&!assist&&!ceilingOn&&!defense::on(defense::Guard)&&!defense::on(defense::Block),"stop all controls");
 std::cout<<passed<<" checks passed; "<<failed<<" failed\n";
 return failed?1:0;
}
