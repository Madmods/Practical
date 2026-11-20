#pragma once
// Constant-acceleration projectile motion and ground-plane contact.
// Lengths are in World units (World::metresPerUnit); time is always seconds.
#include <cmath>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace projectile {

struct Vec3 { double x=0,y=0,z=0; };
inline Vec3 operator+(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 operator-(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 operator-(Vec3 a){return {-a.x,-a.y,-a.z};}
inline Vec3 operator*(Vec3 a,double t){return {a.x*t,a.y*t,a.z*t};}
inline Vec3 operator*(double t,Vec3 a){return a*t;}
inline Vec3 operator/(Vec3 a,double t){return {a.x/t,a.y/t,a.z/t};}
inline double dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline double length(Vec3 a){return std::hypot(a.x,a.y,a.z);}
inline Vec3 horizontal(Vec3 a){return {a.x,0,a.z};}
inline double horizontalLength(Vec3 a){return std::hypot(a.x,a.z);}
inline double maxAbs(Vec3 a){return std::fmax(std::fabs(a.x),std::fmax(std::fabs(a.y),std::fabs(a.z)));}
inline bool finite(Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}

struct State { Vec3 position,velocity; };

// Rounding allowance for a value computed from terms whose magnitudes sum to `scale`.
inline double roundingTolerance(double scale,double ulps=64){
 return ulps*std::numeric_limits<double>::epsilon()*std::fabs(scale);
}

// The single gravity/units configuration shared by motion, contact and planning.
// Gravity points down the y axis; the ground is the infinite plane y == groundY.
struct World {
 double metresPerUnit=1;      // 1 = metres, 0.9144 = yards, 0.3048 = feet
 double gravityMps2=9.80665;  // always specified in m/s^2, converted by gravity()
 double groundY=0;
 double gravity() const {return gravityMps2/metresPerUnit;}
 Vec3 gravityVector() const {return {0,-gravity(),0};}
 double fromMetres(double metres) const {return metres/metresPerUnit;}
 double toMetres(double units) const {return units*metresPerUnit;}
 void validate() const {
  if(!(std::isfinite(metresPerUnit)&&metresPerUnit>0&&std::isfinite(gravityMps2)&&gravityMps2>0&&std::isfinite(groundY)&&std::isfinite(gravity())))
   throw std::invalid_argument("World needs a positive finite unit scale and gravity and a finite ground height");
 }
 static World metres(){return {};}
 static World yards(){World w;w.metresPerUnit=.9144;return w;}
};

inline State advance(const State& s,Vec3 acceleration,double seconds){
 if(!finite(s.position)||!finite(s.velocity)||!finite(acceleration)||!std::isfinite(seconds)||seconds<0)
  throw std::invalid_argument("Expected finite state and nonnegative duration");
 // Multiply by t/2 and then by t: forming t*t first overflows for long durations
 // even when the acceleration is zero (0*inf is NaN) or small.
 const Vec3 halfDelta=acceleration*(.5*seconds);
 State next{s.position+s.velocity*seconds+halfDelta*seconds,s.velocity+acceleration*seconds};
 if(!finite(next.position)||!finite(next.velocity))throw std::overflow_error("State exceeds numeric range");
 return next;
}
inline State advance(const State& s,const World& world,double seconds){return advance(s,world.gravityVector(),seconds);}

// Sum of the magnitudes advance() adds to form the position; rounding error in the
// result is proportional to it, so tolerances scale with the problem.
inline double positionScale(const State& s,Vec3 acceleration,double seconds){
 return maxAbs(s.position)+maxAbs(s.velocity)*seconds+maxAbs(acceleration)*(.5*seconds)*seconds;
}

enum class ContactKind {
 Airborne,   // above the plane, or on it and rising: lands at time > 0
 OnPlane,    // on the plane (within rounding) and not rising: contact at time 0
 BelowPlane  // below the plane beyond rounding: there is no physical contact time
};
inline const char* toString(ContactKind k){
 return k==ContactKind::Airborne?"airborne":k==ContactKind::OnPlane?"on-plane":"below-plane";
}
struct Contact { ContactKind kind; double time; Vec3 point; };

// Heights within `heightTolerance` of the plane count as on it, so a state produced
// by advancing to a previous contact (residual ~1e-15) is not treated as underground.
// The default is rounding at the scale of the position's coordinates, which is the
// scale the residual of an earlier advance() is computed at.
inline Contact groundContact(const State& s,const World& world,double heightTolerance=-1){
 world.validate();
 if(!finite(s.position)||!finite(s.velocity))throw std::invalid_argument("Expected finite state");
 const double gravity=world.gravity();
 const double height=s.position.y-world.groundY;
 if(!std::isfinite(height))throw std::overflow_error("Height exceeds numeric range");
 const double tolerance=heightTolerance>=0?heightTolerance:roundingTolerance(maxAbs(s.position)+std::fabs(world.groundY));
 const Vec3 onPlane{s.position.x,world.groundY,s.position.z};
 if(height< -tolerance)return {ContactKind::BelowPlane,0.,s.position};
 if(height<=tolerance&&s.velocity.y<=0)return {ContactKind::OnPlane,0.,onPlane};
 const double h=std::fmax(height,0.);
 const double root=std::hypot(s.velocity.y,std::sqrt(gravity)*std::sqrt(h)*std::sqrt(2.));
 // The conjugate expression avoids subtracting similar values on descent.
 const double time=s.velocity.y<0 ? h/(root*.5-s.velocity.y*.5) : s.velocity.y/gravity+root/gravity;
 if(!std::isfinite(time))throw std::overflow_error("Contact time exceeds numeric range");
 Vec3 point=advance(s,world,time).position;
 point.y=world.groundY;  // exact by definition; the computed residual is rounding only
 return {ContactKind::Airborne,time,point};
}

struct Sample { double time; Vec3 position,velocity; };

// intervals+1 evenly spaced samples on [start,end]; both endpoints are exact.
inline std::vector<Sample> sampleFlight(const State& s,const World& world,double start,double end,int intervals){
 if(intervals<1||!std::isfinite(start)||!std::isfinite(end)||start<0||end<start)
  throw std::invalid_argument("Expected 0 <= start <= end and at least one interval");
 std::vector<Sample> out;
 out.reserve(intervals+1);
 for(int i=0;i<=intervals;++i){
  const double t=i==0?start:i==intervals?end:start+(end-start)*i/intervals;
  const State at=advance(s,world,t);
  out.push_back({t,at.position,at.velocity});
 }
 return out;
}

// Full round-trip precision so the CSV reproduces the computed values.
inline void writeCsv(std::ostream& out,const std::vector<Sample>& samples){
 const auto old=out.precision(std::numeric_limits<double>::max_digits10);
 out<<"seconds,x,y,z,vx,vy,vz\n";
 for(const Sample& s:samples)
  out<<s.time<<","<<s.position.x<<","<<s.position.y<<","<<s.position.z<<","
     <<s.velocity.x<<","<<s.velocity.y<<","<<s.velocity.z<<"\n";
 out.precision(old);
}

}
