#define NETWORK_INSPECTOR_MODEL_ONLY
#include "../src/network_inspector.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
int wmain(int argc,wchar_t** argv){winrt::init_apartment();int checks=0;auto check=[&](bool ok){++checks;if(!ok){std::cerr<<"Failed check "<<checks<<'\n';exit(1);}};
 json::Value out;std::wstring error;
 auto good=json::parse(L"{\"calls\":[{\"path\":\"Workspace.Ball.Remote\",\"method\":\"FireServer\",\"timestamp\":\"fixture\",\"args\":[\"Clicked\",{\"x\":1},95]}]}");
 check(networkInspector::validateLog(good,out,error));check(out.a.size()==1);check(out.a[0][L"source"].str()==L"Imported log");
 const auto saved=json::dump(out);
 for(auto bad:{L"null",L"{}",L"{\"calls\":[{\"path\":\"X\",\"method\":\"send\",\"timestamp\":\"x\",\"args\":[]}]}",L"{\"calls\":[{\"path\":\"X\",\"method\":\"FireServer\",\"args\":[]}]}",L"{\"calls\":[{\"path\":\"\",\"method\":\"FireServer\",\"timestamp\":\"x\",\"args\":[]}]}",L"{\"calls\":[{\"path\":\"X\",\"method\":\"FireServer\",\"timestamp\":\"x\",\"args\":{}}]}"}){check(!networkInspector::validateLog(json::parse(bad),out,error));check(json::dump(out)==saved);}
 auto oversized=good;oversized.o[L"calls"].a.resize(501,good[L"calls"].a[0]);check(!networkInspector::validateLog(oversized,out,error));
 auto mixed=good;mixed.o[L"calls"].a.push_back(json::Value{});check(!networkInspector::validateLog(mixed,out,error));check(json::dump(out)==saved);
 auto malicious=good;malicious.o[L"calls"].a[0].o[L"path"]=networkInspector::text(L"Workspace.<img>.Remote");check(networkInspector::validateLog(malicious,out,error));check(out.a[0][L"path"].str()==L"Workspace.<img>.Remote");
 check(!networkInspector::validPath(L"Workspace.\nRemote"));check(!networkInspector::validPath(std::wstring(2049,L'a')));check(networkInspector::method(L"OnClientInvoke"));
 auto deep=networkInspector::array();for(int i=0;i<18;++i){auto next=networkInspector::array();next.a.push_back(deep);deep=next;}check(!networkInspector::validValue(deep));
 if(argc==2){
  check(std::filesystem::file_size(argv[1])<=65536);std::ifstream input(std::filesystem::path(argv[1]),std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(input)),{});
  auto captured=json::parse(std::wstring(winrt::to_hstring(bytes)));check(networkInspector::validateLog(captured,out,error));check(!out.a.empty());std::cout<<"Validated "<<out.a.size()<<" captured records through the native JSON boundary\n";
 }
 std::cout<<"PASS "<<checks<<" inspector validation checks\n";winrt::uninit_apartment();
}
