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

