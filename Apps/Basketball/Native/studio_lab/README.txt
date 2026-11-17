VANTA QB LAB - DIRECT EXTERNAL TEST

Run Launch Lab.cmd, then press Play in Roblox Studio.
Keep the Vanta Studio test connection window open. Only one lab bridge can run.
The in-game status should say Vanta connected. The football equips automatically.

Q near a WR selects that receiver (purple outline and ground ring).
Click the field to throw. Use Add Defender / Add Receiver, then click to place.
E near any bot selects it for Route or Move selected bot.
Run routes starts Still / Go / Slant / Comeback / Delayed movement.
J makes bots jump; G makes them dive. Reset returns them to their start positions.
WR misses disables the selected WR's catch, so the ball continues for DB testing.
Bots react controls independent jump/dive/pursuit after the ball is released.
Tab hides the test panel. Catch height and Extra lead have minus/plus controls.

This is the actual Vanta executable in --studio-lab mode. Its C++
ff3_qb_logic.hpp computes Beam / regular Smart fit and full-flight risk.
Studio sends snapshots through a token-protected loopback bridge. It receives
launch position and velocity, draws their trajectory, and uses those same values
for the fake ball. Luau has no replacement QB aim solver. No Daybreak is used.
The live-game packet transport, AutoTime, and production overlay UI are outside
this test adapter. The regular Vanta installation is unchanged.

CALIBRATION
Ball gravity 28, max speed/power 95, launch offset 5 studs, radius 0.6.
Character gravity 196.2, walking speed 20, jump power 50 (6.37-stud rise).
Dive speed 43.45, dive duration 0.35 seconds, reach 12.
Catch envelope: radius 5.88186, above head 3.88186, below head 4.88186.
Change ReplicatedStorage > VantaQBLab > Profile in Studio, then restart Play.

Ballistics and the movement baselines match the available local measurements.
Dive duration, bot reaction/pursuit, ellipsoid catch activation, and tie-breaking
are explicit test assumptions. Bots use scripted kinematic movement with R6-sized
visuals, not the original game's server code. This is not a verified 1:1 replica.
A clear estimate is a model forecast, not a guarantee against interception.

The bridge requests plans at 5 Hz to stay below Studio's HTTP request quota.
Clicks request a fresh plan. Ball rendering runs each render frame. This tests
planner behavior, not live-game latency or packet-rewrite reliability.

VERIFICATION
7 bridge checks passed; 126 existing native checks passed.
Studio connected and completed a real simulated mouse-click throw.
Covered test: intercepted by DB. Open test: caught by WR.
WR-misses control: incomplete. Studio console had no script errors.
No HTTP quota errors in the final run (more than 900 successful requests).

DIAGNOSTICS
last-plan.json contains the latest native plan.
ServerScriptService > VantaLabServer attributes expose PlanCount, ThrowCount,
LastLaunch, LastResult, and any LastBridgeError while Play is running.
Keep vanta-lab-connection.json beside Vanta.exe; the place uses its matching token.
The Source folder is a readable copy; edit scripts inside Studio to change play.
