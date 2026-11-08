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

