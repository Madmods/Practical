#pragma once
#include <string>
#include "../../../build-offsets.hpp"

namespace transportClient {
// Share the reader's build pin; matching a substring can admit old/suffixed builds.
inline bool supportedImage(const std::wstring& image){
 const auto end=image.find_last_of(L"\\/");
 if(end==std::wstring::npos||_wcsicmp(image.c_str()+end+1,L"RobloxPlayerBeta.exe"))return false;
 const auto start=end?image.find_last_of(L"\\/",end-1):std::wstring::npos;
 const auto begin=start==std::wstring::npos?0:start+1;
 const std::wstring expected(Offsets::ClientVersion.begin(),Offsets::ClientVersion.end());
 const auto folder=image.substr(begin,end-begin);
 return !_wcsicmp(folder.c_str(),expected.c_str());
}
}
