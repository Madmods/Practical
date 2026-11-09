local function create(class,props,parent)
    local o=Instance.new(class)
    for k,v in pairs(props) do o[k]=v end
    o.Parent=parent return o
end
local colors={bg=Color3.fromRGB(17,20,26),card=Color3.fromRGB(26,31,40),line=Color3.fromRGB(48,57,72),
    text=Color3.fromRGB(229,235,245),muted=Color3.fromRGB(142,156,178),accent=Color3.fromRGB(63,181,143)}
local panel=create("Frame",{Size=UDim2.fromOffset(450,622),Position=UDim2.new(0,24,0.5,-311),BackgroundColor3=colors.bg,BorderSizePixel=0},gui)
create("UICorner",{CornerRadius=UDim.new(0,12)},panel)
create("UIStroke",{Color=colors.line,Thickness=1},panel)
local function label(parent,value,y,height,size)
    return create("TextLabel",{Size=UDim2.new(1,-32,0,height),Position=UDim2.fromOffset(16,y),BackgroundTransparency=1,
        Text=value,TextSize=size or 13,TextColor3=colors.text,Font=Enum.Font.GothamMedium,TextWrapped=true,TextXAlignment=Enum.TextXAlignment.Left},parent)
end
local header=label(panel,"PRACTICAL  /  CONTROL",10,30,17) header.Active=true
label(panel,"Suite 2.3  ·  Peak V2.1 / movement / defense",39,22,11).TextColor3=colors.muted
local pages,nav={},{}
for i,name in ipairs({"Shooting","Movement","Defense","Stats"}) do
    local button=create("TextButton",{Size=UDim2.fromOffset(98,32),Position=UDim2.fromOffset(16+(i-1)*106,75),
        BackgroundColor3=colors.card,BorderSizePixel=0,Text=name,TextSize=12,TextColor3=colors.text,Font=Enum.Font.GothamMedium},panel)
    create("UICorner",{CornerRadius=UDim.new(0,6)},button)
    nav[name]=button
    pages[name]=create("ScrollingFrame",{ScrollBarThickness=3,CanvasSize=UDim2.fromOffset(0,name=="Defense" and 1820 or 380),BorderSizePixel=0,Position=UDim2.fromOffset(0,119),Size=UDim2.new(1,0,0,380),BackgroundTransparency=1,Visible=i==1},panel)
end
local function row(parent,title,y)
    local b=create("TextButton",{Size=UDim2.new(1,-32,0,40),Position=UDim2.fromOffset(16,y),BackgroundColor3=colors.card,
        BorderSizePixel=0,Text=title,TextSize=13,TextColor3=colors.text,Font=Enum.Font.GothamMedium},parent)
    create("UICorner",{CornerRadius=UDim.new(0,7)},b) return b
end
local toggle=row(pages.Shooting,"Auto Time · OFF",8)
label(pages.Shooting,"Uses the unchanged V2.1 release predictor.\nBat meter, rotation 270. No manual learning.",60,52,12).TextColor3=colors.muted
local contestLabel=label(pages.Shooting,"Last shot contest: waiting for game feedback",144,45,15)
local contestToggle=row(pages.Shooting,"Contest display · ON",203)
label(pages.Shooting,"Contest comes from actual shot feedback; it is not a live pre-shot estimate.",255,45,11).TextColor3=colors.muted
local status=label(panel,"Connecting...",508,47,12)
local metrics=label(pages.Stats,"No results yet",8,95,14)
label(pages.Stats,"Movement features are experimental. The timing predictor is backed up separately.",129,65,12).TextColor3=colors.muted
local featureStatus=label(pages.Stats,"Feature controller: waiting",205,80,12)
local close=row(panel,"Unload  ·  Ctrl + Alt + T hides panel",570)
local settings={boost=false,boostMultiplier=1.15,boostDuration=0.25,stamina=false,staminaFloor=100,
    guard=false,guardRange=20,guardGap=5,guardStrength=1,guardReaction=0.16,guardPrediction=0.08,
    guardHold=true,contest=true,visuals=true,closeout=true,closeGap=1.5,closeReaction=.12,
    steal=false,stealRange=3.5,stealDelay=.18,stealCooldown=1.4,
    blockCommitFilter=false,
    block=false,blockRange=5,blockDelay=.22,jumpCooldown=1.2,
    rebound=false,reboundRange=5,reboundHeight=7,
    guardSprint=true,sprintDistance=2.5,faceTarget=true,guardDeadzone=.65,
    stealAngle=55,stealMaxSpeed=12,blockOnce=true,
    reboundChase=false,reboundChaseRange=12,reboundLead=.15,liveContest=true,assignmentLock=false,fakeFilter=true,smartPosition=true,landing=true,manualResume=.12}
local guardKey=Enum.KeyCode.G
local boostToggle=row(pages.Movement,"Dribble boost · OFF (experimental)",0)
local staminaToggle=row(pages.Movement,"Infinite stamina · unavailable / unverified",188)
staminaToggle.Active=false
staminaToggle.AutoButtonColor=false
label(pages.Movement,"The old stamina bypass had no effect and has been removed.",294,24,11).TextColor3=colors.muted
local guardToggle=row(pages.Defense,"Auto guard · OFF (experimental)",0)
local holdToggle=row(pages.Defense,"Activation · hold G",320)
local function setting(parent,title,key,y,minimum,maximum,step,format)
    local caption=label(parent,title,y,21,12)
    local value=create("TextLabel",{Size=UDim2.fromOffset(105,25),Position=UDim2.new(1,-155,0,y+24),BackgroundTransparency=1,
        TextColor3=colors.text,TextSize=12,Font=Enum.Font.GothamMedium},parent)
    local function refresh() value.Text=string.format(format,key=="guardStrength" and settings[key]*100 or settings[key]) end
    local buttons={}
    for i,delta in ipairs({-step,step}) do
        local b=create("TextButton",{Size=UDim2.fromOffset(30,25),Position=UDim2.new(1,i==1 and -190 or -45,0,y+24),
            BackgroundColor3=colors.card,BorderSizePixel=0,Text=i==1 and "−" or "+",TextColor3=colors.text,TextSize=18},parent)
        buttons[#buttons+1]={button=b,delta=delta}
    end
    return {refresh=refresh,buttons=buttons,key=key,min=minimum,max=maximum,widgets={caption,value,buttons[1].button,buttons[2].button}}
end
local defenseAdvanced=create("Frame",{Visible=false,Size=UDim2.fromScale(1,1),BackgroundTransparency=1},pages.Defense)
local settingRows={
    setting(pages.Movement,"Glide multiplier","boostMultiplier",52,1,1.5,0.05,"%.2fx"),
    setting(pages.Movement,"Glide tail duration","boostDuration",112,0.1,0.5,0.05,"%.2f s"),

    setting(defenseAdvanced,"Acquire range (studs)","guardRange",47,4,30,2,"%.0f"),
    setting(defenseAdvanced,"Stand-off distance (studs)","guardGap",100,2,8,0.25,"%.2f"),
    setting(defenseAdvanced,"Turn responsiveness (full-speed input)","guardStrength",153,0.1,1,0.1,"%.0f%%"),
    setting(defenseAdvanced,"Reaction delay","guardReaction",206,0,0.4,0.02,"%.2f s"),
    setting(defenseAdvanced,"Movement prediction","guardPrediction",267,0,0.2,0.02,"%.2f s")}

local visualToggle=row(defenseAdvanced,"Guard floor visuals · ON",370)
local closeoutToggle=row(defenseAdvanced,"Shot closeout · ON",418)
local stealToggle=row(defenseAdvanced,"Auto steal · OFF (experimental)",574)
local blockToggle=row(defenseAdvanced,"Auto block · OFF (experimental)",792)
local reboundToggle=row(defenseAdvanced,"Nearby auto rebound · OFF (experimental)",954)
local extraRows={
 setting(defenseAdvanced,"Closeout distance","closeGap",466,.75,4,.25,"%.2f"),
 setting(defenseAdvanced,"Closeout reaction","closeReaction",519,0,.4,.02,"%.2f s"),
 setting(defenseAdvanced,"Steal ball reach","stealRange",622,1,6,.25,"%.2f"),
 setting(defenseAdvanced,"Opportunity hold time","stealDelay",675,.05,.5,.025,"%.3f s"),
 setting(defenseAdvanced,"Steal cooldown","stealCooldown",728,.6,3,.2,"%.1f s"),
 setting(defenseAdvanced,"Block reach","blockRange",840,2,8,.5,"%.1f"),
 setting(defenseAdvanced,"Block shot delay","blockDelay",893,.05,.5,.025,"%.3f s"),
 setting(defenseAdvanced,"Rebound reach","reboundRange",1002,2,10,.5,"%.1f"),
 setting(defenseAdvanced,"Rebound height above root","reboundHeight",1055,3,12,.5,"%.1f"),
 setting(defenseAdvanced,"Jump cooldown","jumpCooldown",1108,.6,2,.1,"%.1f s")}
local sprintToggle=row(defenseAdvanced,"Guard sprint · ON",1170)
local facingToggle=row(defenseAdvanced,"Face handler · ON",1218)
local onceToggle=row(defenseAdvanced,"One block attempt per shot · ON",1266)
local chaseToggle=row(defenseAdvanced,"Chase nearby rebounds · OFF",1314)
local moreRows={
 setting(defenseAdvanced,"Sprint only beyond distance","sprintDistance",1362,.5,8,.5,"%.1f"),
 setting(defenseAdvanced,"Position dead zone","guardDeadzone",1415,.25,2,.1,"%.2f"),
 setting(defenseAdvanced,"Steal forward angle","stealAngle",1468,15,85,5,"%.0f degrees"),
 setting(defenseAdvanced,"Maximum handler speed for steal","stealMaxSpeed",1521,2,25,1,"%.0f"),
 setting(defenseAdvanced,"Rebound pursuit range","reboundChaseRange",1574,4,20,1,"%.0f"),
 setting(defenseAdvanced,"Rebound trajectory lookahead","reboundLead",1627,0,.35,.025,"%.3f s")}
for _,r in ipairs(moreRows) do table.insert(extraRows,r) end
label(defenseAdvanced,"Steal/block/rebound use native actions. Results depend on the server; no independent hand placement.",1690,65,11).TextColor3=colors.muted
local liveToggle=row(pages.Shooting,"Live contest estimate · ON",312)
local relaxedPreset=row(pages.Defense,"Response · Balanced",312)
for i,button in ipairs({guardToggle,stealToggle,blockToggle,reboundToggle,holdToggle}) do
 button.Parent=pages.Defense;button.Position=UDim2.fromOffset(16,(i-1)*47)
end
-- Keep one distance control; advanced heuristics use the selected response preset.
local gapRow=settingRows[4]
for i,widget in ipairs(gapRow.widgets) do
 widget.Parent=pages.Defense
 widget.Position=UDim2.new(widget.Position.X.Scale,widget.Position.X.Offset,0,i==1 and 240 or 264)
end
pages.Defense.CanvasSize=UDim2.fromOffset(0,405)
local defenseHint=label(pages.Defense,"Hold G to engage. Actions below are attempts, not confirmed results.",356,45,11)
local liveHud=create("Frame",{Size=UDim2.fromOffset(300,74),Position=UDim2.new(.5,-150,0,60),
 BackgroundColor3=colors.bg,BackgroundTransparency=.12,BorderSizePixel=0},gui)
create("UICorner",{CornerRadius=UDim.new(0,9)},liveHud)
local liveTitle=label(liveHud,"ESTIMATED CONTEST · waiting",3,25,13)
local liveDetail=label(liveHud,"Distance / positioning estimate",29,21,11)
local liveTrack=create("Frame",{Size=UDim2.new(1,-32,0,4),Position=UDim2.fromOffset(16,59),BackgroundColor3=colors.line,BorderSizePixel=0},liveHud)
local liveFill=create("Frame",{Size=UDim2.fromScale(0,1),BackgroundColor3=colors.accent,BorderSizePixel=0},liveTrack)
for _,r in ipairs(extraRows) do table.insert(settingRows,r) end
local lastErrorAt=0
