#define NOMINMAX
#include "../src/engine.hpp"
#include <array>
#include <iostream>

struct InstanceFixture {
 std::array<unsigned char,256> object{},descriptor{};
 std::string name,kind;
 InstanceFixture(std::string n,std::string k):name(n),kind(k){
  put(object,Offsets::Instance::NameContainer,(addr)&name);
  put(object,Offsets::Instance::ClassDescriptor,(addr)descriptor.data());
  put(descriptor,Offsets::Instance::ClassName,(addr)&kind);
 }
 static void put(std::array<unsigned char,256>& bytes,size_t offset,addr value){memcpy(bytes.data()+offset,&value,sizeof value);}
 addr address(){return (addr)object.data();}
};
int main(){
 InstanceFixture root("game","DataModel"),folder("folder","Model"),workspace("renamed-world","Workspace"),players("renamed-roster","Players"),part("Football","Part");
 std::array<addr,6> entries{workspace.address(),0,players.address(),0,part.address(),0};
 std::array<addr,2> range{(addr)entries.data(),(addr)(entries.data()+entries.size())};
 InstanceFixture::put(root.object,Offsets::Instance::ChildrenStart,(addr)range.data());
 InstanceFixture::put(folder.object,Offsets::Instance::ChildrenStart,(addr)range.data());
 HANDLE h=GetCurrentProcess();int failures=0,checks=0;
 auto check=[&](bool ok,const char* name){++checks;if(!ok){++failures;std::cerr<<"FAIL "<<name<<'\n';}};
 check(child(h,root.address(),"Workspace")==workspace.address(),"renamed Workspace resolves by class");
 check(child(h,root.address(),"Players")==players.address(),"renamed Players resolves by class");
 check(child(h,folder.address(),"Workspace")==0,"nested objects do not gain service fallback");
 check(child(h,root.address(),"Football")==part.address(),"normal name lookup preserved");
 check(child(h,root.address(),"Part")==0,"arbitrary classes do not gain fallback");
 check(child(h,root.address(),"Missing")==0,"missing child remains missing");
 check(!instanceClassIs(h,0,"DataModel"),"invalid instance rejected");
 std::cout<<checks<<" service lookup checks, "<<failures<<" failures\n";return failures?1:0;
}
