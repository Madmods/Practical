--[[
	practical · UI 2.0
	Roblox Studio LocalScript. Put it in StarterPlayer > StarterPlayerScripts.

	Connected Practical Suite interface. Use PracticalComplete.luau to run the engine and UI together.

	  RightShift ............ show / hide (rebindable in Preferences)
	  Ctrl + K .............. search settings
	  Double-click title .... maximise / restore
	  Drag bottom-right ..... resize
]]

local runtime=(getgenv and getgenv()) or _G
local api=assert(runtime.PracticalSuiteAPI,"Run PracticalComplete.luau to load the feature engine")
local UI = {} -- Private helper namespace; avoids Luau's 200-local limit.
local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local TextService = game:GetService("TextService")
local TweenService = game:GetService("TweenService")
local UserInputService = game:GetService("UserInputService")

local player = assert(Players.LocalPlayer, "Run this UI as a LocalScript on the client")
local playerGui = player:WaitForChild("PlayerGui")

-------------------------------------------------------------------- lifecycle
local alive = true
local connections = {}

-- long-lived service connections; all dropped when the ScreenGui is destroyed
function UI.connect(signal, fn)
	local c = signal:Connect(function(...)
		if alive then
			fn(...)
		end
	end)
	table.insert(connections, c)
	return c
end

function UI.after(t, fn)
	return task.delay(t, function()
		if alive then
			fn()
		end
	end)
end

-------------------------------------------------------------------- config
local VERSION = "2.0"
local DEFAULT_SCALE = 1.0
local DEFAULT_KEY = Enum.KeyCode.RightShift
local FONT_FAMILY = "rbxasset://fonts/families/BuilderSans.json"
local FONT_ENUM = Enum.Font.BuilderSans -- same family, only used to measure text

local C = {
	bg = Color3.fromRGB(10, 12, 18),
	rail = Color3.fromRGB(13, 16, 23),
	surface = Color3.fromRGB(17, 20, 29),
	raised = Color3.fromRGB(23, 27, 38),
	field = Color3.fromRGB(27, 32, 45),
	hover = Color3.fromRGB(32, 38, 53),
	line = Color3.fromRGB(31, 37, 51),
	stroke = Color3.fromRGB(40, 47, 64),
	strokeHi = Color3.fromRGB(68, 80, 106),
	text = Color3.fromRGB(240, 242, 248),
	text2 = Color3.fromRGB(196, 202, 216),
	sub = Color3.fromRGB(132, 141, 162),
	muted = Color3.fromRGB(88, 96, 116),
	white = Color3.new(1, 1, 1),
	black = Color3.new(0, 0, 0),
	green = Color3.fromRGB(52, 211, 120),
	danger = Color3.fromRGB(255, 104, 112),
	dangerFill = Color3.fromRGB(190, 52, 64),
}

-- Accent pairs: `a` is the main accent, `b` the gradient partner.
local THEMES = {
	{ name = "Cobalt", a = Color3.fromRGB(72, 134, 255), b = Color3.fromRGB(134, 98, 255) },
	{ name = "Violet", a = Color3.fromRGB(152, 100, 255), b = Color3.fromRGB(236, 92, 210) },
	{ name = "Aqua", a = Color3.fromRGB(34, 196, 222), b = Color3.fromRGB(66, 122, 255) },
	{ name = "Emerald", a = Color3.fromRGB(38, 204, 140), b = Color3.fromRGB(46, 170, 230) },
	{ name = "Sunset", a = Color3.fromRGB(255, 128, 72), b = Color3.fromRGB(255, 72, 132) },
	{ name = "Gold", a = Color3.fromRGB(244, 184, 62), b = Color3.fromRGB(255, 118, 72) },
}
local THEME_BY_NAME, THEME_NAMES = {}, {}
for _, th in ipairs(THEMES) do
	THEME_BY_NAME[th.name] = th
	table.insert(THEME_NAMES, th.name)
end

-- live accent palette, re-derived whenever the theme changes
local A = {}
function UI.deriveTheme(th)
	A.name = th.name
	A.a = th.a
	A.b = th.b
	A.hi = th.a:Lerp(C.white, 0.25)
	A.soft = th.a:Lerp(C.bg, 0.45)
	A.dim = th.a:Lerp(C.bg, 0.78)
end
UI.deriveTheme(THEMES[1])

-------------------------------------------------------------------- motion
local QUINT = Enum.EasingStyle.Quint
local BACK = Enum.EasingStyle.Back
local SINE = Enum.EasingStyle.Sine
local LINEAR = Enum.EasingStyle.Linear
local OUT = Enum.EasingDirection.Out
local IN = Enum.EasingDirection.In
local INOUT = Enum.EasingDirection.InOut

-- Driven by the Animation page: duration multiplier, default easing, reduce-motion.
local MOTION = { factor = 1, style = QUINT, reduce = false }

function UI.motionTime(t)
	if MOTION.reduce then
		return math.min(t, 0.06)
	end
	return t * MOTION.factor
end

-- Every transition goes through here. t <= 0 applies instantly and returns nil;
-- any t > 0 always returns a playing Tween (safe to chain .Completed).
function UI.tween(inst, t, goal, style, dir, delay)
	if t <= 0 then
		for k, v in pairs(goal) do
			inst[k] = v
		end
		return nil
	end
	local info = TweenInfo.new(
		math.max(UI.motionTime(t), 0.01),
		style or MOTION.style,
		dir or OUT,
		0,
		false,
		delay and UI.motionTime(delay) or 0
	)
	local tw = TweenService:Create(inst, info, goal)
	tw:Play()
	return tw
end

-------------------------------------------------------------------- theme registry
-- Static accent-coloured properties register once; a theme switch re-tints them all.
-- Anything whose colour depends on state (switches, checkboxes, nav) reads A.* when it paints.
-- (Strong table pruned on Destroying: Roblox can drop an Instance from a weak-keyed table
-- while it is still alive, which would silently break re-tinting.)
local themed = {}

function UI.themeValue(key)
	if key == "grad" then
		return ColorSequence.new(A.a, A.b)
	elseif key == "border" then
		return ColorSequence.new({
			ColorSequenceKeypoint.new(0, C.stroke),
			ColorSequenceKeypoint.new(0.34, C.stroke),
			ColorSequenceKeypoint.new(0.46, A.a),
			ColorSequenceKeypoint.new(0.54, A.b),
			ColorSequenceKeypoint.new(0.66, C.stroke),
			ColorSequenceKeypoint.new(1, C.stroke),
		})
	end
	return A[key]
end

function UI.bindTheme(inst, prop, key)
	local map = themed[inst]
	if not map then
		map = {}
		themed[inst] = map
		inst.Destroying:Connect(function()
			themed[inst] = nil
		end)
	end
	map[prop] = key
	inst[prop] = UI.themeValue(key)
	return inst
end

function UI.applyThemeRegistry(t)
	for inst, map in pairs(themed) do
		if inst.Parent then
			for prop, key in pairs(map) do
				local v = UI.themeValue(key)
				if typeof(v) == "Color3" and t > 0 then
					UI.tween(inst, t, { [prop] = v })
				else
					inst[prop] = v
				end
			end
		end
	end
end

-------------------------------------------------------------------- builders
local MEDIUM = Enum.FontWeight.Medium
local SEMIBOLD = Enum.FontWeight.SemiBold
local BOLD = Enum.FontWeight.Bold

function UI.make(class, props, children)
	local inst = Instance.new(class)
	local parent
	for k, v in pairs(props or {}) do
		if k == "Parent" then
			parent = v
		else
			inst[k] = v
		end
	end
	for _, child in ipairs(children or {}) do
		child.Parent = inst
	end
	inst.Parent = parent
	return inst
end

function UI.font(weight)
	return Font.new(FONT_FAMILY, weight or Enum.FontWeight.Regular)
end

function UI.textWidth(str, size)
	return TextService:GetTextSize(str, size, FONT_ENUM, Vector2.new(4000, 200)).X
end

function UI.trim(s)
	return (string.gsub(s, "^%s*(.-)%s*$", "%1"))
end

function UI.corner(r)
	return UI.make("UICorner", { CornerRadius = UDim.new(0, r) })
end

function UI.round()
	return UI.make("UICorner", { CornerRadius = UDim.new(1, 0) })
end

function UI.stroke(color, thickness, transparency)
	return UI.make("UIStroke", {
		Color = color,
		Thickness = thickness or 1,
		Transparency = transparency or 0,
		ApplyStrokeMode = Enum.ApplyStrokeMode.Border,
	})
end

function UI.padding(l, r, t, b)
	return UI.make("UIPadding", {
		PaddingLeft = UDim.new(0, l or 0),
		PaddingRight = UDim.new(0, r or 0),
		PaddingTop = UDim.new(0, t or 0),
		PaddingBottom = UDim.new(0, b or 0),
	})
end

function UI.accentGradient(rotation, transparency)
	local g = UI.make("UIGradient", { Rotation = rotation or 0 })
	if transparency then
		g.Transparency = transparency
	end
	return UI.bindTheme(g, "Color", "grad")
end

-- transparent at both ends, solid in the middle (hairline highlights)
local EDGE_FADE = NumberSequence.new({
	NumberSequenceKeypoint.new(0, 1),
	NumberSequenceKeypoint.new(0.5, 0),
	NumberSequenceKeypoint.new(1, 1),
})

-- solid strip positioned with UDim2s (lines that stretch or hug an edge)
function UI.strip(parent, pos, size, color)
	return UI.make("Frame", {
		BackgroundColor3 = color or C.line,
		BorderSizePixel = 0,
		Position = pos,
		Size = size,
		Parent = parent,
	})
end

-- Label at (x, y) = left edge (right with ax = 1, centre with ax = 0.5) / vertical centre.
-- `x` may be a UDim2, in which case `y` is ignored.
local function text(parent, str, x, y, size, opts)
	opts = opts or {}
	local ax = opts.ax or 0
	local label = UI.make(opts.button and "TextButton" or "TextLabel", {
		BackgroundTransparency = 1,
		Text = str,
		RichText = opts.rich == true,
		FontFace = UI.font(opts.weight),
		TextSize = size,
		TextColor3 = opts.color or C.text,
		TextXAlignment = ax == 1 and Enum.TextXAlignment.Right
			or ax == 0.5 and Enum.TextXAlignment.Center
			or Enum.TextXAlignment.Left,
		AutomaticSize = Enum.AutomaticSize.X,
		AnchorPoint = Vector2.new(ax, 0.5),
		Position = typeof(x) == "UDim2" and x or UDim2.fromOffset(x, y),
		Size = UDim2.fromOffset(0, size + 8),
		ZIndex = opts.z or 1,
		Parent = parent,
	})
	if opts.button then
		label.AutoButtonColor = false
	end
	return label
end

-- fixes a label's width (offset or UDim) and truncates with an ellipsis
function UI.clip(label, w)
	label.AutomaticSize = Enum.AutomaticSize.None
	local h = label.Size.Y.Offset
	if typeof(w) == "UDim" then
		label.Size = UDim2.new(w.Scale, w.Offset, 0, h)
	else
		label.Size = UDim2.fromOffset(w, h)
	end
	label.TextTruncate = Enum.TextTruncate.AtEnd
	return label
end

-- Letter-spaced caps ("MENU"). Roblox has no tracking, so one label per glyph.
function UI.tracked(parent, str, x, y, size, color, spacing)
	local holder = UI.make("Frame", {
		BackgroundTransparency = 1,
		AutomaticSize = Enum.AutomaticSize.X,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = typeof(x) == "UDim2" and x or UDim2.fromOffset(x, y),
		Size = UDim2.fromOffset(0, size + 8),
		Parent = parent,
	})
	UI.make("UIListLayout", {
		FillDirection = Enum.FillDirection.Horizontal,
		Padding = UDim.new(0, spacing or 1.5),
		SortOrder = Enum.SortOrder.LayoutOrder,
		VerticalAlignment = Enum.VerticalAlignment.Center,
		Parent = holder,
	})
	for i = 1, #str do
		UI.make("TextLabel", {
			BackgroundTransparency = 1,
			Text = str:sub(i, i),
			FontFace = UI.font(SEMIBOLD),
			TextSize = size,
			TextColor3 = color or C.sub,
			AutomaticSize = Enum.AutomaticSize.X,
			Size = UDim2.fromOffset(0, size + 8),
			LayoutOrder = i,
			Parent = holder,
		})
	end
	return holder
end

