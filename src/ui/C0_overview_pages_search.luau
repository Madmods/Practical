-------------------------------------------------------------------- live stats (overview)
local stats = { fps = 60, ping = 0, fpsHist = {}, pingHist = {}, started = os.clock() }
do
	local frames, acc = 0, 0
	UI.connect(RunService.RenderStepped, function(dt)
		frames += 1
		acc += dt
		if acc >= 0.5 then
			stats.fps = frames / acc
			frames, acc = 0, 0
			local ok, p = pcall(function()
				return player:GetNetworkPing()
			end)
			stats.ping = ok and (p or 0) * 1000 or 0
			table.insert(stats.fpsHist, stats.fps)
			table.insert(stats.pingHist, stats.ping)
			if #stats.fpsHist > 30 then
				table.remove(stats.fpsHist, 1)
				table.remove(stats.pingHist, 1)
			end
			if statsListener then
				statsListener()
			end
		end
	end)
end

function UI.maxOf(t, floor)
	local m = floor
	for _, v in ipairs(t) do
		m = math.max(m, v)
	end
	return m
end

function UI.statTile(parent, label, iconName, order)
	local tile = UI.make("Frame", {
		BackgroundColor3 = C.surface,
		Size = UDim2.new(0.25, -10.5, 1, -4),
		LayoutOrder = order,
		Parent = parent,
	}, { UI.corner(14), UI.stroke(C.stroke, 1) })
	text(tile, string.upper(label), 18, 24, 12, { weight = BOLD, color = C.muted })
	UI.icon(tile, iconName, UDim2.new(1, -26, 0, 24), nil, 17, C.muted, 1.7)
	local value = text(tile, "–", 18, 58, 30, { weight = BOLD, rich = true })
	return tile, value
end

-- bottom-aligned bar chart; older bars fade out
function UI.sparkline(parent, n)
	local area = UI.make("Frame", {
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0, 1),
		Position = UDim2.new(0, 18, 1, -16),
		Size = UDim2.new(1, -36, 0, 28),
		Parent = parent,
	})
	local bars = {}
	for i = 1, n do
		bars[i] = UI.make("Frame", {
			BackgroundColor3 = A.a,
			BackgroundTransparency = 0.1 + 0.7 * (1 - i / n),
			BorderSizePixel = 0,
			AnchorPoint = Vector2.new(0, 1),
			Position = UDim2.new((i - 1) / n, 0, 1, 0),
			Size = UDim2.new(1 / n, -3, 0, 2),
			Parent = area,
		}, { UI.corner(2) })
	end
	return function(hist, hi)
		for i = 1, n do
			local v = hist[#hist - n + i]
			local k = v and math.clamp(v / math.max(hi, 1e-3), 0.06, 1) or 0.06
			UI.tween(bars[i], 0.35, { Size = UDim2.new(1 / n, -3, 0, math.max(2, k * 28)), BackgroundColor3 = A.a })
		end
	end
end

function UI.greeting()
	local hour = os.date("*t").hour
	if hour < 12 then
		return "Good morning"
	elseif hour < 18 then
		return "Good afternoon"
	end
	return "Good evening"
end

function UI.renderOverview(page)
	local order = 0
	local function nextOrder()
		order += 1
		return order
	end

	-- hero banner
	local _, heroBody = UI.newSlot(176, nextOrder(), "hero")
	local hero = UI.make("Frame", {
		BackgroundColor3 = C.surface,
		Position = UDim2.fromOffset(2, 2),
		Size = UDim2.new(1, -4, 1, -4),
		Parent = heroBody,
	}, { UI.corner(16), UI.bindTheme(UI.stroke(A.soft, 1), "Color", "soft") })
	local art = UI.make("CanvasGroup", {
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Parent = hero,
	}, { UI.corner(16) })
	UI.make("Frame", {
		BackgroundColor3 = C.white,
		BorderSizePixel = 0,
		Size = UDim2.fromScale(1, 1),
		Parent = art,
	}, {
		UI.accentGradient(0, NumberSequence.new({
			NumberSequenceKeypoint.new(0, 0.68),
			NumberSequenceKeypoint.new(0.55, 0.9),
			NumberSequenceKeypoint.new(1, 0.78),
		})),
	})
	for i, d in ipairs({ 150, 250, 350, 450 }) do
		local ring = UI.make("Frame", {
			BackgroundTransparency = 1,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Position = UDim2.new(1, -110, 0.5, 0),
			Size = UDim2.fromOffset(d, d),
			Parent = art,
		}, { UI.round() })
		UI.bindTheme(UI.stroke(A.hi, 1, 0.5 + i * 0.1), "Color", "hi").Parent = ring
	end
	local orbiters = {}
	for i, spec in ipairs({ { r = 125, size = 10, speed = 0.55 }, { r = 175, size = 7, speed = -0.35 }, { r = 75, size = 6, speed = 0.9 } }) do
		local dot = UI.make("Frame", {
			BackgroundColor3 = C.white,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Size = UDim2.fromOffset(spec.size, spec.size),
			Parent = art,
		}, { UI.round() })
		orbiters[i] = { dot = dot, r = spec.r, speed = spec.speed, phase = i * 2.2 }
	end
	local t0 = os.clock()
	table.insert(rowConns, RunService.RenderStepped:Connect(function()
		local t = os.clock() - t0
		for _, o in ipairs(orbiters) do
			local a = o.phase + t * o.speed
			o.dot.Position = UDim2.new(1, -110 + math.cos(a) * o.r, 0.5, math.sin(a) * o.r)
		end
	end))
	-- hero logo in the middle of the rings
	local heroMark = UI.make("Frame", {
		BackgroundColor3 = C.white,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.new(1, -110, 0.5, 0),
		Size = UDim2.fromOffset(52, 52),
		Parent = hero,
	}, { UI.corner(16), UI.accentGradient(45), UI.bindTheme(UI.stroke(A.hi, 1, 0.3), "Color", "hi") })
	UI.icon(heroMark, "logo", 26, 26, 28, C.white, 2)

	text(hero, UI.greeting() .. ", " .. player.DisplayName, 30, 46, 30, { weight = BOLD })
	UI.make("TextLabel", {
		BackgroundTransparency = 1,
		Position = UDim2.fromOffset(30, 66),
		Size = UDim2.new(1, -320, 0, 44),
		Text = "Every control in this window changes the window itself. Try a new accent, a motion curve or a denser layout.",
		TextWrapped = true,
		TextXAlignment = Enum.TextXAlignment.Left,
		TextYAlignment = Enum.TextYAlignment.Top,
		FontFace = UI.font(),
		TextSize = 16,
		TextColor3 = C.text2,
		Parent = hero,
	})
	local tryBtn = UI.primaryButton(hero, "Preview notification", "bell", 40)
	tryBtn.AnchorPoint = Vector2.new(0, 0.5)
	tryBtn.Position = UDim2.fromOffset(30, 136)
	tryBtn.MouseButton1Click:Connect(function()
		toast("Hello there", "This is how a notification looks.", "bell", true)
	end)
	local themeBtn = UI.ghostButton(hero, "Customize theme", "sparkle", 40)
	themeBtn.AnchorPoint = Vector2.new(0, 0.5)
	themeBtn.Position = UDim2.fromOffset(30 + tryBtn.Size.X.Offset + 10, 136)
	themeBtn.MouseButton1Click:Connect(function()
		currentTab.Workspace = "Appearance"
		openPage("Workspace", true)
	end)

	-- live stat tiles
	if layoutState.captions then
		caption(nextOrder(), "LIVE")
	end
	local _, statsBody = UI.newSlot(128, nextOrder(), "stats")
	local row = UI.make("Frame", { BackgroundTransparency = 1, Position = UDim2.fromOffset(2, 2), Size = UDim2.new(1, -4, 1, 0), Parent = statsBody })
	UI.make("UIListLayout", {
		FillDirection = Enum.FillDirection.Horizontal,
		Padding = UDim.new(0, 14),
		SortOrder = Enum.SortOrder.LayoutOrder,
		Parent = row,
	})
	local fpsTile, fpsValue = UI.statTile(row, "Frame rate", "bolt", 1)
	local fpsSpark = UI.sparkline(fpsTile, 22)
	local pingTile, pingValue = UI.statTile(row, "Latency", "signal", 2)
	local pingSpark = UI.sparkline(pingTile, 22)
	local upTile, upValue = UI.statTile(row, "Session", "clock", 3)
	text(upTile, "since this window opened", 18, 96, 14, { color = C.sub })
	local featTile, featValue = UI.statTile(row, "Features", "layers", 4)
	local featTrack = UI.make("Frame", {
		BackgroundColor3 = C.field,
		BorderSizePixel = 0,
		Position = UDim2.new(0, 18, 1, -26),
		Size = UDim2.new(1, -36, 0, 6),
		Parent = featTile,
	}, { UI.round() })
	local featFill = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BorderSizePixel = 0,
		Size = UDim2.fromScale(0, 1),
		Parent = featTrack,
	}, { UI.round(), UI.accentGradient(0) })

	local unit = '<font size="15" color="#848DA2"> %s</font>'
	statsListener = function()
		fpsValue.Text = math.round(stats.fps) .. string.format(unit, "fps")
		pingValue.Text = math.round(stats.ping) .. string.format(unit, "ms")
		local up = math.floor(os.clock() - stats.started)
		upValue.Text = string.format("%d:%02d", math.floor(up / 60), up % 60)
		local on, total = UI.featureCounts()
		featValue.Text = on .. string.format(unit, "of " .. total)
		UI.tween(featFill, 0.4, { Size = UDim2.fromScale(total > 0 and on / total or 0, 1) })
		fpsSpark(stats.fpsHist, UI.maxOf(stats.fpsHist, 60))
		pingSpark(stats.pingHist, UI.maxOf(stats.pingHist, 80))
	end
	statsListener()

	-- the page's own feature cards
	if layoutState.captions then
		caption(nextOrder(), "FEATURES")
	end
	for _, ctrl in ipairs(page.controls) do
		UI.buildControl(ctrl, nextOrder())
	end
end

-------------------------------------------------------------------- pages + search
function UI.clearList()
	closePopup()
	for _, conn in ipairs(rowConns) do
		conn:Disconnect()
	end
	table.clear(rowConns)
	table.clear(rowAppliers)
	if rebinding then
		rebinding = nil
	end
	configNameBox = nil
	statsListener = nil
	playMotionPreview = nil
	for _, child in ipairs(list:GetChildren()) do
		if child:IsA("GuiObject") then
			child:Destroy()
		end
	end
end

function UI.animateEntrance()
	local kids = {}
	for _, c in ipairs(list:GetChildren()) do
		if c:IsA("GuiObject") then
			table.insert(kids, c)
		end
	end
	table.sort(kids, function(a, b)
		return a.LayoutOrder < b.LayoutOrder
	end)
	for i, slot in ipairs(kids) do
		local body = slot:FindFirstChild("Body")
		if body then
			local d = math.min((i - 1) * 0.045, 0.27)
			body.Position = UDim2.fromOffset(0, 16)
			UI.groupFade(body, 0, 0.3, d)
			UI.tween(body, 0.45, { Position = UDim2.fromOffset(0, 0) }, QUINT, OUT, d)
		end
	end
end

function UI.setHeader(crumbText, title, subtitle, animate)
	crumb.Text = crumbText
	pageTitle.Text = title
	pageSub.Text = subtitle
	if animate then
		crumb.Position = UDim2.fromOffset(CONTENT_X, TITLE_H + 32)
		pageTitle.Position = UDim2.fromOffset(CONTENT_X, TITLE_H + 64)
		pageSub.Position = UDim2.fromOffset(CONTENT_X, TITLE_H + 96)
		UI.riseIn(crumb, 0)
		UI.riseIn(pageTitle, 0.03)
		UI.riseIn(pageSub, 0.06)
	end
end

local CRUMB_SEP = '  <font color="#586074">/</font>  '

renderPage = function(name, animate)
	if animate == nil then
		animate = true
	end
	local page = PAGE_BY_NAME[name]
	local scroll = list.CanvasPosition
	UI.clearList()
	renderedQuery = ""
	if tabsFor ~= (page.tabs and name or nil) then
		UI.buildTabs(page)
	end
	UI.setListTop(page.tabs and LIST_TOP_TABS or LIST_TOP, animate and 0.3 or 0)

	local tab = page.tabs and currentTab[name]
	UI.setHeader("practical" .. CRUMB_SEP .. name .. (tab and (CRUMB_SEP .. tab) or ""), name, page.subtitle or "", animate)

	local count = 0
	if page.custom == "configs" then
		UI.renderConfigs()
		count = 1
	elseif page.custom == "overview" then
		UI.renderOverview(page)
		count = 1
	else
		local order = 0
		for _, ctrl in ipairs(page.controls) do
			if not page.tabs or ctrl.tab == tab then
				order += 1
				UI.buildControl(ctrl, order)
			end
		end
		count = order
	end
	emptyLabel.Visible = count == 0
	emptyLabel.Text = "Nothing here yet"
	if animate then
		list.CanvasPosition = Vector2.zero
		UI.animateEntrance()
	else
		list.CanvasPosition = scroll
	end
end

-- text a control can be found by: its name, page, tab, and every label / option
function UI.searchText(ctrl, page)
	local parts = { ctrl.name, ctrl.desc or "", page.name, ctrl.tab or "" }
	for _, r in ipairs(ctrl.rows or {}) do
		table.insert(parts, r.label or "")
		table.insert(parts, r.desc or "")
		for _, it in ipairs(r.items or {}) do
			table.insert(parts, it.label or "")
		end
		for _, opt in ipairs(r.options or {}) do
			table.insert(parts, opt)
		end
	end
	return string.lower(table.concat(parts, " "))
end

function UI.renderSearch(q, animate)
	local scroll = list.CanvasPosition
	UI.clearList()
	renderedQuery = q
	UI.buildTabs(nil)
	UI.setListTop(LIST_TOP, 0.25)
	local order, count = 0, 0
	for _, page in ipairs(PAGES) do
		if page.custom ~= "configs" then
			local found = {}
			for _, ctrl in ipairs(page.controls) do
				if string.find(UI.searchText(ctrl, page), q, 1, true) then
					table.insert(found, ctrl)
				end
			end
			if #found > 0 then
				order += 1
				caption(order, string.upper(page.name))
				for _, ctrl in ipairs(found) do
					order += 1
					count += 1
					UI.buildControl(ctrl, order)
				end
			end
		end
	end
	UI.setHeader(
		"practical" .. CRUMB_SEP .. "Search",
		"Search",
		count .. (count == 1 and " result" or " results") .. " for “" .. q .. "”",
		animate
	)
	emptyLabel.Visible = count == 0
	emptyLabel.Text = "No settings match that search"
	if animate then
		UI.animateEntrance()
	else
		list.CanvasPosition = scroll
	end
end

-- redraw in place (keeps scroll), e.g. after density changes
local rerenderToken = 0
rerender = function(delay)
	rerenderToken += 1
	local mine = rerenderToken
	UI.after(delay or 0, function()
		if mine ~= rerenderToken or not currentPage then
			return
		end
		if renderedQuery ~= "" then
			UI.renderSearch(renderedQuery, false)
		else
			renderPage(currentPage, false)
		end
	end)
end

local searchToken = 0
searchBox:GetPropertyChangedSignal("Text"):Connect(function()
	local q = string.lower(UI.trim(searchBox.Text))
	local has = searchBox.Text ~= ""
	clearSearch.Visible = has
	kbd.Visible = not has
	searchToken += 1
	local mine = searchToken
	UI.after(0.14, function()
		if mine ~= searchToken or q == renderedQuery then
			return
		end
		if q == "" then
			renderPage(currentPage)
		else
			UI.renderSearch(q, true)
		end
	end)
end)

openPage = function(name, force)
	local page = PAGE_BY_NAME[name]
	if not page or (name == currentPage and renderedQuery == "" and not force) then
		return
	end
	currentPage = name
	if searchBox.Text ~= "" then
		renderedQuery = "" -- the cleared box won't trigger a second render
		searchBox.Text = ""
	end
	navHighlight.Parent=nav[name].btn.Parent
	UI.tween(navHighlight, 0.45, { Position = nav[name].pos })
	refreshNav(0.3)
	renderPage(name)
end

