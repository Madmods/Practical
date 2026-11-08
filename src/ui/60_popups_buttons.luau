-------------------------------------------------------------------- popups + tooltips
local overlay = UI.make("TextButton", {
	Text = "",
	AutoButtonColor = false,
	BackgroundTransparency = 1,
	Size = UDim2.fromScale(1, 1),
	Visible = false,
	ZIndex = 60,
	Parent = win,
})
local activePopup
closePopup = function()
	if activePopup then
		local close = activePopup
		activePopup = nil
		overlay.Visible = false
		close()
	end
end
overlay.MouseButton1Click:Connect(function()
	closePopup()
end)

-- Hover tooltip above (or below, near the top edge) `target`.
function UI.tooltip(target, str)
	if not target:IsA("GuiButton") then
		return
	end
	local tip
	local token = 0
	target.MouseEnter:Connect(function()
		token += 1
		if tip then
			tip:Destroy()
			tip = nil
		end
		local bx, by, bw, bh = UI.localBox(target)
		local tw = math.min(360, UI.textWidth(str, 15) + 28)
		local th = 32
		local tx = math.clamp(bx + bw / 2 - tw / 2, 8, math.max(8, winW - tw - 8))
		local below = by - th - 12 < 8
		local ty = below and by + bh + 10 or by - th - 10
		tip = UI.make("Frame", {
			BackgroundTransparency = 1,
			Position = UDim2.fromOffset(tx, ty + (below and -5 or 5)),
			Size = UDim2.fromOffset(tw, th),
			ZIndex = 55,
			Parent = win,
		})
		local body = UI.make("Frame", {
			BackgroundColor3 = C.raised,
			Size = UDim2.fromScale(1, 1),
			Parent = tip,
		}, { UI.corner(8), UI.stroke(C.strokeHi, 1) })
		UI.make("TextLabel", {
			BackgroundTransparency = 1,
			Size = UDim2.fromScale(1, 1),
			Text = str,
			FontFace = UI.font(MEDIUM),
			TextSize = 15,
			TextColor3 = C.text,
			Parent = body,
		})
		UI.groupFade(tip, 0, 0.2, 0.35)
		UI.tween(tip, 0.3, { Position = UDim2.fromOffset(tx, ty) }, QUINT, OUT, 0.35)
	end)
	local function hide()
		token += 1
		if tip then
			local old = tip
			tip = nil
			UI.groupFade(old, 1, 0.12).Completed:Connect(function()
				old:Destroy()
			end)
		end
	end
	target.Destroying:Connect(function()
		if tip then
			tip:Destroy()
			tip = nil
		end
	end)
	target.MouseLeave:Connect(hide)
	target.MouseButton1Down:Connect(hide)
end

-- Select box. `pos` = left edge / vertical centre. `options` is a list or a function returning one.
-- `extra` = optional { label, icon, onClick } shown under a divider at the bottom of the menu.
local function dropdown(parent, pos, w, h, options, value, onChange, extra)
	local st = UI.stroke(C.stroke, 1)
	local btn = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.raised,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = pos,
		Size = UDim2.fromOffset(w, h),
		Parent = parent,
	}, { UI.corner(10), st })
	UI.bindTheme(UI.make("Frame", {
		BorderSizePixel = 0,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromOffset(20, h / 2),
		Size = UDim2.fromOffset(8, 8),
		Parent = btn,
	}, { UI.round() }), "BackgroundColor3", "a")
	local label = UI.clip(text(btn, value, 34, h / 2, 17, { weight = MEDIUM }), w - 70)
	local chev = UI.make("Frame", {
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromOffset(w - 22, h / 2),
		Size = UDim2.fromOffset(16, 16),
		Parent = btn,
	})
	UI.spinIcon(chev, "chevronRight", C.sub, 2, 90, 0)
	UI.pressable(btn, 0.98)

	local isOpen = false
	local current = value
	btn.MouseEnter:Connect(function()
		if not isOpen then
			UI.tween(st, 0.15, { Color = C.strokeHi })
			UI.tween(btn, 0.15, { BackgroundColor3 = C.hover })
		end
	end)
	btn.MouseLeave:Connect(function()
		if not isOpen then
			UI.tween(st, 0.25, { Color = C.stroke })
			UI.tween(btn, 0.25, { BackgroundColor3 = C.raised })
		end
	end)

	local function set(v, t)
		current = v
		label.Text = v
		if t and t > 0 then
			label.TextTransparency = 1
			UI.tween(label, t, { TextTransparency = 0 })
		end
	end

	btn.MouseButton1Click:Connect(function()
		if isOpen then
			closePopup()
			return
		end
		closePopup()
		isOpen = true
		local opts = typeof(options) == "function" and options() or options
		local bx, by, bw, bh = UI.localBox(btn)
		local itemH = 40
		local menuH = #opts * itemH + 10 + (extra and itemH + 9 or 0)
		local mx, my = bx, by + bh + 6
		if my + menuH > winH - 10 then
			my = by - menuH - 6 -- open upward near the bottom edge
		end
		local M = 12 -- margin so the shadow survives the fade's CanvasGroup
		local menu = UI.make("Frame", {
			BackgroundTransparency = 1,
			Position = UDim2.fromOffset(mx - M, my - M - 8),
			Size = UDim2.fromOffset(bw + M * 2, menuH + M * 2),
			ZIndex = 61,
			Parent = win,
		})
		local shade = UI.make("Frame", {
			BackgroundTransparency = 1,
			Position = UDim2.fromOffset(M, M),
			Size = UDim2.fromOffset(bw, menuH),
			Parent = menu,
		})
		UI.softShadow(shade, 12, 4, 3, 0.07, 5)
		local body = UI.make("Frame", {
			BackgroundColor3 = C.raised,
			Position = UDim2.fromOffset(M, M),
			Size = UDim2.fromOffset(bw, menuH),
			ZIndex = 2,
			Parent = menu,
		}, { UI.corner(12), UI.stroke(C.strokeHi, 1) })

		local function item(i, labelText, isSelected, iconName, onClick)
			local y0 = 5 + (i - 1) * itemH + (i > #opts and 9 or 0)
			local it = UI.make("TextButton", {
				Text = "",
				AutoButtonColor = false,
				BackgroundColor3 = isSelected and A.dim or C.hover,
				BackgroundTransparency = isSelected and 0 or 1,
				Position = UDim2.fromOffset(5, y0),
				Size = UDim2.fromOffset(bw - 10, itemH - 2),
				Parent = body,
			}, { UI.corner(8) })
			local l = UI.clip(text(it, labelText, iconName and not isSelected and 42 or 14, (itemH - 2) / 2, 16, {
				color = isSelected and C.white or C.text2,
				weight = isSelected and SEMIBOLD or nil,
			}), bw - 70)
			if isSelected then
				UI.icon(it, "check", UDim2.new(1, -20, 0.5, 0), nil, 16, A.hi, 2.2)
			elseif iconName then
				UI.icon(it, iconName, 24, (itemH - 2) / 2, 16, C.sub, 2)
			end
			if not isSelected then
				it.MouseEnter:Connect(function()
					UI.tween(it, 0.12, { BackgroundTransparency = 0 })
					UI.tween(l, 0.12, { TextColor3 = C.white })
				end)
				it.MouseLeave:Connect(function()
					UI.tween(it, 0.2, { BackgroundTransparency = 1 })
					UI.tween(l, 0.2, { TextColor3 = C.text2 })
				end)
			end
			it.MouseButton1Click:Connect(onClick)
		end
		for i, opt in ipairs(opts) do
			item(i, opt, opt == current, nil, function()
				closePopup()
				if opt ~= current then
					set(opt, 0.25)
					onChange(opt)
				end
			end)
		end
		if extra then
			UI.strip(body, UDim2.fromOffset(12, 5 + #opts * itemH + 4), UDim2.new(1, -24, 0, 1), C.line)
			item(#opts + 1, extra.label, false, extra.icon, function()
				closePopup()
				extra.onClick()
			end)
		end

		overlay.Visible = true
		UI.groupFade(menu, 0, 0.25)
		UI.tween(menu, 0.32, { Position = UDim2.fromOffset(mx - M, my - M) })
		UI.spinIcon(chev, "chevronRight", C.sub, 2, -90, 0.3)
		UI.tween(st, 0.2, { Color = A.a })
		activePopup = function()
			isOpen = false
			UI.spinIcon(chev, "chevronRight", C.sub, 2, 90, 0.3)
			UI.tween(st, 0.25, { Color = C.stroke })
			UI.tween(btn, 0.25, { BackgroundColor3 = C.raised })
			UI.tween(menu, 0.15, { Position = UDim2.fromOffset(mx - M, my - M - 4) })
			UI.groupFade(menu, 1, 0.15).Completed:Connect(function()
				menu:Destroy()
			end)
		end
	end)

	return { button = btn, set = set }
end

-------------------------------------------------------------------- buttons
-- Icon-only button. `pos` is its centre.
function UI.iconButton(parent, pos, size, iconName, opts)
	opts = opts or {}
	local st = opts.solid and UI.stroke(C.stroke, 1) or nil
	local b = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = opts.solid and C.raised or (opts.hoverColor or C.hover),
		BackgroundTransparency = opts.solid and 0 or 1,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = pos,
		Size = UDim2.fromOffset(size, size),
		ZIndex = opts.z or 1,
		Parent = parent,
	}, { UI.corner(opts.radius or 10), st })
	local idle = opts.color or C.text2
	local ic = UI.icon(b, iconName, size / 2, size / 2, opts.iconSize or 18, idle, opts.thickness or 1.8)
	b.MouseEnter:Connect(function()
		if opts.solid then
			UI.tween(b, 0.15, { BackgroundColor3 = opts.hoverColor or C.hover })
			UI.tween(st, 0.15, { Color = opts.hoverColor and opts.hoverColor or C.strokeHi })
		else
			UI.tween(b, 0.15, { BackgroundTransparency = 0 })
		end
		UI.tintIcon(ic, opts.hot or C.white, 0.15)
	end)
	b.MouseLeave:Connect(function()
		if opts.solid then
			UI.tween(b, 0.25, { BackgroundColor3 = C.raised })
			UI.tween(st, 0.25, { Color = C.stroke })
		else
			UI.tween(b, 0.25, { BackgroundTransparency = 1 })
		end
		UI.tintIcon(ic, idle, 0.25)
	end)
	UI.pressable(b, 0.88)
	if opts.tip then
		UI.tooltip(b, opts.tip)
	end
	return b, ic
end

-- Quiet outlined button with an optional leading icon. Width fits the label.
function UI.ghostButton(parent, label, iconName, h)
	h = h or 40
	local w = math.round((iconName and 42 or 18) + UI.textWidth(label, 16) + 18)
	local st = UI.stroke(C.stroke, 1)
	local b = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.raised,
		Size = UDim2.fromOffset(w, h),
		Parent = parent,
	}, { UI.corner(10), st })
	local ic = iconName and UI.icon(b, iconName, 23, h / 2, 17, C.text2, 1.8) or nil
	if ic then
		ic.ZIndex = 2
	end
	local l = text(b, label, iconName and 42 or 18, h / 2, 16, { weight = MEDIUM, color = C.text2, z = 2 })
	b.MouseEnter:Connect(function()
		UI.tween(b, 0.15, { BackgroundColor3 = C.hover })
		UI.tween(st, 0.15, { Color = C.strokeHi })
		UI.tween(l, 0.15, { TextColor3 = C.white })
		if ic then
			UI.tintIcon(ic, C.white, 0.15)
		end
	end)
	b.MouseLeave:Connect(function()
		UI.tween(b, 0.25, { BackgroundColor3 = C.raised })
		UI.tween(st, 0.25, { Color = C.stroke })
		UI.tween(l, 0.25, { TextColor3 = C.text2 })
		if ic then
			UI.tintIcon(ic, C.text2, 0.25)
		end
	end)
	UI.pressable(b)
	UI.ripple(b)
	return b, l, ic
end

-- Gradient call-to-action with a hover sheen and a glow ring.
function UI.primaryButton(parent, label, iconName, h)
	h = h or 42
	local w = math.round((iconName and 44 or 22) + UI.textWidth(label, 17) + 22)
	local glow = UI.bindTheme(UI.stroke(A.hi, 2, 1), "Color", "hi")
	local idle = Color3.fromRGB(226, 229, 240)
	local b = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = idle,
		Size = UDim2.fromOffset(w, h),
		Parent = parent,
	}, { UI.corner(10), glow, UI.accentGradient(20) })
	local ic = iconName and UI.icon(b, iconName, 24, h / 2, 18, C.white, 1.9) or nil
	if ic then
		ic.ZIndex = 2
	end
	local l = text(b, label, iconName and 44 or 22, h / 2, 17, { weight = SEMIBOLD, color = C.white, z = 2 })
	b.MouseEnter:Connect(function()
		UI.tween(b, 0.2, { BackgroundColor3 = C.white })
		UI.tween(glow, 0.25, { Transparency = 0.45 })
	end)
	b.MouseLeave:Connect(function()
		UI.tween(b, 0.3, { BackgroundColor3 = idle })
		UI.tween(glow, 0.3, { Transparency = 1 })
	end)
	UI.shimmer(b)
	UI.ripple(b)
	UI.pressable(b)
	return b, l, ic
end

-- Rounded square with the brand gradient and logo glyph.
function UI.logoMark(parent, x, y, size)
	local r = math.floor(size * 0.3)
	UI.bindTheme(UI.make("Frame", {
		BackgroundTransparency = 0.9,
		BorderSizePixel = 0,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromOffset(x, y),
		Size = UDim2.fromOffset(size + 16, size + 16),
		Parent = parent,
	}, { UI.corner(r + 8) }), "BackgroundColor3", "a")
	local mark = UI.make("Frame", {
		BackgroundColor3 = C.white,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromOffset(x, y),
		Size = UDim2.fromOffset(size, size),
		Parent = parent,
	}, { UI.corner(r), UI.accentGradient(45) })
	UI.icon(mark, "logo", size / 2, size / 2, math.floor(size * 0.56), C.white, 1.8)
	return mark
end

