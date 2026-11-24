#pragma once
#include "ff3_packet_bridge.hpp"
#include <condition_variable>

namespace ff3Packets {
// Preparation belongs to the application lifetime, not the UI/workspace or QB
// toggle. The shared reader still supplies the version-checked process identity.
class StartupPreparation {
 Service& service_;EarlyListener& listener_;
 std::function<void()> discover_,stopDiscovery_;
 std::function<gameModules::Snapshot()> connection_;
 std::function<bool()> networkingWanted_;
 std::thread reader_,worker_;std::atomic<bool> stopping_{false};
 std::mutex waitMutex_;std::condition_variable wake_;
public:
 StartupPreparation(Service& service,EarlyListener& listener,std::function<void()> discover,
                    std::function<void()> stopDiscovery,std::function<gameModules::Snapshot()> connection,std::function<bool()> networkingWanted=[](){return true;})
  :service_(service),listener_(listener),discover_(std::move(discover)),stopDiscovery_(std::move(stopDiscovery)),connection_(std::move(connection)),networkingWanted_(std::move(networkingWanted)){}
 StartupPreparation(const StartupPreparation&)=delete;
 StartupPreparation& operator=(const StartupPreparation&)=delete;
 bool start(){
  if(reader_.joinable()||worker_.joinable())return true;
  stopping_=false;
  try{
   reader_=std::thread(discover_);
   worker_=std::thread([this](){bool networkStarted=false;
    while(!stopping_){
     const bool wanted=networkingWanted_();
     if(wanted){
      listener_.ensure();service_.joinBuffer(listener_.folder(),listener_.ready());
      const auto connection=connection_();
      if(autoPrepare(connection,false))service_.prepare(connection.pid);
      else if(knownOtherGame(connection))service_.stop();
     }else if(networkStarted){service_.stop();listener_.stop();service_.joinBuffer({},false);}
     networkStarted=wanted;
     service_.poll();
     std::unique_lock<std::mutex> lock(waitMutex_);
     wake_.wait_for(lock,std::chrono::milliseconds(100),[this](){return stopping_.load();});
    }
   });
   return true;
  }catch(...){stop();return false;}
 }
 void stop(){
  if(!reader_.joinable()&&!worker_.joinable())return;
  stopping_=true;wake_.notify_all();stopDiscovery_();
  if(reader_.joinable())reader_.join();
  if(worker_.joinable())worker_.join();
 }
 ~StartupPreparation(){stop();}
};
}
