#pragma once
template<class Check> void mouseAimChecks(Check check){
 using namespace ff3Native;using namespace ff3qb;
 for(Vec camera:std::vector<Vec>{{18,18,-35},{-12,30,-55},{0,8,0}})for(double distance:{80.,180.,260.,300.}){
  const Vec head{0,8,0};const auto solution=solve(head,{0,8,distance},{},95,28,.06,false,5);check(solution.valid);
  const auto endpoint=mouseAimWorld(head,camera,solution.velocity),direction=solution.velocity*(1/length(solution.velocity));
  check(finite(endpoint)&&std::abs(length(endpoint-camera)-1000)<1e-7);
  check(length((endpoint-head)*(1/length(endpoint-head))-direction)<1e-10);
  const auto actualHit=camera+(endpoint-camera)*(1000/length(endpoint-camera));check(length((actualHit-head)*(1/length(actualHit-head))-direction)<1e-10);
 }
 check(!finite(mouseAimWorld({},{},{},1000))&&!finite(mouseAimWorld({},{NAN,0,0},{0,30,80})));
 {float matrix[16]{};matrix[0]=matrix[5]=matrix[14]=1;auto screen=projectAim({0,1,2},matrix,1280,720);check(screen.valid&&screen.x==640&&screen.y==180);check(aimInViewport(screen,1280,720));
  const auto outside=projectAim({0,3,1},matrix,1280,720);check(outside.valid&&outside.y<0&&!aimInViewport(outside,1280,720));auto cue=mouseAimGuide(outside,{640,180,true},1280,720);check(!cue.onScreen&&cue.marker.valid&&cue.marker.y>=27&&cue.marker.y<30);
  check(!project({0,3,1},matrix,1280,720).valid);check(!projectAim({0,0,-1},matrix,1280,720).valid);
  auto aligned=mouseAimGuide(screen,screen,1280,720);check(aligned.onScreen&&aligned.aligned&&aligned.marker.x==screen.x);
 }
 {CursorAimTracker tracker;check(!tracker.update({120,140,true},{100,140,true},{100,140,true},1));check(!tracker.update({120,140,true},{120,140,true},{100,140,true},1.01));check(!tracker.update({120,140,true},{120,140,true},{120,140,true},1.02));check(tracker.update({121,140,true},{121,140,true},{121,140,true},1.036));check(!tracker.update({130,140,true},{121,140,true},{121,140,true},1.05));}
 {CursorAimTracker tracker;check(!tracker.update({1220.2,479.35,true},{1218,480,true},{1218,480,true},1));check(tracker.update({1220.2,479.35,true},{1218,480,true},{1218,480,true},1.02));check(!tracker.update({1222,480,true},{1218,480,true},{1218,480,true},1.04));check(!tracker.update({1220.2,479.35,true},{1218,480,true},{1218,480,true},1.06));}
 {auto upward=ff3Input::cameraPanStep({640,-400,true},1280,720),right=ff3Input::cameraPanStep({1800,360,true},1280,720),done=ff3Input::cameraPanStep({640,360,true},1280,720);check(upward.valid&&upward.dy<0&&upward.dy>=-64&&upward.dx==0);check(right.valid&&right.dx>0&&right.dx<=64&&right.dy==0);check(done.valid&&done.done&&!done.dx&&!done.dy);check(!ff3Input::cameraPanStep({},1280,720).valid);
  std::vector<bool> events;bool physical=false;{ff3Input::CameraPanButton button([&](bool down){events.push_back(down);return true;},[&](){return physical;});check(button.active());}check(events==std::vector<bool>({true,false}));
  events.clear();physical=true;{ff3Input::CameraPanButton button([&](bool down){events.push_back(down);return true;},[&](){return physical;});check(!button.active());}check(events.empty());
  events.clear();physical=false;{ff3Input::CameraPanButton button([&](bool down){events.push_back(down);return true;},[&](){return physical;});physical=true;check(!button.active());}check(events==std::vector<bool>({true}));
 }
 {Frame request;request.pid=1;request.model=2;request.own=3;request.tool=4;request.target=5;request.window=(HWND)6;request.ready=request.inputThrow=true;request.power=request.gamePower=95;request.gravity=28;request.aim={100,100,true};request.gameMouse={100,100,0};request.client={0,0,800,600};request.matrix[0]=request.matrix[5]=1;
  auto live=request;live.aim.x=121;live.gameMouse.x=121;check(!dispatchConsistent(request,request,live,{121,100}));check(dispatchConsistent(request,live,live,{121,100}));live.gamePower=94;check(!dispatchConsistent(request,live,live,{121,100}));
 }
 {const bool oldVisual=visualOnly,oldAutomatic=autoPower,oldBeam=force95,oldAngle=automaticAngle;visualOnly=true;autoPower=true;force95=false;automaticAngle=true;
  Frame frame;frame.at=10;frame.head={0,8,0};frame.targetPosition={0,8,220};frame.targetVelocity={4,0,0};frame.power=frame.gamePower=95;frame.gravity=28;frame.solution=solve(frame.head,frame.targetPosition,frame.targetVelocity,95,28,.05,false,5);frame.maximumAngle=85;
  check(!calculatesPower()&&!anglePowerPlan(frame)&&!wheelMethod());const auto forecast=predictedThrow(frame,10.08);check(forecast.valid&&std::abs(length(forecast.velocity)-frame.gamePower)<1e-6);
  check(length(point(forecast,28,forecast.time)-(frame.targetPosition+frame.targetVelocity*(.08+forecast.time+.05)))<1e-4);
  frame.targetPosition={0,8,500};check(!predictedThrow(frame,10.08).valid);frame.targetPosition={0,8,220};frame.minimumAngle=80;check(!predictedThrow(frame,10.08).valid);
  visualOnly=oldVisual;autoPower=oldAutomatic;force95=oldBeam;automaticAngle=oldAngle;
 }
 {ff3qb::SelectionLatch latch;latch.current=42;check(latch.update(0,true,1,false,0)==42);check(latch.update(43,true,1.1,false,0)==43);}
 {using namespace Gdiplus;const bool oldVisual=visualOnly;visualOnly=true;ULONG_PTR token=0;GdiplusStartupInput startup;check(GdiplusStartup(&token,&startup,nullptr)==Ok);
  Frame frame;frame.ready=true;frame.menuOpen=frame.textFocus=false;frame.client={0,0,1280,720};frame.head={0,8,0};frame.cameraPosition={8,12,-25};frame.power=95;frame.solution=solve(frame.head,{0,8,220},{},95,28,.05,false,5);frame.matrix[0]=frame.matrix[5]=frame.matrix[14]=1;frame.target=123;
  const auto target=projectAim(mouseAimWorld(frame.head,frame.cameraPosition,frame.solution.velocity),frame.matrix,1280,720);frame.cursor={LONG(target.x-80),LONG(target.y+40)};
  {Bitmap image(1280,720,PixelFormat32bppARGB);Graphics graphics(&image);graphics.Clear(Color(255,18,38,42));graphics.SetSmoothingMode(SmoothingModeAntiAlias);check(ff3Visual::drawMouseAim(graphics,frame));CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};check(image.Save(L"build/ff3-visual-aim-cursor-preview.png",&png,nullptr)==Ok);
  frame.solution.velocity={0,90,20};check(ff3Visual::drawMouseAim(graphics,frame));frame.afterRelease=true;check(!ff3Visual::drawMouseAim(graphics,frame));frame.afterRelease=false;frame.menuOpen=true;check(!ff3Visual::drawMouseAim(graphics,frame));
  }GdiplusShutdown(token);visualOnly=oldVisual;
 }
}
