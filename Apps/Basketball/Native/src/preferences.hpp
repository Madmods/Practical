#pragma once
static void loadGameplay(const fs::path& source){
 {std::ifstream f(source);std::string s((std::istreambuf_iterator<char>(f)),{});auto get=[&](const char*k,int d){std::smatch m;if(std::regex_search(s,m,std::regex(std::string("\"")+k+"\"\\s*:\\s*(-?[0-9]+)")))try{return std::stoi(m[1]);}catch(...){}return d;};
 trim=std::clamp(get("earlier_ms",40),-30,60);minRelease=std::clamp(get("minimum_percent",80),1,100);maxRelease=std::clamp(get("maximum_percent",100),minRelease.load(),100);maxPing=std::clamp(get("max_ping",1000),50,1000);freshnessMs=std::clamp(get("freshness_ms",120),60,200);holdEarly=get("hold_early",1)!=0;protectFall=get("protect_fall",1)!=0;contestLimit=std::clamp(get("contest_limit",30),5,100);closeoutOn=get("closeout",0)!=0;overholdLeadMs=std::clamp(get("overhold_lead_ms",40),0,120);pumpRange=std::clamp(get("pump_range",12),4,16);pumpDelay=std::clamp(get("pump_delay",120),60,300);pumpContest=std::clamp(get("pump_contest",10),0,30);pumpVisuals=get("pump_visuals",1)!=0;ballArcOn=get("ball_arc",1)!=0;ballLandingOn=get("ball_landing",1)!=0;rollingOn=get("rolling_intercept",1)!=0;
 defense::loadSettings(get);
 int k=get("shoot_key",'E');shootKey=(k==VK_MBUTTON||k==VK_XBUTTON1||k==VK_XBUTTON2||k>=8&&k<255)&&k!=VK_F8&&k!=VK_ESCAPE?k:'E';
 int alt=get("shoot_key_alt",VK_SPACE);shootKeyAlt=(alt==0||(alt>=8&&alt<255||alt==VK_MBUTTON||alt==VK_XBUTTON1||alt==VK_XBUTTON2)&&alt!=VK_F8&&alt!=VK_ESCAPE&&alt!=VK_LWIN&&alt!=VK_RWIN)&&alt!=shootKey.load()?alt:0;}
}
static void loadAppearance(const fs::path& source){
 {std::ifstream f(source);unsigned a,b,c;if(f>>a>>b>>c&&a<=0xffffff&&b<=0xffffff&&c<=0xffffff){theme[0]=a;theme[1]=b;theme[2]=c;int n;if(f>>n)railOpacity=std::clamp(n,0,100);if(f>>n)windowOpacity=std::clamp(n,0,100);if(f>>n)uiScale=std::clamp(n,80,120);if(f>>n)showTooltips=n!=0;if(f>>n)reducedMotion=n!=0;if(f>>n)alwaysOnTop=n!=0;if(f>>n)compactBar=n!=0;if(f>>n)lockWindow=n!=0;if(f>>n)pinned=n!=0;windowAlpha=windowOpacity*.01f;}}
}
