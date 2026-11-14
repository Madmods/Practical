#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include "windivert.h"
#include <fstream>
#include <set>
#include <vector>
#include <thread>
#include <atomic>
#include <string>
#include <iostream>
#include <chrono>
static void le32(std::ofstream& out,uint32_t x){out.write((char*)&x,4);}
static unsigned be16(const unsigned char* p){return (unsigned(p[0])<<8)|p[1];}
static constexpr const char* gameFilter="ip and udp and !loopback and (udp.Payload32[0] == 0x01000017 or udp.Payload32[0] == 0x0100001f) and udp.Payload16[4b] == 0x0111 and (udp.Payload[6] == 1 or udp.Payload[6] == 2)";
static std::wstring dependency(){
 wchar_t configured[32768]{};DWORD n=GetEnvironmentVariableW(L"VANTA_WINDIVERT_DLL",configured,32768);if(n&&n<32768)return configured;
 wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);std::wstring path=exe;return path.substr(0,path.find_last_of(L"\\/")+1)+L"WinDivert.dll";
}
static int selfTest(){
 HMODULE dll=LoadLibraryExW(dependency().c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!dll)return 21;
 auto compile=(decltype(&WinDivertHelperCompileFilter))GetProcAddress(dll,"WinDivertHelperCompileFilter");auto eval=(decltype(&WinDivertHelperEvalFilter))GetProcAddress(dll,"WinDivertHelperEvalFilter");
 if(!compile||!eval){FreeLibrary(dll);return 22;}
 const char* error=nullptr;UINT position=0;if(!compile(gameFilter,WINDIVERT_LAYER_NETWORK,nullptr,0,&error,&position)){std::cerr<<"Filter compilation failed at "<<position<<"\n";FreeLibrary(dll);return 23;}
 unsigned char packet[40]{};packet[0]=0x45;packet[3]=40;packet[8]=64;packet[9]=17;packet[12]=10;packet[15]=1;packet[16]=128;packet[17]=116;packet[19]=1;packet[20]=0xc0;packet[21]=1;packet[22]=0xc0;packet[23]=2;packet[25]=20;packet[28]=1;packet[31]=31;packet[32]=1;packet[33]=17;packet[34]=2;
 WINDIVERT_ADDRESS address{};address.Outbound=1;bool ok=eval(gameFilter,packet,sizeof packet,&address);
 packet[34]=1;ok=ok&&eval(gameFilter,packet,sizeof packet,&address);packet[31]=23;ok=ok&&eval(gameFilter,packet,sizeof packet,&address);
 packet[28]=0x40;ok=ok&&!eval(gameFilter,packet,sizeof packet,&address);packet[28]=1;packet[34]=3;ok=ok&&!eval(gameFilter,packet,sizeof packet,&address);
 FreeLibrary(dll);std::cout<<(ok?"Capture filter: 5 checks passed; no capture opened\n":"Capture filter checks failed\n");return ok?0:24;
}
static bool supported(DWORD pid){HANDLE h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!h)return false;wchar_t image[32768]{};DWORD size=32768;bool ok=QueryFullProcessImageNameW(h,0,image,&size);CloseHandle(h);if(!ok)return false;std::wstring name=image;auto slash=name.find_last_of(L"\\/");return slash!=std::wstring::npos&&!_wcsicmp(name.c_str()+slash+1,L"RobloxPlayerBeta.exe")&&name.find(L"version-4ea8e85481e34f46")!=std::wstring::npos;}
static std::set<unsigned> ports(DWORD pid){
 std::set<DWORD> allowed;if(pid){if(supported(pid))allowed.insert(pid);}else{HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snapshot!=INVALID_HANDLE_VALUE){PROCESSENTRY32W entry{sizeof entry};if(Process32FirstW(snapshot,&entry))do{if(!_wcsicmp(entry.szExeFile,L"RobloxPlayerBeta.exe")&&supported(entry.th32ProcessID))allowed.insert(entry.th32ProcessID);}while(Process32NextW(snapshot,&entry));CloseHandle(snapshot);}}
 ULONG bytes=0;GetExtendedUdpTable(nullptr,&bytes,FALSE,AF_INET,UDP_TABLE_OWNER_PID,0);std::vector<unsigned char> buffer(bytes);std::set<unsigned> result;if(bytes&&GetExtendedUdpTable(buffer.data(),&bytes,FALSE,AF_INET,UDP_TABLE_OWNER_PID,0)==NO_ERROR){auto table=(MIB_UDPTABLE_OWNER_PID*)buffer.data();for(DWORD i=0;i<table->dwNumEntries;i++)if(allowed.count(table->table[i].dwOwningPid))result.insert(ntohs((USHORT)table->table[i].dwLocalPort));}return result;
}
int main(int argc,char** argv){
 if(argc==2&&std::string(argv[1])=="--self-test")return selfTest();
 if(argc!=5||std::string(argv[1])!="--capture"){std::cerr<<"Requires --capture OUTPUT.pcap PID SECONDS\n";return 2;}
 std::string path=argv[2];DWORD pid=strtoul(argv[3],nullptr,10);unsigned seconds=strtoul(argv[4],nullptr,10);if(seconds<5||seconds>90)return 3;
 HANDLE game=pid?OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,pid):nullptr;
 if(pid&&(!game||!supported(pid))){if(game)CloseHandle(game);return 4;}
 unsigned received=0,rejected=0,lookups=0;uint64_t lookupMs=0;bool queueConfigured=false;
 auto state=[&](const char* status,unsigned count){std::ofstream info(path+".json");info<<"{\"status\":\""<<status<<"\",\"pid\":"<<pid<<",\"packets\":"<<count<<",\"seconds\":"<<seconds<<",\"mode\":\"sniff-only\",\"filter\":\"game-mux-only\",\"received\":"<<received<<",\"rejected\":"<<rejected<<",\"portLookups\":"<<lookups<<",\"lookupMilliseconds\":"<<lookupMs<<",\"queueConfigured\":"<<(queueConfigured?"true":"false")<<"}";};
 HMODULE dll=LoadLibraryExW(dependency().c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!dll){state("load-failed",0);CloseHandle(game);return 6;}
 auto open=(decltype(&WinDivertOpen))GetProcAddress(dll,"WinDivertOpen");auto recv=(decltype(&WinDivertRecv))GetProcAddress(dll,"WinDivertRecv");auto shutdown=(decltype(&WinDivertShutdown))GetProcAddress(dll,"WinDivertShutdown");auto close=(decltype(&WinDivertClose))GetProcAddress(dll,"WinDivertClose");
 if(!open||!recv||!shutdown||!close){state("api-failed",0);FreeLibrary(dll);CloseHandle(game);return 7;}
 auto setParam=(decltype(&WinDivertSetParam))GetProcAddress(dll,"WinDivertSetParam");
 HANDLE capture=open(gameFilter,WINDIVERT_LAYER_NETWORK,0,WINDIVERT_FLAG_SNIFF|WINDIVERT_FLAG_RECV_ONLY);
 if(capture==INVALID_HANDLE_VALUE){state("open-failed",0);FreeLibrary(dll);CloseHandle(game);return 8;}
 queueConfigured=setParam&&setParam(capture,WINDIVERT_PARAM_QUEUE_LENGTH,16384)&&setParam(capture,WINDIVERT_PARAM_QUEUE_SIZE,32*1024*1024)&&setParam(capture,WINDIVERT_PARAM_QUEUE_TIME,4000);
 std::ofstream out(path,std::ios::binary);if(!out){state("file-failed",0);close(capture);FreeLibrary(dll);CloseHandle(game);return 9;}
 le32(out,0xa1b2c3d4);le32(out,0x00040002);le32(out,0);le32(out,0);le32(out,65535);le32(out,101);
 std::atomic<bool> done=false;std::thread timeout([&](){auto begin=GetTickCount64();while(!done&&GetTickCount64()-begin<seconds*1000ULL&&GetFileAttributesA((path+".stop").c_str())==INVALID_FILE_ATTRIBUTES&&(!game||WaitForSingleObject(game,0)==WAIT_TIMEOUT))Sleep(100);shutdown(capture,WINDIVERT_SHUTDOWN_RECV);});
 state("capturing",0);unsigned count=0;auto currentPorts=ports(pid);ULONGLONG lastPorts=GetTickCount64();unsigned char packet[65535];
 while(count<20000){UINT length=0;WINDIVERT_ADDRESS address{};if(!recv(capture,packet,sizeof packet,&length,&address))break;
  ++received;
  if(length<28||(packet[0]>>4)!=4||packet[9]!=17){++rejected;continue;}unsigned ihl=(packet[0]&15)*4;if(ihl<20||ihl+8>length||be16(packet+6)&0x3fff){++rejected;continue;}
  unsigned local=be16(packet+ihl+(address.Outbound?0:2));if(GetTickCount64()-lastPorts>200||!currentPorts.count(local)){auto began=GetTickCount64();currentPorts=ports(pid);lastPorts=GetTickCount64();lookupMs+=lastPorts-began;++lookups;}if(!currentPorts.count(local)){++rejected;continue;}
  FILETIME ft{};GetSystemTimePreciseAsFileTime(&ft);ULARGE_INTEGER ticks{};ticks.LowPart=ft.dwLowDateTime;ticks.HighPart=ft.dwHighDateTime;auto us=(ticks.QuadPart-116444736000000000ULL)/10;
  le32(out,(uint32_t)(us/1000000));le32(out,(uint32_t)(us%1000000));le32(out,length);le32(out,length);out.write((char*)packet,length);++count;
 }
 done=true;timeout.join();close(capture);out.close();state("complete",count);FreeLibrary(dll);CloseHandle(game);return 0;
}
