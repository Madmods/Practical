#include "../diagnostics/network_decoder/client_version.hpp"
#include <iostream>
int main(){
 const std::wstring version(Offsets::ClientVersion.begin(),Offsets::ClientVersion.end());
 int failures=0,checks=0;auto check=[&](bool result){++checks;if(!result)++failures;};
 const auto path=L"C:\\Roblox\\Versions\\"+version;
 check(transportClient::supportedImage(path+L"\\RobloxPlayerBeta.exe"));
 check(transportClient::supportedImage(L"C:/Roblox/Versions/"+version+L"/robloxplayerbeta.EXE"));
 check(!transportClient::supportedImage(path+L"-extra\\RobloxPlayerBeta.exe"));
 check(!transportClient::supportedImage(path+L"\\other\\RobloxPlayerBeta.exe"));
 check(!transportClient::supportedImage(path+L"\\RobloxStudioBeta.exe"));
 check(!transportClient::supportedImage(L"C:\\Versions\\version-old\\RobloxPlayerBeta.exe"));
 check(!transportClient::supportedImage(L"RobloxPlayerBeta.exe"));
 check(!transportClient::supportedImage(L""));
 std::cout<<checks<<" transport version checks; failures="<<failures<<'\n';return failures?1:0;
}
