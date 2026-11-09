-------------------------------------------------------------------- profiles
function UI.snapshot()
	local snap = {}
	UI.forEachControl(function(ctrl)
		local values = {}
		UI.forEachValue(ctrl, function(s)
			values[s.id] = s.value
		end)
		snap[ctrl.id] = { enabled = ctrl.enabled, values = values }
	end, true)
	return snap
end

repaintAll = function(t)
	for _, appliers in pairs(rowAppliers) do
		for _, apply in ipairs(appliers) do
			apply(t or 0.4)
		end
	end
end

function UI.restore(snap)
	UI.forEachControl(function(ctrl)
		local entry = snap[ctrl.id]
		if entry then
			ctrl.enabled = entry.enabled
			UI.forEachValue(ctrl, function(s)
				if entry.values[s.id] ~= nil then
					s.value = entry.values[s.id]
     if s.onSet then s.onSet(s.value)end
				end
			end)
		end
	end, true)
	applySettings()
	repaintAll()
	refreshNav()
	if refreshMini then
		refreshMini()
	end
end

-- three starting profiles, each a variation on the defaults
function UI.preset(name, edits)
	local snap = UI.snapshot()
	for ctrlId, e in pairs(edits) do
		for k, v in pairs(e) do
			if k == "enabled" then
				snap[ctrlId].enabled = v
			else
				snap[ctrlId].values[k] = v
			end
		end
	end
	return { name = name, snap = snap, savedAt = os.time() }
end
local PROFILES = {
	UI.preset("Default", {}),
	UI.preset("Custom", {}),
	UI.preset("Practice", {}),
}
local currentProfile = PROFILES[1]

function UI.profileNames()
	local names = {}
	for i, p in ipairs(PROFILES) do
		names[i] = p.name
	end
	return names
end

function UI.findProfile(name)
	for _, p in ipairs(PROFILES) do
		if string.lower(p.name) == string.lower(name) then
			return p
		end
	end
	return nil
end

function UI.uniqueName(name, except)
	local base, n, candidate = name, 2, name
	while true do
		local found = UI.findProfile(candidate)
		if not found or found == except then
			return candidate
		end
		candidate = base .. " (" .. n .. ")"
		n += 1
	end
end

function UI.ago(ts)
	local d = os.time() - (ts or os.time())
	if d < 10 then
		return "just now"
	elseif d < 60 then
		return d .. "s ago"
	elseif d < 3600 then
		return math.floor(d / 60) .. "m ago"
	end
	return math.floor(d / 3600) .. "h ago"
end

function UI.profileMeta(p)
	local n = 0
	for id, e in pairs(p.snap) do
		local ctrl = CTRL[id]
		if ctrl and ctrl.kind ~= "section" and e.enabled then
			n += 1
		end
	end
	local theme = p.snap.appearance and p.snap.appearance.values.theme or THEMES[1].name
	return string.format("%s accent  ·  %d feature%s on  ·  saved %s", theme, n, n == 1 and "" or "s", UI.ago(p.savedAt))
end

local configNameBox -- the "new profile" text box while the Configs page is open
local profileDD -- header dropdown, created below

function UI.loadProfile(p)
	currentProfile = p
	profileDD.set(p.name, 0.25)
	UI.restore(p.snap)
	setStatus(p.name .. " loaded")
	toast("Profile loaded", '"' .. p.name .. '" is now active', "folder")
	controlChanged:Fire("profile", "selected", p.name)
end

function UI.createConfig(raw)
	local name = UI.trim(raw or "")
	if name == "" then
		if configNameBox then
			UI.shake(configNameBox.Parent)
		end
		return
	end
	local p = { name = UI.uniqueName(name), snap = UI.snapshot(), savedAt = os.time() }
	table.insert(PROFILES, p)
	currentProfile = p
	profileDD.set(p.name, 0.25)
	setStatus('Profile "' .. p.name .. '" created')
	toast("Profile created", '"' .. p.name .. '" saved from current settings', "plus")
	controlChanged:Fire("profile", "created", p.name)
	renderPage("Configs")
end

local function caption(order, str)
	local _, body = UI.newSlot(24, order)
	UI.tracked(body, str, 8, 14, 12, C.muted, 1.6)
end

function UI.renderConfigs()
	local order = 0
	local function nextOrder()
		order += 1
		return order
	end

	-- new profile
	if layoutState.captions then
		caption(nextOrder(), "NEW PROFILE")
	end
	local _, newBody = UI.newSlot(78, nextOrder(), "newProfile")
	local newPanel = UI.make("Frame", {
		BackgroundColor3 = C.surface,
		Position = UDim2.fromOffset(2, 2),
		Size = UDim2.new(1, -4, 0, 78),
		Parent = newBody,
	}, { UI.corner(14), UI.stroke(C.stroke, 1) })
	local create = UI.primaryButton(newPanel, "Create from current", "plus", 44)
	create.AnchorPoint = Vector2.new(1, 0.5)
	create.Position = UDim2.new(1, -18, 0.5, 0)
	local fieldStroke = UI.stroke(C.stroke, 1)
	local field = UI.make("Frame", {
		BackgroundColor3 = C.field,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, 18, 0.5, 0),
		Size = UDim2.new(1, -18 - 12 - 18 - create.Size.X.Offset, 0, 44),
		Parent = newPanel,
	}, { UI.corner(10), fieldStroke })
	local fieldIcon = UI.icon(field, "pencil", 22, 22, 16, C.sub, 1.7)
	local box = UI.make("TextBox", {
		BackgroundTransparency = 1,
		Position = UDim2.fromOffset(42, 0),
		Size = UDim2.new(1, -56, 1, 0),
		Text = "",
		PlaceholderText = "Name a new profile",
		PlaceholderColor3 = C.muted,
		TextColor3 = C.text,
		FontFace = UI.font(),
		TextSize = 16,
		TextXAlignment = Enum.TextXAlignment.Left,
		ClearTextOnFocus = false,
		Parent = field,
	})
	configNameBox = box
	box.Focused:Connect(function()
		UI.tween(fieldStroke, 0.2, { Color = A.a })
		UI.tintIcon(fieldIcon, A.hi, 0.2)
	end)
	box.FocusLost:Connect(function(enter)
		UI.tween(fieldStroke, 0.3, { Color = C.stroke })
		UI.tintIcon(fieldIcon, C.sub, 0.3)
		if enter then
			UI.createConfig(box.Text)
		end
	end)
	create.MouseButton1Click:Connect(function()
		UI.createConfig(box.Text)
	end)

	if layoutState.captions then
		caption(nextOrder(), "SAVED PROFILES  ·  " .. #PROFILES)
	end

	for _, p in ipairs(PROFILES) do
		local isCurrent = p == currentProfile
		local slot, body = UI.newSlot(80, nextOrder(), "profile")
		local row = UI.make("Frame", {
			BackgroundColor3 = C.surface,
			Position = UDim2.fromOffset(2, 2),
			Size = UDim2.new(1, -4, 0, 80),
			Parent = body,
		}, { UI.corner(14), UI.stroke(isCurrent and A.soft or C.stroke, 1) })
		if isCurrent then
			UI.make("Frame", {
				BackgroundColor3 = C.white,
				BackgroundTransparency = 0.15,
				BorderSizePixel = 0,
				Position = UDim2.fromOffset(28, 0),
				Size = UDim2.new(1, -56, 0, 1),
				Parent = row,
			}, { UI.accentGradient(0, EDGE_FADE) })
		end

		-- initial tile
		local tile = UI.make("Frame", {
			BackgroundColor3 = isCurrent and C.white or C.field,
			AnchorPoint = Vector2.new(0, 0.5),
			Position = UDim2.new(0, 18, 0.5, 0),
			Size = UDim2.fromOffset(44, 44),
			Parent = row,
		}, { UI.corner(12) })
		if isCurrent then
			UI.accentGradient(45).Parent = tile
		end
		UI.make("TextLabel", {
			BackgroundTransparency = 1,
			Size = UDim2.fromScale(1, 1),
			Text = string.upper(string.sub(p.name, 1, 1)),
			FontFace = UI.font(BOLD),
			TextSize = 19,
			TextColor3 = isCurrent and C.white or C.text2,
			Parent = tile,
		})

		-- name is an editable box styled like a label; the pencil focuses it
		local nameStroke = UI.stroke(C.stroke, 1, 1)
		local nameFrame = UI.make("Frame", {
			BackgroundColor3 = C.field,
			BackgroundTransparency = 1,
			AnchorPoint = Vector2.new(0, 0.5),
			Position = UDim2.fromOffset(68, 28),
			Size = UDim2.fromOffset(280, 32),
			Parent = row,
		}, { UI.corner(8), nameStroke })
		local nameBox = UI.make("TextBox", {
			BackgroundTransparency = 1,
			Position = UDim2.fromOffset(8, 0),
			Size = UDim2.new(1, -16, 1, 0),
			Text = p.name,
			TextColor3 = C.text,
			FontFace = UI.font(SEMIBOLD),
			TextSize = 18,
			TextXAlignment = Enum.TextXAlignment.Left,
			TextTruncate = Enum.TextTruncate.AtEnd,
			ClearTextOnFocus = false,
			Parent = nameFrame,
		})
		UI.clip(text(row, UI.profileMeta(p), 76, 54, 14, { color = C.sub }), UDim.new(1, -400))
		if isCurrent then
			local bx = 76 + math.min(UI.textWidth(p.name, 18), 264) + 12
			local badge = UI.make("Frame", {
				AnchorPoint = Vector2.new(0, 0.5),
				Position = UDim2.fromOffset(bx, 28),
				Size = UDim2.fromOffset(62, 22),
				Parent = row,
			}, { UI.round() })
			UI.bindTheme(badge, "BackgroundColor3", "dim")
			UI.bindTheme(UI.stroke(A.soft, 1), "Color", "soft").Parent = badge
			UI.bindTheme(UI.make("TextLabel", {
				BackgroundTransparency = 1,
				Size = UDim2.fromScale(1, 1),
				Text = "ACTIVE",
				FontFace = UI.font(BOLD),
				TextSize = 11,
				Parent = badge,
			}), "TextColor3", "hi")
		end
		nameBox.Focused:Connect(function()
			UI.tween(nameStroke, 0.2, { Transparency = 0, Color = A.a })
			UI.tween(nameFrame, 0.2, { BackgroundTransparency = 0 })
		end)
		nameBox.FocusLost:Connect(function()
			UI.tween(nameStroke, 0.25, { Transparency = 1 })
			UI.tween(nameFrame, 0.25, { BackgroundTransparency = 1 })
			local new = UI.trim(nameBox.Text)
			if new == "" or new == p.name then
				nameBox.Text = p.name
				return
			end
			local old = p.name
			p.name = UI.uniqueName(new, p)
			nameBox.Text = p.name
			if p == currentProfile then
				profileDD.set(p.name, 0.25)
			end
			setStatus('Renamed to "' .. p.name .. '"')
			controlChanged:Fire("profile", "renamed", old .. " -> " .. p.name)
			renderPage("Configs", false)
		end)

		-- actions
		local actions = UI.make("Frame", {
			BackgroundTransparency = 1,
			AutomaticSize = Enum.AutomaticSize.X,
			AnchorPoint = Vector2.new(1, 0.5),
			Position = UDim2.new(1, -18, 0.5, 0),
			Size = UDim2.fromOffset(0, 40),
			Parent = row,
		})
		UI.make("UIListLayout", {
			FillDirection = Enum.FillDirection.Horizontal,
			Padding = UDim.new(0, 8),
			SortOrder = Enum.SortOrder.LayoutOrder,
			VerticalAlignment = Enum.VerticalAlignment.Center,
			Parent = actions,
		})
		local loadB, loadLabel = UI.ghostButton(actions, isCurrent and "Loaded" or "Load", isCurrent and "check" or "play", 40)
		loadB.LayoutOrder = 1
		if isCurrent then
			loadLabel.TextColor3 = C.sub
		end
		local function small(order2, iconName, tip, danger)
			local b = UI.iconButton(actions, UDim2.new(), 40, iconName, {
				solid = true,
				tip = tip,
				hoverColor = danger and C.dangerFill or nil,
				color = danger and C.danger or nil,
				iconSize = 17,
			})
			b.AnchorPoint = Vector2.zero
			b.LayoutOrder = order2
			return b
		end
		local saveB = small(2, "save", "Save current settings here")
		local renameB = small(3, "pencil", "Rename")
		local deleteB = small(4, "trash", #PROFILES > 1 and "Delete" or "Can't delete the last profile", true)

		loadB.MouseButton1Click:Connect(function()
			if p ~= currentProfile then
				UI.loadProfile(p)
				renderPage("Configs", false)
			end
		end)
		saveB.MouseButton1Click:Connect(function()
			p.snap = UI.snapshot()
			p.savedAt = os.time()
			setStatus('Saved current settings to "' .. p.name .. '"')
			toast("Profile saved", '"' .. p.name .. '" updated', "save")
			controlChanged:Fire("profile", "saved", p.name)
			renderPage("Configs", false)
		end)
		renameB.MouseButton1Click:Connect(function()
			nameBox:CaptureFocus()
			nameBox.SelectionStart = 1
			nameBox.CursorPosition = #nameBox.Text + 1
		end)
		deleteB.MouseButton1Click:Connect(function()
			if #PROFILES <= 1 then
				UI.shake(row)
				return
			end
			local idx = table.find(PROFILES, p)
			if idx then
				table.remove(PROFILES, idx)
			end
			controlChanged:Fire("profile", "deleted", p.name)
			setStatus('Deleted "' .. p.name .. '"')
			toast("Profile deleted", '"' .. p.name .. '" was removed', "trash")
			-- collapse the row away, then redraw
			UI.groupFade(body, 1, 0.2)
			UI.tween(slot, 0.3, { Size = UDim2.new(1, 0, 0, 0) }).Completed:Connect(function()
				if p == currentProfile then
					UI.loadProfile(PROFILES[1])
				end
				if currentPage == "Configs" and renderedQuery == "" then
					renderPage("Configs", false)
				end
			end)
		end)
	end
end

