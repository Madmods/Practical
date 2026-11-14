from pathlib import Path
import ctypes as c, ctypes.wintypes as w, tempfile, subprocess, shutil, time, json
u=c.WinDLL('user32',use_last_error=True)
u.EnumWindows.argtypes=[c.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM),w.LPARAM]
u.GetWindowThreadProcessId.argtypes=[w.HWND,c.POINTER(w.DWORD)]
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM];u.SendMessageW.restype=w.LPARAM
u.GetClientRect.argtypes=[w.HWND,c.POINTER(w.RECT)]
p=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='vanta-shooting-qa-') as tmp:
 d=Path(tmp);shutil.copy(p/'Native'/'build'/'Vanta-preview.exe',d/'Vanta-test.exe')
 (d/'bat-timing-settings.json').write_text('{"earlier_ms":37,"shoot_key":74}')
 proc=subprocess.Popen([str(d/'Vanta-test.exe'),'--ui-test']);time.sleep(.7)
 found=[]
 @c.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM)
 def enum(hwnd,arg):
  pid=w.DWORD();u.GetWindowThreadProcessId(hwnd,c.byref(pid))
  if pid.value==proc.pid: found.append(hwnd)
  return True
 u.EnumWindows(enum,0);assert found,'Test window missing';h=found[0]
 rect=w.RECT();u.GetClientRect(h,c.byref(rect));scale=rect.right/960
 def move(x,y):u.SendMessageW(h,0x200,0,(round(x*scale)&65535)|(round(y*scale)<<16))
 def click(x,y):
  lp=(round(x*scale)&65535)|(round(y*scale)<<16)
  u.SendMessageW(h,0x201,1,lp);u.SendMessageW(h,0x202,0,lp)
 def offset():return u.SendMessageW(h,0x8017,0,0)/1000
 def track_y(i):return u.SendMessageW(h,0x8019,i,0)/1000
 def header_y(i):return u.SendMessageW(h,0x801a,i,0)/1000
 def wheel(delta):
  move(650,450);u.SendMessageW(h,0x20a,(delta&65535)<<16,0);time.sleep(.55)
 def config():return json.loads((d/'bat-timing-settings.json').read_text())
 def search(text):
  click(810,72)
  for ch in text:u.SendMessageW(h,0x102,ord(ch),0)
  u.SendMessageW(h,0x100,13,0);time.sleep(.55)
 try:
  move(930,560);time.sleep(.6)
  # Direct entry replaces selected text, clamps bounds, and Escape cancels.
  click(875,244)
  for ch in '47':u.SendMessageW(h,0x102,ord(ch),0)
  u.SendMessageW(h,0x100,13,0);assert config()['earlier_ms']==47
  click(875,244);u.SendMessageW(h,0x102,ord('2'),0);u.SendMessageW(h,0x100,27,0)
  assert config()['earlier_ms']==47
  click(875,244)
  for ch in '999':u.SendMessageW(h,0x102,ord(ch),0)
  u.SendMessageW(h,0x100,13,0);assert config()['earlier_ms']==60
  click(824,244);assert config()['earlier_ms']==60
  click(310,296);assert config()['max_ping']==50
  click(824,348);assert config()['freshness_ms']==200
  click(220,76);time.sleep(.55)
  click(102+59/99*810,270);conf=config();assert conf['minimum_percent']==60 and conf['maximum_percent']==100,conf
  assert conf['earlier_ms']==60
  click(890,305);assert config()['hold_early']==0
  move(890,350);time.sleep(.5);assert u.SendMessageW(h,0x8012,0,0)==0
  move(160,351);time.sleep(.5);assert u.SendMessageW(h,0x8012,0,0)>20
  move(600,392);time.sleep(.5);assert u.SendMessageW(h,0x8012,0,0)==0
  move(150,392);time.sleep(.5);assert u.SendMessageW(h,0x8012,0,0)>20
  click(824,392);assert config()['overhold_lead_ms']==120
  click(350,header_y(2)+24);time.sleep(.5)
  assert u.SendMessageW(h,0x8018,0,0)&6==6,'Opening Contest collapsed Shot assist'
  wheel(-360);assert offset()>60,'Expanded cards did not scroll'
  click(890,header_y(2)+142);assert config()['closeout']==1
  click(824,track_y(3));assert config()['contest_limit']==100
  # Scroll back to the first card and collapse it independently.
  wheel(360);click(350,header_y(1)+24);time.sleep(.5)
  assert u.SendMessageW(h,0x8018,0,0)&6==4
  assert offset()<1,'Scroll did not clamp after collapse' 
  # A collapsed assist row cannot modify hidden settings.
  before=(d/'bat-timing-settings.json').read_text();click(180,270)
  assert (d/'bat-timing-settings.json').read_text()==before
  search('meter freshness');click(310,348);assert config()['freshness_ms']==60
  search('shot assist');click(730,595)
  conf=config();assert conf['minimum_percent']==80 and conf['maximum_percent']==100 and conf['contest_limit']==100 and conf['overhold_lead_ms']==40
  search('contest ceiling');click(730,595);assert config()['contest_limit']==30
  search('pump fake punish')
  assert u.SendMessageW(h,0x8018,0,0)&9==9,'Opening Pump collapsed Auto time'
  assert offset()>150,'Search did not reveal expanded Pump controls'
  click(824,track_y(5));assert config()['pump_range']==16
  click(824,track_y(6));assert config()['pump_delay']==300
  click(824,track_y(7));assert config()['pump_contest']==30
  click(890,header_y(3)+226);assert config()['pump_visuals']==0
  click(730,595);assert config()['pump_range']==12 and config()['pump_delay']==120 and config()['pump_visuals']==1
  assert config()['earlier_ms']==60
  # Drag the scrollbar to the top, then back to the bottom.
  click(946,160);time.sleep(.1);assert offset()<1
  click(946,560);time.sleep(.1);assert offset()>150
  move(140,track_y(6));time.sleep(.5);assert u.SendMessageW(h,0x8012,0,0)>20,'Scrolled tooltip not aligned'
  beforeScroll=config();click(824,140);assert config()==beforeScroll,'Clipped control received a click'
  search('ball arc');click(890,188);assert config()['ball_arc']==0
  click(890,250);assert config()['ball_landing']==0
  click(890,330);assert config()['rolling_intercept']==0
  move(500,330);time.sleep(.5);assert u.SendMessageW(h,0x8012,0,0)==0
  move(160,330);time.sleep(.5);assert u.SendMessageW(h,0x8012,0,0)>20
  assert u.SendMessageW(h,0x8015,0,0)==4
  # Hidden shooting controls must not change on the Visuals tab.
  before=config();click(500,392);assert config()==before
  move(160,188);time.sleep(.5);assert u.SendMessageW(h,0x8012,0,0)>20
  click(730,595);assert config()['ball_arc']==1 and config()['ball_landing']==1 and config()['rolling_intercept']==1
  assert config()['earlier_ms']==60
  # System and the top tab reach the same independent settings page.
  before=config()
  move(28,129);time.sleep(.7);click(110,131);time.sleep(.5)
  assert u.SendMessageW(h,0x8016,2,0)>990
  click(209,133);time.sleep(.5);assert u.SendMessageW(h,0x8016,2,0)<2
  click(110,133);time.sleep(.5);assert u.SendMessageW(h,0x8016,2,0)>990
  click(125,201);time.sleep(.4);assert u.SendMessageW(h,0x8013,0,0)//10000==6
  click(125,169);time.sleep(.4);assert u.SendMessageW(h,0x8013,0,0)//10000==3
  move(64,81);time.sleep(.025);click(65,132);time.sleep(.4)
  assert u.SendMessageW(h,0x8013,0,0)//10000==0
  move(175,81);time.sleep(.025);click(65,132);time.sleep(.4)
  assert u.SendMessageW(h,0x8013,0,0)//10000==3
  click(90,545);move(930,560);time.sleep(.7)
  assert u.SendMessageW(h,0x8011,0,0)%10000>990,'Pinned navigation collapsed'
  assert (d/'vanta-theme.txt').read_text().split()[11]=='1'
  click(90,545);move(930,560);time.sleep(.7)
  assert u.SendMessageW(h,0x8011,0,0)%10000<2,'Unpinned navigation stayed open'
  assert u.SendMessageW(h,0x8013,0,0)//10000==3
  click(741.37,260)
  tokens=(d/'vanta-theme.txt').read_text().split();assert int(tokens[4])==50,tokens
  click(916,325);assert int((d/'vanta-theme.txt').read_text().split()[3])==100
  move(120,365);time.sleep(.5);assert u.SendMessageW(h,0x8012,0,0)==0
  move(120,188);time.sleep(.5);assert u.SendMessageW(h,0x8012,0,0)>20
  move(120,365)
  oldAccent=u.SendMessageW(h,0x8014,0,0)
  click(120,365) # Indigo palette
  immediate=u.SendMessageW(h,0x8014,0,0)
  assert immediate!=0xa29ded,'Palette snapped to target'
  time.sleep(.12);middle=u.SendMessageW(h,0x8014,0,0)
  assert middle not in [oldAccent,0xa29ded],'Palette did not interpolate'
  time.sleep(.3);assert u.SendMessageW(h,0x8014,0,0)==0xa29ded
  assert u.SendMessageW(h,0x8012,0,0)==0
  tokens=(d/'vanta-theme.txt').read_text().split();assert int(tokens[0])==0xa29ded,tokens
  # Original Moonlight remains available after choosing another palette.
  click(230,245);time.sleep(.35)
  tokens=(d/'vanta-theme.txt').read_text().split()
  assert [int(v) for v in tokens[:3]]==[0x899aff,0x292e43,0x191e2f],tokens
  assert u.SendMessageW(h,0x8014,0,0)==0x899aff
  click(120,365);time.sleep(.35);click(230,245);time.sleep(.35)
  assert u.SendMessageW(h,0x8014,0,0)==0x899aff
  click(890,465);assert u.SendMessageW(h,0x8013,0,0)%10000==90
  move(600,392);time.sleep(.5);assert u.SendMessageW(h,0x8012,0,0)==0
  click(890,511);assert u.SendMessageW(h,0x8013,0,0)%10000==2090
  click(878,404) # 110% size; use the new physical coordinates afterward
  u.GetClientRect(h,c.byref(rect));newScale=rect.right/960
  assert newScale>scale;scale=newScale
  assert u.SendMessageW(h,0x8013,0,0)%10000==2110
  assert config()==before,'UI settings changed gameplay config'
  click(730,595);u.GetClientRect(h,c.byref(rect));scale=rect.right/960
  assert u.SendMessageW(h,0x8013,0,0)%10000==1090
  assert config()==before,'Appearance reset changed gameplay config'
  # Settings top tabs contain Appearance and Window, not shooting panels.
  click(245,76);time.sleep(.3);assert u.SendMessageW(h,0x8013,0,0)//10000==6
  click(890,194)
  u.GetWindowLongPtrW.argtypes=[w.HWND,c.c_int];u.GetWindowLongPtrW.restype=c.c_ssize_t
  assert u.GetWindowLongPtrW(h,-20)&8,'Always on top not applied'
  click(890,258);click(890,322)
  tokens=(d/'vanta-theme.txt').read_text().split();assert tokens[8:11]==['1','0','1'],tokens
  # Native Guarding section has no shooting hit targets.
  search('guarding');assert u.SendMessageW(h,0x8013,0,0)//10000==4
  beforeGuard=config();click(890,188);assert config()==beforeGuard
  click(860,596);time.sleep(.35);assert u.SendMessageW(h,0x8013,0,0)//10000==5
  search('configs');assert u.SendMessageW(h,0x8013,0,0)//10000==5
  click(220,455)
  for ch in 'Test setup':u.SendMessageW(h,0x102,ord(ch),0)
  click(750,455)
  assert (d/'Configs'/'Test setup'/'settings.json').exists()
  assert (d/'Configs'/'Test setup'/'appearance.txt').exists()
  saved=config()
  search('release adjustment');click(310,244);assert config()['earlier_ms']==-30
  search('configs');click(250,240);click(860,455)
  assert config()==saved,'Profile did not restore settings'
  assert u.GetWindowLongPtrW(h,-20)&8
  search('appearance')
  assert u.SendMessageW(h,0x8013,0,0)//10000==3
  # Restart an isolated UI-only process to verify loading saved preferences.
  click(790,404);u.GetClientRect(h,c.byref(rect));scale=rect.right/960
  click(890,465)
  u.SendMessageW(h,0x10,0,0);proc.wait(timeout=5)
  proc=subprocess.Popen([str(d/'Vanta-test.exe'),'--ui-test']);time.sleep(.7)
  found.clear();u.EnumWindows(enum,0);assert found;h=found[0]
  assert u.SendMessageW(h,0x8013,0,0)==100,'Saved UI preferences not restored'
  assert config()==before
  print('UI checks passed: contextual navigation, guarding section, profile round-trip, native topmost,  shooting regressions, settings navigation, palettes, opacity, size, reduced motion, tooltip toggle and appearance-only reset.')
 finally:
  u.SendMessageW(h,0x10,0,0);proc.wait(timeout=5)
