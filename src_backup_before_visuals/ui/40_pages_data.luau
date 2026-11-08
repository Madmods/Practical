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
  {id="visualStyle",tab="Style",kind="section",name="Appearance",rows={
   {type="checks",items={{id="enabled",label="Modern court visuals",value=true},{id="labels",label="Floor labels",value=true}}},
   {type="segment",id="preset",label="Style",options={"Minimal","Balanced","Vivid"},value="Balanced"},
   {type="segment",id="palette",label="Accent",options={"Cobalt","Glacier","Mint","Amber"},value="Cobalt"},
   {type="segment",id="route",label="Route pattern",options={"Dashed","Solid","Chevrons"},value="Dashed"},
   {type="checks",items={{id="motion",label="Animated rings and pulses",value=false}}},
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
  {id="visualLabels",tab="Details",kind="section",name="Labels & cards",rows={
   {type="checks",items={{id="passPanel",label="Extra pass-target corner card",value=false}}},
   {type="slider",id="size",label="Label text size",min=9,max=16,step=1,value=11,format="%.0f",suffix=" px"},
   {type="slider",id="opacity",label="Label background",min=0,max=95,step=5,value=80,format="%.0f",suffix="%"},
  }},
  {id="visualColor",tab="Details",kind="section",name="Custom accent",rows={
   {type="checks",items={{id="enabled",label="Use custom RGB accent",value=false}}},
   {type="slider",id="red",label="Red",min=0,max=255,step=1,value=64,format="%.0f"},
   {type="slider",id="green",label="Green",min=0,max=255,step=1,value=132,format="%.0f"},
   {type="slider",id="blue",label="Blue",min=0,max=255,step=1,value=255,format="%.0f"},
  }}
 }},
 {name="Tools",icon="wrench",subtitle="Queue and session utilities",controls={
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

