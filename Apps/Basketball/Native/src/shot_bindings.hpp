#pragma once
#include <array>
// The first physical input owns a shot; another held binding cannot steal it.
struct ShotBindingRouter {
 enum Edge { Ignore, Start, Repeat, Release };
 std::array<bool,256> held{};int owner=0;
 Edge observe(int key,bool down,bool allowed){
  if(key<=0||key>=256)return Ignore;
  bool was=held[key];held[key]=down;
  if(!down){if(owner==key){owner=0;return Release;}return Ignore;}
  if(was)return owner==key?Repeat:Ignore;
  if(!allowed||owner&&owner!=key)return Ignore;
  owner=key;return Start;
 }
};
