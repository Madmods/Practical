#pragma once
#include <string>
#include <algorithm>
// Local preview sign-in. Replace this validator with real authentication later.
// No credentials are persisted or sent over the network.
struct LaunchFlow {
 enum Stage { Intro, Login, Verifying, Connect, Entering, Ready };
 Stage stage=Intro,previous=Intro;float age=0,previousAge=0;bool invalid=false;
 void move(Stage next){previous=stage;previousAge=age;stage=next;age=0;}
 bool submit(const std::wstring& user,const std::wstring& password){
  if(stage!=Login)return false;
  invalid=user!=L"1"||password!=L"1";
  if(invalid)return false;move(Verifying);return true;
 }
 bool proceed(bool connected){if(stage!=Connect||!connected)return false;move(Entering);return true;}
 void tick(float dt,bool connected,bool reduced){
  age+=std::max(0.f,dt);
  if(stage==Intro&&age>=(reduced?.1f:2.6f))move(Login);
  else if(stage==Verifying&&age>=(reduced?.05f:.45f))move(Connect);
  else if(stage==Entering){if(!connected)move(Connect);else if(age>=(reduced?.05f:.5f))move(Ready);}
 }
};
