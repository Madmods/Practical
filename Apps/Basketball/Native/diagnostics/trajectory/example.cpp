// Prints one throw's trajectory as CSV, from release to ground contact.
#include "projectile.hpp"
#include <iostream>
int main(){
 const projectile::World world=projectile::World::metres();
 const projectile::State initial{{0,2,0},{10,12,0}};
 const projectile::Contact contact=projectile::groundContact(initial,world);
 if(contact.kind!=projectile::ContactKind::Airborne){
  std::cerr<<"no flight: "<<projectile::toString(contact.kind)<<"\n";
  return 1;
 }
 auto samples=projectile::sampleFlight(initial,world,0,contact.time,50);
 // The final sample is the contact itself: exactly on the ground (the computed
 // residual there is rounding, within roundingTolerance of the summed terms).
 samples.back().position=contact.point;
 projectile::writeCsv(std::cout,samples);
}
