]====],"PracticalUI2")
assert(run,err)
local ok,message=xpcall(run,debug.traceback)
if not ok then warn("Practical UI 2 startup failed: "..tostring(message))end
end)()end
