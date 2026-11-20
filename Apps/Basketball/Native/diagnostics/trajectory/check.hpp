#pragma once
// Minimal test harness. Unlike assert it works with NDEBUG, reports every failing
// check rather than stopping at the first, and makes the process exit nonzero.
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>

namespace check {
inline int& failures(){static int n=0;return n;}
inline int& passes(){static int n=0;return n;}
inline void record(bool ok,const char* expr,const char* file,int line){
 if(ok){++passes();return;}
 ++failures();
 std::cerr<<file<<"("<<line<<"): CHECK failed: "<<expr<<"\n";
}
// Runs one group of checks; an unexpected exception is a reported failure, not a crash.
template<class F>
void run(const char* name,F f){
 try{f();}
 catch(const std::exception& e){++failures();std::cerr<<name<<": unexpected exception: "<<e.what()<<"\n";}
 catch(...){++failures();std::cerr<<name<<": unexpected exception\n";}
}
inline int finish(const char* suite){
 std::cout<<suite<<": "<<passes()<<" checks passed, "<<failures()<<" failed\n";
 return failures()?1:0;
}
// Equal within `ulps` rounding steps of `scale`, the summed magnitude of the terms
// that produced the values. Scale-aware, so it holds for 1e-6 and 1e6 alike.
inline bool near(double a,double b,double scale,double ulps=64){
 return std::fabs(a-b)<=ulps*std::numeric_limits<double>::epsilon()*std::fabs(scale);
}
inline bool relNear(double a,double b,double rel){return std::fabs(a-b)<=rel*std::fmax(std::fabs(a),std::fabs(b));}
}

#define CHECK(expr) ::check::record(static_cast<bool>(expr),#expr,__FILE__,__LINE__)
#define CHECK_THROWS(type,expr) do{bool thrown_=false;try{(void)(expr);}catch(const type&){thrown_=true;}\
 ::check::record(thrown_,"throws " #type ": " #expr,__FILE__,__LINE__);}while(0)
