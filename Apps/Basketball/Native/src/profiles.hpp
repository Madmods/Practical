#pragma once
static std::wstring profileName,profileStatus;static bool profileEditing=false;static int profileSelected=-1,profileOffset=0;static std::vector<std::wstring>profileNames;
static fs::path profilesRoot(){return cfg.parent_path()/L"Configs";}
static void refreshProfiles(){profileNames.clear();std::error_code ec;if(!fs::exists(profilesRoot(),ec))return;for(auto it=fs::directory_iterator(profilesRoot(),ec);!ec&&it!=fs::directory_iterator();it.increment(ec))if(it->is_directory(ec)&&fs::exists(it->path()/L"settings.json",ec))profileNames.push_back(it->path().filename().wstring());std::sort(profileNames.begin(),profileNames.end());profileOffset=std::clamp(profileOffset,0,std::max(0,(int)profileNames.size()-5));}
static void saveProfile(){
 auto name=profileName;while(!name.empty()&&name.back()==L' ')name.pop_back();while(!name.empty()&&name.front()==L' ')name.erase(name.begin());
 if(name.empty()){profileStatus=L"Enter a profile name first.";return;}
 if(name.find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-")!=std::wstring::npos){profileStatus=L"Use letters, numbers, spaces or dashes.";return;}
 save();saveTheme();std::error_code ec;auto dest=profilesRoot()/name;fs::create_directories(dest,ec);
 if(!ec)fs::copy_file(cfg,dest/L"settings.json",fs::copy_options::overwrite_existing,ec);
 if(!ec)fs::copy_file(cfg.parent_path()/L"vanta-theme.txt",dest/L"appearance.txt",fs::copy_options::overwrite_existing,ec);
 if(ec){profileStatus=L"Could not save this profile.";return;}
 profileName=name;refreshProfiles();auto it=std::find(profileNames.begin(),profileNames.end(),name);profileSelected=(int)(it-profileNames.begin());profileOffset=std::max(0,profileSelected-4);profileStatus=L"Saved "+name;announce(L"Profile saved");
}
static void loadProfile(){if(profileSelected<0||profileSelected>=(int)profileNames.size()){profileStatus=L"Choose a profile first.";return;}if(gameDown||physicalDown){profileStatus=L"Release your shot input before loading.";return;}
 auto source=profilesRoot()/profileNames[profileSelected];loadGameplay(source/L"settings.json");loadAppearance(source/L"appearance.txt");save();themeChanged();applyUISize();applyWindowOptions();profileStatus=L"Loaded "+profileNames[profileSelected];announce(L"Profile loaded");}
static void drawProfiles(){static double checked=-10;if(now()-checked>1){refreshProfiles();checked=now();}float l=leftEdge();R r{l,164,936-l,384};panel(r);Tleft(L"Saved profiles",F_TAB,l+24,190,C(0xf6f8ff));Tleft(std::to_wstring(profileNames.size())+L" saved",F_SMALL,818,190,C(0xb9c9e4));line(l+20,212,916,212,C(visualTheme[0],.16f));
 if(profileNames.empty()){float cx=(l+936)/2;fillR({cx-29,253,58,54},12,C(visualTheme[0],.07f));strokeR({cx-14,266,27,22},4,C(visualTheme[0],.6f));strokeR({cx-8,273,27,22},4,C(visualTheme[0]));T(L"Your settings, saved together",F_CTRL,cx-180,318,360,25,C(0xe3ebfa));auto align=fmt[F_SMALL]->GetTextAlignment();fmt[F_SMALL]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);T(L"Name your setup below to create a profile.",F_SMALL,cx-180,345,360,25,C(0x9cadc7));fmt[F_SMALL]->SetTextAlignment(align);}
 for(int i=0;i<5&&i+profileOffset<(int)profileNames.size();i++){int idx=i+profileOffset;R row{l+20,224+39.f*i,896-l,34};bool on=idx==profileSelected;float hover=heat(row,inR(row,mx,my));fillR(row,6,C(visualTheme[0],on?.17f:.035f+.05f*hover));strokeR(row,6,C(visualTheme[0],on?.25f:.04f));if(on)fillR({row.x,row.y+7,2,20},1,C(visualTheme[0]));navIcon(5,row.x+20,row.y+17,C(on?visualTheme[0]:0x8293ae));Tleft(profileNames[idx],F_TAB,row.x+40,row.y+17,C(on?0xf7f9ff:0xcad5eb));}
 R input{l+24,438,936-l-280,34};fillR(input,6,C(visualTheme[2],.7f));strokeR(input,6,C(visualTheme[0],profileEditing?.8f:.3f));auto text=profileName.empty()&&!profileEditing?L"Profile name":profileName;if(profileEditing&&int(tsec*2)%2==0)text+=L"|";T(text,F_LABEL,input.x+12,input.y,input.w-24,input.h,C(0xeaf0ff));keycap({706,438,96,34},L"Save",inR({706,438,96,34},mx,my));keycap({814,438,98,34},L"Load",inR({814,438,98,34},mx,my));T(profileStatus,F_SMALL,l+24,490,864-l,32,C(0xc0cce3));}
static void clickProfiles(float x,float y){float l=leftEdge();profileEditing=inR({l+24,438,936-l-280,34},x,y);if(profileEditing)return;
 for(int i=0;i<5&&i+profileOffset<(int)profileNames.size();i++)if(inR({l+20,224+39.f*i,896-l,34},x,y)){profileSelected=i+profileOffset;profileName=profileNames[profileSelected];return;}
 if(inR({706,438,96,34},x,y))saveProfile();else if(inR({814,438,98,34},x,y))loadProfile();}
