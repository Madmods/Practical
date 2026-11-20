// Runs every synthetic scenario, then writes for each one an SVG diagram, a text
// report and a per-frame CSV, plus index.html and summary.txt.
//   simulate [output-directory]   (default: out)
#include "visualize.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc,char** argv){
 using namespace planner;
 namespace fs=std::filesystem;
 const fs::path outDir=argc>1?argv[1]:"out";
 fs::create_directories(outDir);
 const World world=World::metres();
 const PlannerConfig cfg=PlannerConfig::defaults(world);
 std::ofstream index(outDir/"index.html"),summary(outDir/"summary.txt");
 index<<"<!doctype html><meta charset=\"utf-8\"><title>Trajectory planner scenarios</title>"
        "<body style=\"font-family:Segoe UI,Helvetica,Arial,sans-serif;margin:16px;background:#fff;color:#222\">"
        "<h1>Trajectory planner scenarios</h1>";
 summary<<"scenario | verdict | selection | T s | speed | elev deg | outcome | switches | max ms\n";
 for(const scenarios::Spec& spec:scenarios::standardScenarios(world)){
  const scenarios::RunResult r=scenarios::run(spec,cfg);
  const std::vector<Evaluation> grid=Planner(cfg).evaluateGrid(r.throwScene);
  {
   std::ofstream svg(outDir/(spec.name+".svg"));
   viz::writeSvg(svg,spec.name+": "+spec.description.substr(0,spec.description.find('.')),r.throwScene,r.throwDecision,grid,cfg);
  }
  {
   std::ofstream report(outDir/(spec.name+"-report.txt"));
   viz::writeReport(report,spec,r,grid,cfg);
  }
  {
   std::ofstream csv(outDir/(spec.name+"-frames.csv"));
   csv.precision(10);
   csv<<"time,has_selection,flight_time,depth,lateral,mode,verdict,dest_x,dest_z,cost,switched,blending,work,ms\n";
   for(const auto& f:r.frames)
    csv<<f.time<<","<<f.has<<","<<f.candidate.flightTime<<","<<f.candidate.depth<<","<<f.candidate.lateral<<","<<f.candidate.mode<<","
       <<toString(f.verdict)<<","<<f.destination.x<<","<<f.destination.z<<","<<f.cost<<","<<f.switched<<","<<f.blending<<","<<f.work<<","<<f.ms<<"\n";
  }
  const Decision& d=r.throwDecision;
  const std::string line=spec.name+" | "+(d.hasSelection?toString(d.selected.verdict):"none")+" | "+(d.hasSelection?d.selected.label:"-")
   +(d.hasSelection?format(" | %.2f | %.1f | %.1f",d.selected.candidate.flightTime,d.selected.speed,d.selected.elevationDeg):" | - | - | -")
   +" | "+r.outcome.result+(r.outcome.by.empty()?"":" ("+r.outcome.by+")")
   +" | "+std::to_string(r.frames.empty()?0:r.frames.back().switches)+format(" | %.2f",r.maxMs);
  summary<<line<<"\n";
  std::cout<<line<<"\n";
  index<<"<h2>"<<spec.name<<"</h2><p>"<<viz::escape(spec.description)<<"<br><a href=\""<<spec.name<<"-report.txt\">report</a> &middot; <a href=\""
       <<spec.name<<"-frames.csv\">frames</a></p><img src=\""<<spec.name<<".svg\" style=\"max-width:100%;height:auto;border:1px solid #ddd\">";
 }
 index<<"</body>";
 std::cout<<"wrote "<<fs::absolute(outDir).string()<<"\n";
}
