return function()
    local existing=workspace:FindFirstChild("VantaTestField");if existing then return existing end
    local field=Instance.new("Folder");field.Name="VantaTestField";field.Parent=workspace
    local function part(name,size,pos,color,material)
        local p=Instance.new("Part");p.Name=name;p.Anchored=true;p.Size=size;p.Position=pos;p.Color=color;p.Material=material or Enum.Material.SmoothPlastic;p.TopSurface=Enum.SurfaceType.Smooth;p.BottomSurface=Enum.SurfaceType.Smooth;p.Parent=field;return p
    end
    local dark=Color3.fromRGB(29,71,53);local light=Color3.fromRGB(34,81,58)
    part("Field",Vector3.new(160,2,360),Vector3.new(0,-1,-125),dark,Enum.Material.Grass)
    for i=0,11 do
        local z=40-i*30
        part("Stripe",Vector3.new(158,0.02,15),Vector3.new(0,0.01,z-7.5),i%2==0 and light or dark,Enum.Material.Grass)
        part("Yard line",Vector3.new(160,0.035,0.22),Vector3.new(0,0.04,z),Color3.fromRGB(225,234,224))
        for _,x in ipairs({-55,55}) do
            local marker=part("Yard number",Vector3.new(8,0.03,5),Vector3.new(x,0.055,z+3),dark);marker.Transparency=1;marker.CanCollide=false
            local gui=Instance.new("SurfaceGui");gui.Face=Enum.NormalId.Top;gui.CanvasSize=Vector2.new(300,200);gui.Parent=marker
            local t=Instance.new("TextLabel");t.Size=UDim2.fromScale(1,1);t.BackgroundTransparency=1;t.Text=tostring(math.min(i,10-i)*10);t.TextColor3=Color3.fromRGB(225,234,224);t.TextScaled=true;t.Font=Enum.Font.GothamBold;t.Parent=gui
        end
    end
    for _,x in ipairs({-79,79}) do part("Sideline",Vector3.new(0.4,0.04,360),Vector3.new(x,0.045,-125),Color3.fromRGB(235,241,233)) end
    for z=35,-290,-3 do for _,x in ipairs({-18,18}) do part("Hash",Vector3.new(1.8,0.04,0.14),Vector3.new(x,0.045,z),Color3.fromRGB(208,222,208)) end end
    for _,z in ipairs({53,-303}) do
        part("End zone",Vector3.new(158,0.025,15),Vector3.new(0,0.03,z),Color3.fromRGB(25,39,58))
        for _,x in ipairs({-82,82}) do part("Fence",Vector3.new(2,8,370),Vector3.new(x,4,-125),Color3.fromRGB(28,37,48)) end
    end
    local spawn=Instance.new("SpawnLocation");spawn.Name="QB Spawn";spawn.Anchored=true;spawn.Size=Vector3.new(6,0.2,6);spawn.Position=Vector3.new(0,0.2,30);spawn.Neutral=true;spawn.Transparency=1;spawn.CanCollide=false;spawn.Parent=field
    workspace.Gravity=196.2
    local lighting=game:GetService("Lighting");lighting.ClockTime=14;lighting.Brightness=2;lighting.Ambient=Color3.fromRGB(135,143,158);lighting.OutdoorAmbient=Color3.fromRGB(145,151,167)
    return field
end
