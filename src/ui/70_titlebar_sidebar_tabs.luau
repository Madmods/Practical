-------------------------------------------------------------------- title bar
local titleBar = UI.make("Frame", {
	BackgroundTransparency = 1,
	Size = UDim2.new(1, 0, 0, TITLE_H),
	Active = true,
	Parent = win,
})
UI.strip(win, UDim2.fromOffset(0, TITLE_H), UDim2.new(1, 0, 0, 1), C.line)

UI.logoMark(titleBar, 38, TITLE_H / 2, 34)
text(titleBar, "practical", 68, TITLE_H / 2, 21, { weight = SEMIBOLD })
do
	local x = 68 + UI.textWidth("practical", 21) + 12
	local chip = UI.make("Frame", {
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.fromOffset(x, TITLE_H / 2 + 1),
		Size = UDim2.fromOffset(UI.textWidth(VERSION, 13) + 16, 20),
		Parent = titleBar,
	}, { UI.corner(6) })
	UI.bindTheme(chip, "BackgroundColor3", "dim")
	UI.bindTheme(UI.stroke(A.soft, 1), "Color", "soft").Parent = chip
	UI.bindTheme(UI.make("TextLabel", {
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Text = VERSION,
		FontFace = UI.font(SEMIBOLD),
		TextSize = 13,
		Parent = chip,
	}), "TextColor3", "hi")
end

-- search: lives in the title bar, centred over the content column
local searchStroke = UI.stroke(C.line, 1)
local search = UI.make("Frame", {
	BackgroundColor3 = C.field,
	BackgroundTransparency = 0.25,
	AnchorPoint = Vector2.new(0.5, 0.5),
	Position = UDim2.new(0.5, SIDE_W / 2 - 60, 0, TITLE_H / 2),
	Size = UDim2.fromOffset(420, 40),
	ZIndex = 3,
	Parent = win,
}, { UI.corner(12), searchStroke })
local searchIcon = UI.icon(search, "search", 22, 20, 17, C.sub, 1.9)
local searchBox = UI.make("TextBox", {
	BackgroundTransparency = 1,
	Position = UDim2.fromOffset(42, 0),
	Size = UDim2.new(1, -124, 1, 0),
	Text = "",
	PlaceholderText = "Search settings",
	PlaceholderColor3 = C.muted,
	TextColor3 = C.text,
	FontFace = UI.font(),
	TextSize = 16,
	TextXAlignment = Enum.TextXAlignment.Left,
	TextTruncate = Enum.TextTruncate.AtEnd,
	ClearTextOnFocus = false,
	Parent = search,
})
local kbd = UI.make("Frame", {
	BackgroundColor3 = C.raised,
	AnchorPoint = Vector2.new(1, 0.5),
	Position = UDim2.new(1, -10, 0.5, 0),
	Size = UDim2.fromOffset(58, 24),
	Parent = search,
}, { UI.corner(6), UI.stroke(C.stroke, 1) })
UI.make("TextLabel", {
	BackgroundTransparency = 1,
	Size = UDim2.fromScale(1, 1),
	Text = "Ctrl K",
	FontFace = UI.font(SEMIBOLD),
	TextSize = 13,
	TextColor3 = C.sub,
	Parent = kbd,
})
local clearSearch = UI.iconButton(search, UDim2.new(1, -24, 0.5, 0), 28, "close", { iconSize = 13, radius = 8 })
clearSearch.Visible = false
clearSearch.MouseButton1Click:Connect(function()
	searchBox.Text = ""
end)
searchBox.Focused:Connect(function()
	UI.tween(searchStroke, 0.2, { Color = A.a })
	UI.tween(search, 0.2, { BackgroundTransparency = 0 })
	UI.tintIcon(searchIcon, A.hi, 0.2)
end)
searchBox.FocusLost:Connect(function()
	UI.tween(searchStroke, 0.3, { Color = C.line })
	UI.tween(search, 0.3, { BackgroundTransparency = 0.25 })
	UI.tintIcon(searchIcon, C.sub, 0.3)
end)

local clock = text(win, os.date("%H:%M"), UDim2.new(1, -112, 0, TITLE_H / 2), 0, 15, { color = C.sub, ax = 1, weight = MEDIUM })
local minBtn = UI.iconButton(win, UDim2.new(1, -78, 0, TITLE_H / 2), 36, "minus", { tip = "Minimize" })
local closeBtn = UI.iconButton(win, UDim2.new(1, -36, 0, TITLE_H / 2), 36, "close", { hoverColor = C.dangerFill, tip = "Close" })
task.spawn(function()
	while alive do
		clock.Text = os.date("%H:%M")
		task.wait(10)
	end
end)

-------------------------------------------------------------------- sidebar
local rail = UI.make("Frame", {
	Name = "Sidebar",
	BackgroundColor3 = C.rail,
	BorderSizePixel = 0,
	Position = UDim2.fromOffset(0, TITLE_H + 1),
	Size = UDim2.new(0, SIDE_W, 1, -TITLE_H - 1),
	Parent = win,
}, { UI.corner(16) })
-- square off every corner except bottom-left, which follows the window's rounding
UI.strip(rail, UDim2.fromOffset(0, 0), UDim2.new(1, 0, 0, 20), C.rail)
UI.strip(rail, UDim2.new(1, -20, 0, 0), UDim2.new(0, 20, 1, 0), C.rail)
local railFillers = {}
for _, child in ipairs(rail:GetChildren()) do
	if child:IsA("Frame") then
		child.ZIndex = 0 -- under the nav highlight
		table.insert(railFillers, child)
	end
end
UI.strip(win, UDim2.fromOffset(SIDE_W, TITLE_H + 1), UDim2.new(0, 1, 1, -TITLE_H - 1), C.line)

local NAV_X, NAV_W, NAV_H, NAV_STEP = 14, SIDE_W - 28, 46, 50
UI.tracked(rail, "MENU", 26, 30, 12, C.muted, 1.8)

local navHighlight = UI.make("Frame", {
	BackgroundColor3 = C.white,
	BackgroundTransparency = 0.84,
	BorderSizePixel = 0,
	Position = UDim2.fromOffset(NAV_X, 52),
	Size = UDim2.fromOffset(NAV_W, NAV_H),
	Parent = rail,
}, {
	UI.corner(10),
	UI.accentGradient(0, NumberSequence.new(0, 0.8)),
	UI.bindTheme(UI.stroke(A.a, 1, 0.65), "Color", "a"),
})
UI.bindTheme(UI.make("Frame", {
	BorderSizePixel = 0,
	AnchorPoint = Vector2.new(0, 0.5),
	Position = UDim2.new(0, 0, 0.5, 0),
	Size = UDim2.fromOffset(3, 20),
	Parent = navHighlight,
}, { UI.round() }), "BackgroundColor3", "hi")

UI.navScroll=UI.make("ScrollingFrame",{
 Name="NavigationScroll",BackgroundTransparency=1,BorderSizePixel=0,
 Position=UDim2.fromOffset(0,46),Size=UDim2.new(1,0,1,-222),
 CanvasSize=UDim2.fromOffset(0,0),ScrollBarThickness=3,ScrollBarImageColor3=C.sub,
 Parent=rail,
})
local nav = {} -- page name -> { btn, icon, label, badge, badgeText, pos, page }
UI.navScroll.CanvasSize=UDim2.fromOffset(0,4+(#PAGES-1)*NAV_STEP)
local currentPage
local renderedQuery = ""

do
	local y = 4
	for _, page in ipairs(PAGES) do
		local pos
		if page.bottom then
			pos = UDim2.new(0, NAV_X, 1, -94 - NAV_H)
			UI.tracked(rail, "SYSTEM", UDim2.new(0, 26, 1, -94 - NAV_H - 18), nil, 12, C.muted, 1.8)
		else
			pos = UDim2.fromOffset(NAV_X, y)
			y += NAV_STEP
		end
		local name = page.name
		local btn = UI.make("TextButton", {
			Text = "",
			AutoButtonColor = false,
			BackgroundColor3 = C.hover,
			BackgroundTransparency = 1,
			Position = pos,
			Size = UDim2.fromOffset(NAV_W, NAV_H),
			ZIndex = 2,
			Parent = page.bottom and rail or UI.navScroll,
		}, { UI.corner(10) })
		local ic = UI.icon(btn, page.icon, 26, NAV_H / 2, 19, C.sub, 1.7)
		local label = text(btn, name, 50, NAV_H / 2, 17, { color = C.text2, weight = MEDIUM })
		local badge = UI.make("Frame", {
			BackgroundColor3 = C.field,
			AnchorPoint = Vector2.new(1, 0.5),
			Position = UDim2.new(1, -12, 0.5, 0),
			Size = UDim2.fromOffset(24, 20),
			Visible = false,
			Parent = btn,
		}, { UI.round() })
		local badgeText = UI.make("TextLabel", {
			BackgroundTransparency = 1,
			Size = UDim2.fromScale(1, 1),
			Text = "0",
			FontFace = UI.font(SEMIBOLD),
			TextSize = 13,
			TextColor3 = C.sub,
			Parent = badge,
		})
		nav[name] = { btn = btn, icon = ic, label = label, badge = badge, badgeText = badgeText, pos = pos, page = page }
		btn.MouseEnter:Connect(function()
			if currentPage ~= name then
				UI.tween(btn, 0.15, { BackgroundTransparency = 0.4 })
				UI.tween(label, 0.15, { TextColor3 = C.white })
			end
		end)
		btn.MouseLeave:Connect(function()
			UI.tween(btn, 0.25, { BackgroundTransparency = 1 })
			if currentPage ~= name then
				UI.tween(label, 0.25, { TextColor3 = C.text2 })
			end
		end)
		UI.pressable(btn, 0.97)
		btn.MouseButton1Click:Connect(function()
			openPage(name)
		end)
	end
end

refreshNav = function(t)
	t = t or 0.25
	for name, it in pairs(nav) do
		local sel = name == currentPage
		UI.tween(it.label, t, { TextColor3 = sel and C.white or C.text2 })
		UI.tintIcon(it.icon, sel and C.white or C.sub, t)
		local on, total = UI.pageCounts(it.page)
		it.badge.Visible = total > 0 and on > 0
		it.badgeText.Text = tostring(on)
		UI.tween(it.badge, t, { BackgroundColor3 = sel and A.dim or C.field })
		UI.tween(it.badgeText, t, { TextColor3 = sel and A.hi or C.sub })
	end
end

-- who's here
do
	local card = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.surface,
		Position = UDim2.new(0, NAV_X, 1, -80),
		Size = UDim2.fromOffset(NAV_W, 64),
		Parent = rail,
	}, { UI.corner(12), UI.stroke(C.line, 1) })
	local avatarRing = UI.bindTheme(UI.stroke(A.a, 2, 0.2), "Color", "a")
	local avatar = UI.make("ImageLabel", {
		BackgroundColor3 = C.field,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, 12, 0.5, 0),
		Size = UDim2.fromOffset(40, 40),
		Image = "",
		Parent = card,
	}, { UI.round(), avatarRing })
	-- initial letter until (unless) the headshot loads
	local initial = UI.make("TextLabel", {
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Text = string.upper(string.sub(player.DisplayName, 1, 1)),
		FontFace = UI.font(BOLD),
		TextSize = 18,
		TextColor3 = C.text2,
		Parent = avatar,
	})
	local online = UI.make("Frame", {
		BackgroundColor3 = C.green,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.new(1, -4, 1, -4),
		Size = UDim2.fromOffset(11, 11),
		ZIndex = 2,
		Parent = avatar,
	}, { UI.round(), UI.stroke(C.surface, 2) })
	online.ZIndex = 3
	UI.clip(text(card, player.DisplayName, 64, 24, 16, { weight = SEMIBOLD }), NAV_W - 80)
	UI.clip(text(card, "@" .. player.Name, 64, 43, 13, { color = C.sub }), NAV_W - 80)
	card.MouseEnter:Connect(function()
		UI.tween(card, 0.15, { BackgroundColor3 = C.raised })
	end)
	card.MouseLeave:Connect(function()
		UI.tween(card, 0.25, { BackgroundColor3 = C.surface })
	end)
	UI.pressable(card, 0.98)
	card.MouseButton1Click:Connect(function()
		openPage("Preferences")
	end)
	task.spawn(function()
		local ok, img = pcall(function()
			return Players:GetUserThumbnailAsync(player.UserId, Enum.ThumbnailType.HeadShot, Enum.ThumbnailSize.Size100x100)
		end)
		if ok and alive and img then
			avatar.Image = img
			initial.Visible = false
		end
	end)
end

-------------------------------------------------------------------- page header + tabs
local LIST_TOP, LIST_TOP_TABS = TITLE_H + 138, TITLE_H + 176

local crumb = text(win, "", CONTENT_X, TITLE_H + 32, 14, { color = C.sub, rich = true, weight = MEDIUM })
local pageTitle = text(win, "", CONTENT_X, TITLE_H + 64, 30, { weight = BOLD })
local pageSub = text(win, "", CONTENT_X, TITLE_H + 96, 16, { color = C.sub })

text(win, "PROFILE", UDim2.new(1, -32 - 230, 0, TITLE_H + 32), 0, 12, { color = C.muted, weight = SEMIBOLD })

local tabBar = UI.make("Frame", {
	BackgroundTransparency = 1,
	Position = UDim2.fromOffset(CONTENT_X, TITLE_H + 120),
	Size = UDim2.new(1, -CONTENT_X - 32, 0, 42),
	Visible = false,
	Parent = win,
})
local tabUnderline = UI.make("Frame", {
	BackgroundColor3 = C.white,
	BorderSizePixel = 0,
	AnchorPoint = Vector2.new(0, 1),
	Position = UDim2.new(0, 0, 1, 0),
	Size = UDim2.fromOffset(40, 2),
	ZIndex = 2,
	Parent = tabBar,
}, { UI.round(), UI.accentGradient(0) })
local headerLine = UI.strip(win, UDim2.fromOffset(CONTENT_X, TITLE_H + 124), UDim2.new(1, -CONTENT_X - 32, 0, 1), C.line)

local tabButtons = {}
local tabsFor -- page whose tabs are built
local currentTab = {} -- page name -> selected tab

function UI.riseIn(label, delay)
	local p = label.Position
	label.TextTransparency = 1
	label.Position = p + UDim2.fromOffset(0, 10)
	UI.tween(label, 0.5, { TextTransparency = 0, Position = p }, QUINT, OUT, delay)
end

function UI.paintTabs(t)
	local page = PAGE_BY_NAME[tabsFor]
	if not page then
		return
	end
	local sel = currentTab[page.name]
	for name, b in pairs(tabButtons) do
		local on = name == sel
		UI.tween(b, t, { TextColor3 = on and C.white or C.sub })
		if on then
			UI.tween(tabUnderline, t, {
				Position = UDim2.new(0, b.Position.X.Offset, 1, 0),
				Size = UDim2.fromOffset(b.Size.X.Offset, 2),
			})
		end
	end
end

function UI.buildTabs(page)
	for _, b in pairs(tabButtons) do
		b:Destroy()
	end
	table.clear(tabButtons)
	tabsFor = page and page.tabs and page.name or nil
	tabBar.Visible = tabsFor ~= nil
	headerLine.Position = UDim2.fromOffset(CONTENT_X, TITLE_H + (tabsFor and 162 or 124))
	if not tabsFor then
		return
	end
	currentTab[page.name] = currentTab[page.name] or page.tabs[1]
	local x = 0
	for _, name in ipairs(page.tabs) do
		local w = math.ceil(UI.textWidth(name, 16))
		local b = UI.make("TextButton", {
			AutoButtonColor = false,
			BackgroundTransparency = 1,
			Text = name,
			FontFace = UI.font(SEMIBOLD),
			TextSize = 16,
			TextColor3 = C.sub,
			Position = UDim2.fromOffset(x, 0),
			Size = UDim2.fromOffset(w, 40),
			Parent = tabBar,
		})
		tabButtons[name] = b
		b.MouseEnter:Connect(function()
			if currentTab[page.name] ~= name then
				UI.tween(b, 0.15, { TextColor3 = C.text2 })
			end
		end)
		b.MouseLeave:Connect(function()
			if currentTab[page.name] ~= name then
				UI.tween(b, 0.2, { TextColor3 = C.sub })
			end
		end)
		b.MouseButton1Click:Connect(function()
			if currentTab[page.name] ~= name or renderedQuery ~= "" then
				currentTab[page.name] = name
				UI.paintTabs(0.35)
				openPage(page.name, true)
			end
		end)
		x += w + 30
	end
	UI.paintTabs(0)
end

