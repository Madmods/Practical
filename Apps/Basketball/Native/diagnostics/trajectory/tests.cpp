// Regression tests for projectile.hpp.
#include "check.hpp"
#include "projectile.hpp"
#include <sstream>
#include <string>
using namespace projectile;

namespace {
World withGravity(double g,double groundY=0){World w;w.gravityMps2=g;w.groundY=groundY;return w;}

void integration(){
 const World w=withGravity(9.81);
 const State initial{{0,10,0},{4,8,2}};
 const State s=advance(initial,w,.5);
 CHECK(check::near(s.position.x,2,2)&&check::near(s.position.y,12.77375,13)&&check::near(s.velocity.y,3.095,8));
 CHECK(check::near(s.position.z,1,1));
 const State one=advance(initial,w,1),two=advance(s,w,.5);
 CHECK(check::near(one.position.y,two.position.y,20)&&check::near(one.velocity.y,two.velocity.y,20));
 // Every component of the acceleration is applied, not only y.
 const State side=advance({{1,2,3},{0,0,0}},Vec3{2,-4,6},3);
 CHECK(check::near(side.position.x,10,10)&&check::near(side.position.y,-16,20)&&check::near(side.position.z,30,30));
 CHECK(check::near(side.velocity.x,6,6)&&check::near(side.velocity.y,-12,12)&&check::near(side.velocity.z,18,18));
}

// Regression: t*t used to overflow first, so zero acceleration with a huge duration
// produced 0*inf = NaN and a spurious overflow_error.
void overflowOrdering(){
 const State rest{{1,2,3},{0,0,0}};
 const State far=advance(rest,Vec3{0,0,0},1e200);
 CHECK(far.position.x==1&&far.position.y==2&&far.position.z==3);
 const State drift=advance({{0,0,0},{1e-150,0,0}},Vec3{1e-300,0,0},1e200);
 CHECK(std::isfinite(drift.position.x)&&check::relNear(drift.position.x,1e50+.5e100,1e-12));
 CHECK_THROWS(std::overflow_error,advance({{0,0,0},{0,0,0}},Vec3{0,-9.81,0},1e200));
}

void contactCases(){
 int cases=0;
 for(double metresPerUnit:{1.,.9144})
 for(double groundY:{0.,-3.,250.})
 for(double height:{0.,1e-3,1.,100.,1e6})
 for(double vertical:{-1e4,-100.,-1.,0.,1.,100.,1e4})
 for(double g:{1.,9.81,100.}){
  World w=withGravity(g,groundY);
  w.metresPerUnit=metresPerUnit;
  const State start{{3,groundY+height,-2},{3,vertical,1}};
  const Contact c=groundContact(start,w);
  if(height==0&&vertical<=0){CHECK(c.kind==ContactKind::OnPlane&&c.time==0);++cases;continue;}
  CHECK(c.kind==ContactKind::Airborne&&c.time>0);
  const State end=advance(start,w,c.time);
  const Vec3 a=w.gravityVector();
  // The residual is rounding, so it is bounded relative to the summed terms.
  CHECK(check::near(end.position.y,groundY,positionScale(start,a,c.time)+std::fabs(groundY),256));
  CHECK(c.point.y==groundY&&check::near(c.point.x,end.position.x,positionScale(start,a,c.time)));
  // Landing speed is exactly sqrt(vy^2 + 2gh) downward.
  const double expected=-std::sqrt(vertical*vertical+2*w.gravity()*height);
  CHECK(check::relNear(end.velocity.y,expected,1e-9)||check::near(end.velocity.y,expected,std::fabs(vertical)+w.gravity()*c.time,256));
  ++cases;
 }
 CHECK(cases==630);
 // Tiny height and fast descent: the conjugate form keeps full relative accuracy.
 const Contact small=groundContact({{0,1e-6,0},{0,-1e8,0}},withGravity(9.81));
 CHECK(small.kind==ContactKind::Airborne&&check::relNear(small.time,1e-14,1e-12));
}

// Regression: a state left ~1e-15 below the plane by rounding (as after advancing to
// a contact) and rising used to be reported as "already in contact".
void explicitContactKinds(){
 const World w=withGravity(9.81);
 const Contact below=groundContact({{0,-1,0},{0,1,0}},w);
 CHECK(below.kind==ContactKind::BelowPlane&&below.time==0);
 const Contact resting=groundContact({{0,0,0},{2,0,0}},w);
 CHECK(resting.kind==ContactKind::OnPlane&&resting.time==0);
 // The old example's final sample: x = 26.03, y = -7.1e-15 after advancing to contact.
 const Contact rising=groundContact({{26.03,-7.1e-15,0},{0,5,0}},w);
 CHECK(rising.kind==ContactKind::Airborne&&check::relNear(rising.time,2*5/9.81,1e-12));
 CHECK(groundContact({{0,-7.1e-15,0},{0,5,0}},w,1e-12).kind==ContactKind::Airborne);
 CHECK(groundContact({{0,-1e-9,0},{0,5,0}},w,1e-12).kind==ContactKind::BelowPlane);
 const Contact bounce=groundContact({{0,0,0},{0,3,0}},w);
 CHECK(bounce.kind==ContactKind::Airborne&&check::relNear(bounce.time,2*3/9.81,1e-12));
 // Nonzero ground height.
 const Contact raised=groundContact({{0,5,0},{0,0,0}},withGravity(9.81,2));
 CHECK(raised.kind==ContactKind::Airborne&&check::relNear(raised.time,std::sqrt(2*3/9.81),1e-12)&&raised.point.y==2);
 CHECK(std::string(toString(ContactKind::BelowPlane))=="below-plane");
}

void invalidInput(){
 const World w=withGravity(9.81);
 const State initial{{0,10,0},{4,8,2}};
 const double nan=std::numeric_limits<double>::quiet_NaN(),inf=std::numeric_limits<double>::infinity();
 CHECK_THROWS(std::invalid_argument,advance(initial,w,-1));
 CHECK_THROWS(std::invalid_argument,advance(initial,w,inf));
 CHECK_THROWS(std::invalid_argument,advance(initial,w,nan));
 CHECK_THROWS(std::invalid_argument,advance({{nan,0,0},{0,0,0}},w,1));
 CHECK_THROWS(std::invalid_argument,advance(initial,Vec3{0,inf,0},1));
 CHECK_THROWS(std::invalid_argument,groundContact(initial,withGravity(0)));
 CHECK_THROWS(std::invalid_argument,groundContact(initial,withGravity(-9.81)));
 CHECK_THROWS(std::invalid_argument,groundContact(initial,withGravity(nan)));
 CHECK_THROWS(std::invalid_argument,groundContact({{0,1,0},{0,nan,0}},w));
 CHECK_THROWS(std::invalid_argument,groundContact(initial,withGravity(9.81,inf)));
 World badUnits=w;badUnits.metresPerUnit=0;
 CHECK_THROWS(std::invalid_argument,groundContact(initial,badUnits));
 CHECK_THROWS(std::overflow_error,groundContact({{0,1e308,0},{0,0,0}},withGravity(9.81,-1e308)));
 CHECK_THROWS(std::invalid_argument,sampleFlight(initial,w,0,1,0));
 CHECK_THROWS(std::invalid_argument,sampleFlight(initial,w,1,.5,4));
}

// Regression: the old fixed 1e-7 tolerance is not justified once the summed terms
// exceed ~1e8; the scaled tolerance holds from millimetres to 1e9.
void scaledTolerance(){
 const World w=withGravity(9.81);
 for(double scale:{1e-3,1.,1e3,1e6,1e9}){
  const State st{{scale,scale,0},{0,std::sqrt(scale)*10,0}};
  const Contact c=groundContact(st,w);
  const State end=advance(st,w,c.time);
  const double terms=positionScale(st,w.gravityVector(),c.time);
  CHECK(check::near(end.position.y,0,terms,256));
  if(scale>=1e9)CHECK(roundingTolerance(terms,256)>1e-7);
 }
}

void unitsAgree(){
 // The same physical throw in metres and in yards lands at the same time and place.
 const World m=World::metres(),y=World::yards();
 const State inMetres{{0,2,0},{10,12,3}};
 const State inYards{{0,m.toMetres(2)/y.metresPerUnit,0},{10/y.metresPerUnit,12/y.metresPerUnit,3/y.metresPerUnit}};
 const Contact a=groundContact(inMetres,m),b=groundContact(inYards,y);
 CHECK(check::relNear(a.time,b.time,1e-13));
 CHECK(check::relNear(a.point.x,y.toMetres(b.point.x),1e-13)&&check::relNear(a.point.z,y.toMetres(b.point.z),1e-13));
 CHECK(check::relNear(y.gravity(),9.80665/.9144,1e-15));
}

void samplingAndCsv(){
 const World w=withGravity(9.81);
 const State st{{0,2,0},{10,12,0}};
 const Contact c=groundContact(st,w);
 const auto samples=sampleFlight(st,w,0,c.time,50);
 CHECK(samples.size()==51&&samples.front().time==0&&samples.back().time==c.time);
 // CSV keeps full precision: every value reads back bit-identical.
 std::ostringstream out;
 writeCsv(out,samples);
 std::istringstream in(out.str());
 std::string line;
 std::getline(in,line);
 CHECK(line=="seconds,x,y,z,vx,vy,vz");
 bool exact=true;
 for(const Sample& s:samples){
  std::getline(in,line);
  std::istringstream row(line);
  double v[7];char comma;
  row>>v[0];for(int i=1;i<7;++i)row>>comma>>v[i];
  exact=exact&&v[0]==s.time&&v[1]==s.position.x&&v[2]==s.position.y&&v[3]==s.position.z&&v[5]==s.velocity.y;
 }
 CHECK(exact);
}
}

int main(){
 check::run("integration",integration);
 check::run("overflowOrdering",overflowOrdering);
 check::run("contactCases",contactCases);
 check::run("explicitContactKinds",explicitContactKinds);
 check::run("invalidInput",invalidInput);
 check::run("scaledTolerance",scaledTolerance);
 check::run("unitsAgree",unitsAgree);
 check::run("samplingAndCsv",samplingAndCsv);
 return check::finish("physics");
}
