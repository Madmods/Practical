static bool typed(){return query.find_first_not_of(L" \t")!=std::wstring::npos;}
static const std::vector<int>& matches(){
 static std::wstring last;static std::vector<int> ids;
 if(last==query)return ids;last=query;ids.clear();if(!typed())return ids;
 auto lower=[](std::wstring v){for(auto&ch:v)ch=towlower(ch);return v;};
 std::wistringstream ss(lower(query));std::vector<std::wstring> words;std::wstring word;while(ss>>word)words.push_back(word);
 std::vector<std::pair<int,int>> ranked;
 for(int i=0;i<(int)std::size(indexList);i++){auto&e=indexList[i];auto n=lower(e.name),all=lower(n+L" "+e.group+L" "+e.tags);int score=0;
 for(auto&w:words){if(all.find(w)==std::wstring::npos){score=-1000;break;}score+=n.find(w)==0?8:n.find(w)!=std::wstring::npos?4:1;}
 if(score>=0)ranked.emplace_back(-score,i);}
 std::sort(ranked.begin(),ranked.end());for(auto&v:ranked)ids.push_back(v.second);return ids;}
static R finderBox{746,58,191,28};static float searchOpen=0,searchFocus=0,selectionY=0;
static void openFinder(){finding=true;matchPos=0;query.clear();openDrop=dropShown=0;showUser=false;searchOpen=selectionY=0;}
static void jump(int i){auto e=indexList[i];selectFeature(e.tab,e.offset);finding=false;}
static void drawFinderButton(){R r=finderBox;fillR(r,6,C(visualTheme[1],.25f+.15f*searchFocus));strokeR(r,6,C(visualTheme[0],.14f+.36f*searchFocus));ring(r.x+12,r.y+12,3.8f,C(0xcad4e9),1.25f);line(r.x+15,r.y+15,r.x+18,r.y+18,C(0xcad4e9),1.25f);
 auto text=finding?(query.empty()?std::wstring(L"Type to search"):query.substr(query.size()>19?query.size()-19:0)):std::wstring(L"Search");if(finding&&int(tsec*2)%2==0)text+=L"│";T(text,F_SMALL,r.x+26,r.y,finding?160:103,28,C(0xf7f9ff));if(!finding){R key{r.x+r.w-47,r.y+5,41,18};fillR(key,4,C(visualTheme[0],.075f));strokeR(key,4,C(visualTheme[0],.13f),.7f);T(L"Ctrl K",F_CTRL,key.x,key.y,key.w,key.h,C(0xb7c6df));}}
static int firstMatch(){return std::max(0,matchPos-4);}
struct FindLayout{int n;R box;explicit FindLayout(int count):n(std::min(5,count)),box{559,92,378,n?46+42.f*n:74}{}};
static void drawFinder(){if(!finding||!typed())return;auto&v=matches();FindLayout l((int)v.size());int a=firstMatch();
 dc->PushAxisAlignedClip({l.box.x-1,l.box.y-1,939,l.box.y+l.box.h*searchOpen+1},D2D1_ANTIALIAS_MODE_ALIASED);fade=searchOpen;
 fillR(l.box,8,ink(blend(visualTheme[2],0x11141c,.55f),.99f));strokeR(l.box,8,ink(blend(visualTheme[1],visualTheme[0],.35f),.82f));
 if(!l.n){Tleft(L"No settings found",F_TAB,576,111,C(0xf1f5fc));Tleft(L"Try a feature name or a shorter search.",F_SMALL,576,132,C(0xd9e3f2));}
 else{
 float sy=98+selectionY*42;fillR({566,sy,364,40},5,ink(visualTheme[0],.17f));fillR({567,sy+8,2,15},1,ink(visualTheme[0]));
 for(int j=0;j<l.n;j++){auto&e=indexList[v[a+j]];float y=98+42.f*j;bool active=a+j==matchPos;
 if(!active&&inR({566,y,364,40},mx,my))fillR({566,y,364,40},5,ink(visualTheme[0],.07f));
 T(e.name,F_TAB,579,y+2,280,20,C(active?0xffffff:0xf2f5fb));T(e.group,F_SMALL,579,y+21,280,17,C(0xa7b6d0));if(active||inR({566,y,364,40},mx,my)){line(914,y+12,918,y+16,C(0xeaf1ff));line(918,y+16,914,y+20,C(0xeaf1ff));}}
 }
 if(l.n){float fy=104+42.f*l.n;line(574,fy,922,fy,C(visualTheme[0],.14f));Tleft(L"Enter to open",F_SMALL,579,fy+17,C(0xa7b6d0));T(L"Esc to close",F_SMALL,805,fy+4,112,26,C(0xa7b6d0));}
 fade=1;dc->PopAxisAlignedClip();}
static bool finderClick(float x,float y){auto&v=matches();FindLayout l((int)v.size());if(!typed()||!inR(l.box,x,y))return false;
 int j=(int)floor((y-98)/42);if(j>=0&&j<l.n)jump(v[firstMatch()+j]);return true;}
