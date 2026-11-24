#pragma once
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstdint>
namespace ff3qb {
enum class DiscoveryKind {Skip,Container,Ball};
struct DiscoveryInfo {DiscoveryKind kind=DiscoveryKind::Skip;bool priority=false;};
// Round-robin traversal keeps large scenery folders from starving later ball
// containers. Known balls survive partial sweeps; live validation removes them.
struct BallDiscovery {
 using Id=std::uint64_t;struct Work{Id id=0;unsigned depth=0;std::vector<Id> children;size_t cursor=0;bool loaded=false;};
 Id root=0;std::deque<Work> work;std::unordered_set<Id> seen,known;std::vector<Id> balls,fastRoots;std::unordered_map<Id,size_t> fastCursor;double nextSweep=0,nextFast=0;size_t visited=0;bool overflow=false;
 void reset(Id id=0){root=id;work.clear();seen.clear();known.clear();balls.clear();fastRoots.clear();fastCursor.clear();nextSweep=nextFast=0;visited=0;overflow=false;if(id)fastRoots.push_back(id);}
 void watch(Id id){if(id&&fastRoots.size()<24&&std::find(fastRoots.begin(),fastRoots.end(),id)==fastRoots.end())fastRoots.push_back(id);}
 void remember(Id id){if(known.count(id))return;if(balls.size()>=1024){overflow=true;return;}known.insert(id);balls.push_back(id);}
 void forget(Id id){known.erase(id);balls.erase(std::remove(balls.begin(),balls.end(),id),balls.end());}
 template<class Children,class Inspect> void poll(Id id,double time,Children children,Inspect inspect,size_t budget=64){
  if(root!=id)reset(id);if(!id)return;
  if(time>=nextFast){const auto roots=fastRoots;for(auto parent:roots){auto items=children(parent);if(items.empty())continue;auto& cursor=fastCursor[parent];for(size_t i=0,n=std::min<size_t>(96,items.size());i<n;++i){const auto item=items[(cursor+i)%items.size()];auto info=inspect(item);if(info.kind==DiscoveryKind::Ball)remember(item);else if(info.kind==DiscoveryKind::Container&&info.priority)watch(item);}cursor=(cursor+std::min<size_t>(96,items.size()))%items.size();}nextFast=time+.075;}
  if(work.empty()&&time>=nextSweep){seen.clear();seen.insert(id);work.push_back({id,0});visited=0;nextSweep=time+.35;}
  for(size_t used=0;used<budget&&!work.empty();++used){auto node=std::move(work.front());work.pop_front();if(!node.loaded){node.children=children(node.id);node.loaded=true;}
   if(node.cursor>=node.children.size())continue;auto item=node.children[node.cursor++];auto info=inspect(item);++visited;
   if(info.kind==DiscoveryKind::Ball)remember(item);
   else if(info.kind==DiscoveryKind::Container&&node.depth<16&&seen.size()<65536&&seen.insert(item).second){Work child{item,node.depth+1};if(info.priority){watch(item);work.push_front(std::move(child));}else work.push_back(std::move(child));}
   if(node.cursor<node.children.size())work.push_back(std::move(node));
  }
 }
};
}
