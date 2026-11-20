#pragma once
// SVG diagrams and text reports for planner decisions.
#include "scenarios.hpp"
#include <map>
#include <ostream>
#include <sstream>

namespace viz {
using namespace planner;

// Boundary of a star-shaped region around `centre` by radial bisection.
template<class Inside>
std::vector<Vec3> regionPolygon(Vec3 centre,double maxRadius,Inside inside,int directions=90){
 std::vector<Vec3> out;
 for(int k=0;k<directions;++k){
  const double a=2*kPi*k/directions;
  const Vec3 dir{std::cos(a),0,std::sin(a)};
  double lo=0,hi=maxRadius;
  if(inside(centre+dir*hi))lo=hi;
  else for(int i=0;i<32;++i){const double mid=.5*(lo+hi);if(inside(centre+dir*mid))lo=mid;else hi=mid;}
  out.push_back(centre+dir*lo);
 }
 return out;
}

// Body positions the receiver can reach for a catch mode at flight time t.
inline std::vector<Vec3> receiverRegion(const Agent& r,const CatchMode& mode,double t){
 const double move=std::fmax(0.,t-mode.timeCost);
 const Vec3 centre=horizontal(r.position)+horizontal(r.velocity)*move;
 const double radius=r.params.maxSpeed*move+horizontalLength(r.velocity)*move+1e-9;
 return regionPolygon(centre,radius,[&](Vec3 q){return innerReach(r,q,move).feasible;});
}
// Body positions a defender may reach by time t (outer bound).
inline std::vector<Vec3> defenderRegion(const Agent& d,double t){
 const Vec3 centre=horizontal(d.position)+horizontal(d.velocity)*t;
 const double radius=std::fmax(d.params.maxSpeed,horizontalLength(d.velocity))*t*2+1e-9;
 return regionPolygon(centre,radius,[&](Vec3 q){return outerReachGap(d,q,t)<=0;});
}

inline const char* verdictColour(Verdict v){
 switch(v){
  case Verdict::PredictedClear:return "#1a9850";
  case Verdict::Uncertain:return "#e6a100";
  case Verdict::Contested:return "#d73027";
  default:return "#9a9a9a";
 }
}
inline std::string escape(const std::string& s){
 std::string o;
 for(char c:s){
  if(c=='&')o+="&amp;";else if(c=='<')o+="&lt;";else if(c=='>')o+="&gt;";else o+=c;
 }
 return o;
}
inline double timeCostFor(const char* mode,const MoverParams& m){return std::string(mode)=="jump"?m.jumpPrepTime:0;}

// Top-down view (x right, z down), side view (distance along the throw vs height)
// and a text panel. `grid` is every evaluated candidate, for context.
inline void writeSvg(std::ostream& o,const std::string& title,const Scene& s,const Decision& d,const std::vector<Evaluation>& grid,const PlannerConfig& cfg){
 const World& w=s.world;
 const double W=1240,H=860;
 const double mapL=20,mapT=50,mapW=780,mapH=520;
 const double sideL=20,sideT=610,sideW=780,sideH=220;
 const Evaluation* sel=d.hasSelection?&d.selected:nullptr;

 std::vector<const Evaluation*> ranked;
 for(const Evaluation& e:grid)if(e.feasible())ranked.push_back(&e);
 std::sort(ranked.begin(),ranked.end(),[](const Evaluation* a,const Evaluation* b){return a->cost<b->cost;});
 if(ranked.size()>14)ranked.resize(14);

 // Bounds.
 double minX=kInf,maxX=-kInf,minZ=kInf,maxZ=-kInf;
 auto include=[&](Vec3 p){minX=std::fmin(minX,p.x);maxX=std::fmax(maxX,p.x);minZ=std::fmin(minZ,p.z);maxZ=std::fmax(maxZ,p.z);};
 include(s.passer);include(s.receiver.position);
 for(const Agent& a:s.defenders)include(a.position);
 for(const Evaluation* e:ranked)include(e->destination);
 if(sel){include(sel->destination);include(sel->landingPoint);}
 const double pad=w.fromMetres(4);
 minX-=pad;maxX+=pad;minZ-=pad;maxZ+=pad;
 const double scale=std::fmin(mapW/(maxX-minX),mapH/(maxZ-minZ));
 auto X=[&](Vec3 p){return mapL+(p.x-minX)*scale;};
 auto Z=[&](Vec3 p){return mapT+(p.z-minZ)*scale;};
 auto pt=[&](Vec3 p){std::ostringstream t;t<<X(p)<<","<<Z(p);return t.str();};
 auto poly=[&](const std::vector<Vec3>& pts,const std::string& attrs){
  o<<"<polygon points=\"";for(const Vec3& p:pts)o<<pt(p)<<" ";o<<"\" "<<attrs<<"/>\n";
 };
 auto line=[&](const std::vector<Vec3>& pts,const std::string& attrs){
  o<<"<polyline fill=\"none\" points=\"";for(const Vec3& p:pts)o<<pt(p)<<" ";o<<"\" "<<attrs<<"/>\n";
 };
 auto text=[&](double x,double y,const std::string& t,const std::string& attrs=""){
  o<<"<text x=\""<<x<<"\" y=\""<<y<<"\" "<<attrs<<">"<<escape(t)<<"</text>\n";
 };
 auto arcPoints=[&](const Evaluation& e,bool afterMiss){
  std::vector<Vec3> p;for(const auto& sm:displayPath(w,e,40,afterMiss))p.push_back(sm.position);return p;
 };

 o<<"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""<<W<<"\" height=\""<<H<<"\" viewBox=\"0 0 "<<W<<" "<<H<<"\" font-family=\"Segoe UI, Helvetica, Arial, sans-serif\" font-size=\"12\">\n";
 o<<"<rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>\n";
 text(mapL,28,title,"font-size=\"18\" font-weight=\"600\"");
 o<<"<defs><clipPath id=\"map\"><rect x=\""<<mapL<<"\" y=\""<<mapT<<"\" width=\""<<mapW<<"\" height=\""<<mapH<<"\"/></clipPath></defs>\n";
 o<<"<rect x=\""<<mapL<<"\" y=\""<<mapT<<"\" width=\""<<mapW<<"\" height=\""<<mapH<<"\" fill=\"#f4f8f1\" stroke=\"#c8d3c0\"/>\n";
 o<<"<g clip-path=\"url(#map)\">\n";
 const double gridStep=w.fromMetres(5);
 for(double x=std::ceil(minX/gridStep)*gridStep;x<=maxX;x+=gridStep){
  const double px=mapL+(x-minX)*scale;
  if(px<=mapL+mapW)o<<"<line x1=\""<<px<<"\" y1=\""<<mapT<<"\" x2=\""<<px<<"\" y2=\""<<mapT+std::fmin(mapH,(maxZ-minZ)*scale)<<"\" stroke=\"#dde6d6\"/>\n";
 }
 text(mapL+6,mapT+16,"top-down: x downfield (right), z across (down); grid 5 m","fill=\"#667\"");

 // Every candidate destination, coloured by verdict.
 for(const Evaluation& e:grid){
  if(e.destination.x<minX||e.destination.x>maxX||e.destination.z<minZ||e.destination.z>maxZ)continue;
  o<<"<circle cx=\""<<X(e.destination)<<"\" cy=\""<<Z(e.destination)<<"\" r=\""<<(e.feasible()?2.2:1.2)
   <<"\" fill=\""<<verdictColour(e.verdict)<<"\" fill-opacity=\""<<(e.feasible()?.55:.25)<<"\"/>\n";
 }
 // Receiver: route, reach region at the selected catch time, position and heading.
 if(!s.receiverRoute.empty()){
  std::vector<Vec3> r{s.receiver.position};
  for(Vec3 p:s.receiverRoute)r.push_back(p);
  line(r,"stroke=\"#3060c0\" stroke-dasharray=\"5,4\" stroke-width=\"1.2\"");
 }
 if(sel){
  const CatchMode mode=catchModes(s.receiver.params)[sel->candidate.mode];
  poly(receiverRegion(s.receiver,mode,sel->candidate.flightTime),"fill=\"#3060c0\" fill-opacity=\"0.10\" stroke=\"#3060c0\" stroke-opacity=\"0.6\"");
  line({s.receiver.position,sel->receiverPlan.body},"stroke=\"#3060c0\" stroke-width=\"1\" stroke-dasharray=\"2,2\"");
 }
 // Defenders: reach region at their most dangerous moment, and interceptions.
 if(sel){
  for(size_t i=0;i<s.defenders.size();++i){
   const Agent& a=s.defenders[i];
   const Threat& th=sel->threats[2*i];
   if(!std::isfinite(th.minSigmas))continue;
   const double t=th.intercept?th.interceptTime:th.time;
   const char* mode=th.mode;
   poly(defenderRegion(a,std::fmax(0.,t-timeCostFor(mode,a.params))),"fill=\"#d73027\" fill-opacity=\"0.07\" stroke=\"#d73027\" stroke-opacity=\"0.5\" stroke-dasharray=\"4,3\"");
   const Vec3 ball=th.intercept?th.interceptPoint:th.ball;
   line({a.position,ball},"stroke=\"#d73027\" stroke-width=\"0.8\" stroke-dasharray=\"2,3\"");
  }
 }
 // Candidate arcs and the selected arc (the same samples the planner validated).
 for(auto it=ranked.rbegin();it!=ranked.rend();++it)
  line(arcPoints(**it,false),std::string("stroke=\"")+verdictColour((*it)->verdict)+"\" stroke-opacity=\"0.45\" stroke-width=\"1\"");
 if(sel){
  line(arcPoints(*sel,true),"stroke=\"#202020\" stroke-width=\"1.2\" stroke-dasharray=\"3,3\"");
  line(arcPoints(*sel,false),"stroke=\"#1040a0\" stroke-width=\"3\"");
  o<<"<circle cx=\""<<X(sel->destination)<<"\" cy=\""<<Z(sel->destination)<<"\" r=\"6\" fill=\"none\" stroke=\"#1040a0\" stroke-width=\"2.5\"/>\n";
 }
 // Threat markers on top: red X / ring = earliest reach / closest moment before the
 // catch, orange X = reach after a miss. Short tags here; details in the panel.
 const std::string halo=" paint-order=\"stroke\" stroke=\"#ffffff\" stroke-width=\"3\"";
 auto cross=[&](Vec3 p,double r,const char* colour,double width){
  const double x=X(p),y=Z(p);
  o<<"<path d=\"M"<<x-r<<","<<y-r<<" L"<<x+r<<","<<y+r<<" M"<<x-r<<","<<y+r<<" L"<<x+r<<","<<y-r<<"\" stroke=\""<<colour<<"\" stroke-width=\""<<width<<"\"/>\n";
 };
 if(sel){
  for(size_t i=0;i<s.defenders.size();++i){
   const Threat& th=sel->threats[2*i];
   const Threat& miss=sel->threats[2*i+1];
   const double stagger=13*static_cast<double>(i);
   if(std::isfinite(th.minSigmas)){
    const Vec3 ball=th.intercept?th.interceptPoint:th.ball;
    if(th.intercept)cross(ball,6,"#d73027",2.5);
    else o<<"<circle cx=\""<<X(ball)<<"\" cy=\""<<Z(ball)<<"\" r=\"3\" fill=\"none\" stroke=\"#d73027\"/>\n";
    text(X(ball)-8,Z(ball)-8-stagger,s.defenders[i].name+format(th.intercept?" %.2fs":" %.1f\xCF\x83",th.intercept?th.interceptTime:th.minSigmas),
         "fill=\"#a02020\" font-size=\"11\" text-anchor=\"end\""+halo);
   }
   if(miss.intercept){
    cross(miss.interceptPoint,5,"#e08000",2);
    text(X(miss.interceptPoint)+8,Z(miss.interceptPoint)+14+stagger,s.defenders[i].name+format(" miss %.2fs",miss.interceptTime),"fill=\"#a05000\" font-size=\"11\""+halo);
   }
  }
  text(X(sel->destination)+9,Z(sel->destination)-8,"destination","fill=\"#1040a0\" font-weight=\"600\""+halo);
 }
 auto agentMark=[&](const Agent& a,const char* colour){
  o<<"<circle cx=\""<<X(a.position)<<"\" cy=\""<<Z(a.position)<<"\" r=\"6\" fill=\""<<colour<<"\"/>\n";
  const Vec3 tip=a.position+horizontal(a.velocity)*.5;
  o<<"<line x1=\""<<X(a.position)<<"\" y1=\""<<Z(a.position)<<"\" x2=\""<<X(tip)<<"\" y2=\""<<Z(tip)<<"\" stroke=\""<<colour<<"\" stroke-width=\"2\"/>\n";
  text(X(a.position)-8,Z(a.position)-10,a.name,std::string("fill=\"")+colour+"\" font-weight=\"600\"");
 };
 agentMark(s.receiver,"#1f4fbf");
 for(const Agent& a:s.defenders)agentMark(a,"#b01818");
 o<<"<rect x=\""<<X(s.passer)-6<<"\" y=\""<<Z(s.passer)-6<<"\" width=\"12\" height=\"12\" fill=\"#222\"/>\n";
 text(X(s.passer)-10,Z(s.passer)+22,"QB","font-weight=\"600\"");
 o<<"</g>\n";

 // Side view.
 o<<"<rect x=\""<<sideL<<"\" y=\""<<sideT<<"\" width=\""<<sideW<<"\" height=\""<<sideH<<"\" fill=\"#f7f7fb\" stroke=\"#ccd\"/>\n";
 text(sideL+6,sideT+16,"side view: distance from release along the throw vs height; bars = defender reach height when the ball passes","fill=\"#667\"");
 if(sel){
  const Vec3 release=sel->launch.position;
  double maxD=0,maxH=0;
  std::vector<std::vector<Vec3>> arcs;
  for(const Evaluation* e:ranked)arcs.push_back(arcPoints(*e,false));
  arcs.push_back(arcPoints(*sel,true));
  for(const auto& a:arcs)for(Vec3 p:a){maxD=std::fmax(maxD,horizontalLength(p-release));maxH=std::fmax(maxH,p.y-w.groundY);}
  for(const Agent& a:s.defenders)maxH=std::fmax(maxH,a.params.jumpReachHeight);
  maxD*=1.05;maxH*=1.15;
  auto SX=[&](Vec3 p){return sideL+10+horizontalLength(p-release)/maxD*(sideW-20);};
  auto SY=[&](double h){return sideT+sideH-10-h/maxH*(sideH-30);};
  auto sideLine=[&](const std::vector<Vec3>& pts,const std::string& attrs){
   o<<"<polyline fill=\"none\" points=\"";for(Vec3 p:pts)o<<SX(p)<<","<<SY(p.y-w.groundY)<<" ";o<<"\" "<<attrs<<"/>\n";
  };
  o<<"<line x1=\""<<sideL<<"\" y1=\""<<SY(0)<<"\" x2=\""<<sideL+sideW<<"\" y2=\""<<SY(0)<<"\" stroke=\"#888\"/>\n";
  for(size_t i=0;i<ranked.size();++i)sideLine(arcs[i],std::string("stroke=\"")+verdictColour(ranked[i]->verdict)+"\" stroke-opacity=\"0.45\"");
  for(size_t i=0;i<s.defenders.size();++i){
   const Threat& th=sel->threats[2*i];
   if(!std::isfinite(th.minSigmas))continue;
   const Vec3 ball=th.intercept?th.interceptPoint:th.ball;
   const double x=SX(ball);
   o<<"<line x1=\""<<x<<"\" y1=\""<<SY(0)<<"\" x2=\""<<x<<"\" y2=\""<<SY(s.defenders[i].params.jumpReachHeight)<<"\" stroke=\"#d73027\" stroke-width=\"5\" stroke-opacity=\"0.35\"/>\n";
   text(x+4,SY(s.defenders[i].params.jumpReachHeight)-3,s.defenders[i].name,"fill=\"#a02020\" font-size=\"11\"");
   if(th.intercept){
    const double y=SY(ball.y-w.groundY);
    o<<"<path d=\"M"<<x-5<<","<<y-5<<" L"<<x+5<<","<<y+5<<" M"<<x-5<<","<<y+5<<" L"<<x+5<<","<<y-5<<"\" stroke=\"#d73027\" stroke-width=\"2.5\"/>\n";
   }
  }
  sideLine(arcPoints(*sel,true),"stroke=\"#202020\" stroke-dasharray=\"3,3\"");
  sideLine(arcPoints(*sel,false),"stroke=\"#1040a0\" stroke-width=\"3\"");
 }

 // Text panel.
 double y=mapT+8;
 const double tx=mapL+mapW+20;
 auto say=[&](const std::string& t,const std::string& attrs=""){text(tx,y,t,attrs);y+=17;};
 auto wrap=[&](const std::string& t,const std::string& attrs=""){
  std::string lineText;std::istringstream in(t);std::string word;
  while(in>>word){if(lineText.size()+word.size()>58){say(lineText,attrs);lineText.clear();}lineText+=(lineText.empty()?"":" ")+word;}
  if(!lineText.empty())say(lineText,attrs);
 };
 if(sel){
  say("Selected: "+sel->label,"font-size=\"15\" font-weight=\"600\"");
  say(std::string("Verdict: ")+toString(sel->verdict),std::string("fill=\"")+verdictColour(sel->verdict)+"\" font-size=\"14\" font-weight=\"600\"");
  wrap(sel->why);
  y+=4;
  say(format("flight %.2f s   speed %.1f units/s   elevation %.1f deg",sel->candidate.flightTime,sel->speed,sel->elevationDeg));
  say(format("depth %+.2f   lateral %+.2f   catch height %.2f",sel->candidate.depth,sel->candidate.lateral,sel->destination.y-w.groundY));
  say(format("risk before catch %.3f   after miss %.3f",sel->preCatchRisk,sel->postMissRisk));
  say(format("receiver effort %.0f%%   robust to %.1f sigma",100*sel->receiverPlan.effort,sel->receiverMarginSigmas));
  y+=4;
  say("Why selected:","font-weight=\"600\"");
  wrap(d.reason);
  y+=4;
  say("Threats (before catch | after miss):","font-weight=\"600\"");
  for(size_t i=0;i<s.defenders.size();++i){
   const Threat& b=sel->threats[2*i];
   const Threat& a=sel->threats[2*i+1];
   auto describe=[&](const Threat& t){
    if(!std::isfinite(t.minSigmas))return std::string("never in reach");
    return t.intercept?format("REACH t=%.2fs P=%.2f",t.interceptTime,t.probability):format("%.1f sigma P=%.3f",t.minSigmas,t.probability);
   };
   wrap(s.defenders[i].name+": "+describe(b)+" | "+describe(a));
  }
 }else say("No feasible selection","font-size=\"15\" font-weight=\"600\" fill=\"#d73027\"");
 y+=6;
 std::map<std::string,int> counts;
 for(const Evaluation& e:grid)++counts[e.feasible()?std::string(toString(e.verdict)):std::string("rejected: ")+toString(e.reject)];
 say("All "+std::to_string(grid.size())+" grid candidates:","font-weight=\"600\"");
 for(const auto& c:counts)say("  "+c.first+": "+std::to_string(c.second));
 y+=6;
 say("Legend","font-weight=\"600\"");
 for(Verdict v:{Verdict::PredictedClear,Verdict::Uncertain,Verdict::Contested,Verdict::Invalid}){
  o<<"<circle cx=\""<<tx+5<<"\" cy=\""<<y-4<<"\" r=\"5\" fill=\""<<verdictColour(v)<<"\"/>\n";
  text(tx+16,y,toString(v));y+=16;
 }
 say("blue arc: selected (validated)  dashed: after a miss");
 say("blue area: receiver body reach at catch time");
 say("red dashed areas: defender reach (outer bound) at the");
 say("  marked moment; red X = earliest interception");
 say("orange X = interception if the receiver misses");
 (void)cfg;
 o<<"</svg>\n";
}

inline void writeReport(std::ostream& o,const scenarios::Spec& spec,const scenarios::RunResult& r,const std::vector<Evaluation>& grid,const PlannerConfig& cfg){
 const Decision& d=r.throwDecision;
 const World& w=spec.world;
 o<<"Scenario: "<<spec.name<<"\n"<<spec.description<<"\n";
 o<<format("Units: %.4f m per unit. Throw at t=%.2f s. Observation noise %.3f units, %.3f units/s.",w.metresPerUnit,spec.snapshotTime,spec.positionNoise,spec.velocityNoise)<<"\n\n";
 o<<"Assumptions behind every risk number:\n";
 for(const std::string& a:riskAssumptions(cfg))o<<"  - "<<a<<"\n";
 o<<"\n";
 if(d.hasSelection){
  const Evaluation& e=d.selected;
  o<<"SELECTED: "<<e.label<<"  ["<<toString(e.verdict)<<"]\n";
  o<<"  why verdict:  "<<e.why<<"\n";
  o<<"  why selected: "<<d.reason<<"\n";
  o<<format("  cost %.3f = ",e.cost)<<e.costTerms.describe()<<"\n";
  o<<format("  flight %.3f s, speed %.3f units/s, elevation %.2f deg, depth %+.2f, lateral %+.2f",e.candidate.flightTime,e.speed,e.elevationDeg,e.candidate.depth,e.candidate.lateral)<<"\n";
  o<<format("  destination (%.3f, %.3f, %.3f), lands at t=%.3f s",e.destination.x,e.destination.y,e.destination.z,e.landingTime)<<"\n";
  o<<format("  receiver plan effort %.0f%%, robust to %.1f sigma of receiver error",100*e.receiverPlan.effort,e.receiverMarginSigmas)<<"\n";
  o<<format("  risk before catch %.4f, after a miss %.4f; validated in update %.0f",e.preCatchRisk,e.postMissRisk,static_cast<double>(e.validatedUpdate))<<"\n";
  o<<"  threats:\n";
  for(const Threat& t:e.threats){
   o<<"    "<<t.defender<<" ("<<toString(t.phase)<<"): ";
   if(!std::isfinite(t.minSigmas)){o<<"ball never within this defender's height range\n";continue;}
   o<<format("gap %.3f units = %.2f sigma (sigma %.3f) at t=%.3f s",t.gap,t.minSigmas,t.sigma,t.time)
    <<format(", ball height %.2f, mode ",t.ball.y-w.groundY)<<t.mode<<format(", P(reach)=%.4f",t.probability);
   if(t.intercept)o<<format("; FIRST REACH t=%.4f s at (%.2f, %.2f, %.2f)",t.interceptTime,t.interceptPoint.x,t.interceptPoint.y,t.interceptPoint.z);
   o<<"\n";
  }
 }else o<<"NO SELECTION: "<<d.reason<<"\n";
 o<<"\nThrow played out against the scripted truth: "<<r.outcome.result;
 if(!r.outcome.by.empty())o<<" ("<<r.outcome.by<<format(" at t=%.3f s)",r.outcome.time);
 if(r.outcome.result!="not thrown")o<<format("; receiver distance outside catch reach at T: %.3f",r.outcome.receiverMiss);
 o<<"\n\n";

 std::vector<const Evaluation*> ranked;
 for(const Evaluation& e:grid)if(e.feasible())ranked.push_back(&e);
 std::sort(ranked.begin(),ranked.end(),[](const Evaluation* a,const Evaluation* b){return a->cost<b->cost;});
 o<<"Best 12 grid candidates by cost (full search at throw time):\n";
 for(size_t i=0;i<ranked.size()&&i<12;++i){
  const Evaluation& e=*ranked[i];
  o<<format("  %6.3f  T=%.2f v=%5.2f el=%5.1f  ",e.cost,e.candidate.flightTime,e.speed,e.elevationDeg)<<toString(e.verdict)<<"  "<<e.label<<"  -- "<<e.why<<"\n";
  o<<"          "<<e.costTerms.describe()<<"\n";
 }
 std::map<std::string,int> counts;
 std::map<std::string,std::string> example;
 for(const Evaluation& e:grid){
  const std::string key=e.feasible()?std::string(toString(e.verdict)):std::string("rejected: ")+toString(e.reject);
  ++counts[key];
  if(!example.count(key))example[key]=e.label+format(" T=%.2f: ",e.candidate.flightTime)+e.why;
 }
 o<<"\nAll "<<grid.size()<<" grid candidates by outcome (with one example each):\n";
 for(const auto& c:counts)o<<format("  %5.0f  ",c.second)<<c.first<<"   e.g. "<<example[c.first]<<"\n";

 int switches=0,verdictChanges=0,blendFrames=0;
 double maxJump=0;
 for(size_t i=1;i<r.frames.size();++i){
  const auto& a=r.frames[i-1];const auto& b=r.frames[i];
  if(b.blending)++blendFrames;
  if(a.has&&b.has){maxJump=std::fmax(maxJump,horizontalLength(b.destination-a.destination));if(a.verdict!=b.verdict)++verdictChanges;}
 }
 if(!r.frames.empty())switches=r.frames.back().switches;
 o<<"\nFrame-to-frame ("<<r.frames.size()<<" updates at 60 Hz): "<<switches<<" selection switches, "<<blendFrames<<" blend steps, "
  <<verdictChanges<<" verdict changes"<<format(", largest destination jump %.2f units",maxJump)
  <<format(", update time mean %.3f ms / max %.3f ms",r.meanMs,r.maxMs)<<"\n";
}

}
