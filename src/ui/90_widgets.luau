-------------------------------------------------------------------- widgets
local rowConns = {} -- service connections owned by the current page
local rowAppliers = {} -- ctrl -> { apply(t) } repaint functions for the current page
local function bindingLabel(key)
 if key==Enum.KeyCode.F13 then return "Mouse 4"elseif key==Enum.KeyCode.F14 then return "Mouse 5"end
 return key.Name
end
local rebinding -- { setting, ctrl, stop } while waiting for a new key

-- Pill switch. `pos` = right edge / vertical centre. Returns paint(on, t).
function UI.toggleSwitch(parent, pos, on, onChange)
	local TW, TH, K = 50, 28, 20
	local st = UI.stroke(C.strokeHi, 1)
	local track = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.field,
		AnchorPoint = Vector2.new(1, 0.5),
		Position = pos,
		Size = UDim2.fromOffset(TW, TH),
		ZIndex = 6,
		Parent = parent,
	}, { UI.round(), st })
	local fill = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BackgroundTransparency = on and 0 or 1,
		Size = UDim2.fromScale(1, 1),
		ZIndex = 6,
		Parent = track,
	}, { UI.round(), UI.accentGradient(0) })
	local knob = UI.make("Frame", {
		BackgroundColor3 = on and C.white or C.text2,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, on and (TW - 4 - K) or 4, 0.5, 0),
		Size = UDim2.fromOffset(K, K),
		ZIndex = 7,
		Parent = track,
	}, { UI.round() })
	local state, pressed = on, false

	local function knobGoal(width)
		return { Size = UDim2.fromOffset(width, K), Position = UDim2.new(0, state and (TW - 4 - width) or 4, 0.5, 0) }
	end
	local function paint(v, t)
		state = v
		UI.tween(fill, t, { BackgroundTransparency = v and 0 or 1 })
		UI.tween(st, t, { Color = v and A.a or C.strokeHi, Transparency = v and 0.35 or 0 })
		local goal = knobGoal(K)
		goal.BackgroundColor3 = v and C.white or C.text2
		UI.tween(knob, t, goal)
	end
	paint(on, 0)
	track.MouseButton1Down:Connect(function()
		pressed = true
		UI.tween(knob, 0.18, knobGoal(K + 6))
	end)
	track.MouseLeave:Connect(function()
		if pressed then
			pressed = false
			UI.tween(knob, 0.25, knobGoal(K))
		end
	end)
	track.MouseButton1Click:Connect(function()
		pressed = false
		paint(not state, 0.28)
		onChange(state)
	end)
	return paint
end

-- Slider. The knob glides after the pointer each frame (critically damped follow) so dragging
-- feels continuous while the stored value still snaps to `step`. A bubble shows the value.
function UI.slider(holder, s, ctrl)
	local stepper = s.stepper
	local trackW = stepper and 196 or 232
	local boxW = stepper and 132 or 88
	holder.Size = UDim2.fromOffset(trackW + 20 + boxW, 40)

	local track = UI.make("Frame", {
		BackgroundColor3 = C.field,
		BorderSizePixel = 0,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, 0, 0.5, 0),
		Size = UDim2.fromOffset(trackW, 6),
		Parent = holder,
	}, { UI.round() })
	local fill = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BorderSizePixel = 0,
		Size = UDim2.fromScale(0, 1),
		Parent = track,
	}, { UI.round(), UI.accentGradient(0) })
	local halo = UI.bindTheme(UI.make("Frame", {
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromScale(0, 0.5),
		Size = UDim2.fromOffset(18, 18),
		ZIndex = 2,
		Parent = track,
	}, { UI.round() }), "BackgroundColor3", "a")
	local knob = UI.make("Frame", {
		BackgroundColor3 = C.white,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromScale(0, 0.5),
		Size = UDim2.fromOffset(16, 16),
		ZIndex = 3,
		Parent = track,
	}, { UI.round(), UI.bindTheme(UI.stroke(A.a, 3), "Color", "a") })
	local bubbleStroke = UI.stroke(C.strokeHi, 1, 1)
	local bubble = UI.make("Frame", {
		BackgroundColor3 = C.raised,
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0.5, 1),
		Position = UDim2.new(0, 0, 0, -12),
		Size = UDim2.fromOffset(60, 24),
		ZIndex = 4,
		Parent = track,
	}, { UI.corner(7), bubbleStroke })
	local bubbleText = UI.make("TextLabel", {
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Text = "",
		TextTransparency = 1,
		FontFace = UI.font(SEMIBOLD),
		TextSize = 14,
		TextColor3 = C.text,
		ZIndex = 4,
		Parent = bubble,
	})
	local grab = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, -10, 0.5, 0),
		Size = UDim2.fromOffset(trackW + 20, 34),
		ZIndex = 5,
		Parent = holder,
	})

	-- value readout: stepper (− value +) or a plain box
	local boxStroke = UI.stroke(C.stroke, 1)
	local box = UI.make("Frame", {
		BackgroundColor3 = C.field,
		AnchorPoint = Vector2.new(1, 0.5),
		Position = UDim2.new(1, 0, 0.5, 0),
		Size = UDim2.fromOffset(boxW, 36),
		Parent = holder,
	}, { UI.corner(9), boxStroke })
	local input
	local set -- forward
	local function display(v)
		return string.format(s.format or "%.1f", v)
	end
	if stepper then
		UI.strip(box, UDim2.fromOffset(36, 7), UDim2.fromOffset(1, 22), C.stroke)
		UI.strip(box, UDim2.new(1, -37, 0, 7), UDim2.fromOffset(1, 22), C.stroke)
		local function stepBtn(pos, iconName, dir)
			local b = UI.make("TextButton", {
				Text = "",
				AutoButtonColor = false,
				BackgroundColor3 = C.hover,
				BackgroundTransparency = 1,
				Position = pos,
				Size = UDim2.fromOffset(36, 36),
				Parent = box,
			}, { UI.corner(9) })
			local ic = UI.icon(b, iconName, 18, 18, 15, C.text2, 1.9)
			b.MouseEnter:Connect(function()
				UI.tween(b, 0.15, { BackgroundTransparency = 0 })
				UI.tintIcon(ic, C.white, 0.15)
			end)
			b.MouseLeave:Connect(function()
				UI.tween(b, 0.2, { BackgroundTransparency = 1 })
				UI.tintIcon(ic, C.text2, 0.2)
			end)
			UI.pressable(b, 0.85)
			UI.repeatButton(b, function()
				set(s.value + dir * (s.nudge or s.step), true)
			end)
		end
		stepBtn(UDim2.fromOffset(0, 0), "minus", -1)
		stepBtn(UDim2.new(1, -36, 0, 0), "plus", 1)
		input = UI.make("TextBox", {
			BackgroundTransparency = 1,
			Position = UDim2.fromOffset(37, 0),
			Size = UDim2.new(1, -74, 1, 0),
			Text = "",
			TextColor3 = C.text,
			FontFace = UI.font(SEMIBOLD),
			TextSize = 16,
			ClearTextOnFocus = false,
			Parent = box,
		})
	else
		input = UI.make("TextBox", {
			BackgroundTransparency = 1,
			Size = UDim2.fromScale(1, 1),
			Text = "",
			TextColor3 = C.text,
			FontFace = UI.font(SEMIBOLD),
			TextSize = 16,
			ClearTextOnFocus = false,
			Parent = box,
		})
	end

	local function toP(v)
		return (v - s.min) / (s.max - s.min)
	end
	local visP = toP(s.value) -- where the knob is drawn
	local targetP = visP -- where it is heading
	local dragging, hovering, bubbleOn = false, false, false
	local function draw()
		fill.Size = UDim2.fromScale(visP, 1)
		knob.Position = UDim2.fromScale(visP, 0.5)
		halo.Position = knob.Position
		bubble.Position = UDim2.new(visP, 0, 0, -12)
	end
	draw()

	-- `raw` (0..1) lets the knob follow the pointer exactly while the value snaps
	set = function(v, fire, raw)
		v = math.clamp(math.round(v / s.step) * s.step, s.min, s.max)
		local changed = v ~= s.value
		s.value = v
		targetP = raw or toP(v)
		bubbleText.Text = display(v) .. (s.suffix or "")
		if not input:IsFocused() then
			input.Text = display(v) .. (stepper and "" or (s.suffix or ""))
		end
		if fire and changed then
			if s.onSet then
				s.onSet(v)
			end
			controlChanged:Fire(ctrl.id, s.id, v)
		end
	end
	set(s.value, false)

	table.insert(rowConns, RunService.RenderStepped:Connect(function(dt)
		if math.abs(visP - targetP) > 0.0005 then
			visP += (targetP - visP) * (1 - math.exp(-dt * (dragging and 30 or 16)))
			draw()
		elseif visP ~= targetP then
			visP = targetP
			draw()
		end
	end))

	local function refreshKnob()
		local d = dragging and 20 or hovering and 18 or 16
		UI.tween(knob, 0.2, { Size = UDim2.fromOffset(d, d) })
		UI.tween(halo, 0.25, {
			Size = UDim2.fromOffset(dragging and 38 or 18, dragging and 38 or 18),
			BackgroundTransparency = dragging and 0.8 or 1,
		})
		local show = dragging or hovering
		if show ~= bubbleOn then
			bubbleOn = show
			UI.tween(bubble, 0.18, { BackgroundTransparency = show and 0 or 1 })
			UI.tween(bubbleText, 0.18, { TextTransparency = show and 0 or 1 })
			UI.tween(bubbleStroke, 0.18, { Transparency = show and 0 or 1 })
		end
	end
	local function fromX(px)
		local p = math.clamp((px - track.AbsolutePosition.X) / track.AbsoluteSize.X, 0, 1)
		set(s.min + p * (s.max - s.min), true, p)
	end
	grab.MouseEnter:Connect(function()
		hovering = true
		refreshKnob()
	end)
	grab.MouseLeave:Connect(function()
		hovering = false
		refreshKnob()
	end)
	grab.InputBegan:Connect(function(io)
		if io.UserInputType == Enum.UserInputType.MouseButton1 or io.UserInputType == Enum.UserInputType.Touch then
			dragging = true
			refreshKnob()
			fromX(io.Position.X)
		end
	end)
	table.insert(rowConns, UserInputService.InputChanged:Connect(function(io)
		if dragging and (io.UserInputType == Enum.UserInputType.MouseMovement or io.UserInputType == Enum.UserInputType.Touch) then
			fromX(io.Position.X)
		end
	end))
	table.insert(rowConns, UserInputService.InputEnded:Connect(function(io)
		if dragging and (io.UserInputType == Enum.UserInputType.MouseButton1 or io.UserInputType == Enum.UserInputType.Touch) then
			dragging = false
			targetP = toP(s.value) -- settle onto the snapped value
			refreshKnob()
		end
	end))
	input.Focused:Connect(function()
		UI.tween(boxStroke, 0.2, { Color = A.a })
		input.Text = display(s.value)
	end)
	input.FocusLost:Connect(function()
		UI.tween(boxStroke, 0.3, { Color = C.stroke })
		local n = tonumber(string.match(input.Text, "-?[%d%.]+") or "")
		set(n or s.value, n ~= nil)
		input.Text = display(s.value) .. (stepper and "" or (s.suffix or ""))
	end)

	return function(t)
		set(s.value, false)
		if t <= 0 then
			visP = targetP
			draw()
		end
	end
end

-- Segmented control with a gliding pill.
function UI.segmented(holder, s, ctrl)
	local n = #s.options
	local widest = 0
	for _, o in ipairs(s.options) do
		widest = math.max(widest, UI.textWidth(o, 16))
	end
	local segW = math.max(76, math.ceil(widest) + 30)
	local h = 38
	holder.Size = UDim2.fromOffset(segW * n + 8, h)
	holder.BackgroundColor3 = C.field
	holder.BackgroundTransparency = 0
	UI.corner(10).Parent = holder
	UI.stroke(C.line, 1).Parent = holder
	local pill = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BackgroundTransparency = 0.8,
		Position = UDim2.fromOffset(4, 4),
		Size = UDim2.fromOffset(segW, h - 8),
		Parent = holder,
	}, { UI.corner(7), UI.accentGradient(0), UI.bindTheme(UI.stroke(A.a, 1, 0.3), "Color", "a") })
	local buttons = {}
	local function select(i, fire, t)
		s.value = s.options[i]
		UI.tween(pill, t, { Position = UDim2.fromOffset(4 + (i - 1) * segW, 4) })
		for j, b in ipairs(buttons) do
			UI.tween(b, t * 0.8, { TextColor3 = j == i and C.white or C.sub })
		end
		if fire then
			controlChanged:Fire(ctrl.id, s.id, s.value)
		end
	end
	for i, opt in ipairs(s.options) do
		local b = UI.make("TextButton", {
			AutoButtonColor = false,
			BackgroundTransparency = 1,
			Text = opt,
			FontFace = UI.font(MEDIUM),
			TextSize = 16,
			TextColor3 = C.sub,
			Position = UDim2.fromOffset(4 + (i - 1) * segW, 4),
			Size = UDim2.fromOffset(segW, h - 8),
			ZIndex = 2,
			Parent = holder,
		})
		buttons[i] = b
		b.MouseEnter:Connect(function()
			if s.value ~= opt then
				UI.tween(b, 0.15, { TextColor3 = C.text2 })
			end
		end)
		b.MouseLeave:Connect(function()
			if s.value ~= opt then
				UI.tween(b, 0.2, { TextColor3 = C.sub })
			end
		end)
		b.MouseButton1Click:Connect(function()
			if s.value ~= opt then
				select(i, true, 0.35)
			end
		end)
	end
	select(table.find(s.options, s.value) or 1, false, 0)
	return function(t)
		select(table.find(s.options, s.value) or 1, false, t)
	end
end

-- Keycap that listens for the next key press.
function UI.keybind(holder, s, ctrl)
	holder.AutomaticSize = Enum.AutomaticSize.X
	holder.Size = UDim2.fromOffset(0, 38)
	local capStroke = UI.stroke(C.strokeHi, 1)
	local cap = UI.make("TextButton", {
		AutoButtonColor = false,
		BackgroundColor3 = C.raised,
		AutomaticSize = Enum.AutomaticSize.X,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, 0, 0.5, 0),
		Size = UDim2.fromOffset(64, 38),
		Text = bindingLabel(s.value),
		FontFace = UI.font(SEMIBOLD),
		TextSize = 16,
		TextColor3 = C.text,
		Parent = holder,
	}, {
		UI.corner(9),
		capStroke,
		UI.padding(16, 16, 0, 0),
	})
	local capScale = UI.pressable(cap, 0.92)
	cap.MouseEnter:Connect(function()
		UI.tween(cap, 0.15, { BackgroundColor3 = C.hover })
		if not (rebinding and rebinding.setting == s) then
			UI.tween(capStroke, 0.15, { Color = C.sub })
		end
	end)
	cap.MouseLeave:Connect(function()
		UI.tween(cap, 0.25, { BackgroundColor3 = C.raised })
		if not (rebinding and rebinding.setting == s) then
			UI.tween(capStroke, 0.25, { Color = C.strokeHi })
		end
	end)
	cap.MouseButton1Click:Connect(function()
		if rebinding then
			rebinding.stop()
		end
		local beat = TweenService:Create(capStroke, TweenInfo.new(0.55, SINE, INOUT, -1, true), { Color = A.hi })
		capStroke.Color = A.soft
		beat:Play()
		cap.Text = "Press key / mouse…"
		UI.tween(cap, 0.2, { TextColor3 = A.hi })
		rebinding = {
			setting = s,
			ctrl = ctrl,
			stop = function()
				beat:Cancel()
				UI.tween(capStroke, 0.25, { Color = C.strokeHi })
				UI.tween(cap, 0.25, { TextColor3 = C.text })
				cap.Text = bindingLabel(s.value)
				capScale.Scale = 1.12
				UI.tween(capScale, 0.45, { Scale = 1 }, BACK)
			end,
		}
	end)
	return function()
		cap.Text = bindingLabel(s.value)
	end
end

-- Gradient colour dots; the selected one gets a ring and a check.
function UI.swatches(holder, s, ctrl)
	local D, GAP = 30, 12
	local n = #s.options
	holder.Size = UDim2.fromOffset(n * D + (n - 1) * GAP + 8, 40)
	local items = {}
	local function paint(t, pop)
		for _, it in ipairs(items) do
			local sel = it.name == s.value
			UI.tween(it.ring, t, { Transparency = sel and 0.1 or 1 })
			it.mark.Visible = sel
			if sel and pop then
				it.markScale.Scale = 0.3
				UI.tween(it.markScale, 0.45, { Scale = 1 }, BACK)
			end
		end
	end
	for i, name in ipairs(s.options) do
		local th = THEME_BY_NAME[name]
		local b = UI.make("TextButton", {
			Text = "",
			AutoButtonColor = false,
			BackgroundColor3 = C.white,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Position = UDim2.new(0, 4 + D / 2 + (i - 1) * (D + GAP), 0.5, 0),
			Size = UDim2.fromOffset(D, D),
			Parent = holder,
		}, { UI.round(), UI.make("UIGradient", { Rotation = 45, Color = ColorSequence.new(th.a, th.b) }) })
		local ring = UI.stroke(C.white, 2, 1)
		UI.make("Frame", {
			BackgroundTransparency = 1,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Position = UDim2.fromScale(0.5, 0.5),
			Size = UDim2.fromOffset(D + 8, D + 8),
			Parent = b,
		}, { UI.round(), ring })
		local mark = UI.icon(b, "check", D / 2, D / 2, 14, C.white, 2.2)
		local markScale = UI.make("UIScale", { Parent = mark })
		UI.pressable(b, 0.86)
		UI.tooltip(b, name)
		b.MouseButton1Click:Connect(function()
			if s.value ~= name then
				s.value = name
				paint(0.25, true)
				controlChanged:Fire(ctrl.id, s.id, name)
			end
		end)
		items[i] = { name = name, ring = ring, mark = mark, markScale = markScale }
	end
	paint(0, false)
	return function(t)
		paint(t, false)
	end
end

-- Lane with a dot that travels using the current motion settings.
function UI.motionPreview(holder, s, ctrl)
	holder.Size = UDim2.fromOffset(352, 38)
	local lane = UI.make("Frame", {
		BackgroundColor3 = C.field,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, 0, 0.5, 0),
		Size = UDim2.fromOffset(250, 34),
		Parent = holder,
	}, { UI.round(), UI.stroke(C.line, 1) })
	UI.strip(lane, UDim2.new(0, 18, 0.5, 0), UDim2.new(1, -36, 0, 1), C.stroke)
	local dot = UI.make("Frame", {
		BackgroundColor3 = C.white,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, 5, 0.5, 0),
		Size = UDim2.fromOffset(24, 24),
		ZIndex = 2,
		Parent = lane,
	}, { UI.round(), UI.accentGradient(45) })
	local at = 0
	local function play()
		at = 1 - at
		UI.tween(dot, 0.6, { Position = UDim2.new(at, at == 1 and -29 or 5, 0.5, 0) })
	end
	local b = UI.ghostButton(holder, "Play", "play", 36)
	b.AnchorPoint = Vector2.new(1, 0.5)
	b.Position = UDim2.new(1, 0, 0.5, 0)
	b.MouseButton1Click:Connect(play)
	playMotionPreview = play
	return function() end
end

function UI.checkbox(panel, o, x, y, ctrl)
	local boxStroke = UI.stroke(C.strokeHi, 1.5)
	local box = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.white,
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.fromOffset(x, y),
		Size = UDim2.fromOffset(22, 22),
		Parent = panel,
	}, { UI.corner(6), boxStroke, UI.accentGradient(45) })
	local mark = UI.icon(box, "check", 11, 11, 15, C.white, 2.4)
	local markScale = UI.make("UIScale", { Parent = mark })
	local label = text(panel, o.label, x + 34, y, 16, { color = C.text2, button = true, weight = MEDIUM })
	UI.pressable(box, 0.85)

	local function paint(t, pop)
		local on = o.value
		UI.tween(box, t, { BackgroundTransparency = on and 0 or 1 })
		UI.tween(boxStroke, t, { Color = on and A.a or C.strokeHi, Transparency = on and 1 or 0 })
		UI.tween(label, t, { TextColor3 = on and C.text or C.text2 })
		if on then
			mark.Visible = true
			if pop then
				markScale.Scale = 0.3
				UI.tween(markScale, 0.4, { Scale = 1 }, BACK)
			else
				markScale.Scale = 1
			end
		elseif t > 0 then
			UI.tween(markScale, 0.12, { Scale = 0.3 }, QUINT, IN).Completed:Connect(function(state)
				if state == Enum.PlaybackState.Completed and not o.value then
					mark.Visible = false
				end
			end)
		else
			mark.Visible = false
		end
	end
	local function flip()
		o.value = not o.value
		paint(0.2, true)
		controlChanged:Fire(ctrl.id, o.id, o.value)
	end
	box.MouseButton1Click:Connect(flip)
	label.MouseButton1Click:Connect(flip)
	local function hot(v)
		if not o.value then
			UI.tween(boxStroke, 0.15, { Color = v and C.sub or C.strokeHi })
		end
	end
	box.MouseEnter:Connect(function()
		hot(true)
	end)
	box.MouseLeave:Connect(function()
		hot(false)
	end)
	label.MouseEnter:Connect(function()
		hot(true)
	end)
	label.MouseLeave:Connect(function()
		hot(false)
	end)
	paint(0, false)
	return function(t)
		paint(t, t > 0)
	end, 34 + UI.textWidth(o.label, 16)
end

-- Outlined action; fires ControlChanged(ctrl.id, it.id, true). Returns its width.
function UI.actionButton(panel, it, x, y, ctrl)
	local b = UI.ghostButton(panel, it.label, it.icon, 40)
	b.AnchorPoint = Vector2.new(0, 0.5)
	b.Position = UDim2.fromOffset(x, y)
	if it.help then
		UI.tooltip(b, it.help)
	end
	b.MouseButton1Click:Connect(function()
		controlChanged:Fire(ctrl.id, it.id, true)
	end)
	return b.Size.X.Offset
end

-- Live sample text driven by the card's `size` and `weight` settings.
function UI.textPreview(panel, ctrl, y, rh)
	local values = SETTING[ctrl.id]
	local box = UI.make("Frame", {
		BackgroundColor3 = C.bg,
		BackgroundTransparency = 0.3,
		ClipsDescendants = true,
		Position = UDim2.fromOffset(24, y + 10),
		Size = UDim2.new(1, -48, 0, rh - 20),
		Parent = panel,
	}, { UI.corner(10), UI.stroke(C.line, 1) })
	local caption = text(box, "", UDim2.new(1, -14, 0, 14), 0, 12, { color = C.muted, ax = 1, weight = SEMIBOLD })
	local sample = UI.make("TextLabel", {
		BackgroundTransparency = 1,
		Position = UDim2.fromOffset(18, 6),
		Size = UDim2.new(1, -36, 1, -6),
		Text = "The quick brown fox jumps over the lazy dog",
		TextXAlignment = Enum.TextXAlignment.Left,
		TextTruncate = Enum.TextTruncate.AtEnd,
		TextColor3 = C.text,
		FontFace = UI.font(MEDIUM),
		TextSize = 22,
		Parent = box,
	})
	local weights = { Regular = Enum.FontWeight.Regular, Medium = MEDIUM, Bold = BOLD }
	local function apply(t)
		local size = values.size.value
		local w = values.weight.value
		sample.FontFace = UI.font(weights[w] or MEDIUM)
		UI.tween(sample, t or 0, { TextSize = size })
		caption.Text = string.upper(string.format("%d px  ·  %s", size, w))
	end
	apply(0)
	table.insert(rowConns, controlChanged.Event:Connect(function(id)
		if id == ctrl.id then
			apply(0.15)
		end
	end))
	return apply
end

local SETTING_BUILDERS = {
	slider = UI.slider,
	segment = UI.segmented,
	key = UI.keybind,
	swatch = UI.swatches,
	motionPreview = UI.motionPreview,
}

