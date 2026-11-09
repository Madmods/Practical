-------------------------------------------------------------------- cards + sections
SETTING_BUILDERS.combo=function(panel,s,x,y,ctrl)
 local holder=UI.make("Frame",{BackgroundTransparency=1,Position=UDim2.fromOffset(32,y-155),Size=UDim2.new(1,-64,0,310),Parent=panel})
 local steps,selected,lastSource={},1,nil
 local chipLinks,keyButtons={},{}
 local hint=text(holder,"1. ADD MOVES",0,10,15,{color=C.sub})
 local selectionLabel=text(holder,"2. CLICK A STEP TO EDIT",0,74,15,{color=C.sub})
 local tray=UI.make("ScrollingFrame",{CanvasSize=UDim2.fromOffset(0,0),ScrollBarThickness=3,BorderSizePixel=0,BackgroundTransparency=1,Position=UDim2.fromOffset(0,88),Size=UDim2.new(1,0,0,96),Parent=holder})
 local delaySetting={id="sequenceDelay",min=.08,max=1.5,step=.01,value=.25,format="%.2f",suffix=" s"}
 local delayPaint,paint
 local function parse(value)
  local result={}
  for keys,delay in tostring(value):gmatch("([ZXCVWAIT]+)@([%d%.]+)")do
   if #result<12 then result[#result+1]={keys=keys,delay=math.clamp(tonumber(delay)or .25,.08,1.5)}end
  end
  return result
 end
 local function commit()
  local values={}
  for _,step in ipairs(steps)do values[#values+1]=step.keys.."@"..string.format("%.2f",step.delay)end
  s.value=table.concat(values," > ");lastSource=s.value
  controlChanged:Fire(ctrl.id,s.id,s.value);paint()
 end
 local function button(parent,label,pos,size,fn)
  local b=UI.make("TextButton",{Text=label,AutoButtonColor=false,BackgroundColor3=C.hover,TextColor3=C.text,FontFace=UI.font(),TextSize=17,Position=pos,Size=size,Parent=parent},{UI.corner(7),UI.stroke(C.field,1)})
  table.insert(rowConns,b.MouseButton1Click:Connect(fn));return b
 end
 local moves={
  {"Crossover left","Z"},{"Crossover right","C"},{"Stepback","X"},{"Double stepback","XX"},
  {"Behind-back left","CX"},{"Behind-back right","ZX"},
  {"Double behind-back left","XC"},{"Double behind-back right","XZ"},
  {"Spin left","CXZ"},{"Spin right","ZXC"},{"Pause","WAIT"}}
 local moveNames,moveCodes,displayNames={},{},{}
 for _,move in ipairs(moves)do
  moveNames[#moveNames+1]=move[1];moveCodes[move[1]]=move[2];displayNames[move[2]]=move[1]
 end
 local function moveName(code)return displayNames[code]or("Unmapped move ("..code..")")end
 local chosen="Z"
 dropdown(holder,UDim2.fromOffset(0,44),300,32,moveNames,"Crossover left",function(name)chosen=moveCodes[name]end)
 button(holder,"Add move",UDim2.fromOffset(312,28),UDim2.fromOffset(140,32),function()
  if #steps>=12 then hint.Text="Sequence full · remove a move first";return end
  table.insert(steps,{keys=chosen,delay=delaySetting.value});selected=#steps;commit()
 end)
 text(holder,"Replace move",0,206,17,{color=C.text2})
 dropdown(holder,UDim2.fromOffset(145,206),290,32,moveNames,"Choose replacement",function(name)
  if not steps[selected]then selectionLabel.Text="Select a move first";return end
  steps[selected].keys=moveCodes[name];commit()
 end)
 button(holder,"Remove step",UDim2.new(1,-148,0,190),UDim2.fromOffset(148,32),function()if steps[selected]then table.remove(steps,selected);commit()end end)
 local actions={
  {"Duplicate move",function()
   local step=steps[selected]
   if not step then selectionLabel.Text="Select a move first";return end
   if #steps>=12 then selectionLabel.Text="Sequence full";return end
   table.insert(steps,selected+1,{keys=step.keys,delay=step.delay});selected+=1;commit()
  end},
  {"Move left",function()if selected>1 then steps[selected],steps[selected-1]=steps[selected-1],steps[selected];selected=selected-1;commit()end end},
  {"Move right",function()if selected<#steps then steps[selected],steps[selected+1]=steps[selected+1],steps[selected];selected=selected+1;commit()end end},
  {"Clear sequence",function()steps={};selected=1;commit()end},
 }
 for i,a in ipairs(actions)do button(holder,a[1],UDim2.new((i-1)/#actions,0,0,234),UDim2.new(1/#actions,-6,0,30),a[2])end
 text(holder,"Wait after move",0,282,18,{color=C.text2})
 local delayHolder=UI.make("Frame",{BackgroundTransparency=1,AnchorPoint=Vector2.new(1,.5),Position=UDim2.new(1,0,0,282),Parent=holder})
 delayPaint=UI.slider(delayHolder,delaySetting,ctrl)
 paint=function()
  for _,c in ipairs(chipLinks)do c:Disconnect()end;chipLinks={}
  for _,child in ipairs(tray:GetChildren())do child:Destroy()end
  selected=math.clamp(selected,1,math.max(1,#steps))
  hint.Text="DRIBBLE MOVES v3 - "..#steps.." / 12"
  selectionLabel.Text=steps[selected]and("EDIT MOVE "..selected..": "..moveName(steps[selected].keys))or "Select a named move above, then click Add move"
  if #steps==0 then text(tray,"Or choose a preset above and press Play combo.",0,30,18,{color=C.sub})end
  tray.CanvasSize=UDim2.fromOffset(0,math.ceil(#steps/3)*47)
  for i,step in ipairs(steps)do
   local label=moveName(step.keys)
   local b=UI.make("TextButton",{Text=i..". "..label.."\n"..string.format("%.2f s",step.delay),AutoButtonColor=false,TextColor3=C.text,BackgroundColor3=i==selected and A.dim or C.hover,FontFace=UI.font(),TextSize=13,Position=UDim2.new(((i-1)%3)/3,0,0,math.floor((i-1)/3)*47),Size=UDim2.new(1/3,-6,0,41),Parent=tray},{UI.corner(7),UI.stroke(i==selected and A.a or C.field,1)})
   chipLinks[#chipLinks+1]=b.MouseButton1Click:Connect(function()selected=i;paint()end)
  end
  local step=steps[selected]
  for key,b in pairs(keyButtons)do
   local on=step and step.keys~="WAIT"and step.keys:find(key,1,true)
   b.BackgroundColor3=on and A.dim or C.hover;b.TextColor3=on and C.white or C.sub
  end
  delaySetting.value=step and step.delay or .25
  if delayPaint then delayPaint(0)end
 end
 table.insert(rowConns,controlChanged.Event:Connect(function(id,field,value)
  if id==ctrl.id and field=="sequenceDelay"and steps[selected]then steps[selected].delay=math.clamp(value,.08,1.5);commit()end
 end))
 table.insert(rowConns,{Disconnect=function()for _,c in ipairs(chipLinks)do c:Disconnect()end end})
 steps=parse(s.value);lastSource=s.value;paint()
 return function()if s.value~=lastSource then steps=parse(s.value);lastSource=s.value;selected=1 end;paint()end
end

SETTING_BUILDERS.library=function(panel,s,x,y,ctrl)
 local holder=UI.make("Frame",{BackgroundTransparency=1,Position=UDim2.fromOffset(24,y-105),Size=UDim2.new(1,-48,0,210),Parent=panel})
 local selected
 local keySetting={id="savedComboHotkey",value=Enum.KeyCode.K}
 local function names()
  local list={}for _,e in ipairs(api.GetNamedCombos())do list[#list+1]=e.name end
  return #list>0 and list or {"No saved combos"}
 end
 text(holder,"Saved combos",0,20,16,{color=C.sub})
 local box=UI.make("TextBox",{Name="ComboName",Text="My combo",PlaceholderText="Name this combo",ClearTextOnFocus=false,TextColor3=C.text,BackgroundColor3=C.field,FontFace=UI.font(),TextSize=16,Position=UDim2.fromOffset(150,52),Size=UDim2.fromOffset(360,38),Parent=holder},{UI.corner(8)})
 text(holder,"Name",0,71,16,{color=C.sub})
 text(holder,"Hotkey",0,120,16,{color=C.sub})
 local keyHolder=UI.make("Frame",{BackgroundTransparency=1,Position=UDim2.fromOffset(150,101),Parent=holder})
 local paintKey=UI.keybind(keyHolder,keySetting,ctrl)
 local picker=dropdown(holder,UDim2.fromOffset(150,20),360,38,names,"Choose saved combo",function(name)
  local ok,e=api.LoadNamedCombo(name)
  if not ok then setStatus(e);return end
  selected=name;box.Text=name;keySetting.value=e.key;paintKey()
  SETTING.comboBuilder.sequence.value=e.sequence;SETTING.comboBuilder.repeats.value=e.repeats;SETTING.comboBuilder.sprint.value=e.sprint
  SETTING.comboBuilder.preset.value="Custom";repaintAll();setStatus("Loaded "..name)
 end)
 local function action(label,offset,fn)
  local b=UI.ghostButton(holder,label,"folder",38);b.Position=UDim2.fromOffset(offset,155)
  b.MouseButton1Click:Connect(fn)
 end
 action("Save combo + key",0,function()
  if keySetting.value==PREF_KEY.value then setStatus("Choose a different key from the menu shortcut");return end
  local ok,message=api.SaveNamedCombo(box.Text,keySetting.value)
  if ok then
   selected=box.Text:match("^%s*(.-)%s*$");picker.set(selected,0)
   CTRL.comboBuilder.enabled=true;api.SetOption("comboBuilder",true);repaintAll()
  end
  setStatus(message)
 end)
 action("Delete saved",235,function()
  local ok,message=api.DeleteNamedCombo(selected)
  if ok then selected=nil;picker.set("Choose saved combo",0)end
  setStatus(message)
 end)
 return function()end
end

local HEADER_H = 74
local ROW_H = { combo=330, library=230, slider = 66, segment = 66, key = 66, swatch = 66, motionPreview = 66, checks = 56, actions = 66, note = 50, divider = 18, textPreview = 108 }

function UI.rowHeight(r)
	local base = ROW_H[r.type] or 62
	if r.type == "divider" or r.type == "textPreview" or r.type == "combo" or r.type == "library" then
		return base
	end
	return math.floor(base * layoutState.density + 0.5)
end

function UI.panelHeight(rows)
	local h = 12
	for _, r in ipairs(rows) do
		h += UI.rowHeight(r)
	end
	return h
end

function UI.buildPanel(panel, ctrl, appliers)
	local y = 6
	for i, r in ipairs(ctrl.rows) do
		local rh = UI.rowHeight(r)
		local cy = y + rh / 2
		local t = r.type
		if i > 1 and layoutState.dividers and t ~= "divider" then
			UI.strip(panel, UDim2.fromOffset(24, y), UDim2.new(1, -48, 0, 1), C.line)
		end
		if t == "divider" then
			UI.strip(panel, UDim2.fromOffset(24, cy), UDim2.new(1, -48, 0, 1), C.line)
		elseif t == "note" then
			UI.icon(panel, "info", 34, cy, 17, C.sub, 1.6)
			UI.make("TextLabel", {
				BackgroundTransparency = 1,
				AnchorPoint = Vector2.new(0, 0.5),
				Position = UDim2.fromOffset(54, cy),
				Size = UDim2.new(1, -78, 0, rh - 8),
				Text = r.label,
				TextWrapped = true,
				TextXAlignment = Enum.TextXAlignment.Left,
				FontFace = UI.font(),
				TextSize = 15,
				TextColor3 = C.sub,
				Parent = panel,
			})
		elseif t == "checks" then
			local cx = 24
			for _, it in ipairs(r.items) do
				local apply, w = UI.checkbox(panel, it, cx, cy, ctrl)
				table.insert(appliers, apply)
				cx += w + 36
			end
		elseif t == "actions" then
			local cx = 24
			for _, it in ipairs(r.items) do
				cx += UI.actionButton(panel, it, cx, cy, ctrl) + 12
			end
		elseif t == "library" then
   table.insert(appliers,SETTING_BUILDERS.library(panel,r,0,cy,ctrl))
  elseif t == "combo" then
   table.insert(appliers,SETTING_BUILDERS.combo(panel,r,0,cy,ctrl))
  elseif t == "textPreview" then
			table.insert(appliers, UI.textPreview(panel, ctrl, y, rh))
		else
			-- label (+ optional description) on the left, control right-aligned
			local hasDesc = r.desc ~= nil
			local label = text(panel, r.label, 24, hasDesc and cy - 10 or cy, 17, { weight = MEDIUM, button = r.help ~= nil })
			if r.help then
				UI.tooltip(label, r.help)
			end
			if hasDesc then
				text(panel, r.desc, 24, cy + 12, 14, { color = C.sub })
			end
			local holder = UI.make("Frame", {
				BackgroundTransparency = 1,
				AnchorPoint = Vector2.new(1, 0.5),
				Position = UDim2.new(1, -24, 0, cy),
				Size = UDim2.fromOffset(0, 40),
				Parent = panel,
			})
			local build = SETTING_BUILDERS[t]
			if build then
				table.insert(appliers, build(holder, r, ctrl))
			end
		end
		y += rh
	end
end

-- Always-visible group of settings with a caption (no switch).
function UI.buildSection(ctrl, order)
	local appliers = {}
	rowAppliers[ctrl] = appliers
	local ph = UI.panelHeight(ctrl.rows)
	local capH = layoutState.captions and 30 or 0
	local slot, body = UI.newSlot(capH + ph, order, ctrl.id)
	if capH > 0 then
		UI.tracked(body, string.upper(ctrl.name), 8, 14, 12, C.muted, 1.6)
	end
	local panel = UI.make("Frame", {
		BackgroundColor3 = C.surface,
		Position = UDim2.fromOffset(2, capH + 2),
		Size = UDim2.new(1, -4, 0, ph),
		Parent = body,
	}, { UI.corner(14), UI.stroke(C.stroke, 1) })
	UI.buildPanel(panel, ctrl, appliers)
	return slot, body
end

-- Collapsible feature card with an ON/OFF switch. ON opens its settings, OFF closes them;
-- the header still opens/closes them by hand either way.
function UI.buildCard(ctrl, order)
	local rows = ctrl.rows or {}
	local hasPanel = #rows > 0
	local panelH = hasPanel and UI.panelHeight(rows) or 0
	local openH = HEADER_H + panelH
	local appliers = {}
	rowAppliers[ctrl] = appliers

	local slot, body = UI.newSlot(HEADER_H, order, ctrl.id)
	local cardStroke = UI.stroke(C.stroke, 1)
	local card = UI.make("Frame", {
		BackgroundColor3 = C.surface,
		ClipsDescendants = true,
		Position = UDim2.fromOffset(2, 2),
		Size = UDim2.new(1, -4, 0, HEADER_H),
		Parent = body,
	}, { UI.corner(14), cardStroke })
	-- accent hairline along the top edge while the feature is on
	local sheen = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		Position = UDim2.fromOffset(28, 0),
		Size = UDim2.new(1, -56, 0, 1),
		Parent = card,
	}, { UI.accentGradient(0, EDGE_FADE) })

	local header = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.white,
		BackgroundTransparency = 1,
		Size = UDim2.new(1, 0, 0, HEADER_H),
		ZIndex = 5,
		Parent = card,
	})

	local tile = UI.make("Frame", {
		BackgroundColor3 = C.field,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.fromOffset(18, HEADER_H / 2),
		Size = UDim2.fromOffset(42, 42),
		Parent = card,
	}, { UI.corner(12) })
	local tileFill = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Parent = tile,
	}, { UI.corner(12), UI.accentGradient(45) })
	local tileIcon = UI.icon(tile, ctrl.icon or "layers", 21, 21, 21, C.sub, 1.8)
	tileIcon.ZIndex = 2

	local hasDesc = ctrl.desc ~= nil
	local title = text(card, ctrl.name, 76, hasDesc and HEADER_H / 2 - 10 or HEADER_H / 2, 19, { weight = SEMIBOLD })
	if hasDesc then
		text(card, ctrl.desc, 76, HEADER_H / 2 + 13, 14, { color = C.sub })
	end

	local chevron
	if hasPanel then
		chevron = UI.make("Frame", {
			BackgroundTransparency = 1,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Position = UDim2.new(1, -32, 0, HEADER_H / 2),
			Size = UDim2.fromOffset(16, 16),
			Parent = card,
		})
	end

	local panel
	if hasPanel then
		panel = UI.make("Frame", {
			BackgroundTransparency = 1,
			Position = UDim2.fromOffset(0, HEADER_H),
			Size = UDim2.new(1, 0, 0, panelH),
			Parent = card,
		})
		UI.strip(panel, UDim2.fromOffset(0, 0), UDim2.new(1, 0, 0, 1), C.line)
		UI.buildPanel(panel, ctrl, appliers)
	end

	local hovered = false
	local function paintCard(t)
		local on = ctrl.enabled == true
		UI.tween(cardStroke, t, { Color = hovered and C.strokeHi or (on and A.soft or C.stroke) })
		UI.tween(tileFill, t, { BackgroundTransparency = on and 0 or 1 })
		UI.tintIcon(tileIcon, on and C.white or C.sub, t)
		UI.tween(sheen, t, { BackgroundTransparency = on and 0.15 or 1 })
	end

	local function setExpanded(v, t)
		if not hasPanel then
			v = false
		end
		ctrl.expanded = v
		local h = v and openH or HEADER_H
		-- slot and card resize together, so cards below glide instead of jumping
		UI.tween(slot, t, { Size = UDim2.new(1, 0, 0, h + 4) })
		UI.tween(card, t, { Size = UDim2.new(1, -4, 0, h) })
		if chevron then
			UI.spinIcon(chevron, "chevronRight", C.sub, 2, v and 90 or 0, t)
		end
		if panel then
			if t > 0 then
				if v then
					panel.Visible = true
					panel.Position = UDim2.fromOffset(0, HEADER_H - 12)
					UI.groupFade(panel, 0, 0.26, 0.04)
					UI.tween(panel, 0.32, { Position = UDim2.fromOffset(0, HEADER_H) }, QUINT, OUT, 0.04)
				else
					UI.groupFade(panel, 1, 0.16)
					UI.tween(panel, 0.16, { Position = UDim2.fromOffset(0, HEADER_H - 8) })
				end
			else
				panel.Visible = v
				if v and fades[panel] then
					UI.groupFade(panel, 0, 0.01) -- unwrap a panel left faded out by an earlier collapse
				end
			end
		end
	end
	setExpanded(ctrl.expanded == true, 0)
	paintCard(0)

	local paintToggle = UI.toggleSwitch(card, UDim2.new(1, hasPanel and -56 or -22, 0, HEADER_H / 2), ctrl.enabled == true, function(on)
		ctrl.enabled = on
		paintCard(0.25)
		controlChanged:Fire(ctrl.id, "enabled", on)
		if ctrl.expanded ~= on then
			closePopup()
			setExpanded(on, 0.34)
		end
	end)
	table.insert(appliers, function(t)
		paintToggle(ctrl.enabled == true, t)
		paintCard(t)
		
	end)

	header.MouseEnter:Connect(function()
		hovered = true
		UI.tween(header, 0.2, { BackgroundTransparency = 0.985 })
		paintCard(0.2)
	end)
	header.MouseLeave:Connect(function()
		hovered = false
		UI.tween(header, 0.3, { BackgroundTransparency = 1 })
		paintCard(0.3)
	end)
	header.MouseButton1Click:Connect(function()
		if hasPanel then
			closePopup()
			setExpanded(not ctrl.expanded, 0.34)
		end
	end)

	return slot, body
end

function UI.buildControl(ctrl, order)
	if ctrl.kind == "section" then
		return UI.buildSection(ctrl, order)
	end
	return UI.buildCard(ctrl, order)
end

