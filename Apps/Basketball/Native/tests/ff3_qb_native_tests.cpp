#define NOMINMAX
#include "../src/ff3_qb_overlay.hpp"
#include <iostream>
int main(){
 winrt::init_apartment();unsigned checks=0;
 auto check=[&](bool ok){++checks;if(!ok){std::cerr<<"Failed check "<<checks<<'\n';std::exit(1);}};
 using namespace ff3Native;
 const auto basketballBefore=json::dump(basketballUi::snapshot());
 check(!enabled&&!force95);check(!set(L"autotime",basketballUi::boolean(true)));
 check(!set(L"ff3_qb.arc",basketballUi::number(2)));check(!set(L"ff3_qb.arc",basketballUi::number(NAN)));
 check(!set(L"ff3_qb.power",basketballUi::number(1)));check(!set(L"ff3_qb.unknown",basketballUi::boolean(true)));
 check(set(L"ff3_qb.arc",basketballUi::number(1)));check(arc==1);
 check(set(L"ff3_qb.power",basketballUi::boolean(false)));check(set(L"ff3_qb",basketballUi::boolean(true)));
 check(set(L"ff3_qb.preset",basketballUi::number(4)));check(passMode==4);
 check(!set(L"ff3_qb.preset",basketballUi::number(6)));check(!set(L"ff3_qb.angle",basketballUi::number(4)));check(!set(L"ff3_qb.angle",basketballUi::number(76)));check(!set(L"ff3_qb.radius",basketballUi::number(INFINITY)));
 check(set(L"ff3_qb.angle",basketballUi::number(42)));check(set(L"ff3_qb.autoangle",basketballUi::boolean(false)));check(set(L"ff3_qb.fit",basketballUi::boolean(true)));
 check(set(L"ff3_qb.lead",basketballUi::number(-5))&&leadDistance==-5);
 check(set(L"ff3_qb.height",basketballUi::number(4))&&catchHeightAdjustment==4);
 check(!set(L"ff3_qb.height",basketballUi::number(9)));check(!set(L"ff3_qb.height",basketballUi::number(-4)));
 check(!set(L"ff3_qb.lead",basketballUi::number(13)));check(!set(L"ff3_qb.lead",basketballUi::number(-13)));check(!set(L"ff3_qb.lead",basketballUi::number(.5)));
 const auto savedRevision=revision.load();stop();check(!enabled&&revision>savedRevision);
 auto file=std::filesystem::temp_directory_path()/(L"vanta-ff3-test-"+std::to_wstring(GetCurrentProcessId())+L".json");
 check(save(file));set(L"ff3_qb.arc",basketballUi::number(0));set(L"ff3_qb",basketballUi::boolean(true));load(file);
 check(arc==1&&autoPower&&!enabled);check(passMode==4&&manualAngle==42&&!automaticAngle&&smartFit);
 check(catchHeightAdjustment==4&&modules()[L"ff3_qb"][L"rows"][L"height"].num()==4);check(set(L"ff3_qb.height",basketballUi::number(0)));
 check(leadDistance==-5&&modules()[L"ff3_qb"][L"rows"][L"lead"].num()==-5);check(set(L"ff3_qb.lead",basketballUi::number(0)));
 check(set(L"ff3_qb.power",basketballUi::boolean(false)));check(save(file));autoPower=true;load(file);check(!autoPower);
 check(!set(L"ff3_qb.force95",basketballUi::number(1)));check(set(L"ff3_qb.force95",basketballUi::boolean(true)));check(force95&&passMode==0&&automaticAngle&&autoPower);
 check(selectedLead()==3&&selectedHeight()==8);
 check(set(L"ff3_qb.height",basketballUi::number(7))&&beamCatchHeight==7&&catchHeightAdjustment==0);
 check(save(file));beamCatchHeight=8;load(file);check(force95&&selectedHeight()==7&&selectedLead()==3);
 check(set(L"ff3_qb.height",basketballUi::number(8)));
 check(save(file));force95=false;load(file);check(force95);check(set(L"ff3_qb.preset",basketballUi::number(2))&&!force95);std::filesystem::remove(file);
 check(json::dump(basketballUi::snapshot())==basketballBefore);
 Frame f;f.pid=5;f.model=100;f.own=200;f.tool=300;f.target=400;f.revision=revision;f.ready=true;f.power=95;f.gravity=28;f.aim={500,300,true};f.client={0,0,1920,1080};
 {auto visual=f;visual.at=10;visual.head={0,8,0};visual.targetPosition={70,16,0};visual.targetVelocity={0,0,20};visual.ping=20;visual.mode=0;bool priorBeam=force95;force95=true;
  auto drawn=ff3Visual::projectedThrow(visual,10.08);auto sent=ff3qb::solve(visual.head,visual.targetPosition+visual.targetVelocity*.08,visual.targetVelocity,95,28,.06,false,5);
  check(drawn.valid&&sent.valid&&ff3qb::length(drawn.catchPoint-sent.catchPoint)<1e-6&&ff3qb::length(drawn.velocity-sent.velocity)<1e-6);force95=priorBeam;
 }
 {ff3Visual::PreviewPlan preview;auto valid=f;valid.held=valid.solution.valid=true;
  check(preview.select(valid,10).ready);auto gap=valid;gap.ready=gap.solution.valid=false;
  check(preview.select(gap,10.05).ready);check(!gap.ready); // Only the renderer receives the retained plan.
  check(!preview.select(gap,10.2).ready);
  preview.select(valid,11);gap.held=false;check(!preview.select(gap,11.01).ready);
  preview.select(valid,12);gap.held=true;++gap.revision;check(!preview.select(gap,12.01).ready);
  preview.select(valid,13);gap=valid;gap.ready=false;++gap.target;check(!preview.select(gap,13.01).ready);
 }
 f.gameMouse={500,300,0};f.matrix[0]=f.matrix[5]=f.matrix[10]=f.matrix[15]=1;
 check(!eligible(f,now())); // No admitted session or game focus: always closed.
 {
  Frame ready=f;ready.at=10;Frame released=ready;released.at=10.08;released.ready=false;released.tool=0;released.held=false;
  check(clickPlanValid(ready,released,10.02,10.08,1000,1060,ready.revision));
  check(!clickPlanValid(ready,released,10.12,10.15,1000,1030,ready.revision));
  check(!clickPlanValid(ready,released,10.02,10.38,1000,1360,ready.revision));
  check(!clickPlanValid(ready,released,10.02,10.08,1000,1060,ready.revision+1));
  ++released.own;check(!clickPlanValid(ready,released,10.02,10.08,1000,1060,ready.revision));
  --released.own;ready.textFocus=true;check(!clickPlanValid(ready,released,10.02,10.08,1000,1060,ready.revision));
  ready.textFocus=false;ready.at=now()-.02;requested=ready;requestedAt=now()-.01;physicalClickAt=GetTickCount64()-10;
  auto dispatch=packetFrame(released);check(dispatch.retained&&dispatch.frame.tool==ready.tool&&dispatch.clicked==physicalClickAt.load());
  physicalClickAt=0;dispatch=packetFrame(released);check(!dispatch.retained&&!dispatch.frame.tool&&dispatch.clicked==0);requested={};requestedAt=0;
 }
 {float live[16]{},sample[16]{};live[0]=1;
  check(ff3Visual::ViewReader::matrix(GetCurrentProcess(),reinterpret_cast<addr>(live),sample)&&sample[0]==1);
  live[0]=2;check(ff3Visual::ViewReader::matrix(GetCurrentProcess(),reinterpret_cast<addr>(live),sample)&&sample[0]==2);
  live[0]=NAN;check(!ff3Visual::ViewReader::matrix(GetCurrentProcess(),reinterpret_cast<addr>(live),sample));}
 check(dispatchConsistent(f,f,f,{500,300}));
 for(int i=0;i<14;i++){
  Frame changed=f;
  switch(i){case 0:++changed.pid;break;case 1:++changed.model;break;case 2:++changed.own;break;case 3:++changed.tool;break;case 4:++changed.target;break;case 5:++changed.revision;break;case 6:changed.ready=false;break;case 7:changed.power=90;break;case 8:changed.gravity=30;break;case 9:changed.origin.x=10;break;case 10:changed.client.right=1000;break;case 11:changed.aim.x+=30;break;case 12:changed.aim.valid=false;break;case 13:changed.window=(HWND)1;break;}
  check(!dispatchConsistent(f,f,changed,{500,300}));
 }
 check(!dispatchConsistent(f,f,f,{510,300}));
 Frame unacknowledged=f;unacknowledged.gameMouse={400,300,0};check(!dispatchConsistent(f,f,unacknowledged,{500,300}));
 Frame rotated=f;rotated.matrix[0]=.99f;rotated.matrix[2]=.1f;check(!dispatchConsistent(f,f,rotated,{500,300}));
 Frame missing=f;missing.gameMouse.x=NAN;check(!dispatchConsistent(f,f,missing,{500,300}));
 Frame previous=f;++previous.revision;check(!dispatchConsistent(previous,f,f,{500,300}));
 Frame keyFrame;keyFrame.target=456;keyFrame.solution.valid=true;keyFrame.solution.angle=32;lockedTarget=0;
 check(hotkey('Q',keyFrame,false)&&lockedTarget==456);check(hotkey('Q',keyFrame,true)&&lockedTarget==456);check(hotkey('Q',keyFrame,false)&&lockedTarget==0);
 passMode=5;check(hotkey('Z',keyFrame,false)&&passMode==0&&automaticAngle&&autoPower);check(hotkey('Z',keyFrame,true)&&passMode==0);
 force95=true;check(hotkey('R',keyFrame,false)&&manualAngle==33&&!automaticAngle&&autoPower&&!force95);check(hotkey('F',keyFrame,false)&&manualAngle==32);
 check(set(L"ff3_qb.radius",basketballUi::number(600)));check(!set(L"ff3_qb.radius",basketballUi::number(701)));
 check(set(L"ff3_qb.trails",basketballUi::boolean(false))&&!ballTrails);check(set(L"ff3_qb.trails",basketballUi::boolean(true))&&ballTrails);
 check(set(L"ff3_qb.dive",basketballUi::number(16)));check(!set(L"ff3_qb.dive",basketballUi::number(25)));
 int stops=0;packetStop=[&](){++stops;};enabled=true;stop(false);check(!enabled&&stops==0);stop();check(stops==1);packetStop={};
 clicks.down(true,now());stop();check(!clicks.pending&&clicks.up());
 record(f,"Test input only",true);check(report()[L"throws"].num()==1);check(!mouse);
 std::cout<<checks<<" FF3 settings, persistence, session isolation and dispatch cancellation checks passed (no game inputs)\n";
 {
  using namespace Gdiplus;ULONG_PTR token=0;GdiplusStartupInput startup;check(GdiplusStartup(&token,&startup,nullptr)==Ok);
  Frame sample;sample.groundY=0;sample.client={0,0,1280,720};sample.ready=sample.held=sample.wheelVisible=sample.coverageKnown=true;sample.name="Practice receiver";sample.route="Go / deep";sample.coverageCount=3;sample.coverageDetail="3 catchers / jump + dive estimate";sample.locked=true;sample.mode=2;sample.guarded=true;sample.margin=.54;sample.power=78;sample.distance=146;sample.solution.angle=48;sample.solution.time=2.36;
  PumpSurface field;sample.target=123;sample.gravity=28;sample.fitOffset={4,4,0};sample.solution=ff3qb::solveAngle({0,8,0},{104,12,0},{},35,28,.05,5);sample.matrix[0]=.012f;sample.matrix[5]=.025f;sample.matrix[6]=.012f;sample.matrix[3]=-.65f;sample.matrix[7]=-.5f;sample.matrix[15]=1;sample.receiver=ff3qb::project({100,8,0},sample.matrix,1280,720);
  sample.defenders={{{75,8,8},{},20,7,12}};sample.coverageTrace={0,.3,.54,{55,13,0}};ff3Visual::renderField(field,sample,false);check(field.bits&&!field.visible);
  {Bitmap image(field.width,field.height,field.width*4,PixelFormat32bppPARGB,(BYTE*)field.bits);CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};check(image.Save(L"build/ff3-field-preview.png",&png,nullptr)==Ok);}
  sample.ready=false;sample.fitBlocked=true;sample.reason="Coverage incomplete: no assisted throw; normal click unchanged";
  ff3Visual::renderField(field,sample,false);check(field.bits&&!field.visible);
  {
   bool wasReady=sample.ready;sample.ready=true;ff3Visual::renderField(field,sample,false);
   auto* data=static_cast<unsigned char*>(field.bits);size_t bytes=size_t(field.width)*field.height*4;
   std::vector<unsigned char> before(data,data+bytes);check(std::any_of(before.begin(),before.end(),[](auto pixel){return pixel!=0;}));sample.balls.clear();ff3Visual::renderBalls(field,sample,false,false);
   check(std::equal(before.begin(),before.end(),data)); // Compositing balls cannot erase QB geometry.
   ff3Visual::FramePacer pacer;double began=now();for(int i=0;i<12;i++)pacer.wait(now());
   double elapsed=now()-began;check(elapsed>.06&&elapsed<.5);std::cout<<"Overlay pacing sample: "<<12/elapsed<<" Hz (timer only)\n";
   sample.ready=wasReady;ff3Visual::renderField(field,sample,false);
  }
  {Bitmap image(field.width,field.height,field.width*4,PixelFormat32bppPARGB,(BYTE*)field.bits);CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};check(image.Save(L"build/ff3-field-blocked-preview.png",&png,nullptr)==Ok);}
  field.close();PumpSurface balls;ff3qb::BallFlight flight;flight.position={60,22,0};flight.velocity={40,-8,0};flight.gravity=28;for(int i=30;i>=0;i--){double dt=i*.05;flight.trail.push_back(flight.position-flight.velocity*dt+ff3qb::Vec{0,-.5*28*dt*dt,0});}flight.model={true,0,28,0,flight.position,flight.velocity};sample.balls={flight};ff3Visual::renderBalls(balls,sample,false);check(balls.bits&&!balls.visible);
  {Bitmap image(balls.width,balls.height,balls.width*4,PixelFormat32bppPARGB,(BYTE*)balls.bits);CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};check(image.Save(L"build/ff3-balls-preview.png",&png,nullptr)==Ok);}
  balls.close();GdiplusShutdown(token);
 }
 winrt::uninit_apartment();
 std::cout<<checks<<" total native and compositor checks passed\n";
}
