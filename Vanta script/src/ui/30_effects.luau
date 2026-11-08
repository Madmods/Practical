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

