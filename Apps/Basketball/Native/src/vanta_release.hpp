#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include "webui/json_lite.hpp"
#pragma comment(lib,"bcrypt.lib")

// This is a local release catalog check, not an online update/signature service.
namespace vantaRelease {
inline constexpr auto Version = L"1.0.1";
struct Result {bool current=false;std::wstring status=L"Checking",detail=L"Checking the local release catalog";};
inline std::mutex mutex;inline Result result;
inline Result get(){std::lock_guard<std::mutex> lock(mutex);return result;}
inline void publish(Result value){std::lock_guard<std::mutex> lock(mutex);result=std::move(value);}
inline std::wstring sha256(const std::filesystem::path& path){
 struct Handles{BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;~Handles(){if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);}} handles;
 if(BCryptOpenAlgorithmProvider(&handles.algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0||BCryptCreateHash(handles.algorithm,&handles.hash,nullptr,0,nullptr,0,0)<0)return {};
 std::ifstream file(path,std::ios::binary);if(!file)return {};
 unsigned char buffer[65536];while(file){file.read(reinterpret_cast<char*>(buffer),sizeof buffer);const auto count=file.gcount();if(count&&BCryptHashData(handles.hash,buffer,static_cast<ULONG>(count),0)<0)return {};}
 if(!file.eof())return {};
 unsigned char digest[32];if(BCryptFinishHash(handles.hash,digest,sizeof digest,0)<0)return {};
 constexpr wchar_t hex[]=L"0123456789abcdef";std::wstring out;for(auto byte:digest){out+=hex[byte>>4];out+=hex[byte&15];}return out;
}
inline Result verify(const std::filesystem::path& catalog,const std::wstring& executableHash){
 std::ifstream file(catalog,std::ios::binary);if(!file)return {false,L"Unavailable",L"Local release catalog is missing. Reinstall the current local build."};
 std::string text(16385,'\0');file.read(text.data(),text.size());text.resize(static_cast<size_t>(file.gcount()));
 if(text.size()>16384)return {false,L"Invalid",L"Local release catalog is invalid."};
 json::Value data;try{data=json::parse(std::wstring(winrt::to_hstring(text)));}catch(...){return {false,L"Invalid",L"Local release catalog is invalid."};}
 const auto version=data[L"version"].str();auto expected=data[L"sha256"].str();
 if(data.type!=json::Value::Object||version.empty()||expected.size()!=64)return {false,L"Invalid",L"Local release catalog is invalid."};
 for(auto& c:expected){c=towlower(c);if(!((c>=L'0'&&c<=L'9')||(c>=L'a'&&c<=L'f')))return {false,L"Invalid",L"Local release checksum is invalid."};}
 if(version!=Version)return {false,L"Update needed",L"A different local Vanta release is available. Install local release "+version+L"."};
 if(executableHash.empty()||executableHash!=expected)return {false,L"Repair needed",L"Vanta does not match the current local release. Rebuild or reinstall it."};
 return {true,L"Up to date",L"Local release "+version+L" verified"};
}
}
