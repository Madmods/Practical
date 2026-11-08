
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
	-- Keep the on-screen HUD cards in the menu's accent.
	if api.SetUIAccent then
		pcall(api.SetUIAccent, th.a, th.b)
	end
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

-------------------------------------------------------------------- icons (24 x 24 stroke icons)
function UI.push(p, x, y)
	local last = p[#p]
	if last and math.abs(last[1] - x) < 1e-3 and math.abs(last[2] - y) < 1e-3 then
		return
	end
	p[#p + 1] = { x, y }
end

-- degrees, screen space (0 = right, 90 = down)
function UI.arc(p, cx, cy, r, a0, a1, steps)
	steps = steps or 6
	for i = 0, steps do
		local a = math.rad(a0 + (a1 - a0) * i / steps)
		UI.push(p, cx + r * math.cos(a), cy + r * math.sin(a))
	end
end

function UI.cubic(p, x0, y0, x1, y1, x2, y2, x3, y3, steps)
	steps = steps or 8
	UI.push(p, x0, y0)
	for i = 1, steps do
		local t = i / steps
		local u = 1 - t
		local a, b, c, d = u * u * u, 3 * u * u * t, 3 * u * t * t, t * t * t
		UI.push(p, a * x0 + b * x1 + c * x2 + d * x3, a * y0 + b * y1 + c * y2 + d * y3)
	end
end

function UI.S(pts, closed)
	return { pts = pts, closed = closed or false }
end

function UI.circle(cx, cy, r, steps)
	local c = {}
	steps = steps or 20
	UI.arc(c, cx, cy, r, 0, 360 - 360 / steps, steps - 1)
	return UI.S(c, true)
end

function UI.rect(x0, y0, x1, y1)
	return UI.S({ { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } }, true)
end

local ICONS = {}
ICONS.minus = function()
	return { UI.S({ { 5, 12 }, { 19, 12 } }) }
end
ICONS.plus = function()
	return { UI.S({ { 5, 12 }, { 19, 12 } }), UI.S({ { 12, 5 }, { 12, 19 } }) }
end
ICONS.close = function()
	return { UI.S({ { 6, 6 }, { 18, 18 } }), UI.S({ { 18, 6 }, { 6, 18 } }) }
end
ICONS.restore = function()
	return { UI.rect(2.5, 8.5, 16.5, 20.5), UI.S({ { 7, 8.5 }, { 7, 4 }, { 21.5, 4 }, { 21.5, 16 }, { 16.5, 16 } }) }
end
ICONS.grip = function()
	return { UI.S({ { 21, 10 }, { 10, 21 } }), UI.S({ { 21, 16 }, { 16, 21 } }) }
end
ICONS.chevronRight = function()
	return { UI.S({ { 9, 6 }, { 15, 12 }, { 9, 18 } }) }
end
ICONS.check = function()
	return { UI.S({ { 20, 6 }, { 9, 17 }, { 4, 12 } }) }
end
ICONS.search = function()
	return { UI.circle(11, 11, 7, 24), UI.S({ { 21, 21 }, { 16.2, 16.2 } }) }
end
ICONS.logo = function()
	return { UI.rect(3, 9.5, 14.5, 21), UI.S({ { 9.5, 9.5 }, { 9.5, 3 }, { 21, 3 }, { 21, 14.5 }, { 14.5, 14.5 } }) }
end
ICONS.home = function()
	return {
		UI.S({ { 3.5, 11 }, { 12, 3.5 }, { 20.5, 11 } }),
		UI.S({ { 5.5, 9.5 }, { 5.5, 20 }, { 18.5, 20 }, { 18.5, 9.5 } }),
		UI.S({ { 10, 20 }, { 10, 14.5 }, { 14, 14.5 }, { 14, 20 } }),
	}
end
ICONS.grid = function()
	return { UI.rect(3.5, 3.5, 10.5, 10.5), UI.rect(13.5, 3.5, 20.5, 10.5), UI.rect(3.5, 13.5, 10.5, 20.5), UI.rect(13.5, 13.5, 20.5, 20.5) }
end
ICONS.activity = function()
	return { UI.S({ { 2.5, 12 }, { 6.5, 12 }, { 9, 5 }, { 14, 19 }, { 16.5, 12 }, { 21.5, 12 } }) }
end
ICONS.folder = function()
	return { UI.S({ { 3, 5 }, { 9.5, 5 }, { 11.5, 8 }, { 21, 8 }, { 21, 19 }, { 3, 19 } }, true) }
end
ICONS.bell = function()
	local p = {}
	UI.push(p, 4.5, 17)
	UI.push(p, 6, 15)
	UI.push(p, 6, 10)
	UI.arc(p, 12, 10, 6, 180, 360, 10)
	UI.push(p, 18, 15)
	UI.push(p, 19.5, 17)
	return { UI.S(p, true), UI.S({ { 10, 20.5 }, { 14, 20.5 } }) }
end
ICONS.sparkle = function()
	return {
		UI.S({ { 12, 3 }, { 13.8, 10.2 }, { 21, 12 }, { 13.8, 13.8 }, { 12, 21 }, { 10.2, 13.8 }, { 3, 12 }, { 10.2, 10.2 } }, true),
	}
end
ICONS.text = function()
	return { UI.S({ { 5, 7 }, { 5, 4 }, { 19, 4 }, { 19, 7 } }), UI.S({ { 12, 4 }, { 12, 20 } }), UI.S({ { 9, 20 }, { 15, 20 } }) }
end
ICONS.play = function()
	return { UI.S({ { 7, 4.5 }, { 19, 12 }, { 7, 19.5 } }, true) }
end
ICONS.info = function()
	return { UI.circle(12, 12, 9.5, 28), UI.S({ { 12, 11 }, { 12, 16.5 } }), UI.S({ { 12, 7.4 }, { 12, 7.6 } }) }
end
ICONS.pencil = function()
	return { UI.S({ { 4, 20 }, { 4.8, 16 }, { 16, 4.8 }, { 19.2, 8 }, { 8, 19.2 } }, true), UI.S({ { 13.5, 7.3 }, { 16.7, 10.5 } }) }
end
ICONS.save = function()
	return {
		UI.S({ { 4, 4 }, { 16.5, 4 }, { 20, 7.5 }, { 20, 20 }, { 4, 20 } }, true),
		UI.S({ { 8, 4 }, { 8, 8.5 }, { 15, 8.5 }, { 15, 4 } }),
		UI.rect(7.5, 13, 16.5, 20),
	}
end
ICONS.signal = function()
	return { UI.S({ { 5, 19 }, { 5, 16 } }), UI.S({ { 10, 19 }, { 10, 12 } }), UI.S({ { 15, 19 }, { 15, 8 } }), UI.S({ { 20, 19 }, { 20, 4 } }) }
end
ICONS.clock = function()
	return { UI.circle(12, 12, 9.5, 28), UI.S({ { 12, 7 }, { 12, 12 }, { 15.5, 14 } }) }
end
ICONS.bolt = function()
	return { UI.S({ { 13, 2.5 }, { 5, 13.5 }, { 11.5, 13.5 }, { 10.5, 21.5 }, { 19, 10 }, { 12.5, 10 } }, true) }
end
ICONS.run = function()
	return {
		UI.circle(15.5, 4, 2, 14),
		UI.S({ { 14, 8 }, { 11, 14 } }),
		UI.S({ { 7.5, 10 }, { 10, 7.5 }, { 14, 8 }, { 16, 11 }, { 19, 11.5 } }),
		UI.S({ { 11, 14 }, { 14.5, 16.5 }, { 13.5, 21.5 } }),
		UI.S({ { 11, 14 }, { 9, 18 }, { 4.5, 18.5 } }),
	}
end
ICONS.wrench = function()
	local p = {}
	UI.push(p, 14.7, 6.3)
	UI.push(p, 14.4, 7.0)
	UI.push(p, 14.7, 7.7)
	UI.push(p, 16.3, 9.3)
	UI.push(p, 17.0, 9.6)
	UI.push(p, 17.7, 9.3)
	UI.push(p, 21.47, 5.53)
	UI.arc(p, 16.01, 8.01, 6, -24.4, 114.4, 10)
	UI.push(p, 6.62, 20.38)
	UI.arc(p, 5.12, 18.88, 2.12, 45, 225, 8)
	UI.push(p, 10.53, 10.47)
	UI.arc(p, 16.01, 8.01, 6, 155.8, 294.2, 10)
	return { UI.S(p, true) }
end
ICONS.gear = function()
	local p = {}
	local function pol(r, deg)
		local t = math.rad(deg)
		UI.push(p, 12 + r * math.cos(t), 12 + r * math.sin(t))
	end
	for k = 0, 7 do
		local b = k * 45
		pol(7.6, b - 16)
		pol(10, b - 9)
		pol(10, b + 9)
		pol(7.6, b + 16)
		pol(7.6, b + 22.5)
	end
	return { UI.S(p, true), UI.circle(12, 12, 3, 14) }
end
ICONS.layers = function()
	return {
		UI.S({ { 12, 2.5 }, { 21.5, 7 }, { 12, 11.5 }, { 2.5, 7 } }, true),
		UI.S({ { 2.5, 12 }, { 12, 16.5 }, { 21.5, 12 } }),
		UI.S({ { 2.5, 17 }, { 12, 21.5 }, { 21.5, 17 } }),
	}
end
ICONS.bounds = function()
	return {
		UI.S({ { 4, 9 }, { 4, 4 }, { 9, 4 } }),
		UI.S({ { 15, 4 }, { 20, 4 }, { 20, 9 } }),
		UI.S({ { 20, 15 }, { 20, 20 }, { 15, 20 } }),
		UI.S({ { 9, 20 }, { 4, 20 }, { 4, 15 } }),
	}
end
ICONS.trash = function()
	return {
		UI.S({ { 3, 6 }, { 21, 6 } }),
		UI.S({ { 9, 6 }, { 9, 3.5 }, { 15, 3.5 }, { 15, 6 } }),
		UI.S({ { 5.5, 6 }, { 6.5, 21 }, { 17.5, 21 }, { 18.5, 6 } }),
		UI.S({ { 10, 10.5 }, { 10, 17 } }),
		UI.S({ { 14, 10.5 }, { 14, 17 } }),
	}
end
ICONS.refresh = function()
	local a, b = {}, {}
	UI.arc(a, 12, 12, 9, 180, 318, 12)
	UI.push(a, 21, 8)
	UI.arc(b, 12, 12, 9, 0, 138, 12)
	UI.push(b, 3, 16)
	return { UI.S(a), UI.S(b), UI.S({ { 21, 3 }, { 21, 8 }, { 16, 8 } }), UI.S({ { 8, 16 }, { 3, 16 }, { 3, 21 } }) }
end
ICONS.shield = function()
	local p = {}
	UI.cubic(p, 20, 13, 20, 18, 16.5, 20.5, 12.34, 21.95)
	UI.push(p, 11.67, 21.94)
	UI.cubic(p, 11.67, 21.94, 7.5, 20.5, 4, 18, 4, 13)
	UI.arc(p, 5, 6, 1, 180, 270, 3)
	UI.cubic(p, 5, 5, 7, 5, 9.5, 3.8, 11.24, 2.28, 6)
	UI.push(p, 12, 1.98)
	UI.push(p, 12.76, 2.28)
	UI.cubic(p, 12.76, 2.28, 14.51, 3.81, 17, 5, 19, 5, 6)
	UI.arc(p, 19, 6, 1, 270, 360, 3)
	return { UI.S(p, true) }
end

-- Draws (or redraws in place) an icon as capsule strokes, optionally rotated about its centre.
function UI.drawIcon(holder, name, color, thickness, angle)
	local build = ICONS[name]
	if not build then
		return
	end
	for _, child in ipairs(holder:GetChildren()) do
		if child.Name == "IconStroke" then
			child:Destroy()
		end
	end
	thickness = thickness or 2
	local a = math.rad(angle or 0)
	local ca, sa = math.cos(a), math.sin(a)
	local size = holder.Size.X.Offset > 0 and holder.Size.X.Offset or 24
	local k = size / 24
	local function point(p)
		local x, y = p[1] - 12, p[2] - 12
		return x * ca - y * sa + 12, x * sa + y * ca + 12
	end
	for _, shape in ipairs(build()) do
		local count = #shape.pts
		local segments = shape.closed and count or count - 1
		for i = 1, segments do
			local x1, y1 = point(shape.pts[i])
			local x2, y2 = point(shape.pts[i % count + 1])
			local dx, dy = x2 - x1, y2 - y1
			local length = math.sqrt(dx * dx + dy * dy) * k
			if length > 0.01 then
				UI.make("Frame", {
					Name = "IconStroke",
					AnchorPoint = Vector2.new(0.5, 0.5),
					Position = UDim2.fromScale((x1 + x2) / 48, (y1 + y2) / 48),
					-- extended by the stroke width so rounded caps overlap at the joints
					Size = UDim2.fromOffset(length + thickness * 0.9, thickness),
					BackgroundColor3 = color,
					BorderSizePixel = 0,
					Rotation = math.deg(math.atan2(dy, dx)),
					Parent = holder,
				}, { UI.round() })
			end
		end
	end
end

function UI.icon(parent, name, cx, cy, size, color, thickness)
	local holder = UI.make("Frame", {
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = typeof(cx) == "UDim2" and cx or UDim2.fromOffset(cx, cy),
		Size = UDim2.fromOffset(size, size),
		Parent = parent,
	})
	UI.drawIcon(holder, name, color or C.text, thickness)
	return holder
end

-- recolours an icon without redrawing it
function UI.tintIcon(holder, color, t)
	for _, child in ipairs(holder:GetChildren()) do
		if child.Name == "IconStroke" then
			UI.tween(child, t or 0, { BackgroundColor3 = color })
		end
	end
end

-- Smoothly rotates an icon by redrawing its points (works inside clipped frames).
local spinning = {}
function UI.spinIcon(holder, name, color, thickness, toAngle, t)
	local st = spinning[holder]
	if not st then
		st = { angle = 0 }
		spinning[holder] = st
		holder.Destroying:Connect(function()
			spinning[holder] = nil
		end)
	end
	if st.tween then
		st.tween:Cancel()
	end
	if st.value then
		st.value:Destroy()
	end
	if t <= 0 then
		st.angle = toAngle
		UI.drawIcon(holder, name, color, thickness, toAngle)
		return
	end
	local nv = Instance.new("NumberValue")
	nv.Value = st.angle
	nv.Changed:Connect(function(v)
		st.angle = v
		UI.drawIcon(holder, name, color, thickness, v)
	end)
	st.value = nv
	st.tween = TweenService:Create(nv, TweenInfo.new(math.max(UI.motionTime(t), 0.01), QUINT, OUT), { Value = toAngle })
	st.tween:Play()
end

-------------------------------------------------------------------- effects
-- Squish on press, spring back on release.
function UI.pressable(btn, depth)
	local s = UI.make("UIScale", { Parent = btn })
	btn.MouseButton1Down:Connect(function()
		UI.tween(s, 0.1, { Scale = depth or 0.96 }, QUINT)
	end)
	local function release()
		UI.tween(s, 0.4, { Scale = 1 }, BACK)
	end
	btn.MouseButton1Up:Connect(release)
	btn.MouseLeave:Connect(release)
	return s
end

-- Material-style ink ripple from the pointer.
function UI.ripple(btn, color)
	btn.ClipsDescendants = true
	btn.MouseButton1Down:Connect(function()
		local size = btn.AbsoluteSize
		if size.X <= 0 or size.Y <= 0 then
			return
		end
		local rel = (UserInputService:GetMouseLocation() - btn.AbsolutePosition) / size
		local dot = UI.make("Frame", {
			Name = "Ripple",
			BackgroundColor3 = color or C.white,
			BackgroundTransparency = 0.8,
			BorderSizePixel = 0,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Position = UDim2.fromScale(rel.X, rel.Y),
			Size = UDim2.fromScale(0, 0),
			ZIndex = 0,
			Parent = btn,
		}, { UI.round(), UI.make("UIAspectRatioConstraint", { AspectRatio = 1, DominantAxis = Enum.DominantAxis.Width }) })
		UI.tween(dot, 0.6, { Size = UDim2.fromScale(2.4, 2.4), BackgroundTransparency = 1 }, QUINT).Completed:Connect(function()
			dot:Destroy()
		end)
	end)
end

-- A soft highlight sweeps across the button on hover.
function UI.shimmer(btn)
	btn.ClipsDescendants = true
	local sheen = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BorderSizePixel = 0,
		AnchorPoint = Vector2.new(0.5, 0),
		Position = UDim2.fromScale(-0.5, 0),
		Size = UDim2.fromScale(0.6, 1),
		ZIndex = 0,
		Parent = btn,
	}, {
		UI.make("UIGradient", {
			Rotation = 15,
			Transparency = NumberSequence.new({
				NumberSequenceKeypoint.new(0, 1),
				NumberSequenceKeypoint.new(0.5, 0.72),
				NumberSequenceKeypoint.new(1, 1),
			}),
		}),
	})
	btn.MouseEnter:Connect(function()
		sheen.Position = UDim2.fromScale(-0.5, 0)
		UI.tween(sheen, 0.75, { Position = UDim2.fromScale(1.5, 0) }, SINE, INOUT)
	end)
end

-- Slow expanding ring behind a status dot.
function UI.pulse(dot, color)
	local halo = UI.make("Frame", {
		BackgroundColor3 = color,
		BackgroundTransparency = 0.55,
		BorderSizePixel = 0,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromScale(0.5, 0.5),
		Size = UDim2.fromScale(1, 1),
		Parent = dot,
	}, { UI.round() })
	TweenService:Create(halo, TweenInfo.new(1.8, SINE, OUT, -1), {
		Size = UDim2.fromScale(2.8, 2.8),
		BackgroundTransparency = 1,
	}):Play()
	return halo
end

-- Fires `fn` once on press, then repeats while held.
function UI.repeatButton(btn, fn)
	local token = 0
	btn.MouseButton1Down:Connect(function()
		token += 1
		local mine = token
		fn()
		UI.after(0.38, function()
			while token == mine and btn.Parent do
				fn()
				task.wait(0.06)
			end
		end)
	end)
	local function stop()
		token += 1
	end
	btn.MouseButton1Up:Connect(stop)
	btn.MouseLeave:Connect(stop)
end

-- Quick side-to-side shake (invalid input).
function UI.shake(obj)
	local base = obj.Position
	task.spawn(function()
		for _, dx in ipairs({ -9, 8, -6, 4, -2, 0 }) do
			obj.Position = base + UDim2.fromOffset(dx, 0)
			task.wait(0.04)
		end
		obj.Position = base
	end)
end

-- Fades everything inside `host` as one layer. CanvasGroups render text into a texture, which
-- softens it, so the children only live inside one for the duration of a fade-in and are moved
-- back out once fully visible. A fade to 1 leaves them wrapped (hidden) until the next fade-in.
local fades, fadeWatched = {}, {}
function UI.groupFade(host, toT, t, delay, style, dir)
	local st = fades[host]
	if not st then
		if not fadeWatched[host] then
			fadeWatched[host] = true
			host.Destroying:Once(function()
				fades[host] = nil
				fadeWatched[host] = nil
			end)
		end
		local cg = UI.make("CanvasGroup", {
			Name = "Fade",
			BackgroundTransparency = 1,
			Size = UDim2.fromScale(1, 1),
			GroupTransparency = toT == 0 and 1 or 0,
			Parent = host,
		})
		for _, child in ipairs(host:GetChildren()) do
			if child ~= cg and child:IsA("GuiObject") then
				child.Parent = cg
			end
		end
		st = { cg = cg }
		fades[host] = st
	end
	if st.tween then
		st.tween:Cancel()
	end
	local info = TweenInfo.new(math.max(UI.motionTime(t), 0.01), style or QUINT, dir or OUT, 0, false, delay and UI.motionTime(delay) or 0)
	local tw = TweenService:Create(st.cg, info, { GroupTransparency = toT })
	st.tween = tw
	tw.Completed:Connect(function(state)
		if state ~= Enum.PlaybackState.Completed or st.tween ~= tw or toT ~= 0 then
			return
		end
		for _, child in ipairs(st.cg:GetChildren()) do
			child.Parent = host
		end
		st.cg:Destroy()
		fades[host] = nil
	end)
	tw:Play()
	return tw
end

-- Layered soft shadow. `host` must be a transparent frame sized like the thing it shades,
-- placed BEHIND it as a sibling (children always draw over their parent).
function UI.softShadow(host, radius, layers, spread, opacity, dy)
	for i = layers, 1, -1 do
		local g = i * spread
		UI.make("Frame", {
			Name = "Shade",
			BackgroundColor3 = C.black,
			BackgroundTransparency = 1 - opacity,
			BorderSizePixel = 0,
			Position = UDim2.fromOffset(-g, -g + (dy or 0)),
			Size = UDim2.new(1, g * 2, 1, g * 2),
			Parent = host,
		}, { UI.corner(radius + g) })
	end
end

-------------------------------------------------------------------- data
-- Sidebar pages. Each entry in `controls` is either
--   a feature card  { id, name, desc, icon, enabled, rows }   ON/OFF switch; ON opens its settings
--   a section       { id, kind = "section", name, rows }       always-visible settings
-- Pages with `tabs` show one tab's cards at a time (each card names its `tab`).
-- Row types: slider · segment · key · swatch · checks · actions · note · textPreview · motionPreview
local PREF_SCALE = {
	type = "slider", id = "uiScale", label = "Interface scale", desc = "Size of every panel on screen",
	min = 0.4, max = 1.5, step = 0.01, nudge = 0.05, value = DEFAULT_SCALE, format = "%.2f", stepper = true,
}
local PREF_KEY = { type = "key", id = "openKey", label = "Open menu key", desc = "Shows or hides this window", value = DEFAULT_KEY }
local PREF_NOTICE = { id = "hiddenNotice", label = "Remind me of the open key when hidden", value = true }

local PAGES = {
 {name="Shooting",icon="ball",subtitle="Timing and release limits",tabs={"Timing","Shot limits"},controls={
  {id="autoRelease",tab="Timing",name="Auto time",enabled=false,rows={
   {type="slider",id="offset",label="Release adjustment",min=-30,max=30,step=1,value=0,format="%.0f",suffix=" ms"},
   {type="slider",id="maxPing",label="Maximum ping",min=50,max=1000,step=25,value=1000,format="%.0f",suffix=" ms"},
   {type="slider",id="freshness",label="Meter freshness",min=60,max=200,step=10,value=120,format="%.0f",suffix=" ms"},
  }},
  {id="shotAssist",tab="Shot limits",name="Shot assist",enabled=false,rows={
   {type="slider",id="meterMin",label="Minimum release",min=1,max=100,step=1,value=80,format="%.0f",suffix="%"},
   {type="slider",id="meterMax",label="Maximum release",min=1,max=100,step=1,value=100,format="%.0f",suffix="%"},
   {type="checks",items={{id="holdEarly",label="Hold early release",value=true},{id="protectFall",label="Prevent overhold",value=true}}},
  }},
  {id="contestCeiling",tab="Shot limits",name="Contest ceiling",enabled=false,rows={
   {type="slider",id="limit",label="Maximum contest",min=5,max=100,step=5,value=30,format="%.0f",suffix="%"},
   {type="checks",items={{id="release",label="Release before a closeout",value=false}}},
   {type="note",label="Blocks new shots above the estimate. Optional release uses safe meter timing."},
  }}
 }},
 {name="Passing",icon="ball",subtitle="Pass selection and receiver information",tabs={"Smart pass","Receivers"},controls={
  {id="catchShoot",tab="Smart pass",name="Auto shoot off pass",enabled=false,rows={
   {type="actions",items={{type="button",id="status",label="Check early-shot buffer",icon="refresh"}}},
   {type="slider",id="contest",label="Maximum estimated contest",min=0,max=25,step=5,value=10,format="%.0f",suffix="%"},
   {type="note",label="Requires Auto time. Buffers an open catch, waits for standstill, and shoots as soon as ready. Skips if the defender could arrive before release; never queues a late shot."},
  }},
  {id="smartPass",tab="Smart pass",name="Smart pass",enabled=false,rows={
   {type="actions",items={{type="button",id="learnOverhead",label="Learn overhead",icon="plus"},{type="button",id="learnChest",label="Learn chest",icon="plus"}}},
   {type="actions",items={{type="button",id="learnBounce",label="Learn bounce",icon="plus"},{type="button",id="status",label="Pass status",icon="refresh"}}},
   {type="actions",items={{type="button",id="reset",label="Clear learned pass types",icon="close"}}},
   {type="note",label="Setup: learn all pass types with the same teammate. Overhead: number. Chest: Shift + number. Bounce: double-tap number."},
   {type="note",label="Unrecognized passes stay unchanged. Paths are estimates."},
  }},
  {id="passVision",tab="Receivers",name="Pass now · open teammates",enabled=false,rows={
   {type="slider",id="speed",label="Estimated pass speed",min=40,max=140,step=5,value=80,format="%.0f",suffix=" studs/s"},
   {type="slider",id="release",label="Catch-to-release time",min=.2,max=1,step=.05,value=.5,format="%.2f",suffix=" s"},
   {type="note",label="Ranks teammate spacing and time before a defender can contest. Estimates only; cannot guarantee a green."},
  }},
  {id="passingLanes",tab="Receivers",name="Passing lanes",enabled=false,rows={
   {type="slider",id="range",label="Teammate range",min=10,max=100,step=5,value=60,format="%.0f",suffix=" studs"},
   {type="slider",id="clearance",label="Defender clearance",min=1,max=6,step=.25,value=3,format="%.2f",suffix=" studs"},
  }}
 }},
 {name="Defense",icon="shield",subtitle="Choose the part of defense to configure",tabs={"On-ball","Off-ball","Steals","Blocks","Rebounds"},controls={
  {id="autoGuard",tab="On-ball",name="Auto guard",enabled=false,rows={
   {type="checks",items={{id="cutoff",label="Cut off drives",value=false}}},
   {type="checks",items={{id="coverage",label="Opponent coverage outline",value=true}}},
   {type="slider",id="standOff",label="Stand-off distance",min=2,max=8,step=.25,nudge=.5,value=5,format="%.2f",stepper=true},
   {type="segment",id="response",label="Response",options={"Legit","Balanced","Blatant"},value="Balanced"},
   {type="slider",id="delay",label="Reaction delay",min=0,max=.4,step=.02,value=.16,format="%.2f",suffix=" s"},
   {type="key",id="activation",label="Activation",value=Enum.KeyCode.G,hint="Hold"},
   {type="checks",items={{id="fakeFilter",label="Shot-fake filter (estimate)",value=true}}},
   {type="checks",items={{id="humanize",label="Humanize movement",value=true},{id="rimPress",label="Press up near the rim",value=true}}},
   {type="slider",id="humanLevel",label="Humanizer strength",min=0,max=100,step=5,value=60,format="%.0f",suffix="%"},
   {type="slider",id="rimRange",label="Rim press range",min=8,max=30,step=1,value=18,format="%.0f",suffix=" studs"},
   {type="slider",id="rimGap",label="Rim press distance",min=.75,max=3,step=.25,value=1.3,format="%.2f",suffix=" studs"},
  }},
  {id="screens",tab="On-ball",name="Screen avoidance",enabled=false,rows={
   {type="checks",items={{id="visual",label="Show screen warnings",value=true}}},
   {type="note",label="Auto guard routes around players blocking its path. Visual warnings are optional."},
  }},
  {id="assignmentTools",tab="On-ball",kind="section",name="Target lock",rows={
   {type="checks",items={{id="lock",label="Keep selected matchup through passes",value=false}}},
   {type="actions",items={{type="button",id="next",label="Switch matchup (B)",icon="refresh"}}},
  }},
  {id="denyReceiver",tab="Off-ball",name="Off-ball guard",enabled=false,rows={
   {type="actions",items={{type="button",id="select",label="Change matchup",icon="crosshair"},{type="button",id="clear",label="Clear matchup",icon="close"}}},
   {type="slider",id="delay",label="Reaction delay",min=0,max=.6,step=.025,value=.2,format="%.2f",suffix=" sec"},
   {type="note",label="Uses the opponent you click at game start. Hold your guard key to cover them. Your mouse and camera stay unchanged."},
  }},
  {id="autoSteal",tab="Steals",name="Auto steal",enabled=false,rows={
   {type="slider",id="reach",label="Attempt distance",min=1,max=6,step=.25,value=3.5,format="%.2f",suffix=" studs"},
   {type="slider",id="delay",label="Opportunity delay",min=.05,max=.5,step=.025,value=.12,format="%.3f",suffix=" s"},
   {type="slider",id="cooldown",label="Attempt cooldown",min=.6,max=3,step=.1,value=1.6,format="%.1f",suffix=" s"},
  }},
  {id="passIntercept",tab="Steals",name="Pass interception · hold R",enabled=false,rows={
   {type="checks",items={{id="automatic",label="Automatic catches (Auto steal required)",value=false}}},
   {type="checks",items={{id="move",label="Move into flight path",value=true}}},
   {type="slider",id="reach",label="Attempt distance",min=2,max=6,step=.25,value=4,format="%.2f",suffix=" studs"},
   {type="checks",items={{id="shots",label="Include shots and rebounds",value=false}}},
   {type="actions",items={{type="button",id="status",label="Interception status",icon="refresh"}}},
   {type="note",label="Hold R before or during a pass, or enable automatic attempts. Tracks reachable loose flights; native catch success is unverified."},
  }},
  {id="recovery",tab="Steals",name="Loose-ball marker",enabled=false,rows={
   {type="slider",id="range",label="Marker range",min=4,max=24,step=1,value=12,format="%.0f",suffix=" studs"},
   {type="slider",id="window",label="After-steal window",min=1,max=5,step=.25,value=3,format="%.2f",suffix=" s"},
  }},
  {id="autoBlock",tab="Blocks",name="Auto block",enabled=false,rows={
   {type="slider",id="reactionDelay",label="Reaction delay",min=.05,max=.5,step=.01,value=.08,format="%.2f",suffix=" s"},
   {type="checks",items={{id="commit",label="Wait for committed shot",value=false}}},
   {type="checks",items={{id="oneAttempt",label="One attempt per shot",value=true}}},
  }},
  {id="antiOop",tab="Blocks",name="Anti alley-oop",enabled=false,rows={
   {type="checks",items={{id="move",label="Move toward reachable lobs",value=true}}},
   {type="checks",items={{id="guide",label="Show interception point and route",value=true}}},
   {type="slider",id="range",label="Detection distance",min=8,max=26,step=1,value=18,format="%.0f",suffix=" studs"},
   {type="actions",items={{type="button",id="status",label="Lob planner status",icon="refresh"}}},
   {type="note",label="Tracks observed lobs. Automatic catches require Auto steal. Manual R remains available; catches are server-decided."},
  }},
  {id="rebound",tab="Rebounds",name="Auto rebound",enabled=false,rows={
   {type="checks",items={{id="chase",label="Pursue nearby balls",value=true}}},
   {type="checks",items={{id="early",label="Head to the rim when a shot is released",value=false}}},
   {type="slider",id="spot",label="Rim spot distance",min=3,max=9,step=.5,value=5.5,format="%.1f",suffix=" studs"},
   {type="slider",id="range",label="Pursuit distance",min=4,max=20,step=1,value=12,format="%.0f",suffix=" studs"},
   {type="slider",id="reach",label="Jump distance",min=2,max=10,step=.5,value=5,format="%.1f",suffix=" studs"},
   {type="slider",id="height",label="Ball height",min=3,max=12,step=.5,value=7,format="%.1f",suffix=" studs"},
   {type="slider",id="lead",label="Flight lookahead",min=0,max=.35,step=.025,value=.15,format="%.3f",suffix=" s"},
  }}
 }},
 {name="Movement",icon="run",subtitle="Dribbling and saved sequences",tabs={"Dribbling","Combos"},controls={
  {id="smartDribble",tab="Dribbling",name="Smart dribbler",enabled=false,rows={
   {type="key",id="activation",label="Hold key",value=Enum.KeyCode.H,hint="Hold"},
   {type="slider",id="cooldown",label="Combo interval",min=.07,max=.5,step=.01,value=.12,format="%.2f",suffix=" s"},
   {type="actions",items={{type="button",id="status",label="Check dribbler status",icon="refresh"}}},
   {type="note",label="Hold your key to chain dribbles. Moves react to nearby defenders and your facing direction. No hoop detection, automatic shooting, or hoop-directed movement."},
  }},
  {id="ballProtection",tab="Dribbling",name="Ball protection",enabled=false,rows={
   {type="slider",id="range",label="Pressure range",min=2,max=6,step=.25,value=4.5,format="%.2f",suffix=" studs"},
   {type="note",label="Hold the dribble key to react to ball-side pressure. Release to stop new moves. Uses the dribbler stamina reserve and move interval."},
  }},
  {id="boost",tab="Dribbling",name="Dribble boost",enabled=false,rows={
   {type="slider",id="multiplier",label="Boost",min=1,max=1.5,step=.05,value=1.15,format="%.2f",stepper=true},
   {type="slider",id="tail",label="Duration",min=.1,max=.5,step=.05,value=.25,format="%.2f",suffix=" s"},
  }},
  {id="comboBuilder",tab="Combos",name="Combo builder",enabled=false,rows={
   {type="segment",id="preset",label="Preset",options={"Custom","Crossovers","Stepback","Mixed"},value="Custom"},
   {type="combo",id="sequence",label="Sequence",value="Z@0.20 > X@0.25 > C@0.20 > V@0.30"},
   {type="slider",id="repeats",label="Sequence repeats",min=1,max=4,step=1,value=1,format="%.0f"},
   {type="checks",items={{id="sprint",label="Sprint dribble inputs",value=false}}},
   {type="key",id="activation",label="Play sequence",value=Enum.KeyCode.J,hint="Press"},
   {type="actions",items={{type="button",id="play",label="Play combo",icon="run"},{type="button",id="cancel",label="Cancel",icon="close"}}},
  }},
  {id="comboLibrary",tab="Combos",kind="section",name="Saved combos",rows={
   {type="library",id="saved",value=""},
   {type="note",label="Build a combo above, name it here, choose a hotkey and Save. Saved keys play while Dribble builder is enabled."},
  }}
 }},
 {name="Visuals",icon="layers",subtitle="On-court indicators and appearance",tabs={"Ball","Players","Style","Details"},controls={
  {id="landingTools",tab="Ball",kind="section",name="Ball flight & landing",rows={
   {type="checks",items={{id="enabled",label="Predict flight and landing",value=true}}},
   {type="checks",items={{id="trail",label="Ball motion trail",value=false}}},
   {type="checks",items={{id="beam",label="Flight beam",value=true},{id="zone",label="Landing zone",value=true}}},
   {type="slider",id="width",label="Beam width",min=.02,max=.2,step=.01,value=.065,format="%.2f"},
   {type="slider",id="scale",label="Zone size",min=.5,max=2,step=.1,value=1,format="%.1f",suffix="x"},
   {type="slider",id="range",label="Track distance",min=8,max=120,step=4,value=60,format="%.0f",suffix=" studs"},
  }},
  {id="overlayTools",tab="Players",kind="section",name="Player indicators",rows={
   {type="checks",items={{id="contest",label="Contest outline",value=true},{id="guard",label="Guard position guide",value=true}}},
  }},
  {id="defenderAlerts",tab="Players",name="Defender alerts",enabled=false,rows={
   {type="slider",id="range",label="Alert distance",min=6,max=40,step=1,value=16,format="%.0f",suffix=" studs"},
   {type="checks",items={{id="ballOnly",label="Only while I have the ball",value=false}}},
   {type="note",label="Shows an arrow badge at the screen edge for a nearby opponent you can't see (behind or beside you), with their distance. Display only; it sends no input."},
  }},
  {id="visualStyle",tab="Style",kind="section",name="Appearance",rows={
   {type="checks",items={{id="enabled",label="Modern court visuals",value=true}}},
   {type="segment",id="preset",label="Style",options={"Minimal","Balanced","Vivid"},value="Balanced"},
   {type="segment",id="palette",label="Accent",options={"Cobalt","Glacier","Mint","Amber"},value="Cobalt"},
   {type="segment",id="route",label="Route pattern",options={"Dashed","Solid","Chevrons"},value="Dashed"},
   {type="checks",items={{id="motion",label="Animated rings and pulses",value=false}}},
   {type="checks",items={{id="urgency",label="Urgency colour + timer ring",value=true},{id="beacon",label="Landing beacon",value=true}}},
   {type="checks",items={{id="flightShadow",label="Floor shadow + apex",value=true},{id="comet",label="Ball motion trail",value=true}}},
   {type="slider",id="scale",label="Marker scale",min=.6,max=1.6,step=.05,value=1,format="%.2f",suffix="x"},
   {type="slider",id="intensity",label="Glow intensity",min=10,max=100,step=5,value=65,format="%.0f",suffix="%"},
   {type="slider",id="weight",label="Line weight",min=.5,max=2,step=.05,value=1.15,format="%.2f",suffix="x"},
   {type="slider",id="smoothing",label="Display smoothing",min=0,max=100,step=5,value=55,format="%.0f",suffix="%"},
   {type="slider",id="clearance",label="Height above court",min=.04,max=.35,step=.01,value=.12,format="%.2f",suffix=" studs"},
   {type="note",label="Styles the existing flight, landing, positioning and player indicators. Disable to use the previous visuals."},
  }},
  {id="visualLayers",tab="Style",kind="section",name="Visible layers",rows={
   {type="checks",items={{id="landing",label="Landing marker",value=true},{id="flight",label="Flight path",value=true}}},
   {type="checks",items={{id="route",label="Positioning route",value=true},{id="coverage",label="Coverage indicator",value=true}}},
   {type="checks",items={{id="pass",label="Pass markers and lanes",value=true},{id="recovery",label="Recovery marker",value=true}}},
   {type="checks",items={{id="screen",label="Screen warning",value=true},{id="intercept",label="Meeting-point marker",value=true}}},
  }},
  {id="visualLabels",tab="Details",kind="section",name="HUD cards",rows={
   {type="segment",id="corner",label="Corner",options={"Bottom right","Bottom left","Top right"},value="Bottom right"},
   {type="checks",items={{id="passPanel",label="Extra pass-target corner card",value=false}}},
  }},
  {id="visualColor",tab="Details",kind="section",name="Custom accent",rows={
   {type="checks",items={{id="enabled",label="Use custom RGB accent",value=false}}},
   {type="slider",id="red",label="Red",min=0,max=255,step=1,value=64,format="%.0f"},
   {type="slider",id="green",label="Green",min=0,max=255,step=1,value=132,format="%.0f"},
   {type="slider",id="blue",label="Blue",min=0,max=255,step=1,value=255,format="%.0f"},
  }}
 }},
 {name="Tools",icon="wrench",subtitle="Queue and session utilities",controls={
  {id="autofarm",name="Blacktop autofarm",enabled=false,desc="1v1 grinder: guards, grabs loose balls, drives for dunks",rows={
   {type="checks",items={{id="crossover",label="Dribble moves past the defender",value=true},{id="shoot",label="Take open shots",value=true},{id="randomThrees",label="Attempts 3s sometimes",value=false}}},
   {type="note",label="Attempts 3s sometimes: up to 2–3 random attempts per detected game. Shift Z/C → Shift XX → immediate E. Ignores contest and standing-shot checks for this combo; attempts are not guaranteed makes or three-pointers."},
   {type="slider",id="shotRange",label="Shot range",min=12,max=45,step=1,value=20,format="%.0f",suffix=" studs"},
   {type="slider",id="dunkRange",label="Dunk range",min=6,max=24,step=1,value=12,format="%.0f",suffix=" studs"},
   {type="actions",items={{type="button",id="status",label="Farm status",icon="refresh"},{type="button",id="readyNow",label="Ready now",icon="crosshair"}}},
   {type="note",label="Saved when you turn it on. Every time the script runs with it on, a 5 second prompt lets you press No; otherwise it readies up, queues itself for the next teleport and farms."},
  }},
  {id="autoJoin",name="Auto join spot",enabled=false,rows={
   {type="actions",items={{type="button",id="lock",label="Lock nearest spot",icon="crosshair"},{type="button",id="status",label="Join status",icon="refresh"}}},
   {type="note",label="Stand beside your desired spot and lock it. Retries every 0.25 s; pauses away from that spot and stops requesting once joined."},
  }},
  {id="tools",kind="section",name="Session controls",rows={
   {type="actions",items={{type="button",id="assignment",label="Switch opponent",icon="refresh"},{type="button",id="report",label="Match report",icon="layers"}}},
   {type="actions",items={{type="button",id="check",label="Connection check",icon="wrench"},{type="button",id="unload",label="Unload",icon="close"}}},
  }}
 }},
 {name="Configs",icon="layers",subtitle="Saved profiles",custom="configs",controls={}},
 {name="Preferences",icon="gear",tabs={"Themes","Layout","Notifications"},subtitle="Interface and controls",system=true,bottom=true,controls={{id="interface",tab="Layout",kind="section",name="Interface",rows={PREF_SCALE,PREF_KEY,{type="checks",items={PREF_NOTICE}}}},{
				id = "appearance", tab = "Themes", icon = "sparkle", name = "Color themes", enabled = true,
				desc = "Colour, glow and ambience of the window",
				rows = {
					{ type = "swatch", id = "theme", label = "Accent colour", desc = "Re-tints the whole interface", options = THEME_NAMES, value = "Cobalt" },
					{ type = "slider", id = "opacity", label = "Window opacity", min = 70, max = 100, step = 1, value = 98, format = "%.0f", suffix = "%" },
					{ type = "checks", items = {
						{ id = "border", label = "Animated border", value = true },
						{ id = "ambient", label = "Ambient glow", value = true },
					} },
				},
			},{
				id = "layout", tab = "Layout", icon = "bounds", name = "Adaptive layout", enabled = true,
				desc = "Spacing and density of every list",
				rows = {
					{ type = "slider", id = "spacing", label = "Card spacing", desc = "Gap between cards", min = 4, max = 32, step = 1, value = 14, format = "%.0f", suffix = " px" },
					{ type = "segment", id = "density", label = "Density", desc = "Row height inside cards", options = { "Compact", "Comfortable", "Roomy" }, value = "Comfortable" },
					{ type = "checks", items = {
						{ id = "labels", label = "Section captions", value = true },
						{ id = "dividers", label = "Row dividers", value = true },
					} },
					{ type = "actions", items = { { type = "button", id = "resetLayout", label = "Reset layout", icon = "refresh" } } },
				},
			},{
				id = "motion", icon = "run", name = "Interface motion", enabled = true,
				desc = "Applies to the whole window",
				rows = {
					{ type = "slider", id = "speed", label = "Transition length", desc = "Base duration of animations", min = 0.1, max = 1, step = 0.05, value = 0.3, format = "%.2f", suffix = " s" },
					{ type = "segment", id = "curve", label = "Easing", options = { "Smooth", "Linear", "Spring" }, value = "Smooth" },
					{ type = "motionPreview", label = "Preview", desc = "Click Play or press the shortcut" },
					{ type = "key", id = "shortcut", label = "Preview shortcut", desc = "Plays the preview from anywhere", value = Enum.KeyCode.J },
					{ type = "checks", items = { { id = "reduce", label = "Reduce motion", value = false } } },
				},
			},{
				id = "typography", tab = "Layout", icon = "text", name = "Text preview", enabled = false,
				desc = "Try sizes and weights on sample copy",
				rows = {
					{ type = "slider", id = "size", label = "Text size", min = 12, max = 34, step = 1, value = 22, format = "%.0f", suffix = " px" },
					{ type = "segment", id = "weight", label = "Weight", options = { "Regular", "Medium", "Bold" }, value = "Medium" },
					{ type = "textPreview" },
				},
			},{
				id = "notifications", icon = "bell", name = "Notifications", enabled = true,
				desc = "Toasts for saves, profiles and previews",
				rows = {
					{ type = "slider", id = "duration", label = "Display duration", desc = "How long each toast stays up", min = 1, max = 10, step = 0.5, value = 4, format = "%.1f", suffix = " s" },
					{ type = "segment", id = "position", label = "Position", desc = "Corner of the screen", options = { "Left", "Center", "Right" }, value = "Right" },
					{ type = "checks", items = {
						{ id = "progress", label = "Countdown bar", value = true },
						{ id = "stack", label = "Stack multiple toasts", value = true },
					} },
					{ type = "actions", items = { { type = "button", id = "preview", label = "Preview notification", icon = "bell" } } },
				},
			}}},
}

local START_PAGE = "Shooting"

local PAGE_BY_NAME = {}
for _, page in ipairs(PAGES) do
	PAGE_BY_NAME[page.name] = page
end

function UI.forEachControl(fn, includeSystem)
	for _, page in ipairs(PAGES) do
		if includeSystem or not page.system then
			for _, ctrl in ipairs(page.controls) do
				fn(ctrl, page)
			end
		end
	end
end

-- every row/item that holds a value
function UI.forEachValue(ctrl, fn)
	for _, r in ipairs(ctrl.rows or {}) do
		if r.id and r.value ~= nil then
			fn(r)
		end
		for _, it in ipairs(r.items or {}) do
			if it.id and it.value ~= nil then
				fn(it)
			end
		end
	end
end

local CTRL, SETTING = {}, {} -- id -> control, control id -> { setting id -> setting }
UI.forEachControl(function(ctrl)
	ctrl.defaultEnabled = ctrl.enabled
	ctrl.expanded = ctrl.id == "appearance"
	CTRL[ctrl.id] = ctrl
	SETTING[ctrl.id] = {}
	UI.forEachValue(ctrl, function(s)
		s.default = s.value
		SETTING[ctrl.id][s.id] = s
	end)
end, true)

do
local forEachControl=UI.forEachControl
local forEachValue=UI.forEachValue
do
 local help={
  smartPossession={plan="Balanced considers open jumpers and driving lanes. Attack basket prioritizes advancing inside 16 studs. Find a three targets your marked radial arc distance and will not invent an unmeasured line.",steer="While H is held, normal movement follows the chosen available lane. WASD takes priority. This does not change WalkSpeed or teleport your player.",arc="Approximate radial distance from the assigned hoop to the arc. Stand on the actual curved three-point line and use Mark three-point distance here. Corner rules and the game's official shot classification are not inferred."},
  smartDribble={gap="Desired spacing before settling. Automatic shooting can require more space to satisfy its contest limit.",stamina="Pause smart dribbling and ball protection below this stamina value. Does not restore stamina.",cooldown="Minimum time between acknowledged move requests. Default 0.24 seconds; actual accepted timing depends on the game controller. Unacknowledged moves cause backoff.",shoot="Start one shot after a stable open window while H is held. Requires Auto time enabled, possession, an actual basket, settled movement and an acceptable estimated closeout.",shotContest="Maximum estimated pressure now and 0.3 seconds ahead. This estimate is not the game's actual contest formula.",shotRange="Do not start an automatic shot beyond this distance from the assigned basket. Does not determine your player's actual shooting range.",follow="Practice defender moves toward a delayed basket-side position and shades your ball hand. It cannot apply a real contest or steal.",testSpeed="Maximum local practice-defender speed. Acceleration and reaction delay keep it from instantly matching your movement."},
  ballProtection={range="React only when a known opponent is this close on the ball-hand side. Uses the same measured moves, stamina reserve and cooldown as Smart dribbler. This is an escape estimate, not immunity to steals."},
  passVision={speed="Assumed ball speed for pass ETA, not a change to actual pass speed.",release="Estimated time from receiving the pass to releasing a shot. A teammate is marked open only when a defender is estimated to arrive later, with a clear lane."},
  autoRelease={offset="Shifts Auto time's release threshold. Positive values delay release; negative values advance it. Start at 0 ms.",maxPing="Auto time waits rather than releases when the measured ping is above this limit. This does not reduce your ping.",freshness="Maximum age of the latest changing meter sample. A smaller limit rejects delayed reads more aggressively."},
  shotRules={standing="Allow Auto time to manage shots classified as stationary when the meter starts. Turning this off does not block your manual release.",moving="Allow Auto time to manage shots started while moving faster than 4 studs/sec. This is a movement classification, not detection of every fade animation.",distance="Skip Auto time when the known basket is farther away than this distance. 0 disables the limit. If basket data is unavailable, this filter cannot classify range.",hitch="Temporarily pause automatic release for 120 ms after an unusually slow rendered frame. This may miss a timing window; it cannot repair a frame stall.",hitchMs="A rendered frame longer than this duration is considered a hitch. Only used when the frame-hitch pause is enabled."},
  contestCeiling={limit="Your maximum acceptable ESTIMATED contest. It is a local geometry estimate, not the game's exact server percentage. When enabled, new shots above this estimate are blocked.",forecast="How far ahead to project observed movement when estimating a closeout. 0 uses current positions only. Longer forecasts become less reliable when players change direction.",release="Optional: release a held shot before estimated pressure crosses the ceiling. Only uses a fresh rising meter at 90–100% fill, also respecting Shot assist if enabled. Off by default; this may sacrifice perfect timing.",minimum="The earliest meter fill allowed for a contest-triggered release. 90 means the meter must have reached at least 90%; it is not a 90% make chance.",maximum="The latest meter fill allowed for a contest-triggered release. The permitted range is also intersected with Shot assist's range when enabled.",hold="How long estimated pressure must remain at or above the ceiling before a release is considered. Helps reject a brief noisy reading.",rising="Only permit a contest-triggered release while the meter is rising. Prevents this feature from reacting after the peak has passed.",blockStart="Optional: reject a new normal shot input if current estimated contest is already above the ceiling. Does not cancel a shot already started. Block/rebound jump inputs are excluded."},
  shootingInfo={preview="A line on the supported Bat meter showing Auto time's current calculated release threshold. It appears only with an active supported meter and usable timing data; it does not change the green window.",thickness="Visual thickness of the preview line. It changes no shot timing or hitbox.",confidence="Shows meter freshness and latency stability. Stable samples means the inputs look consistent, not that the shot is guaranteed to be Perfect.",history="Shows recent feedback actually received from the game. Perfect, Early, and Late describe release feedback; they do not confirm a basket was made.",rows="Number of recent feedback entries visible in the screen-edge panel. Up to 20 entries are retained for this session.",distance="Show the distance to the known basket when the meter started. This is measured in Roblox studs, not feet.",contest="Show the contest value supplied in the game's post-shot feedback. This is separate from the live estimated contest ceiling.",ping="Show the latest measured ping recorded when shot feedback arrived. This is not a server receipt timestamp.",clear="Clear only this session's displayed recent-shot list. Does not reset Auto time's saved timing profiles."},
  shotAssist={meterMin="If early-release holding is enabled, a release below this meter fill is deferred until the minimum is reached. Inside the permitted range, manual release stays immediate.",meterMax="Upper release limit. Below 100%, this can release a held shot at the selected fill. 100% uses the existing peak predictor where available. This is meter fill, not make chance.",holdEarly="If you let go too early, keep the shot held until the minimum fill. Turning this off preserves your early manual release.",protectFall="If the meter has passed its peak and is falling, request release before the predicted fill drops below your minimum."},
  autoGuard={dunks="While holding the guard key, close the gap to 1.8 studs when the handler is within 9 studs of their rim. Attempt one native jump per observed dunk if in range and grounded. Not a guaranteed block.",cutoff="Lead a ball handler driving toward the basket using observed velocity. Only acts while Auto guard is engaged; movement stays native.",screens="Route earlier around active screens and nearby opponents obstructing the guard path. Uses wider waypoints, side commitment and map raycasts.",coverage="Outline the selected opponent: green for estimated basket-side coverage, amber for partial coverage, red for poor position. Not the server contest formula.",standOff="Desired basket-side spacing from the handler, in studs.",response="Applies a group of reaction and movement settings. It does not alter server rules or guarantee defensive outcomes.",delay="Wait before reacting to a newly selected target.",activation="Hold this key to engage Auto guard.",fakeFilter="Use observed shot persistence to reduce reactions to brief fake-shot states. It is a heuristic and can react late."},
  passIntercept={automatic="Attempt normal steals on eligible nearby moving loose balls without pressing R. This is not a guaranteed interception.",move="Allow normal movement toward a short predicted flight intercept. WASD and typing override pursuit.",reach="Distance at which a normal steal request may be attempted. This does not increase the server catch radius.",jump="Allow the normal Space/Jump input for a nearby high flight. The server still determines whether a catch or block occurs.",shots="Include balls reporting MidShot, such as shots and rebounds. Off limits the selector to other moving loose balls."},
  autoSteal={reach="Distance at which to ATTEMPT a normal steal. Increasing it does not establish increased server steal reach.",delay="An opportunity must stay eligible this long before an attempt.",cooldown="Minimum spacing between steal attempts."},
  autoBlock={reactionDelay="Delay after observing a shooting action before a block can be attempted.",commit="Wait for a release flag or persistent shot state before attempting the block. Can avoid fakes but react later.",oneAttempt="Allow at most one block request for each observed shot."},
  rebound={range="Maximum distance for normal-movement pursuit of a nearby rebound.",reach="Distance within which a descending loose ball can trigger a jump attempt.",height="Maximum ball height above your root for a rebound jump attempt.",lead="How far ahead to project the loose ball's horizontal movement.",chase="Move toward a reachable loose ball or estimated landing position. Manual movement takes priority."},
  comboBuilder={sequence="Click + Z/X/C/V to add steps. Select a step and toggle Combine keys for grouped inputs. Double tap creates two separate requests, with a 0.10 s tap gap that you can edit using Wait after move.",preset="Load an example input sequence that you can edit. The game still decides which dribble animation each input group produces.",interval="Default wait after steps without an explicit @ delay.",repeats="Number of full sequence repetitions. Manual Z/X/C/V or Cancel stops the sequence.",sprint="Send the sprint modifier with dribble groups. Does not restore stamina.",activation="Press this key to play the enabled builder while you have possession."},
  landingTools={enabled="Estimate a moving loose ball's path and landing point. Does not model undisclosed server physics or future collisions.",trail="Short visual trail of the ball's observed recent movement, rather than a prediction.",beam="Display the estimated future flight curve.",zone="Display the estimated landing region. A larger region represents uncertainty, not a guaranteed catch area.",width="Visual width of the predicted flight beam.",scale="Visual size multiplier for the landing region.",range="Maximum nearby distance at which to track the moving ball."}
 }
 for key,value in pairs(help.smartPossession)do help.smartDribble[key]=value end
 for _,page in ipairs(PAGES)do for _,ctrl in ipairs(page.controls or {})do
  local group=help[ctrl.id]
  if group then for _,row in ipairs(ctrl.rows or {})do
   if row.id then row.help=group[row.id]end
   for _,item in ipairs(row.items or {})do item.help=group[item.id]end
  end end
 end end
end

end
function UI.pageCounts(page)
	local on, total = 0, 0
	for _, ctrl in ipairs(page.controls) do
		if ctrl.kind ~= "section" then
			total += 1
			if ctrl.enabled then
				on += 1
			end
		end
	end
	return on, total
end

function UI.featureCounts()
	local on, total = 0, 0
	for _, page in ipairs(PAGES) do
		if not page.system then
			local o, t = UI.pageCounts(page)
			on += o
			total += t
		end
	end
	return on, total
end

-- layout settings that need a re-render when they change
local DENSITY = { Compact = 0.84, Comfortable = 1, Roomy = 1.16 }
local EASINGS = { Smooth = QUINT, Linear = LINEAR, Spring = BACK }
local layoutState = { density = 1, captions = true, dividers = true }

-- forward declarations
local closePopup, openPage, renderPage, rerender, setStatus, toast
local applySettings, repaintAll, refreshMini, refreshNav, playMotionPreview, setMode
local statsListener

-------------------------------------------------------------------- root
local TITLE_H, SIDE_W, FOOT_H = 64, 220, 68
local CONTENT_X = SIDE_W + 32
UI.initialViewport=workspace.CurrentCamera and workspace.CurrentCamera.ViewportSize or Vector2.new(1920,1080)
local W,H=math.clamp(UI.initialViewport.X*.94,1000,1320),math.clamp(UI.initialViewport.Y*.90,560,860)
local MIN_W, MIN_H = 960, 520
local PAD = 12 -- room around the window for its shadow
local winW, winH = W, H
local targetW, targetH = W, H

local previousUI = playerGui:FindFirstChild("PracticalUI")
if previousUI then previousUI:Destroy() end
local gui = UI.make("ScreenGui", {
	Name = "PracticalUI",
	ResetOnSpawn = false,
	IgnoreGuiInset = true,
	ZIndexBehavior = Enum.ZIndexBehavior.Sibling,
	DisplayOrder = 50,
})
-- public hooks: ControlChanged(ctrlId, settingId, value), TestControls()
local controlChanged = UI.make("BindableEvent", { Name = "ControlChanged", Parent = gui })
local testControls = UI.make("BindableEvent", { Name = "TestControls", Parent = gui })

local root = UI.make("Frame", {
	Name = "Root",
	BackgroundTransparency = 1,
	Size = UDim2.fromOffset(W + PAD * 2, H + PAD * 2),
	Visible = false,
	Parent = gui,
})
local fitScale = UI.make("UIScale", { Scale = DEFAULT_SCALE, Parent = root })

-- scaled on open/close and faded with groupFade
local group = UI.make("Frame", {
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(0.5, 0.5),
	Position = UDim2.fromScale(0.5, 0.5),
	Size = UDim2.fromScale(1, 1),
	Parent = root,
})
local popScale = UI.make("UIScale", { Scale = 0.94, Parent = group })

local shadowHost = UI.make("Frame", {
	Name = "Shadow",
	BackgroundTransparency = 1,
	Position = UDim2.fromOffset(PAD, PAD),
	Size = UDim2.fromOffset(W, H),
	Parent = group,
})
UI.softShadow(shadowHost, 16, 7, 4, 0.055, 10)

local winStroke = UI.stroke(C.stroke, 1.2)
local borderGrad = UI.bindTheme(UI.make("UIGradient", { Enabled = false, Parent = winStroke }), "Color", "border")

local win = UI.make("Frame", {
	Name = "Window",
	BackgroundColor3 = C.bg,
	BorderSizePixel = 0,
	ClipsDescendants = true,
	Position = UDim2.fromOffset(PAD, PAD),
	Size = UDim2.fromOffset(W, H),
	ZIndex = 2,
	Parent = group,
}, {
	UI.corner(16),
	winStroke,
	UI.make("UIGradient", { Rotation = 90, Color = ColorSequence.new(C.white, Color3.fromRGB(205, 210, 224)) }),
})

-- Ambient glow: slow-drifting accent light behind everything. A CanvasGroup so the
-- rounded window corners clip it properly.
local ambient = UI.make("CanvasGroup", {
	Name = "Ambient",
	BackgroundTransparency = 1,
	Size = UDim2.fromScale(1, 1),
	ZIndex = 0,
	Parent = win,
}, { UI.corner(16) })
local ambientOn = true
local blobs = {}
for i, spec in ipairs({
	{ key = "a", size = 620, x = 0.82, y = 0.08 },
	{ key = "b", size = 520, x = 0.42, y = 1.0 },
	{ key = "a", size = 420, x = 0.06, y = 0.45 },
}) do
	local blob = UI.make("Frame", {
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromScale(spec.x, spec.y),
		Size = UDim2.fromOffset(spec.size, spec.size),
		Parent = ambient,
	})
	-- concentric discs stack into a soft radial falloff
	for r = 1, 6 do
		local k = r / 6
		UI.bindTheme(UI.make("Frame", {
			BackgroundTransparency = 0.978,
			BorderSizePixel = 0,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Position = UDim2.fromScale(0.5, 0.5),
			Size = UDim2.fromScale(k, k),
			Parent = blob,
		}, { UI.round() }), "BackgroundColor3", spec.key)
	end
	blobs[i] = { frame = blob, x = spec.x, y = spec.y, phase = i * 2.1 }
end

function UI.applySize()
	win.Size = UDim2.fromOffset(winW, winH)
	shadowHost.Size = win.Size
	root.Size = UDim2.fromOffset(winW + PAD * 2, winH + PAD * 2)
end

-- a GuiObject's on-screen box in window design units (for popups / tooltips)
function UI.localBox(obj)
	local s = win.AbsoluteSize.X / winW
	local d = obj.AbsolutePosition - win.AbsolutePosition
	return d.X / s, d.Y / s, obj.AbsoluteSize.X / s, obj.AbsoluteSize.Y / s
end

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

-------------------------------------------------------------------- list
local list = UI.make("ScrollingFrame", {
	BackgroundTransparency = 1,
	BorderSizePixel = 0,
	Position = UDim2.fromOffset(CONTENT_X - 6, LIST_TOP),
	Size = UDim2.new(1, -(CONTENT_X - 6) - 18, 1, -LIST_TOP - FOOT_H - 4),
	CanvasSize = UDim2.new(),
	AutomaticCanvasSize = Enum.AutomaticSize.Y,
	ScrollBarThickness = 4,
	ScrollBarImageColor3 = C.strokeHi,
	ScrollingDirection = Enum.ScrollingDirection.Y,
	Parent = win,
}, { UI.padding(4, 14, 4, 18) })
local listLayout = UI.make("UIListLayout", { SortOrder = Enum.SortOrder.LayoutOrder, Padding = UDim.new(0, 14), Parent = list })
list:GetPropertyChangedSignal("CanvasPosition"):Connect(function()
	closePopup()
end)

function UI.setListTop(top, t)
	UI.tween(list, t, {
		Position = UDim2.fromOffset(CONTENT_X - 6, top),
		Size = UDim2.new(1, -(CONTENT_X - 6) - 18, 1, -top - FOOT_H - 4),
	})
end

local emptyLabel = text(win, "", CONTENT_X, LIST_TOP + 34, 17, { color = C.sub })
emptyLabel.Visible = false

-- every list entry is a slot (sized by the list) holding one "Body" frame (animated)
function UI.newSlot(h, order, name)
	local slot = UI.make("Frame", {
		Name = name or ("slot" .. order),
		BackgroundTransparency = 1,
		Size = UDim2.new(1, 0, 0, h + 4),
		LayoutOrder = order,
		Parent = list,
	})
	local body = UI.make("Frame", {
		Name = "Body",
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Parent = slot,
	})
	return slot, body
end

-------------------------------------------------------------------- footer
UI.strip(win, UDim2.new(0, SIDE_W + 1, 1, -FOOT_H), UDim2.new(1, -SIDE_W - 1, 0, 1), C.line)

local statusDot = UI.make("Frame", {
	BackgroundColor3 = C.green,
	BorderSizePixel = 0,
	AnchorPoint = Vector2.new(0.5, 0.5),
	Position = UDim2.new(0, CONTENT_X + 5, 1, -FOOT_H / 2),
	Size = UDim2.fromOffset(9, 9),
	Parent = win,
}, { UI.round() })
local statusHalo = UI.pulse(statusDot, C.green)

local saveBtn, saveLabel = UI.primaryButton(win, "Save profile", "save", 42)
saveBtn.AnchorPoint = Vector2.new(1, 0.5)
saveBtn.Position = UDim2.new(1, -28, 1, -FOOT_H / 2)
local footX = 28 + saveBtn.Size.X.Offset + 10
local testBtn = UI.ghostButton(win, "Test controls", "check", 42)
testBtn.AnchorPoint = Vector2.new(1, 0.5)
testBtn.Position = UDim2.new(1, -footX, 1, -FOOT_H / 2)
footX += testBtn.Size.X.Offset + 10
local revertBtn, revertIcon = UI.iconButton(win, UDim2.new(1, -footX - 21, 1, -FOOT_H / 2), 42, "refresh", { solid = true, tip = "Revert to saved profile" })
footX += 42 + 18
UI.strip(win, UDim2.new(1, -footX, 1, -FOOT_H / 2 - 12), UDim2.fromOffset(1, 24), C.stroke)
footX += 18
local resetBtn = text(win, "Reset defaults", UDim2.new(1, -footX, 1, -FOOT_H / 2), 0, 15, { color = C.sub, ax = 1, button = true, weight = MEDIUM })
footX += UI.textWidth("Reset defaults", 15) + 24
local statusLabel = text(win, "Ready", UDim2.new(0, CONTENT_X + 20, 1, -FOOT_H / 2), 0, 15, { color = C.text2 })
UI.clip(statusLabel, UDim.new(1, -(CONTENT_X + 20) - footX))
resetBtn.MouseEnter:Connect(function()
	UI.tween(resetBtn, 0.15, { TextColor3 = C.white })
end)
resetBtn.MouseLeave:Connect(function()
	UI.tween(resetBtn, 0.25, { TextColor3 = C.sub })
end)
UI.tooltip(testBtn, "Run a check of every control")
UI.tooltip(saveBtn, "Save current settings to the active profile")

-- resize grip
local grip = UI.make("TextButton", {
	Text = "",
	AutoButtonColor = false,
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(1, 1),
	Position = UDim2.new(1, -3, 1, -3),
	Size = UDim2.fromOffset(20, 20),
	ZIndex = 20,
	Parent = win,
})
local gripIcon = UI.icon(grip, "grip", 10, 10, 18, C.muted, 1.8)
grip.MouseEnter:Connect(function()
	UI.tintIcon(gripIcon, C.white, 0.15)
end)
grip.MouseLeave:Connect(function()
	UI.tintIcon(gripIcon, C.muted, 0.25)
end)

-------------------------------------------------------------------- status line
local statusToken = 0
function UI.baseStatus()
	local on, total = UI.featureCounts()
	if on == 0 then
		return "All features off", C.muted
	end
	return string.format("Ready  ·  %d of %d features on", on, total), C.green
end

function UI.showStatus(str, color)
	statusLabel.Text = str
	statusLabel.TextTransparency = 1
	statusLabel.Position = UDim2.new(0, CONTENT_X + 20, 1, -FOOT_H / 2 + 6)
	UI.tween(statusLabel, 0.3, { TextTransparency = 0, Position = UDim2.new(0, CONTENT_X + 20, 1, -FOOT_H / 2) })
	UI.tween(statusDot, 0.3, { BackgroundColor3 = color })
	statusHalo.BackgroundColor3 = color
end

setStatus = function(str)
	statusToken += 1
	local mine = statusToken
	UI.showStatus(str, A.hi)
	if refreshMini then
		refreshMini()
	end
	UI.after(2.6, function()
		if statusToken == mine then
			UI.showStatus(UI.baseStatus())
		end
	end)
end

function UI.refreshStatus()
	statusToken += 1
	UI.showStatus(UI.baseStatus())
end

-------------------------------------------------------------------- toasts
local TOAST_W, TOAST_H, TM = 420, 96, 16
local toastHost = UI.make("Frame", {
	Name = "Toasts",
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(1, 1),
	Position = UDim2.new(1, -16, 1, -16),
	Size = UDim2.fromOffset(TOAST_W + TM * 2, 600),
	Parent = gui,
})
local toastScale = UI.make("UIScale", { Scale = DEFAULT_SCALE, Parent = toastHost })
UI.make("UIListLayout", {
	SortOrder = Enum.SortOrder.LayoutOrder,
	VerticalAlignment = Enum.VerticalAlignment.Bottom,
	Parent = toastHost,
})
local toastOrder = 0
local liveToasts = {}

function UI.placeToasts()
	local where = SETTING.notifications.position.value
	if where == "Left" then
		toastHost.AnchorPoint = Vector2.new(0, 1)
		toastHost.Position = UDim2.new(0, 16, 1, -16)
	elseif where == "Center" then
		toastHost.AnchorPoint = Vector2.new(0.5, 1)
		toastHost.Position = UDim2.new(0.5, 0, 1, -16)
	else
		toastHost.AnchorPoint = Vector2.new(1, 1)
		toastHost.Position = UDim2.new(1, -16, 1, -16)
	end
end

function UI.dismissToast(entry)
	if entry.gone then
		return
	end
	entry.gone = true
	local i = table.find(liveToasts, entry)
	if i then
		table.remove(liveToasts, i)
	end
	UI.groupFade(entry.wrap, 1, 0.2)
	UI.tween(entry.wrap, 0.25, { Position = UDim2.fromOffset(entry.dx * 0.6, entry.dx == 0 and 10 or 0) })
	UI.tween(entry.slot, 0.3, { Size = UDim2.new(1, 0, 0, 0) }, QUINT, OUT, 0.1).Completed:Connect(function()
		entry.slot:Destroy()
	end)
end

-- `force` shows it even when the Notifications feature is off (e.g. its own Preview button).
toast = function(title, message, iconName, force)
 local lines=math.clamp(math.ceil(UI.textWidth(message or "",14)/(TOAST_W-96))+1,2,6)
 local toastH=math.max(96,50+lines*18)
	if not force and not CTRL.notifications.enabled then
		return
	end
	UI.placeToasts()
	if not SETTING.notifications.stack.value then
		for _, e in ipairs(table.clone(liveToasts)) do
			UI.dismissToast(e)
		end
	elseif #liveToasts >= 3 then
		UI.dismissToast(liveToasts[1])
	end
	toastOrder += 1
	local where = SETTING.notifications.position.value
	local dx = where == "Left" and -56 or where == "Right" and 56 or 0

	local slot = UI.make("Frame", {
		BackgroundTransparency = 1,
		Size = UDim2.new(1, 0, 0, toastH + TM),
		LayoutOrder = toastOrder,
		Parent = toastHost,
	})
	local wrap = UI.make("Frame", { BackgroundTransparency = 1, Size = UDim2.fromScale(1, 1), Parent = slot })
	local shade = UI.make("Frame", {
		BackgroundTransparency = 1,
		Position = UDim2.fromOffset(TM, TM / 2),
		Size = UDim2.fromOffset(TOAST_W, toastH),
		Parent = wrap,
	})
	UI.softShadow(shade, 14, 3, 3, 0.09, 4)
 for layer=1,4 do
  local spread=layer*3
  local glow=UI.make("Frame",{
   Name="NotificationHalo",BackgroundTransparency=1,
   Position=UDim2.fromOffset(TM-spread,TM/2-spread),
   Size=UDim2.fromOffset(TOAST_W+spread*2,toastH+spread*2),Parent=wrap,
  },{UI.corner(16+spread)})
  UI.bindTheme(UI.stroke(A.a,2,.76+layer*.045),"Color","a").Parent=glow
 end
	local body = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.raised,
		Position = UDim2.fromOffset(TM, TM / 2),
		Size = UDim2.fromOffset(TOAST_W, toastH),
		ZIndex = 2,
		Parent = wrap,
	}, { UI.corner(14), UI.stroke(C.strokeHi, 1) })
	local tile = UI.make("Frame", {
		BackgroundColor3 = C.white,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.fromOffset(16, toastH / 2),
		Size = UDim2.fromOffset(38, 38),
		Parent = body,
	}, { UI.corner(11), UI.accentGradient(45) })
	UI.icon(tile, iconName or "bell", 19, 19, 19, C.white, 1.8)
	UI.clip(text(body, title, 68, 25, 17, { weight = SEMIBOLD }), TOAST_W - 112)
	UI.make("TextLabel",{
  Name="NotificationMessage",BackgroundTransparency=1,Position=UDim2.fromOffset(68,43),
  Size=UDim2.fromOffset(TOAST_W-90,toastH-53),Text=message or "",TextWrapped=true,
  TextXAlignment=Enum.TextXAlignment.Left,TextYAlignment=Enum.TextYAlignment.Top,
  TextSize=14,TextColor3=C.sub,FontFace=UI.font(),Parent=body,
 })
	local closeIc = UI.icon(body, "close", TOAST_W - 22, 20, 12, C.muted, 1.8)

	local entry = { slot = slot, wrap = wrap, dx = dx }
	table.insert(liveToasts, entry)
	local dur = SETTING.notifications.duration.value
	if SETTING.notifications.progress.value then
		local bar = UI.make("Frame", {
			BackgroundColor3 = C.white,
			BorderSizePixel = 0,
			Position = UDim2.new(0, 16, 1, -3),
			Size = UDim2.new(1, -32, 0, 2),
			Parent = body,
		}, { UI.round(), UI.accentGradient(0) })
		-- real time, not scaled by the motion settings
		TweenService:Create(bar, TweenInfo.new(dur, LINEAR), { Size = UDim2.new(0, 0, 0, 2) }):Play()
	end
	body.MouseEnter:Connect(function()
		UI.tintIcon(closeIc, C.white, 0.15)
	end)
	body.MouseLeave:Connect(function()
		UI.tintIcon(closeIc, C.muted, 0.2)
	end)
	body.MouseButton1Click:Connect(function()
		UI.dismissToast(entry)
	end)

	wrap.Position = UDim2.fromOffset(dx, dx == 0 and 20 or 0)
	UI.groupFade(wrap, 0, 0.3)
	UI.tween(wrap, 0.55, { Position = UDim2.fromOffset(0, 0) }, BACK)
	UI.after(dur, function()
		UI.dismissToast(entry)
	end)
end

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

-------------------------------------------------------------------- cards + sections
SETTING_BUILDERS.combo=function(panel,s,x,y,ctrl)
 local holder=UI.make("Frame",{BackgroundTransparency=1,Position=UDim2.fromOffset(32,y-155),Size=UDim2.new(1,-64,0,310),Parent=panel})
 local steps,selected,lastSource={},1,nil
 local chipLinks,keyButtons={},{}
 local hint=text(holder,"1. ADD MOVES",0,10,15,{color=C.sub})
 local selectionLabel=text(holder,"2. CLICK A STEP TO EDIT",0,74,15,{color=C.sub})
 local tray=UI.make("Frame",{BackgroundTransparency=1,Position=UDim2.fromOffset(0,88),Size=UDim2.new(1,0,0,96),Parent=holder})
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
 for i,keys in ipairs({"Z","X","C","V","WAIT"})do
  button(holder,keys=="WAIT"and "+ Pause"or "+ "..keys,UDim2.new((i-1)/5,0,0,28),UDim2.new(1/5,-6,0,32),function()
   if #steps>=12 then hint.Text="Sequence full · remove a step first";return end
   table.insert(steps,{keys=keys,delay=delaySetting.value});selected=#steps;commit()
  end)
 end
 text(holder,"Combine keys:",0,206,17,{color=C.text2})
 for i,key in ipairs({"Z","X","C","V"})do
  keyButtons[key]=button(holder,key,UDim2.fromOffset(150+(i-1)*57,190),UDim2.fromOffset(49,32),function()
   local step=steps[selected]
   if not step or step.keys=="WAIT"then selectionLabel.Text="Select a move step first";return end
   if step.keys:find(key,1,true)then
    local remaining=step.keys:gsub(key,"")
    if #remaining==0 then selectionLabel.Text="Keep one key, or Remove the step";return end
    step.keys=remaining
   elseif #step.keys<4 then step.keys=step.keys..key end
   commit()
  end)
 end
 button(holder,"Remove step",UDim2.new(1,-148,0,190),UDim2.fromOffset(148,32),function()if steps[selected]then table.remove(steps,selected);commit()end end)
 local actions={
  {"Double tap",function()
   local step=steps[selected]
   if not step or step.keys=="WAIT"then selectionLabel.Text="Select a move to double tap";return end
   if #steps>=12 then selectionLabel.Text="Remove a step to make room for the second tap";return end
   local after=step.delay;step.delay=.10
   table.insert(steps,selected+1,{keys=step.keys,delay=after});selected=selected+1;commit()
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
  hint.Text="1. ADD MOVES · "..#steps.." / 12"
  selectionLabel.Text=steps[selected]and "2. EDIT STEP "..selected.." · choose keys to combine below"or "Click + Z, + X, + C or + V to start"
  if #steps==0 then text(tray,"Or choose a preset above and press Play combo.",0,30,18,{color=C.sub})end
  for i,step in ipairs(steps)do
   local label=step.keys=="WAIT"and "Pause"or step.keys:gsub(".","%0+"):sub(1,-2)
   local b=UI.make("TextButton",{Text=i..". "..label.."\n"..string.format("%.2f s",step.delay),AutoButtonColor=false,TextColor3=C.text,BackgroundColor3=i==selected and A.dim or C.hover,FontFace=UI.font(),TextSize=16,Position=UDim2.new(((i-1)%6)/6,0,0,math.floor((i-1)/6)*47),Size=UDim2.new(1/6,-6,0,41),Parent=tray},{UI.corner(7),UI.stroke(i==selected and A.a or C.field,1)})
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

-------------------------------------------------------------------- settings → interface
local borderOn = false
function UI.setBorder(on)
	borderOn = on
	borderGrad.Enabled = on
	winStroke.Color = on and C.white or C.stroke
	winStroke.Thickness = on and 1.5 or 1.2
end

function UI.setAmbient(on)
	if on == ambientOn then
		return
	end
	ambientOn = on
	UI.tween(ambient, 0.5, { GroupTransparency = on and 0 or 1 })
end

function UI.setTheme(name, t)
	UI.deriveTheme(THEME_BY_NAME[name] or THEMES[1])
	UI.applyThemeRegistry(t or 0.45)
	repaintAll(0.45)
	refreshNav(0.3)
	if refreshMini then
		refreshMini()
	end
end

applySettings = function(t)
	t = t or 0.35
	local motion = CTRL.motion
	local m = motion.enabled == true
	MOTION.factor = m and SETTING.motion.speed.value / 0.3 or 1
	MOTION.style = m and EASINGS[SETTING.motion.curve.value] or QUINT
	MOTION.reduce = m and SETTING.motion.reduce.value or false

	local ap = CTRL.appearance.enabled == true
	local themeName = ap and SETTING.appearance.theme.value or THEMES[1].name
	if themeName ~= A.name then
		UI.setTheme(themeName, t)
	end
	local alpha = 1 - (ap and SETTING.appearance.opacity.value / 100 or 1)
	UI.tween(win, t, { BackgroundTransparency = alpha })
	UI.tween(rail, t, { BackgroundTransparency = alpha * 0.6 })
	for _, f in ipairs(railFillers) do
		UI.tween(f, t, { BackgroundTransparency = alpha * 0.6 })
	end
	UI.setBorder(ap and SETTING.appearance.border.value or false)
	UI.setAmbient(ap and SETTING.appearance.ambient.value or false)

	local lay = CTRL.layout.enabled == true
	UI.tween(listLayout, t, { Padding = UDim.new(0, lay and SETTING.layout.spacing.value or 14) })
	local density = lay and DENSITY[SETTING.layout.density.value] or 1
	local captions = not lay or SETTING.layout.labels.value
	local dividers = not lay or SETTING.layout.dividers.value
	local relayout = density ~= layoutState.density or captions ~= layoutState.captions or dividers ~= layoutState.dividers
	layoutState.density, layoutState.captions, layoutState.dividers = density, captions, dividers
	if relayout then
		rerender(0.32) -- let the clicked control finish its own animation first
	end
end

function UI.describe(ctrlId, id, value)
	local ctrl = CTRL[ctrlId]
	if not ctrl then
		return nil
	end
	if id == "enabled" then
		return ctrl.name .. (value and " on" or " off")
	end
	local s = SETTING[ctrlId][id]
	if not s then
		return nil
	end
	local v
	if typeof(value) == "number" then
		v = string.format(s.format or "%.1f", value) .. (s.suffix or "")
	elseif typeof(value) == "boolean" then
		v = value and "on" or "off"
	elseif typeof(value) == "EnumItem" then
		v = value.Name
	else
		v = tostring(value)
	end
	return (s.label or id) .. "  ·  " .. v
end

local fit, centerWindow -- forward (window placement)

local ACTIONS = {
	preview = function()
		toast("Notifications", "This is how a notification looks.", "bell", true)
	end,
	resetLayout = function()
		local ctrl = CTRL.layout
		UI.forEachValue(ctrl, function(s)
			s.value = s.default
		end)
		applySettings()
		repaintAll()
		setStatus("Layout reset")
		toast("Layout reset", "Spacing and density are back to default", "refresh")
	end,
	refresh = function()
		renderPage(currentPage)
		setStatus("Page refreshed")
	end,
	burst = function()
		local msgs = {
			{ "Heads up", "Toasts stack from the corner you pick", "bell" },
			{ "Nice", "Click any toast to dismiss it early", "check" },
			{ "One more", "Turn off stacking to keep just one", "layers" },
		}
		for i, m in ipairs(msgs) do
			UI.after((i - 1) * 0.22, function()
				toast(m[1], m[2], m[3], true)
			end)
		end
	end,
	center = function()
		centerWindow()
		setStatus("Window centered")
	end,
}

do (function()
 local path="PracticalUI2Profiles-v1.json"
 local http=game:GetService("HttpService")
 local function encode(v)
  if typeof(v)=="EnumItem"then return {keyCode=v.Name,mouse=v.EnumType==Enum.UserInputType}end
  if type(v)=="table"then local r={}for k,x in pairs(v)do r[k]=encode(x)end return r end
  return v
 end
 local function validated(raw)
  if type(raw)~="table"then return nil end
  local result=UI.snapshot()
  UI.forEachControl(function(ctrl)
   local e=raw[ctrl.id];if type(e)~="table"then return end
   if type(e.enabled)=="boolean"then result[ctrl.id].enabled=e.enabled end
   if type(e.values)~="table"then return end
   UI.forEachValue(ctrl,function(s)
    local v=e.values[s.id]
    if typeof(s.value)=="EnumItem"then
     if type(v)=="table"and type(v.keyCode)=="string"then
      local ok,key=pcall(function()return (v.mouse and Enum.UserInputType or Enum.KeyCode)[v.keyCode]end)
      local mouse=ok and key and(key==Enum.UserInputType.MouseButton1 or key==Enum.UserInputType.MouseButton2 or key==Enum.UserInputType.MouseButton3)
      if ok and key and key~=Enum.KeyCode.Unknown and(not v.mouse or mouse)then result[ctrl.id].values[s.id]=key end
     end
    elseif type(s.value)=="number"then
     if type(v)=="number"and v==v and math.abs(v)<math.huge then result[ctrl.id].values[s.id]=math.clamp(v,s.min or -math.huge,s.max or math.huge)end
    elseif type(v)==type(s.value)and (not s.options or table.find(s.options,v))and (type(v)~="string"or #v<=2048)then result[ctrl.id].values[s.id]=v end
   end)
  end,true)
  return result
 end
 UI.persistProfiles=function()
  if type(writefile)~="function"then setStatus("File saving unavailable in this environment");return false end
  local ok,err=pcall(function()writefile(path,http:JSONEncode({version=1,uiSizingVersion=3,selected=currentProfile.name,profiles=encode(PROFILES)}))end)
  if not ok then setStatus("Config could not be written");warn("Practical config save failed: "..tostring(err))end
  return ok
 end
 UI.loadSavedProfiles=function()
  if type(readfile)~="function"then return false end
  local ok,raw=pcall(function()return http:JSONDecode(readfile(path))end)
  if not ok or type(raw)~="table"or raw.version~=1 or type(raw.profiles)~="table"then return false end
  local profiles,seen={},{}
  for _,e in ipairs(raw.profiles)do
   if #profiles>=100 then break end
   if type(e)=="table"and type(e.name)=="string"and #e.name>0 and #e.name<=80 and not seen[string.lower(e.name)]then
    local snap=validated(e.snap)
    if snap and raw.uiSizingVersion~=3 and snap.interface and snap.interface.values then
     local scale=snap.interface.values.uiScale
     if type(scale)=="number"and scale<1 then snap.interface.values.uiScale=1 end
    end
    if snap then table.insert(profiles,{name=e.name,snap=snap,savedAt=type(e.savedAt)=="number"and e.savedAt or os.time()});seen[string.lower(e.name)]=true end
   end
  end
  if #profiles==0 then return false end
  PROFILES=profiles;currentProfile=UI.findProfile(type(raw.selected)=="string"and raw.selected or "")or profiles[1]
  return true
 end
 UI.loadSavedProfiles()
end)()end

-------------------------------------------------------------------- header profile picker + footer actions
profileDD = dropdown(win, UDim2.new(1, -32 - 230, 0, TITLE_H + 64), 230, 44, UI.profileNames, currentProfile.name, function(name)
	local p = UI.findProfile(name)
	if p then
		UI.loadProfile(p)
		if currentPage == "Configs" and renderedQuery == "" then
			renderPage("Configs", false)
		end
	end
end, {
	label = "New profile…",
	icon = "plus",
	onClick = function()
		openPage("Configs")
		UI.after(0.1, function()
			if configNameBox then
				configNameBox:CaptureFocus()
			end
		end)
	end,
})

local saveToken = 0
saveBtn.MouseButton1Click:Connect(function()
	currentProfile.snap = UI.snapshot()
	currentProfile.savedAt = os.time()
	controlChanged:Fire("profile", "saved", currentProfile.name)
	setStatus('"' .. currentProfile.name .. '" saved')
	toast("Profile saved", '"' .. currentProfile.name .. '" is up to date', "save")
	saveToken += 1
	local mine = saveToken
	saveLabel.Text = "Saved"
	UI.after(1.4, function()
		if saveToken == mine then
			saveLabel.TextTransparency = 1
			saveLabel.Text = "Save profile"
			UI.tween(saveLabel, 0.3, { TextTransparency = 0 })
		end
	end)
	if currentPage == "Configs" and renderedQuery == "" then
		renderPage("Configs", false)
	end
end)

local revertAngle = 0
revertBtn.MouseButton1Click:Connect(function()
	revertAngle += 360
	UI.spinIcon(revertIcon, "refresh", C.white, 1.8, revertAngle, 0.7)
	UI.after(0.75, function()
		UI.tintIcon(revertIcon, C.text2, 0.2)
	end)
	UI.restore(currentProfile.snap)
	controlChanged:Fire("profile", "reverted", currentProfile.name)
	setStatus("Reverted to saved")
end)

resetBtn.MouseButton1Click:Connect(function()
	UI.forEachControl(function(ctrl)
		ctrl.enabled = ctrl.defaultEnabled
		UI.forEachValue(ctrl, function(s)
			s.value = s.default
		end)
	end)
	applySettings()
	repaintAll()
	refreshNav()
	setStatus("Defaults restored")
	toast("Defaults restored", "Every feature is back to its starting value", "refresh")
	controlChanged:Fire("profile", "reset", currentProfile.name)
end)

testBtn.MouseButton1Click:Connect(function()
	testControls:Fire()
end)

-------------------------------------------------------------------- minimized bar + hidden notice
local MINI_W, MINI_H, MM = 460, 60, 16
local mini = UI.make("Frame", {
	Name = "Mini",
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(0.5, 0),
	Position = UDim2.new(0.5, 0, 0, 8),
	Size = UDim2.fromOffset(MINI_W + MM * 2, MINI_H + MM * 2),
	Visible = false,
	Parent = gui,
})
local miniScale = UI.make("UIScale", { Scale = DEFAULT_SCALE, Parent = mini })
local miniShade = UI.make("Frame", {
	BackgroundTransparency = 1,
	Position = UDim2.fromOffset(MM, MM),
	Size = UDim2.fromOffset(MINI_W, MINI_H),
	Parent = mini,
})
UI.softShadow(miniShade, MINI_H / 2, 4, 3, 0.08, 4)
local miniBody = UI.make("TextButton", {
	Text = "",
	AutoButtonColor = false,
	BackgroundColor3 = C.bg,
	BackgroundTransparency = 0.03,
	Position = UDim2.fromOffset(MM, MM),
	Size = UDim2.fromOffset(MINI_W, MINI_H),
	ZIndex = 2,
	Parent = mini,
}, { UI.round(), UI.bindTheme(UI.stroke(A.soft, 1), "Color", "soft") })
UI.logoMark(miniBody, 32, MINI_H / 2, 30)
text(miniBody, "practical", 58, MINI_H / 2, 18, { weight = SEMIBOLD })
UI.strip(miniBody, UDim2.fromOffset(152, 18), UDim2.fromOffset(1, 24), C.stroke)
local miniDot = UI.make("Frame", {
	BackgroundColor3 = A.a,
	BorderSizePixel = 0,
	AnchorPoint = Vector2.new(0.5, 0.5),
	Position = UDim2.fromOffset(172, MINI_H / 2),
	Size = UDim2.fromOffset(9, 9),
	Parent = miniBody,
}, { UI.round() })
local miniHalo = UI.pulse(miniDot, A.a)
local miniStatus = UI.clip(text(miniBody, "", 186, MINI_H / 2, 15, { color = C.text2, weight = MEDIUM }), MINI_W - 186 - 92)
local miniRestore = UI.iconButton(miniBody, UDim2.new(1, -70, 0.5, 0), 34, "restore", { radius = 17 })
local miniClose = UI.iconButton(miniBody, UDim2.new(1, -32, 0.5, 0), 34, "close", { radius = 17, hoverColor = C.dangerFill })

refreshMini = function(override)
	local on = UI.featureCounts()
	miniStatus.Text = override or string.format("%s  ·  %d on", currentProfile and currentProfile.name or "", on)
	local color = on > 0 and A.a or C.muted
	miniDot.BackgroundColor3 = color
	miniHalo.BackgroundColor3 = color
end

-- notice shown after closing the window, so the open key isn't forgotten
local notice = UI.make("Frame", {
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(0.5, 0),
	Position = UDim2.new(0.5, 0, 0, 8),
	Size = UDim2.fromOffset(400, 48 + MM * 2),
	Visible = false,
	Parent = gui,
})
local noticeScale = UI.make("UIScale", { Scale = DEFAULT_SCALE, Parent = notice })
local noticeShade = UI.make("Frame", {
	BackgroundTransparency = 1,
	Position = UDim2.fromOffset(MM, MM),
	Size = UDim2.new(1, -MM * 2, 1, -MM * 2),
	Parent = notice,
})
UI.softShadow(noticeShade, 24, 4, 3, 0.08, 4)
local noticeBody = UI.make("Frame", {
	BackgroundColor3 = C.bg,
	BackgroundTransparency = 0.03,
	Position = UDim2.fromOffset(MM, MM),
	Size = UDim2.new(1, -MM * 2, 1, -MM * 2),
	ZIndex = 2,
	Parent = notice,
}, { UI.round(), UI.stroke(C.stroke, 1) })
local noticeLabel = text(noticeBody, "", 20, 24, 16, { color = C.text2, weight = MEDIUM })
local noticeKey = UI.make("Frame", {
	BackgroundColor3 = C.raised,
	AnchorPoint = Vector2.new(0, 0.5),
	Position = UDim2.fromOffset(0, 24),
	Size = UDim2.fromOffset(60, 28),
	Parent = noticeBody,
}, { UI.corner(7), UI.stroke(C.strokeHi, 1) })
local noticeKeyText = UI.make("TextLabel", {
	BackgroundTransparency = 1,
	Size = UDim2.fromScale(1, 1),
	Text = "",
	FontFace = UI.font(SEMIBOLD),
	TextSize = 14,
	TextColor3 = C.text,
	Parent = noticeKey,
})
local noticeToken = 0
function UI.showNotice()
	noticeToken += 1
	local mine = noticeToken
	local lead = "practical is hidden  ·  press"
	local keyName = PREF_KEY.value.Name
	noticeLabel.Text = lead
	noticeKeyText.Text = keyName
	local lw = UI.textWidth(lead, 16)
	local kw = UI.textWidth(keyName, 14) + 22
	noticeKey.Size = UDim2.fromOffset(kw, 28)
	noticeKey.Position = UDim2.fromOffset(20 + lw + 10, 24)
	notice.Size = UDim2.fromOffset(20 + lw + 10 + kw + 14 + MM * 2, 48 + MM * 2)
	notice.Visible = true
	notice.Position = UDim2.new(0.5, 0, 0, -4)
	UI.groupFade(notice, 0, 0.35)
	UI.tween(notice, 0.5, { Position = UDim2.new(0.5, 0, 0, 8) }, BACK)
	UI.after(3.4, function()
		if mine == noticeToken then
			UI.groupFade(notice, 1, 0.4).Completed:Connect(function(state)
				if state == Enum.PlaybackState.Completed and mine == noticeToken then
					notice.Visible = false
				end
			end)
		end
	end)
end
function UI.hideNotice()
	noticeToken += 1
	if notice.Visible then
		UI.groupFade(notice, 1, 0.15).Completed:Connect(function()
			notice.Visible = false
		end)
	end
end

-------------------------------------------------------------------- open / minimize / close
local mode = "closed" -- "open" | "mini" | "closed"

function UI.showWindow()
	root.Visible = true
	popScale.Scale = 0.94
	UI.groupFade(group, 0, 0.3)
	UI.tween(popScale, 0.55, { Scale = 1 }, BACK)
end

function UI.hideWindow()
	closePopup()
	UI.tween(popScale, 0.22, { Scale = 0.95 }, QUINT, IN)
	UI.groupFade(group, 1, 0.22, 0, QUINT, IN).Completed:Connect(function(state)
		if state == Enum.PlaybackState.Completed and mode ~= "open" then
			root.Visible = false
		end
	end)
end

function UI.showMini()
	refreshMini()
	mini.Visible = true
	mini.Position = UDim2.new(0.5, 0, 0, -14)
	UI.groupFade(mini, 0, 0.35, 0.1)
	UI.tween(mini, 0.55, { Position = UDim2.new(0.5, 0, 0, 8) }, BACK, OUT, 0.1)
end

function UI.hideMini()
	if not mini.Visible then
		return
	end
	UI.tween(mini, 0.2, { Position = UDim2.new(0.5, 0, 0, -4) })
	UI.groupFade(mini, 1, 0.2).Completed:Connect(function(state)
		if state == Enum.PlaybackState.Completed and mode ~= "mini" then
			mini.Visible = false
		end
	end)
end

setMode = function(m)
	if m == mode then
		return
	end
	local prev = mode
	mode = m
	if m == "open" then
		UI.hideMini()
		UI.hideNotice()
		UI.showWindow()
	elseif m == "mini" then
		UI.hideNotice()
		UI.hideWindow()
		UI.showMini()
	else -- closed
		if prev == "open" then
			UI.hideWindow()
		else
			UI.hideMini()
		end
		if PREF_NOTICE.value then
			UI.showNotice()
		end
	end
end

minBtn.MouseButton1Click:Connect(function()
	setMode("mini")
end)
closeBtn.MouseButton1Click:Connect(function()
	setMode("closed")
end)
miniBody.MouseButton1Click:Connect(function()
	setMode("open")
end)
miniRestore.MouseButton1Click:Connect(function()
	setMode("open")
end)
miniClose.MouseButton1Click:Connect(function()
	setMode("closed")
end)

-------------------------------------------------------------------- scale, position, drag, resize
local dragTarget -- top-left of `root` on screen (offsets)
local centered = false

centerWindow = function()
	local s = gui.AbsoluteSize
	local sc = fitScale.Scale
	dragTarget = UDim2.fromOffset(
		math.floor((s.X - (targetW + PAD * 2) * sc) / 2),
		math.floor((s.Y - (targetH + PAD * 2) * sc) / 2)
	)
end

fit = function(t)
	local s = gui.AbsoluteSize
	if s.X == 0 then
		return
	end
	local pref = PREF_SCALE.value
 local availableW=math.max(MIN_W,s.X*.96/pref-PAD*2)
 local availableH=math.max(MIN_H,s.Y*.96/pref-PAD*2)
 targetW=math.min(targetW,availableW);targetH=math.min(targetH,availableH)
 -- Apply immediately when fitting, rather than temporarily scaling the old rectangle.
 winW=targetW;winH=targetH;UI.applySize()
	local sc = math.min(pref, s.X * 0.98 / (winW + PAD * 2), s.Y * 0.98 / (winH + PAD * 2))
	UI.tween(fitScale, t or 0, { Scale = sc })
	UI.tween(miniScale, t or 0, { Scale = pref })
	noticeScale.Scale = pref
	toastScale.Scale = pref
	if not centered then
		centered = true
		fitScale.Scale = sc
		centerWindow()
		root.Position = dragTarget
	end
end
gui:GetPropertyChangedSignal("AbsoluteSize"):Connect(function()
	fit(0)
end)
PREF_SCALE.onSet = function()
	fit(0.2)
end

local dragStart, dragOrigin
local resizing -- { start, w, h }
local lastTitleClick = 0
local restoreRect -- size/position before maximise

function UI.toggleMaximize()
	local s = gui.AbsoluteSize
	local sc = fitScale.Scale
	if restoreRect then
		targetW, targetH = restoreRect.w, restoreRect.h
		dragTarget = restoreRect.pos
		restoreRect = nil
		setStatus("Window restored")
	else
		restoreRect = { w = targetW, h = targetH, pos = dragTarget }
		targetW = math.max(MIN_W, (s.X - 24) / sc)
		targetH = math.max(MIN_H, (s.Y - 24) / sc)
		dragTarget = UDim2.fromOffset(12 - PAD * sc, 12 - PAD * sc)
		setStatus("Window maximized  ·  double-click the title bar to restore")
	end
end

titleBar.InputBegan:Connect(function(io)
	if io.UserInputType == Enum.UserInputType.MouseButton1 or io.UserInputType == Enum.UserInputType.Touch then
		local now = os.clock()
		if now - lastTitleClick < 0.32 then
			lastTitleClick = 0
			dragStart = nil
			UI.toggleMaximize()
			return
		end
		lastTitleClick = now
		dragStart = io.Position
		dragOrigin = dragTarget
	end
end)
grip.InputBegan:Connect(function(io)
	if io.UserInputType == Enum.UserInputType.MouseButton1 or io.UserInputType == Enum.UserInputType.Touch then
		closePopup()
		restoreRect = nil
		resizing = { start = io.Position, w = targetW, h = targetH }
	end
end)
UI.connect(UserInputService.InputChanged, function(io)
	if io.UserInputType ~= Enum.UserInputType.MouseMovement and io.UserInputType ~= Enum.UserInputType.Touch then
		return
	end
	if dragStart and dragOrigin then
		local d = io.Position - dragStart
		if restoreRect and d.Magnitude > 6 then
			restoreRect = nil -- dragging a maximised window un-maximises it in place
		end
		local s = gui.AbsoluteSize
		local sc = fitScale.Scale
		local x = math.clamp(dragOrigin.X.Offset + d.X, -(winW + PAD) * sc + 120, s.X - (PAD * sc) - 120)
		local y = math.clamp(dragOrigin.Y.Offset + d.Y, -PAD * sc, s.Y - (PAD + TITLE_H) * sc)
		dragTarget = UDim2.fromOffset(x, y)
	elseif resizing then
		local d = io.Position - resizing.start
		local sc = fitScale.Scale
		local s = gui.AbsoluteSize
		local maxW = (s.X - root.AbsolutePosition.X) / sc - PAD - 4
		local maxH = (s.Y - root.AbsolutePosition.Y) / sc - PAD - 4
		targetW = math.clamp(resizing.w + d.X / sc, MIN_W, math.max(MIN_W, maxW))
		targetH = math.clamp(resizing.h + d.Y / sc, MIN_H, math.max(MIN_H, maxH))
	end
end)

-- per-frame: window follow, animated border, drifting ambient light
UI.connect(RunService.RenderStepped, function(dt)
	local a = 1 - math.exp(-dt * 22)
	if dragTarget then
		local p = root.Position
		if math.abs(p.X.Offset - dragTarget.X.Offset) + math.abs(p.Y.Offset - dragTarget.Y.Offset) > 0.05 then
			root.Position = p:Lerp(dragTarget, a)
		end
	end
	if math.abs(winW - targetW) + math.abs(winH - targetH) > 0.1 then
		winW += (targetW - winW) * a
		winH += (targetH - winH) * a
		UI.applySize()
	end
	if mode == "open" and not MOTION.reduce then
		if borderOn then
			borderGrad.Rotation = (borderGrad.Rotation + dt * 32) % 360
		end
		if ambientOn then
			local t = os.clock()
			for _, b in ipairs(blobs) do
				b.frame.Position = UDim2.fromScale(
					b.x + math.sin(t * 0.13 + b.phase) * 0.07,
					b.y + math.cos(t * 0.1 + b.phase) * 0.09
				)
			end
		end
	end
end)

-------------------------------------------------------------------- keyboard
UI.connect(UserInputService.InputBegan, function(io, gameProcessed)
	local mouse=io.UserInputType==Enum.UserInputType.MouseButton1 or io.UserInputType==Enum.UserInputType.MouseButton2 or io.UserInputType==Enum.UserInputType.MouseButton3
	if io.UserInputType ~= Enum.UserInputType.Keyboard and not mouse then return end
	local binding=mouse and io.UserInputType or io.KeyCode
	if rebinding then
		local r = rebinding
		rebinding = nil
		if io.KeyCode ~= Enum.KeyCode.Escape then
			r.setting.value = binding
			controlChanged:Fire(r.ctrl.id, r.setting.id, binding)
		end
		r.stop()
		return
	end

	if binding == PREF_KEY.value and not gameProcessed then
		setMode(mode == "open" and "closed" or "open")
		return
	end

	local ctrlHeld = UserInputService:IsKeyDown(Enum.KeyCode.LeftControl) or UserInputService:IsKeyDown(Enum.KeyCode.RightControl)
	if io.KeyCode == Enum.KeyCode.K and ctrlHeld and mode == "open" then
		task.defer(function()
			searchBox:CaptureFocus()
		end)
		return
	end
	if io.KeyCode == Enum.KeyCode.Escape and mode == "open" then
		closePopup()
	end

	if gameProcessed then
		return
	end
	if CTRL.motion.enabled and binding == SETTING.motion.shortcut.value then
		if playMotionPreview then
			playMotionPreview()
		end
		popScale.Scale = 0.985
		UI.tween(popScale, 0.5, { Scale = 1 }, BACK)
		setStatus("Preview shortcut  ·  " .. io.KeyCode.Name)
	end
end)

UI.connect(UserInputService.InputEnded, function(io)
	if io.UserInputType == Enum.UserInputType.MouseButton1 or io.UserInputType == Enum.UserInputType.Touch then
		dragStart = nil
		resizing = nil
	end
end)

gui.Destroying:Connect(function()
	alive = false
	for _, c in ipairs(connections) do
		c:Disconnect()
	end
	for _, c in ipairs(rowConns) do
		c:Disconnect()
	end
end)

do (function()
-- Connect the supplied interface to the existing feature engine.
local booleanMap={visualStyle={enabled="visualPolish",labels="visualLabels",motion="visualMotion",urgency="visualUrgency",beacon="visualBeacon",flightShadow="visualFlightShadow",comet="visualComet"},antiOop={enabled="antiOop",move="oopMove",guide="oopGuide"},passVision={enabled="passVision"},dunkStopper={enabled="dunkStopper"},autoJoin={enabled="autoJoin"},passIntercept={enabled="passIntercept",automatic="passInterceptAuto",move="passInterceptMove",jump="passInterceptJump",shots="passInterceptShots"},contestCeiling={enabled="contestCeiling",release="contestRelease"},recovery={enabled="ballRecovery"},screens={enabled="screenNavigation",visual="screenAwareness"},assignmentTools={lock="assignmentLock"},passingLanes={enabled="passingLanes"},comboBuilder={enabled="comboBuilder",sprint="comboSprint"},autoGuard={cutoff="driveCutoff",screens="screenNavigation",coverage="guardCoverage",enabled="guard",faceHandler="faceTarget",lockAssignment="assignmentLock",showIndicators="visuals",fakeFilter="fakeFilter"},
 autoSteal={enabled="steal"},autoBlock={commit="blockCommitFilter",enabled="block",oneAttempt="blockOnce"},
 rebound={enabled="rebound",chase="reboundChase"},boost={enabled="boost"},
 landingTools={trail="ballTrail",enabled="landing",beam="landingBeam",zone="landingZone"},overlayTools={contest="liveContest",guard="visuals"},
 autoRelease={liveContest="liveContest",contest="contest"},shotAssist={enabled="shotAssist",holdEarly="shotHoldEarly",protectFall="shotProtectFall"}}
local numberMap={visualStyle={scale="visualScale",intensity="visualIntensity",weight="visualWeight"},antiOop={range="oopRange"},passVision={speed="passSpeed",release="catchRelease"},dunkStopper={range="dunkRange",timing="dunkTimingMs"},passIntercept={reach="passInterceptReach"},contestCeiling={limit="contestLimit"},recovery={range="recoveryRange",window="recoveryWindow"},screens={range="screenRange"},passingLanes={range="laneRange",clearance="laneClearance"},comboBuilder={interval="comboInterval",repeats="comboRepeats"},autoGuard={standOff="guardGap",delay="guardReaction"},autoSteal={reach="stealRange",cooldown="stealCooldown",delay="stealDelay"},
 autoRelease={offset="timingOffsetMs",maxPing="timingMaxPingMs",freshness="timingFreshMs"},shotAssist={meterMin="shotMeterMin",meterMax="shotMeterMax"},
 rebound={range="reboundChaseRange",reach="reboundRange",height="reboundHeight",lead="reboundLead"},
 landingTools={width="landingBeamWidth",scale="landingZoneScale",range="landingRange"},
 autoBlock={reactionDelay="blockDelay"},boost={multiplier="boostMultiplier",tail="boostDuration"},}
booleanMap.visualLayers={landing="visualShowLanding",flight="visualShowFlight",route="visualShowRoute",coverage="visualShowCoverage",pass="visualShowPass",recovery="visualShowRecovery",screen="visualShowScreen",intercept="visualShowIntercept"}
booleanMap.visualLabels={passPanel="visualPassPanel"}
booleanMap.defenderAlerts={enabled="defenderAlerts",ballOnly="alertBallOnly"}
numberMap.defenderAlerts={range="alertRange"}
booleanMap.rebound.early="reboundEarly";numberMap.rebound.spot="reboundSpot"
booleanMap.autoGuard.humanize="guardHuman";booleanMap.autoGuard.rimPress="rimPress"
numberMap.autoGuard.humanLevel="guardHumanLevel";numberMap.autoGuard.rimRange="rimPressRange";numberMap.autoGuard.rimGap="rimPressGap"
booleanMap.autofarm={crossover="farmCrossover",shoot="farmShoot",randomThrees="farmRandomThrees"};numberMap.autofarm={dunkRange="farmDunkRange",shotRange="farmShotRange"}
booleanMap.visualColor={enabled="visualCustomColor"}
booleanMap.denyReceiver={enabled="denyReceiver",catch="denyCatch"}
booleanMap.offBallResponse={enabled="denySoft"}
numberMap.offBallResponse={delay="denyDelay",smoothing="denySmoothing",deadzone="denyDeadzone",lead="denyLead"}
booleanMap.smartDribble={enabled="smartDribble",follow="dribbleTestFollow",shoot="smartDribbleShoot",steer="smartDribbleSteer"}
booleanMap.smartPass={enabled="smartPass"}
booleanMap.catchShoot={enabled="catchShoot"}
numberMap.catchShoot={contest="catchShootContest"}
booleanMap.ballProtection={enabled="ballProtection"}
numberMap.smartDribble={gap="smartDribbleGap",stamina="smartDribbleStamina",cooldown="smartDribbleCooldown",testSpeed="dribbleTestSpeed",shotContest="smartDribbleShotContest",shotRange="smartDribbleShotRange",arc="smartDribbleThreeDistance"}
numberMap.ballProtection={range="ballProtectionRange"}
numberMap.visualStyle.smoothing="visualSmoothing";numberMap.visualStyle.clearance="visualClearance"
numberMap.visualLabels={size="visualLabelSize",opacity="visualLabelOpacity"}
numberMap.visualColor={red="visualRed",green="visualGreen",blue="visualBlue"}
numberMap.denyReceiver={range="denyRange",gap="denyGap",switch="denySwitchMargin",delay="denyDelay",smoothing="denySmoothing",deadzone="denyDeadzone"}
local firstCourtCorner
local function applyResponse(name)
 local values=name=="Legit" and {guardReaction=.24,guardPrediction=.05,guardStrength=.4,guardDeadzone=.9,sprintDistance=4,closeReaction=.15,closeGap=2,stealAngle=35,stealMaxSpeed=8,stealDelay=.2,stealCooldown=2,blockDelay=.12,jumpCooldown=1.6,reboundLead=.1}
  or name=="Blatant" and {guardReaction=0,guardPrediction=.12,guardStrength=1,guardDeadzone=.35,sprintDistance=1,closeReaction=0,closeGap=1,stealAngle=65,stealMaxSpeed=20,stealDelay=.05,stealCooldown=.8,blockDelay=.05,jumpCooldown=.8,reboundLead=.2}
  or {guardReaction=.16,guardPrediction=.08,guardStrength=1,guardDeadzone=.65,sprintDistance=2.5,closeReaction=.08,closeGap=1.5,stealAngle=45,stealMaxSpeed=10,stealDelay=.12,stealCooldown=1.6,blockDelay=.08,jumpCooldown=1.2,reboundLead=.15}
 for key,value in pairs(values) do api.SetNumber(key,value) end
end
local screenVisualChoice=true
local function routeControl(id,field,value)
 if id=="screens"and field=="enabled"then
  api.SetOption("screenNavigation",value)
  return api.SetOption("screenAwareness",value and screenVisualChoice)
 end
 if id=="screens"and field=="visual"then
  screenVisualChoice=value
  return api.SetOption("screenAwareness",value and api.GetOption("screenNavigation"))
 end
 if id=="smartDribble"and field=="activation"then return api.SetSmartDribbleKey(value)end
 if id=="smartDribble"and field=="plan"then
  return api.SetNumber("smartDribblePlan",value=="Attack basket"and 2 or value=="Find a three"and 3 or 1)
 end
 if id=="visualStyle"and field=="route"then
  return api.SetNumber("visualRouteStyle",({Dashed=1,Solid=2,Chevrons=3})[value]or 1)
 end
 if id=="visualStyle"and field=="palette"then
  return api.SetNumber("visualPalette",({Cobalt=1,Glacier=2,Mint=3,Amber=4})[value]or 1)
 end
 if id=="visualStyle"and field=="preset"then
  local preset=value=="Minimal"and {intensity=35,weight=.95,scale=.9,smoothing=70,clearance=.12,motion=false}
   or value=="Vivid"and {intensity=85,weight=1.4,scale=1.1,smoothing=45,clearance=.16,motion=true}
   or {intensity=65,weight=1.15,scale=1,smoothing=55,clearance=.12,motion=false}
  for key,settingValue in pairs(preset)do
   if key=="motion"then api.SetOption("visualMotion",settingValue)else api.SetNumber(numberMap.visualStyle[key],settingValue)end
  end
  UI.forEachControl(function(ctrl)if ctrl.id==id then UI.forEachValue(ctrl,function(setting)if preset[setting.id]~=nil then setting.value=preset[setting.id]end end)end end)
  repaintAll();return true
 end
 if id=="visualLabels" and field=="corner" then return api.SetHudCorner(value)end
 if id=="dunkStopper" and field=="activation" then return api.SetDunkKey(value)end
 if id=="comboLibrary" then return true end
 if id=="comboBuilder" and field=="sequenceDelay" then return true end
 if id=="comboBuilder" and field=="sequence" then return api.SetComboSequence(value) end
 if id=="comboBuilder" and field=="preset" then
  local presets={Crossovers="ZC@0.18 > CZ@0.18 > ZX@0.25",Stepback="Z@0.18 > X@0.28 > WAIT@0.15 > C@0.20",Mixed="ZX@0.20 > CV@0.25 > WAIT@0.10 > ZXC@0.30"}
  if presets[value] then
   UI.forEachControl(function(ctrl)if ctrl.id==id then UI.forEachValue(ctrl,function(setting)if setting.id=="sequence" then setting.value=presets[value] end end)end end)
   repaintAll();return api.SetComboSequence(presets[value])
  end
  return true
 end
 if id=="comboBuilder" and field:match("^step[1-4]$") then return api.SetComboSlot(tonumber(field:sub(-1)),value) end
 if id=="comboBuilder" and field=="activation" then return api.SetComboKey(value) end
 if field=="engaged" then return true end -- actual bound key state is read by the engine
 -- The farm toggle is user-driven only (see the controlChanged handler); startup/profile syncs must not flip it.
 if id=="autofarm" and field=="enabled" then return true end
 if id=="autoRelease" and field=="enabled" then return api.SetAutoTime(value) end
 local key=booleanMap[id] and booleanMap[id][field]
 if key then
  local ok=api.SetOption(key,value)
  return ok
 end
 key=numberMap[id] and numberMap[id][field]
 if key then
  local ok=api.SetNumber(key,value)
  if id=="shotAssist" then UI.forEachControl(function(ctrl)
   if ctrl.id==id then UI.forEachValue(ctrl,function(setting)
    local mapped=numberMap[id][setting.id];if mapped then setting.value=api.GetOption(mapped) end
   end) end
  end);repaintAll() end
  return ok
 end
 if id=="autoGuard" and field=="activation" then return api.SetGuardKey(value) end
 if id=="autoGuard" and field=="response" then
  applyResponse(value)
  UI.forEachControl(function(ctrl) UI.forEachValue(ctrl,function(setting)
   local mapped=numberMap[ctrl.id] and numberMap[ctrl.id][setting.id]
   if mapped then setting.value=api.GetOption(mapped) end
  end) end)
  repaintAll();return true
 end
 if id=="interface" or id=="appearance" or id=="layout" or id=="motion" or id=="typography" or id=="notifications" or id=="comboLibrary" then return true end
 return false,"Unsupported control: "..tostring(id).." / "..tostring(field)
end
local function applyAll()
 -- Presets first; explicit sliders then override their corresponding values.
 UI.forEachControl(function(ctrl) UI.forEachValue(ctrl,function(setting)
  if ctrl.id=="autoGuard" and setting.id=="response" then applyResponse(setting.value) end
 end) end)
 UI.forEachControl(function(ctrl)
  if ctrl.kind~="section" then routeControl(ctrl.id,"enabled",ctrl.enabled==true) end
  UI.forEachValue(ctrl,function(setting)
   if setting.id~="response" and setting.type~="button"and not(ctrl.id=="visualStyle"and setting.id=="preset")then routeControl(ctrl.id,setting.id,setting.value)end
  end)
 end)
end

local appearanceApply=applySettings
applySettings=function(t)appearanceApply(t);applyAll()end
UI.connect(controlChanged.Event,function(id,field,value)
 if id=="catchShoot"and field=="status"then
  local state=api.GetCatchShootState()
  local message=string.format("%s · Early E requests: %d · Last lead: %s · %s",state.status,state.earlyRequests or 0,state.lastLeadMs and string.format("%.0f ms",state.lastLeadMs)or "not recorded",state.bufferReason or "unknown")
  setStatus(message);toast("Early-shot buffer",message,"info",true);return
 end
 if id=="smartPass"then
  local name=field=="learnOverhead"and "Overhead"or field=="learnChest"and "Chest"or field=="learnBounce"and "Bounce"
  if name then local _,message=api.LearnSmartPass(name);setStatus(message);return end
  if field=="reset"then local _,message=api.ResetSmartPass();setStatus(message);return end
  if field=="status"then
   local state=api.GetSmartPassState();local names={}
   for _,style in ipairs({"Overhead","Chest","Bounce"})do if state.learned[style]then names[#names+1]=style end end
   setStatus(state.status.." · learned: "..(#names>0 and table.concat(names,", ")or "none"));return
  end
 end
 if id=="smartDribble"and field=="pickBasket"then local _,message=api.PickDribbleBasket();setStatus(message);return end
 if id=="smartDribble"and field=="markThree"then
  local ok,message=api.MarkDribbleThreePointLine()
  if ok then
   UI.forEachControl(function(ctrl)if ctrl.id==id then UI.forEachValue(ctrl,function(setting)if setting.id=="arc"then setting.value=api.GetOption("smartDribbleThreeDistance")end end)end end)
   repaintAll()
  end
  setStatus(message);return
 end
 if id=="smartDribble"and(field=="spawnTest"or field=="stopTest")then
  local _,message
  if field=="spawnTest"then _,message=api.StartDribblePractice()else _,message=api.StopDribblePractice()end
  setStatus(message);return
 end
 if id=="smartDribble"and field=="status"then
  local state=api.GetSmartDribbleState()
  local reason=state.held and state.status or state.lastHeldStatus and("Last hold: "..state.lastHeldStatus)or state.status
  local message=string.format("%s · Key %s (%s) · Action %s · Stamina %s · CanDribble %s · %d opponents · Basket: %s · Arc: %.1f",reason,state.key or "?",state.held and "held"or "released",tostring(state.action or "unknown"),tostring(state.stamina or "unknown"),tostring(state.canDribble),state.defenders or 0,state.basketSource or "not checked",state.threeDistance or 0)
  setStatus(message);toast("Smart dribbler status",message,"info",true);return
 end
 if id=="profile" then
  applyAll()
  UI.persistProfiles();return
 end
 if id=="denyReceiver"and(field=="select"or field=="clear"or field=="next")then
  local _,message
  if field=="select"then
   _,message=api.BeginReceiverSelection();toast("Choose a matchup",message,"crosshair",true)
  elseif field=="next"then _,message=api.CycleReceiverMatchup()
  else
   _,message=api.ClearReceiverMatchup()
   UI.forEachControl(function(ctrl)if ctrl.id=="assignmentTools"then UI.forEachValue(ctrl,function(setting)if setting.id=="lock"then setting.value=false end end)end end)
   repaintAll()
  end
  setStatus(message);return
 end
 if id=="denyReceiver"and field=="status"then
  local state=api.GetReceiverDenialState()
  setStatus(state.status..(state.target and " · "..state.target or "")..(state.handler and " · passer "..state.handler or ""));return
 end
 if id=="antiOop" and field=="status" then
  local state=api.GetAntiOopState();setStatus(state.status..(state.gap and string.format(" · gap %.1f / ETA %.2fs",state.gap,state.time)or "").." · last jump: "..(state.lastJump or "none"));return
 end
 if id=="passIntercept" and field=="status" then setStatus(api.GetDefenseNavigationState().intercept);return end
 if id=="appearance" or id=="motion" or id=="layout" or id=="typography" or id=="notifications" or id=="interface" then
  applySettings();refreshNav();refreshMini()
  if field=="preview" then ACTIONS.preview() end
  if field=="resetLayout" then ACTIONS.resetLayout() end
  return
 end
 if id=="autofarm" and field=="enabled" then local _,message=api.SetFarm(value==true);setStatus(message);toast("Blacktop autofarm",message,"info",true);return end
 if id=="autofarm" and field=="readyNow" then local _,message=api.FarmReadyNow();setStatus(message);return end
 if id=="autofarm" and field=="status" then
  local s=api.GetFarmState()
  local message=string.format("%s · %s · %s · readies %d · dunks %d · loose balls %d · crossovers %d · %s",s.running and "ON" or "OFF",s.status,s.seen or "-",s.ready,s.dunks,s.loose,s.crosses,s.queue)
  setStatus(message);toast("Blacktop autofarm",message,"info",true);return
 end
 if id=="autoJoin" and field=="lock" then local _,message=api.LockJoinSpot();setStatus(message);return end
 if id=="autoJoin" and field=="status" then local state=api.GetAutoJoinState();setStatus(state.status.." · "..state.attempts.." requests");return end
 if id=="comboBuilder" and field=="play" then
  UI.forEachControl(function(ctrl)if ctrl.id==id then ctrl.enabled=true end end)
  api.SetOption("comboBuilder",true);repaintAll()
  local _,message=api.PlayCombo();setStatus(message);return end
 if id=="comboBuilder" and field=="cancel" then api.CancelCombo();setStatus("Combo cancelled");return end
 if id=="assignmentTools" and field=="next" then setStatus(api.SwitchAssignment() and "Matchup selected" or "No opponent in range");return end
 if id=="tools" then
  if field=="report" then local _,message=api.ExportReport();setStatus(message)
  elseif field=="check" then local _,message=api.Check();setStatus(message)
  elseif field=="assignment" then setStatus(api.SwitchAssignment() and "Opponent selected" or "No opponent in range")
  elseif field=="solo" then api.StartSolo();setStatus("Local rehearsal only; no server opponent")
  elseif field=="stopSolo" then api.StopSolo();setStatus("Rehearsal stopped")
  elseif field=="unload" then api.Unload() end
  return
 end
 if id=="rebound" and field=="setCourtBounds" then
  local folder=workspace:FindFirstChild("Characters");local own=folder and folder:FindFirstChild(player.Name) or player.Character
  local r=own and own:FindFirstChild("HumanoidRootPart")
  if not r then setStatus("Character unavailable");return end
  if not firstCourtCorner then firstCourtCorner=r.Position;setStatus("First corner marked. Walk to opposite corner and click again.")
  elseif api.SetCourtBounds(firstCourtCorner,r.Position) then firstCourtCorner=nil;setStatus("Court bounds set")
  else setStatus("Corners must span at least 6 studs on both axes") end
  return
 elseif id=="rebound" and field=="clearBounds" then firstCourtCorner=nil;api.ClearCourtBounds();setStatus("Court bounds cleared");return end
 local ok,message=routeControl(id,field,value)
 if ok and id=="denyReceiver"and field=="enabled"and value and api.GetReceiverDenialState and api.BeginReceiverSelection then
  if not api.GetReceiverDenialState().selected then
   local _,notice=api.BeginReceiverSelection();toast("Choose a matchup",notice,"crosshair",true);setStatus(notice)
  end
 end
 if not ok and message then setStatus(message) end
end)
local lastMatchup,matchupNoticeAt=nil,0
UI.connect(RunService.Heartbeat,function()
 if os.clock()-matchupNoticeAt<.25 then return end
 matchupNoticeAt=os.clock()
 -- The engine owns the autofarm state (saved config, the 5 s prompt); keep the toggle matching it.
 local farmOn=api.GetOption("farm")==true
 UI.forEachControl(function(ctrl)if ctrl.id=="autofarm" and ctrl.enabled~=farmOn then ctrl.enabled=farmOn;repaintAll()end end)
 if not api.GetReceiverDenialState then return end
 local state=api.GetReceiverDenialState()
 if state.selected~=lastMatchup then
  if state.selected then
   toast("Matchup selected",state.selected.." · on and off ball","shield",true)
   UI.forEachControl(function(ctrl)if ctrl.id=="assignmentTools"then UI.forEachValue(ctrl,function(setting)if setting.id=="lock"then setting.value=false end end)end end)
   repaintAll()
  elseif lastMatchup then toast("Following ball handler","Your selected matchup was cleared or is no longer available","shield",true)end
  lastMatchup=state.selected
 end
end)
UI.connect(testControls.Event,function() local _,message=api.Check();setStatus(message) end)

-- Notification and non-interactive cursor visuals; no full-screen modal.
do
 local shownRevision,lastPoll=0,0
 local cursor,hoverOutline,nameLabel
 local function make(class,props)
  local object=Instance.new(class)
  for key,value in pairs(props)do object[key]=value end
  return object
 end
 local function hide()
  if cursor then cursor.Visible=false end
  if hoverOutline then hoverOutline.Enabled=false;hoverOutline.Adornee=nil end
 end
 local function ensure()
  if cursor then return end
  cursor=make("Frame",{Name="MatchupSelectionCursor",Size=UDim2.fromOffset(40,40),BackgroundTransparency=1,BorderSizePixel=0,Visible=false,Active=false,ZIndex=100,Parent=gui})
  local ring=make("Frame",{Size=UDim2.fromOffset(28,28),Position=UDim2.fromOffset(6,6),BackgroundTransparency=1,ZIndex=101,Parent=cursor})
  make("UICorner",{CornerRadius=UDim.new(1,0),Parent=ring})
  make("UIStroke",{Color=Color3.fromRGB(80,167,255),Thickness=1.5,Transparency=.12,Parent=ring})
  for _,line in ipairs({{19,0,2,7},{19,33,2,7},{0,19,7,2},{33,19,7,2}})do
   make("Frame",{Position=UDim2.fromOffset(line[1],line[2]),Size=UDim2.fromOffset(line[3],line[4]),BackgroundColor3=Color3.fromRGB(109,191,255),BorderSizePixel=0,ZIndex=101,Parent=cursor})
  end
  local pill=make("Frame",{Position=UDim2.fromOffset(45,8),Size=UDim2.fromOffset(220,44),BackgroundColor3=Color3.fromRGB(17,20,29),BackgroundTransparency=.05,BorderSizePixel=0,ZIndex=101,Parent=cursor})
  make("UICorner",{CornerRadius=UDim.new(0,12),Parent=pill})
  make("UIStroke",{Color=Color3.fromRGB(40,47,64),Thickness=1,Parent=pill})
  nameLabel=make("TextLabel",{Size=UDim2.new(1,-20,1,0),Position=UDim2.fromOffset(10,0),BackgroundTransparency=1,Text="",TextColor3=Color3.fromRGB(227,240,255),Font=Enum.Font.GothamMedium,TextSize=12,TextWrapped=true,ZIndex=102,Parent=pill})
  hoverOutline=make("Highlight",{Name="MatchupSelectionHover",Enabled=false,FillColor=Color3.fromRGB(49,135,255),FillTransparency=.87,OutlineColor=Color3.fromRGB(106,202,255),OutlineTransparency=.05,DepthMode=Enum.HighlightDepthMode.Occluded,Parent=workspace.CurrentCamera or workspace})
 end
 UI.connect(RunService.Heartbeat,function()
  if not api.GetMatchupPickerState or os.clock()-lastPoll<.25 then return end
  lastPoll=os.clock()
  local state=api.GetMatchupPickerState()
  if not state.active or state.revision<=shownRevision then return end
  shownRevision=state.revision
  api.BeginReceiverSelection()
  toast("Choose your matchup","Hover an opponent, then click to select. Right-click or Escape cancels.","crosshair",true)
 end)
 UI.connect(RunService.RenderStepped,function()
  if not api.GetReceiverDenialState or not api.GetReceiverDenialState().picking then hide();return end
  ensure()
  local position=UserInputService:GetMouseLocation()
  if not gui.IgnoreGuiInset then position=position-game:GetService("GuiService"):GetGuiInset()end
  cursor.Position=UDim2.fromOffset(position.X-20,position.Y-20);cursor.Visible=true
  local character,adornee
  if api.GetReceiverSelectionHover then character,adornee=api.GetReceiverSelectionHover()end
  hoverOutline.Adornee=adornee;hoverOutline.Enabled=adornee~=nil
  nameLabel.Text=character and("CLICK TO GUARD\n"..character.Name)or "SELECT MATCHUP\nHover an opposing player"
  hoverOutline.FillTransparency=.87+math.sin(os.clock()*3)*.025
 end)
 api.RegisterUICleanup(function()
  if hoverOutline then hoverOutline:Destroy()end
  if cursor then cursor:Destroy()end
  if api.CancelReceiverSelection then api.CancelReceiverSelection()end
 end)
end

api.IsUICapturingInput=function()return rebinding~=nil end
api.IsUIReservedKey=function(key)return key==PREF_KEY.value end
api.RegisterUICleanup(function()if alive then gui:Destroy()end end)
api.SetPanelVisible(false)
UI.restore(currentProfile.snap)
end)() end

-------------------------------------------------------------------- go
applySettings(0)
openPage(START_PAGE)
navHighlight.Parent=nav[START_PAGE].btn.Parent
navHighlight.Position = nav[START_PAGE].pos
refreshNav(0)
UI.refreshStatus()
refreshMini()
gui.Parent = playerGui
fit(0)
task.defer(setMode, "open")

