#pragma once
#include <atomic>
#include <array>
#include <ostream>
#include <algorithm>
#include <string>
#include <cstdint>
#include <iterator>
namespace defense {
enum Kind{Toggle,Number,Key,Choice};
enum Id {Guard,Gap,Reaction,Activation,NativeGuard,Sprint,Acquire,Cutoff,Adaptive,AntiPush,RimPress,RimRange,RimGap,RimBlock,Humanize,Softness,Idle,Coverage,Route,Screens,ScreenVisual,Lock,Offball,OffDelay,OffGap,SelectAtStart,Steal,StealReach,StealDelay,StealCooldown,StealKey,Intercept,InterceptAuto,InterceptMove,InterceptReach,InterceptJump,InterceptShots,InterceptKey,Recovery,RecoveryRange,RecoveryWindow,Block,BlockDelay,BlockCommit,BlockOnce,BlockRange,JumpKey,JumpCooldown,Oop,OopMove,OopGuide,OopRange,Rebound,ReboundChase,ReboundEarly,ReboundSpot,ReboundRange,ReboundReach,ReboundHeight,ReboundLead,Response,FakeFilter,SwitchKey,PracticeAI,AntiPost,Count};
struct Option{const char*key;const wchar_t*name;const wchar_t*hint;int card;Kind kind;int initial,lo,hi,step;float scale;bool advanced;std::atomic<int> value;
 Option(const char*k,const wchar_t*n,const wchar_t*h,int c,Kind type,int d,int low=0,int high=1,int st=1,float sc=1,bool adv=false):key(k),name(n),hint(h),card(c),kind(type),initial(d),lo(low),hi(high),step(st),scale(sc),advanced(adv),value(d){}
};
inline Option options[]={
 {"guard",L"Auto guard",L"Hold Activation for basket-side coverage. Sharp backward cuts trigger immediate brief pressure; shot cues tighten the fadeaway closeout. Requires a known ball handler and goal.",0,Toggle,0},
 {"gap",L"Stand-off",L"Desired basket-side distance from the handler, in studs. Shot cues temporarily close the gap to 2 studs.",0,Number,500,200,800,25,.01f},
 {"reaction",L"Delay",L"Wait before following a newly acquired matchup, in milliseconds. Zero responds immediately; this does not delay ongoing tracking.",0,Number,0,0,400,20,1,true},
 {"activation",L"Activation",L"Hold to engage on-ball or selected off-ball guarding. Releasing stops movement immediately.",0,Key,'G',1,254},
 {"guard_input",L"In-game guard key",L"The actual guard binding configured in Roblox. Uses key down/up; no remote calls.",0,Key,'G',1,254,1,1,true},
 {"sprint",L"Sprint to catch up",L"Holds normal Shift when the guard destination is more than 3.5 studs away, or during ball pursuit. Close coverage uses ordinary steps.",0,Toggle,1,0,1,1,1,true},
 {"acquire",L"Target range",L"Maximum distance for acquiring a ball handler, in studs.",0,Number,35,8,60,1,1,true},
 {"cutoff",L"Cut off drives",L"Lead the handler's observed motion toward the defended basket.",0,Toggle,0},
 {"adaptive",L"Adaptive spacing",L"Back up against a fast approach; close toward the normal stand-off when they slow down.",0,Toggle,1,0,1,1,1,true},
 {"anti_push",L"Avoid rim contact",L"Keep a non-contact gap near the rim. Cannot prevent the server from moving a character.",0,Toggle,1,0,1,1,1,true},
 {"rim_press",L"Press near the rim",L"Reduce stand-off near the verified target goal.",0,Toggle,1,0,1,1,1,true},
 {"rim_range",L"Rim press range",L"Distance from the hoop that activates closer coverage, in studs.",0,Number,18,8,30,1,1,true},
 {"rim_gap",L"Rim press distance",L"Close coverage gap in studs. Contact avoidance can increase it.",0,Number,130,75,300,25,.01f,true},
 {"rim_block",L"Block nearby dunks",L"Jump only for a high ball near the rim and an airborne handler. Shares the block/jump cooldown.",0,Toggle,0,0,1,1,1,true},
 {"human",L"Movement softness",L"Smooth small steering corrections using the latest position every frame. Reversals respond immediately; there is no observation delay.",0,Toggle,1,0,1,1,1,true},
 {"softness",L"Softness amount",L"Controls small-turn smoothing only. Does not add a start delay or delay position updates.",0,Number,60,0,100,5,1,true},
 {"idle",L"Idle stance adjustments",L"Small occasional lateral stance changes while covering a stationary handler. Off by default; response presets turn this off.",0,Toggle,0,0,1,1,1,true},
 {"coverage",L"Coverage visual",L"A court-facing coverage ribbon. Coverage is a positioning estimate, not server contest.",0,Toggle,1},
 {"route",L"Guard destination",L"Walking path with stride marks, a guard-position ring, and amber opponent predictions at 175 and 350 ms. Predictions use accepted player motion and are estimates.",0,Toggle,1},
 {"screens",L"Screen avoidance",L"Route around player bodies intersecting the pursuit path. This is geometric avoidance, not game screen-state data.",1,Toggle,0},
 {"screen_visual",L"Screen warning",L"Mark the player currently obstructing the guard route.",1,Toggle,1},
 {"lock",L"Target lock",L"Keep the selected matchup through passes. Use Select matchup to change it.",2,Toggle,0},
 {"offball",L"Off-ball guard",L"Hold Activation to deny the selected receiver. If that receiver gains possession, on-ball Auto guard takes over.",3,Toggle,0},
 {"off_delay",L"Reaction delay",L"Delay before tracking a new receiver, in milliseconds.",3,Number,200,0,600,25},
 {"off_gap",L"Passing-lane gap",L"Stand this far from the receiver toward the ball handler, in studs.",3,Number,250,100,500,25,.01f},
 {"select_start",L"Select at match start",L"Show click-to-select when an InGame BoolValue changes to true. If unavailable, use Select matchup manually.",3,Toggle,1},
 {"steal",L"Auto steal",L"Attempt the normal steal input only while a visible ball is within reach and in front. Guarding alone never steals.",4,Toggle,0},
 {"steal_reach",L"Attempt distance",L"Geometry threshold in studs. Does not enlarge the server hitbox.",4,Number,350,100,600,25,.01f},
 {"steal_delay",L"Opportunity delay",L"Require a continuous valid opportunity for this many milliseconds.",4,Number,120,50,500,25},
 {"steal_cooldown",L"Attempt cooldown",L"Minimum time between steal/catch attempts, in milliseconds.",4,Number,1600,600,3000,100},
 {"steal_key",L"In-game steal key",L"Normal steal/catch input. Side mouse buttons are supported.",4,Key,'R',1,254},
 {"intercept",L"Pass interception",L"Hold the interception key to pursue a reachable loose flight. Classification uses mirrored shot values when present, otherwise conservative geometry.",5,Toggle,0},
 {"intercept_auto",L"Automatic catches",L"Requires Auto steal as well as Pass interception. No automatic steal is sent with Auto steal off.",5,Toggle,0},
 {"intercept_move",L"Move into flight path",L"Run toward a predicted reachable ball position. Manual WASD temporarily takes priority.",5,Toggle,1},
 {"intercept_reach",L"Attempt distance",L"Distance for a normal catch attempt. Does not change the game's catch radius.",5,Number,400,200,600,25,.01f},
 {"intercept_jump",L"Jump for high passes",L"Allow one normal jump for a reachable high ball. Low balls never trigger Space.",5,Toggle,0,0,1,1,1,true},
 {"intercept_shots",L"Include shots",L"Also consider loose flights identified as shots. Alley-oop planning has higher priority.",5,Toggle,0,0,1,1,1,true},
 {"intercept_key",L"Interception hold key",L"Hold to request pass pursuit. The in-game steal key still performs the catch.",5,Key,'R',1,254},
 {"recovery",L"Loose-ball marker",L"Visual only: mark the nearest loose ball for a short period after a steal attempt. Never dives.",6,Toggle,0},
 {"recovery_range",L"Marker range",L"Maximum nearby-ball distance in studs.",6,Number,12,4,24},
 {"recovery_window",L"After-steal window",L"How long the marker remains eligible after an attempt, in milliseconds.",6,Number,3000,1000,5000,250},
 {"block",L"Auto block",L"Jump during the early advancing shot gather. Late charge, released balls and missed windows are skipped. Requires upward lift, ground contact, reach and facing.",7,Toggle,0},
 {"block_delay",L"Reaction delay",L"Minimum time since advancing charge was detected. The attempt is skipped if this delay misses the windup window. Guard acquisition delay is not added.",7,Number,80,0,500,10},
 {"block_commit",L"Committed-shot filter",L"Require upward release velocity near a known goal. Reduces false positives but can miss shots.",7,Toggle,1},
 {"block_once",L"One attempt per flight",L"Do not retry a block on the same observed flight. All attempts also share a jump cooldown.",7,Toggle,1},
 {"block_range",L"Block distance",L"Maximum horizontal distance from the high ball, in studs.",7,Number,500,200,800,25,.01f,true},
 {"jump_key",L"In-game jump key",L"Normal Space input by default. Sent only for high balls to avoid low-ball dives.",7,Key,32,1,254,1,1,true},
 {"jump_cooldown",L"Jump cooldown",L"Shared minimum time between all defensive jump requests, in milliseconds.",7,Number,1200,600,2500,100,1,true},
 {"oop",L"Anti alley-oop",L"Track a lob, pursue a reachable point, and jump after observed backboard deflection. Without lob metadata only near-board returns qualify.",8,Toggle,0},
 {"oop_move",L"Move toward lobs",L"Pursue the predicted reachable high-ball point. Overrides ordinary guarding while a lob is reserved.",8,Toggle,1},
 {"oop_guide",L"Interception guide",L"Draw the selected meeting point and route on the court.",8,Toggle,1},
 {"oop_range",L"Detection distance",L"Maximum nearby lob distance in studs.",8,Number,18,8,26},
 {"rebound",L"Auto rebound",L"Pursue descending loose balls and jump only within reach. Never presses Space for floor-level balls.",9,Toggle,0},
 {"rebound_chase",L"Pursue nearby balls",L"Move toward a reachable predicted descending-ball position. Manual WASD takes priority.",9,Toggle,1},
 {"rebound_early",L"Early rim positioning",L"Move to a rim-side spot on a recognized shot while it is still rising.",9,Toggle,0},
 {"rebound_spot",L"Rim spot distance",L"Early positioning distance from the rim, in studs.",9,Number,550,300,900,50,.01f,true},
 {"rebound_range",L"Pursuit distance",L"Maximum ball pursuit distance in studs.",9,Number,12,4,20},
 {"rebound_reach",L"Jump distance",L"Maximum horizontal jump-attempt distance, in studs.",9,Number,500,200,1000,50,.01f},
 {"rebound_height",L"Ball height",L"Maximum ball height above the root for a rebound jump, in studs.",9,Number,700,300,1200,50,.01f},
 {"rebound_lead",L"Flight lookahead",L"Additional descending-ball lookahead, in milliseconds.",9,Number,150,0,350,25,1,true},
 {"response",L"Mode",L"Legit: softer steering, 80 ms acquisition, block window 50-85%. Balanced: moderate steering, 20 ms, 40-80%. Blatant: direct steering, 0 ms, 30-75%. All require block reach/height checks. Stand-off stays unchanged; Delay can be adjusted afterward.",0,Choice,1,0,2},
 {"fake_filter",L"Shot-fake filter",L"Legacy saved option. Gather qualification is always enabled; meter-driven blocks require late charge plus a high rising ball.",0,Toggle,1,0,1,1,1,true},
 {"switch_key",L"Switch matchup",L"Press to cycle nearby opponents while Roblox is focused. Default B.",2,Key,'B',1,254},
 {"practice_ai",L"AI practice opponents",L"Treat named practice bots (AI1, AI2...) with a humanoid and explicit goal as opponents, even with a shared goal. Turn off for team-based AI matches. Human team checks remain active.",0,Toggle,0,0,1,1,1,true},
 {"anti_post",L"Anti-post",L"Read sustained back-to-basket positioning, stay close and give space when the handler backs into you. Uses facing and movement estimates; cannot guarantee push prevention. Shot cues take priority.",0,Toggle,1}
};
static_assert(std::size(options)==Count);
inline int get(Id i){return options[i].value.load();}inline float val(Id i){return get(i)*options[i].scale;}inline bool on(Id i){return get(i)!=0;}
inline void applyResponsePreset(int mode){mode=std::clamp(mode,0,2);options[Response].value=mode;options[Reaction].value=mode==0?80:mode==1?20:0;options[Softness].value=mode==0?90:mode==1?60:0;options[Humanize].value=mode!=2;options[Idle].value=0;}
inline bool any(){return on(Guard)||on(Offball)||on(Steal)||on(Intercept)||on(Block)||on(Oop)||on(Rebound)||on(Recovery);}
inline void writeSettings(std::ostream&o){for(auto&x:options)o<<",\"def_"<<x.key<<"\":"<<x.value.load();}
template<class Getter>void loadSettings(Getter get){for(auto&x:options){std::string key="def_";key+=x.key;int v=std::clamp(get(key.c_str(),x.initial),x.lo,x.hi);if(x.kind==Key&&(v==27||v==119||v==91||v==92))v=x.initial;x.value=v;}}
inline std::array<std::atomic<bool>,256> physical{},changed{};inline std::atomic<unsigned> clicks{0};inline std::atomic<bool> chat{false},pick{false};inline std::atomic<uintptr_t> selected{0};
inline void physicalEdge(int key,bool down){if(key>0&&key<256){if(physical[key].exchange(down)!=down){changed[key]=true;if(down&&key==1)++clicks;}}}
inline bool held(int key){if(key==16)return physical[160]||physical[161];if(key==17)return physical[162]||physical[163];if(key==18)return physical[164]||physical[165];return key>0&&key<256&&physical[key].load();}
}






