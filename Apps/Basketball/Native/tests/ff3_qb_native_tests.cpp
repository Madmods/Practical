#define NOMINMAX
#include "../src/ff3_qb_overlay.hpp"
#include <iostream>
int main(){
 winrt::init_apartment();unsigned checks=0;
 auto check=[&](bool ok){++checks;if(!ok){std::cerr<<"Failed check "<<checks<<'\n';std::exit(1);}};
 using namespace ff3Native;
 const auto basketballBefore=json::dump(basketballUi::snapshot());
 check(!enabled);check(!set(L"autotime",basketballUi::boolean(true)));
 check(!set(L"ff3_qb.arc",basketballUi::number(2)));check(!set(L"ff3_qb.arc",basketballUi::number(NAN)));
 check(!set(L"ff3_qb.power",basketballUi::number(1)));check(!set(L"ff3_qb.unknown",basketballUi::boolean(true)));
 check(set(L"ff3_qb.arc",basketballUi::number(1)));check(arc==1);
 check(set(L"ff3_qb.power",basketballUi::boolean(false)));check(set(L"ff3_qb",basketballUi::boolean(true)));
 check(set(L"ff3_qb.preset",basketballUi::number(4)));check(passMode==4);
 check(!set(L"ff3_qb.preset",basketballUi::number(6)));check(!set(L"ff3_qb.angle",basketballUi::number(4)));check(!set(L"ff3_qb.angle",basketballUi::number(76)));check(!set(L"ff3_qb.radius",basketballUi::number(INFINITY)));
 check(set(L"ff3_qb.angle",basketballUi::number(42)));check(set(L"ff3_qb.autoangle",basketballUi::boolean(false)));check(set(L"ff3_qb.fit",basketballUi::boolean(true)));
 const auto savedRevision=revision.load();stop();check(!enabled&&revision>savedRevision);
 auto file=std::filesystem::temp_directory_path()/(L"vanta-ff3-test-"+std::to_wstring(GetCurrentProcessId())+L".json");
 check(save(file));set(L"ff3_qb.arc",basketballUi::number(0));set(L"ff3_qb",basketballUi::boolean(true));load(file);
 check(arc==1&&!autoPower&&!enabled);check(passMode==4&&manualAngle==42&&!automaticAngle&&smartFit);std::filesystem::remove(file);
 check(json::dump(basketballUi::snapshot())==basketballBefore);
 Frame f;f.pid=5;f.model=100;f.own=200;f.tool=300;f.target=400;f.revision=revision;f.ready=true;f.power=95;f.gravity=28;f.aim={500,300,true};f.client={0,0,1920,1080};
 f.gameMouse={500,300,0};f.matrix[0]=f.matrix[5]=f.matrix[10]=f.matrix[15]=1;
 check(!eligible(f,now())); // No admitted session or game focus: always closed.
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
 clicks.down(true,now());stop();check(!clicks.pending&&clicks.up());
 record(f,"Test input only",true);check(report()[L"throws"].num()==1);check(!mouse);
 std::cout<<checks<<" FF3 settings, persistence, session isolation and dispatch cancellation checks passed (no game inputs)\n";
 {
  using namespace Gdiplus;ULONG_PTR token=0;GdiplusStartupInput startup;check(GdiplusStartup(&token,&startup,nullptr)==Ok);
  Frame sample;sample.client={0,0,1280,720};sample.ready=sample.held=sample.wheelVisible=sample.coverageKnown=true;sample.name="Practice receiver";sample.route="Deep route";sample.mode=2;sample.margin=.24;sample.power=78;sample.distance=146;sample.solution.angle=48;sample.solution.time=2.36;
  PumpSurface hud;ff3Visual::renderHud(hud,sample,false);check(hud.bits&&!hud.visible);
  {Bitmap image(hud.width,hud.height,hud.width*4,PixelFormat32bppPARGB,(BYTE*)hud.bits);CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};check(image.Save(L"build/ff3-hud-preview.png",&png,nullptr)==Ok);}
  hud.close();GdiplusShutdown(token);
 }
 winrt::uninit_apartment();
}
