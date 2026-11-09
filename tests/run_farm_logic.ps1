# Run pure production-logic scenarios with an available 64-bit Lua 5.1 DLL.
# This does not emulate Roblox or replace Luau/in-game validation.
param([string]$LuaDll = "$env:ProgramFiles\obs-studio\bin\64bit\lua51.dll")
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $LuaDll)) { throw 'Supply -LuaDll with a 64-bit Lua 5.1 DLL path.' }
$dllPath = (Resolve-Path -LiteralPath $LuaDll).Path.Replace('"', '""')
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class FarmLogicLua {
 [DllImport(@"$dllPath", CallingConvention=CallingConvention.Cdecl)] public static extern IntPtr luaL_newstate();
 [DllImport(@"$dllPath", CallingConvention=CallingConvention.Cdecl)] public static extern void luaL_openlibs(IntPtr state);
 [DllImport(@"$dllPath", CallingConvention=CallingConvention.Cdecl)] public static extern int luaL_loadstring(IntPtr state, string code);
 [DllImport(@"$dllPath", CallingConvention=CallingConvention.Cdecl)] public static extern int lua_pcall(IntPtr state, int args, int results, int error);
 [DllImport(@"$dllPath", CallingConvention=CallingConvention.Cdecl)] public static extern IntPtr lua_tolstring(IntPtr state, int index, out UIntPtr length);
 [DllImport(@"$dllPath", CallingConvention=CallingConvention.Cdecl)] public static extern void lua_close(IntPtr state);
}
"@
$state = [FarmLogicLua]::luaL_newstate()
if ($state -eq [IntPtr]::Zero) { throw 'Lua state allocation failed' }
$oldDirectory = [Environment]::CurrentDirectory
try {
 [Environment]::CurrentDirectory = Split-Path -Parent $PSScriptRoot
 [FarmLogicLua]::luaL_openlibs($state)
 $code = @'
math.clamp=function(n,lo,hi)return math.max(lo,math.min(hi,n))end
function readfile(path)
 local f=assert(io.open(path,"rb"));local text=f:read("*a");f:close();return text
end
local messages={}
print=function(text)messages[#messages+1]=tostring(text)end
assert(loadstring(readfile("tests/farm_planner_arrival.luau")))()
assert(loadstring(readfile("tests/farm_space_reset.luau")))()
assert(loadstring(readfile("tests/stepback_immediate_e.luau")))()
assert(loadstring(readfile("tests/farm_open_shots.luau")))()
assert(loadstring(readfile("tests/api_profile_scope.luau")))()
local core={}
for _,name in ipairs({"00_startup_wait","01_setup_bindings","02_debug_panel_widgets","03_autotime_peak_predictor",
 "04_panel_features_presets","05_defense_shared_helpers","06_guard_input_movement","07_assists_landing_visuals",
 "08_guard_state_solo_test","09_suite_api_profiles"})do
 core[#core+1]=readfile("src/engine/"..name..".luau")
end
assert(loadstring(table.concat(core,"\n"),"@shared_engine_core"))
print("Shared startup core: Lua 5.1 compilation passed (Luau register allocation still requires Luau)")
-- Syntax smoke checks only. These files use two Luau += expressions;
-- normalize those in memory for this Lua 5.1 parser, never rewrite source.
assert(loadstring(readfile("src/engine/23_smart_dribble.luau"),"@smart_dribble"))
local source=readfile("src/engine/26_blacktop_autofarm.luau"):gsub("resetCandidates%+=1","resetCandidates=resetCandidates+1"):gsub("count%+=1","count=count+1")
-- Lua 5.1 permits fewer captured locals than Luau. Parse the farm in three
-- sections so that limitation does not hide syntax errors in changed branches.
local first=assert(source:find(" local function think(now)",1,true))
local last=assert(source:find(" local function disarm(persist)",first,true))
assert(loadstring(source:sub(1,first-1).."\nend) end)()end","@farm_planning"))
assert(loadstring(source:sub(first,last-1),"@farm_think"))
assert(loadstring("do (function() local farmLoaded,farmError=pcall(function()\n"..source:sub(last),"@farm_lifecycle"))
print("Changed engine files: segmented Lua 5.1 syntax smoke checks passed (not a Luau compilation)")
return table.concat(messages,"\n")
'@
 $result = [FarmLogicLua]::luaL_loadstring($state, $code)
 if ($result -eq 0) { $result = [FarmLogicLua]::lua_pcall($state, 0, 1, 0) }
 $length = [UIntPtr]::Zero
 $pointer = [FarmLogicLua]::lua_tolstring($state, -1, [ref]$length)
 $message = [Runtime.InteropServices.Marshal]::PtrToStringAnsi($pointer)
 if ($result -ne 0) { throw $message }
 $message
} finally {
 [Environment]::CurrentDirectory = $oldDirectory
 [FarmLogicLua]::lua_close($state)
}
