#pragma once
#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace ff3GuiPower {
struct SmallText {std::array<char,16> bytes{};std::uint64_t size=0,capacity=0;};
static_assert(sizeof(SmallText)==32);
inline int value(const SmallText& text){
 if(text.capacity!=15||text.size<1||text.size>3||text.bytes[text.size]!=0)return 0;
 int result=0;for(size_t i=0;i<text.size;i++){if(text.bytes[i]<'0'||text.bytes[i]>'9')return 0;result=result*10+text.bytes[i]-'0';}
 return result>=1&&result<=100?result:0;
}
inline bool read(HANDLE process,std::uintptr_t at,SmallText& out){SIZE_T n=0;return at&&ReadProcessMemory(process,reinterpret_cast<void*>(at),&out,sizeof out,&n)&&n==sizeof out;}
// Storage-only diagnostic: does not invoke the GUI setter, refresh rendering,
// or prove effective throw power. It is not used by the production runtime.
// Only the verified inline numeric text representation is supported. Never
// allocate in the client or replace an engine-owned pointer/capacity.
inline bool write(HANDLE process,std::uintptr_t at,int expected,int desired){
 if(expected<1||expected>100||desired<1||desired>95)return false;
 SmallText before;if(!read(process,at,before)||value(before)!=expected)return false;
 if(expected==desired)return true;
 auto after=before;const auto digits=std::to_string(desired);std::memcpy(after.bytes.data(),digits.c_str(),digits.size()+1);after.size=digits.size();
 SmallText fresh;if(!read(process,at,fresh)||std::memcmp(&fresh,&before,sizeof before))return false;
 SIZE_T n=0;constexpr size_t prefix=offsetof(SmallText,capacity);
 if(!WriteProcessMemory(process,reinterpret_cast<void*>(at),&after,prefix,&n)||n!=prefix)return false;
 SmallText actual;return read(process,at,actual)&&value(actual)==desired&&actual.capacity==before.capacity;
}
}
