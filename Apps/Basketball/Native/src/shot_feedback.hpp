#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cctype>
#include <string>
namespace shotFeedback {
enum class Grade {Unknown,Early,Perfect,Late,Good};
enum class Phase {Unknown,Rising,Falling};
inline const char* phaseName(Phase p){return p==Phase::Rising?"rising":p==Phase::Falling?"falling":"unknown";}
// Direction immediately before the meter froze, observed AFTER key-up. A
// final percentage by itself cannot distinguish an early Good from a late one.
class Motion {
 double lastAt=-1,lastValue=0,travel=0;int direction=0,changes=0;bool bad=false;
public:
 void reset(){*this=Motion{};}
 void invalidate(){bad=true;}
 void add(double t,double value){
  if(bad)return;
  if(!std::isfinite(t)||!std::isfinite(value)||value<0||value>1.05||(lastAt>=0&&(t<=lastAt||t-lastAt>.06))){bad=true;return;}
  if(lastAt<0){lastAt=t;lastValue=value;return;}
  lastAt=t;const double delta=value-lastValue;if(std::abs(delta)<.002)return;
  const int next=delta>0?1:-1;
  if(next!=direction){direction=next;changes=0;travel=0;}
  ++changes;travel+=std::abs(delta);lastValue=value;
 }
 Phase phase(double finalValue)const{
  if(bad||lastAt<0||!std::isfinite(finalValue)||std::abs(finalValue-lastValue)>.003||changes<(direction<0?2:3)||travel<(direction<0?.015:.025))return Phase::Unknown;
  return direction>0?Phase::Rising:Phase::Falling;
 }
};
inline Grade parseGrade(std::string s){
 std::string normalized;bool space=false;
 for(unsigned char c:s){if(std::isspace(c)){space=!normalized.empty();continue;}if(space){normalized+=' ';space=false;}normalized+=(char)std::tolower(c);}
 if(normalized=="perfect release")return Grade::Perfect;
 if(normalized=="good release")return Grade::Good;
 if(normalized=="early release"||normalized=="slightly early release"||normalized=="very early release")return Grade::Early;
 if(normalized=="late release"||normalized=="slightly late release"||normalized=="very late release")return Grade::Late;
 return Grade::Unknown;
}
inline double parseContest(const std::string& s){
 const auto at=s.find("% Contested");if(at==std::string::npos||at+11!=s.size()||at==0||at>6)return NAN;
 double n=0;bool dot=false;double place=.1;
 for(size_t i=0;i<at;i++){char c=s[i];if(c=='.'&&!dot){dot=true;continue;}if(c<'0'||c>'9')return NAN;if(dot){n+=(c-'0')*place;place*=.1;}else n=n*10+c-'0';}
 return s[0]>='0'&&s[0]<='9'&&s[at-1]>='0'&&s[at-1]<='9'&&n>=0&&n<=100?n:NAN;
}
struct Frame{std::uintptr_t owner=0,object=0;bool valid=false;double alpha=1,meter=NAN,contest=NAN;Grade grade=Grade::Unknown;};
struct Shot{unsigned id=0,press=0;std::uintptr_t owner=0;double started=0,sent=0;};
struct Result{unsigned id=0;Grade grade=Grade::Unknown;double at=0,contest=NAN,meter=NAN;Phase phase=Phase::Unknown;};
class Tracker {
 std::uintptr_t owner=0,object=0;
 double hiddenAt=-1,candidateAt=-1,lastAt=-1;
 unsigned seen=0,finished=0;
 Frame candidate;
 Motion motion;bool motionFrozen=false;
public:
 void reset(){*this=Tracker{};}
 Result observe(const Frame& f,const Shot& shot,unsigned currentPress,double t){
  if(!std::isfinite(t)||!f.valid||!f.owner||!f.object||!std::isfinite(f.alpha)||f.alpha<0||f.alpha>1){hiddenAt=candidateAt=lastAt=-1;motion.invalidate();return {};}
  if(f.owner!=owner||f.object!=object||t<lastAt||t-lastAt>.25){const auto done=finished;reset();finished=done;owner=f.owner;object=f.object;}
  lastAt=t;
  if(f.alpha>=.98)hiddenAt=t;
  if(shot.id!=seen){seen=shot.id;candidateAt=-1;motion.reset();motionFrozen=false;if(t-shot.sent>.06)motion.invalidate();}
  if(shot.id&&shot.owner==f.owner&&shot.press==currentPress&&t>=shot.sent&&t-shot.sent<=3&&hiddenAt>=shot.started&&!motionFrozen){
   motion.add(t,f.meter);if(f.alpha<.98)motionFrozen=true;
  }
  if(!shot.id||shot.id<=finished||shot.owner!=f.owner||shot.press!=currentPress||
     !std::isfinite(shot.sent)||!std::isfinite(shot.started)||shot.started>shot.sent||t<shot.sent||t-shot.sent>3||
     hiddenAt<shot.started||f.alpha>.9||f.grade==Grade::Unknown||!std::isfinite(f.meter)||f.meter<0||f.meter>1.05||
     !std::isfinite(f.contest)||f.contest<0||f.contest>100){candidateAt=-1;return {};}
  if(candidateAt<0||candidate.grade!=f.grade||std::abs(candidate.meter-f.meter)>.002||std::abs(candidate.contest-f.contest)>.01){candidate=f;candidateAt=t;return {};}
  if(t-candidateAt<.02)return {};
  finished=shot.id;return {shot.id,f.grade,t,f.contest,f.meter,motion.phase(f.meter)};
 }
};
}
