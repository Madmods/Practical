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

