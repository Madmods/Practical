#define NOMINMAX
#include <windows.h>
#include <array>
#include <vector>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <string>

using Key=std::array<unsigned char,32>;
static unsigned char mul(unsigned char a,unsigned char b){unsigned char r=0;for(int i=0;i<8;i++){if(b&1)r^=a;bool high=a&128;a<<=1;if(high)a^=0x1b;b>>=1;}return r;}
static const auto sbox=[](){std::array<unsigned char,256>s{};for(int i=0;i<256;i++){unsigned char x=1,b=(unsigned char)i;int e=254;if(!i)x=0;else while(e){if(e&1)x=mul(x,b);b=mul(b,b);e>>=1;}auto rot=[](unsigned char v,int n){return (unsigned char)((v<<n)|(v>>(8-n)));};s[i]=x^rot(x,1)^rot(x,2)^rot(x,3)^rot(x,4)^0x63;}return s;}();
static bool verify(const unsigned char* input,Key& key,bool reverse){
 unsigned char words[240];for(int i=0;i<32;i++)words[i]=reverse?input[(i&~3)+(3-(i&3))]:input[i];
 unsigned char rcon=1;
 for(int pos=32;pos<240;pos+=4){unsigned char t[4];memcpy(t,words+pos-4,4);
  if(pos%32==0){auto first=t[0];t[0]=sbox[t[1]]^rcon;t[1]=sbox[t[2]];t[2]=sbox[t[3]];t[3]=sbox[first];rcon=mul(rcon,2);}
  else if(pos%32==16)for(auto& v:t)v=sbox[v];
  for(int i=0;i<4;i++){words[pos+i]=words[pos-32+i]^t[i];int original=reverse?((pos+i)&~3)+(3-((pos+i)&3)):pos+i;if(words[pos+i]!=input[original])return false;}
 }
 memcpy(key.data(),words,32);return true;
}
static void scan(const unsigned char* data,size_t size,std::vector<Key>& keys,size_t maximum){
 for(size_t p=0;p+240<=size&&keys.size()<maximum;p+=4){uint32_t a,b,c,d,e;memcpy(&a,data+p+4,4);memcpy(&b,data+p+32,4);memcpy(&c,data+p+36,4);if((b^c)!=a)continue;
  memcpy(&a,data+p+8,4);memcpy(&d,data+p+40,4);if((c^d)!=a)continue;memcpy(&a,data+p+12,4);memcpy(&e,data+p+44,4);if((d^e)!=a)continue;
  uint32_t all=0;for(int i=0;i<9;i++){memcpy(&a,data+p+i*4,4);all|=a;}if(!all)continue;
  Key key{};if(!verify(data+p,key,false)&&!verify(data+p,key,true))continue;
  if(std::find(keys.begin(),keys.end(),key)==keys.end())keys.push_back(key);
 }
}
extern "C" __declspec(dllexport) unsigned __cdecl ScanBuffer(const unsigned char* data,size_t size,unsigned char* out,unsigned capacity){
 std::vector<Key> keys;if(!data||!out||!capacity||capacity>512)return 0;scan(data,size,keys,capacity);for(size_t i=0;i<keys.size();i++)memcpy(out+i*32,keys[i].data(),32);return (unsigned)keys.size();
}
struct ScanStats{uint64_t bytesRead;uint32_t regions;uint32_t error;uint32_t milliseconds;uint32_t limitReached;uint64_t nextAddress;};
extern "C" __declspec(dllexport) unsigned __cdecl ScanProcess(DWORD pid,unsigned char* out,unsigned capacity,uint64_t maxBytes,unsigned maxMs,volatile LONG* cancel,ScanStats* stats){
 if(!out||!stats||!capacity||capacity>512||maxMs>30000||maxBytes>4294967296ULL)return 0;
 auto resume=stats->nextAddress;*stats={};HANDLE process=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION,FALSE,pid);if(!process){stats->error=GetLastError();return 0;}
 wchar_t image[32768]{};DWORD imageSize=32768;
 if(!QueryFullProcessImageNameW(process,0,image,&imageSize)){stats->error=GetLastError();CloseHandle(process);return 0;}
 std::wstring imagePath=image;auto separator=imagePath.find_last_of(L"\\/");
 if(separator==std::wstring::npos||_wcsicmp(imagePath.c_str()+separator+1,L"RobloxPlayerBeta.exe")||imagePath.find(L"version-4ea8e85481e34f46")==std::wstring::npos){stats->error=ERROR_REVISION_MISMATCH;CloseHandle(process);return 0;}
 auto began=std::chrono::steady_clock::now();auto elapsed=[&](){return (unsigned)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count();};
 std::vector<Key> keys;std::vector<unsigned char> chunk(1024*1024+240);uintptr_t address=(uintptr_t)std::max<uint64_t>(65536,resume);MEMORY_BASIC_INFORMATION region{};
 while(VirtualQueryEx(process,(void*)address,&region,sizeof region)==sizeof region){
  auto base=(uintptr_t)region.BaseAddress;auto end=base+region.RegionSize;if(end<=address)break;address=end;
  if((cancel&&*cancel)||elapsed()>=maxMs||stats->bytesRead>=maxBytes||keys.size()>=capacity){stats->limitReached=1;break;}
  DWORD protection=region.Protect&0xff;
  if(region.State!=MEM_COMMIT||(region.Protect&PAGE_GUARD)||protection==PAGE_NOACCESS||region.Type!=MEM_PRIVATE)continue;
  if(protection!=PAGE_READWRITE&&protection!=PAGE_WRITECOPY&&protection!=PAGE_READONLY)continue;
  ++stats->regions;
  for(uintptr_t pos=std::max<uintptr_t>(base,(uintptr_t)resume);pos<end;){
   stats->nextAddress=pos;
   if((cancel&&*cancel)||elapsed()>=maxMs||stats->bytesRead>=maxBytes||keys.size()>=capacity){stats->limitReached=1;break;}
   size_t want=(size_t)std::min<uint64_t>({end-pos,chunk.size(),maxBytes-stats->bytesRead});if(want<240)break;
   SIZE_T got=0;if(ReadProcessMemory(process,(void*)pos,chunk.data(),want,&got)&&got>=240){stats->bytesRead+=got;scan(chunk.data(),got,keys,capacity);}
   if(want<=240)break;pos+=want-240;stats->nextAddress=pos;Sleep(1);
  }
 }
 stats->milliseconds=elapsed();CloseHandle(process);for(size_t i=0;i<keys.size();i++)memcpy(out+i*32,keys[i].data(),32);
 SecureZeroMemory(chunk.data(),chunk.size());for(auto& key:keys)SecureZeroMemory(key.data(),key.size());return (unsigned)keys.size();
}
