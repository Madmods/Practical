static bool bench=false;static std::vector<double> intervals;
static void recordFrame(){if(!bench)return;static double first=now(),prev=first;double t=now();if(t-first>.6)intervals.push_back((t-prev)*1000);prev=t;
 static int nav=0;int step=int((t-first)*3);if(step!=nav){nav=step;PostMessage(wnd,WM_APP+2,step%3,0);}
 if(t-first>5 && intervals.size()){auto sorted=intervals;std::sort(sorted.begin(),sorted.end());double sum=0;for(auto d:sorted)sum+=d;
 std::ofstream(cfg.parent_path()/L"frame-test.txt")<<"fps "<<1000*sorted.size()/sum<<" median_ms "<<sorted[sorted.size()/2]<<" p95_ms "<<sorted[sorted.size()*95/100]<<" frames "<<sorted.size();
 PostMessage(wnd,WM_CLOSE,0,0);}}
