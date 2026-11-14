// Exercise the real form handlers and settings persistence without creating a
// window, installing input hooks, starting readers, or sending input.
#include "main.cpp"
#include <iostream>
#include <cstdlib>
static int checks=0;
static void check(bool ok,const char* why){++checks;if(!ok){std::cerr<<"FAIL: "<<why<<"\n";std::exit(1);}}
int main(){
 wchar_t temp[MAX_PATH];GetTempPathW(MAX_PATH,temp);
 cfg=fs::path(temp)/(L"vanta-settings-test-"+std::to_wstring(GetCurrentProcessId())+L".json");
 shootKey='E';shootKeyAlt=VK_SPACE;trim=37;save();
 shootKey='Q';shootKeyAlt=VK_XBUTTON1;trim=0;loadGameplay(cfg);
 check(shootKey=='E'&&shootKeyAlt==VK_SPACE&&trim==37,"both bindings survive save/load");
 shootKeyAlt=VK_XBUTTON2;save();shootKeyAlt=0;loadGameplay(cfg);check(shootKeyAlt==VK_XBUTTON2,"mouse alternate binding persists");
 {std::ofstream out(cfg);out<<"{\"shoot_key\":69,\"earlier_ms\":37}";}
 loadGameplay(cfg);check(shootKey=='E'&&shootKeyAlt==VK_SPACE,"legacy config gains Space without changing primary");
 {std::ofstream out(cfg);out<<"{\"shoot_key\":69,\"shoot_key_alt\":69}";}
 loadGameplay(cfg);check(shootKeyAlt==0,"duplicate bindings do not create two handlers");
 std::error_code ec;fs::remove(cfg,ec);
 launchFlow.move(LaunchFlow::Login);LRESULT result;loginFocus=0;
 launchEvent(nullptr,WM_CHAR,'1',0,result);
 launchEvent(nullptr,WM_KEYDOWN,VK_TAB,0,result);
 launchEvent(nullptr,WM_CHAR,'2',0,result);
 launchEvent(nullptr,WM_KEYDOWN,VK_RETURN,0,result);
 check(launchFlow.invalid&&launchFlow.stage==LaunchFlow::Login,"form rejects incorrect password");
 launchEvent(nullptr,WM_CHAR,8,0,result);launchEvent(nullptr,WM_CHAR,'1',0,result);
 launchEvent(nullptr,WM_KEYDOWN,VK_RETURN,0,result);
 check(launchFlow.stage==LaunchFlow::Verifying&&loginPassword.empty(),"form accepts preview credentials and clears password");
 linked=false;launchFlow.tick(.5f,false,false);launchEvent(nullptr,WM_KEYDOWN,VK_RETURN,0,result);
 check(launchFlow.stage==LaunchFlow::Connect,"Enter cannot bypass missing Roblox");
 linked=true;launchEvent(nullptr,WM_KEYDOWN,VK_RETURN,0,result);
 check(launchFlow.stage==LaunchFlow::Entering,"Enter continues when connected");
 launchFlow.move(LaunchFlow::Login);loginFocus=2;loginUser=L"1";loginPassword=L"1";
 launchEvent(nullptr,WM_CHAR,'x',0,result);
 check(loginUser==L"1"&&loginPassword==L"1","button focus cannot append text to credentials");
 launchEvent(nullptr,WM_KEYDOWN,VK_TAB,0,result);
 check(loginFocus==0,"keyboard navigation wraps from submit to username");
 loginShowPassword=false;int eyeX=(int)((launchPassBox.x+launchPassBox.w-21)*scale),eyeY=(int)((launchPassBox.y+22)*scale);
 launchEvent(nullptr,WM_LBUTTONDOWN,0,MAKELPARAM(eyeX,eyeY),result);
 check(loginShowPassword&&loginPassword==L"1","visibility button changes display without editing password");
 loginFocus=0;loginUser=L"old";loginSelect=true;launchFlow.invalid=true;
 pasteLoginText(L"new\r\nname\t",10);
 check(loginUser==L"newname"&&!loginSelect&&!launchFlow.invalid,"paste replaces selection and excludes control characters");
 std::wstring longPaste(500,L'x');pasteLoginText(longPaste.c_str(),longPaste.size());
 check(loginUser.size()==40,"paste respects credential length limit");
 loginFocus=2;pasteLoginText(L"ignored",7);
 check(loginUser.size()==40,"button focus cannot paste into credentials");
 loginUser=L"1";loginPassword=L"1";loginShowPassword=true;
 launchEvent(nullptr,WM_KEYDOWN,VK_SPACE,0,result);
 check(launchFlow.stage==LaunchFlow::Verifying&&loginPassword.empty()&&!loginShowPassword,"Space submits focused button and resets password visibility");
 pasteLoginText(L"ignored",7);
 check(loginPassword.empty(),"paste cannot modify credentials while verifying");
 std::cout<<checks<<" form and persistence checks passed (no UI or gameplay input)\n";
}
