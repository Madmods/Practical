PROJECTILE PHYSICS AND TRAJECTORY PLANNING

A self-contained, header-only C++17 package: constant-acceleration projectile
physics, a pass trajectory planner built on it, reproducible synthetic scenarios,
SVG diagnostics and benchmarks. No dependencies beyond the standard library.

FILES
 projectile.hpp     physics: World (units + gravity), advance, groundContact, sampling, CSV
 planner.hpp        planner: reach models, candidate evaluation, threat sweeps, Planner
 scenarios.hpp      scripted synthetic scenarios, noisy observation, throw outcome simulation
 visualize.hpp      SVG diagram and text report for a decision
 check.hpp          tiny test harness (works with NDEBUG, catches unexpected exceptions)
 tests.cpp          physics regression tests
 planner_tests.cpp  planner regression tests
 simulate.cpp       runs every scenario -> out\<name>.svg, -report.txt, -frames.csv, index.html
 bench.cpp          accuracy, worst-case update time, sensitivity, stability benchmarks
 example.cpp        one trajectory as CSV

BUILD AND RUN
 build-test.cmd          builds everything with MSVC, runs both test suites, writes
                         build\trajectory.csv and out\ (open out\index.html)
 build-test.cmd bench    ...and runs the benchmarks
 or CMake: targets projectile_tests, planner_tests, simulate, bench, projectile_example;
 "ctest" runs both test suites.
 Lengths are in World units (World::metresPerUnit: 1 metres, 0.9144 yards). Gravity is
 given in m/s^2 and converted. Time is always seconds. Defaults are written in metres
 and converted with MoverParams::scaledTo / PlannerConfig::defaults(world).

------------------------------------------------------------------------------
1. PHYSICS FIXES (projectile.hpp)
 - advance(): computes (a*t/2)*t instead of a*(t*t/2). Zero or small acceleration with
   a huge duration no longer produces 0*inf = NaN and a spurious overflow_error.
 - One gravity configuration: World holds units, gravity and ground height; advance(),
   groundContact() and the planner all take it, so they cannot disagree.
 - groundContact() returns Contact{kind, time, point} instead of an optional that was
   never empty. kind is Airborne, OnPlane or BelowPlane, so "below the plane" is
   explicit rather than silently "contact at t=0". Heights within rounding of the
   plane count as on it: a state left 7e-15 below the ground by an earlier advance()
   and moving up is airborne, not "already landed". The contact point is exactly on
   the plane.
 - Scale-aware tolerances: roundingTolerance(scale) and positionScale(); tests compare
   in ulps of the summed term magnitudes instead of a fixed 1e-7.
 - CSV output writes max_digits10 digits (values read back bit-identical) and includes
   velocity; the example's last row is the exact contact point.

2. PLANNER DESIGN (planner.hpp)
 Joint search. A candidate is (flight time, depth offset, lateral offset, catch mode)
   relative to where the receiver is predicted to be. The launch velocity reaching that
   destination at that time is solved in closed form, so speed, elevation, flight time
   and destination are searched together; nothing adjusts elevation in isolation.
   Default grid: 32 flight times x 7 depths (-4..+4 m: negative = comeback) x 5 lateral
   offsets (-3..+3 m) x 3 catch modes (chest / high-jump / dive) = 3360 candidates,
   plus continuous local refinement around the best, clamped to the same ranges.
 Receiver reachability is an INNER bound: an explicit plan (coast through the reaction
   time, constant acceleration, then cruise) checked against top speed and an
   acceleration ellipse (accel / decel along the heading, turnAccel across it). Jumps
   cost jumpPrepTime; dives add reach. A destination is only accepted if such a plan
   exists, so the planner never claims a catch the movement model cannot produce.
 Defenders are modelled individually with an OUTER bound: they coast at their current
   velocity until they react, then may accelerate in any direction at their largest
   limit with speed capped. Zero velocity therefore does NOT mean stationary.
 Interception is checked over the whole flight (sampled every 1/60 s, the most
   dangerous moment refined by golden-section search, the first reachable moment by
   bisection), using ball HEIGHT (stand / jump / dive reach) and TIME, not horizontal
   distance alone. The flight after the catch time is checked separately: what a
   defender can do if the receiver misses.
 Uncertainty: each agent has position / velocity / reaction-time errors combined into
   sigma(t). Each threat reports its gap, sigma, gap/sigma, P(reach), time, ball
   position, height and mode. riskAssumptions() lists every assumption used.
 Verdicts: "invalid" (impossible or unsupported: launch limits, unreachable, bad input),
   "contested" (a defender can reach the ball before the catch under the outer bound),
   "uncertain" (clear under the bound but within clearSigmas, or the catch does not
   survive receiverSigmas of receiver error, or uses > 90% of a movement limit),
   "predicted clear". Invalid candidates can never be labelled clear.
 Cost (lower is better), reported term by term: verdict class, risk before the catch,
   after-miss risk weighted by the catch type's miss probability, flight time (prefers
   fast flat throws), adjustment distance, catch mode, receiver effort, robustness.
   Slower or higher throws win when they reduce risk.
 Stability: the selection is re-validated every update. A challenger replaces it only
   if better by switchMargin + noiseSigmas * (measured cost noise) including a
   continuity penalty; remembered candidates rank on an exponentially weighted cost
   (the verdict term is never smoothed). An invalid selection is dropped in the same
   update; a contested one is replaced in the same update if an uncontested option was
   evaluated. Non-forced changes move in validated steps (blending): each intermediate
   is a full candidate, evaluated like any other.
 Display = selection: displayPath() samples the selected evaluation's own launch state
   with the same advance(); Evaluation::validatedUpdate records which update produced it.
 Bounded work: each update spends at most workBudget reach evaluations (deterministic)
   and optionally timeBudgetMs; the grid pass resumes where the last update stopped.

3. WHAT IS VERIFIED (numbers from this machine; rerun "build-test.cmd bench")
 Tests: physics 2348 checks, planner 66324 checks, all passing. Mutation check: four
   deliberate regressions (old overflow ordering, "stationary" defenders, hysteresis
   off, "clear" ignoring sigma) were each caught by the tests.
 Accuracy
   - ground contact residual over 200k random states (heights 1e-6..1e6): <= 2.5 ulps
     of the summed term scale
   - launch solutions: 26,880 arcs end at their destination within 0.5 ulps
   - receiver plans: 4,631 random feasible plans replayed; arrival error <= 7.3e-15,
     speed use <= 0.99983, acceleration-ellipse use <= 0.99999 (limit 1)
   - defender outer bound: 1.8 M positions from 3,000 random limit-respecting manoeuvres,
     0 outside the bound; the closest came within 1.7e-13 (the bound is tight)
   - first-interception time vs a closed-form case: 6.5e-13 s error
   - a yards scene ranks and judges candidates like the same scene in metres
 Computation (default budget 40,000, 1-4 defenders): per update p50 2.6 ms,
   p99 6.0 ms, max 8.9 ms; a full 3360-candidate search 36-164 ms.
 Stability over the 8 noisy scenarios (per scenario, 60 Hz):
                         switches/s  A->B->A oscillations  jumps > 1 unit
   with hysteresis           8.7            1.6                 6.1
   without                  23.6           21.2                10.9
   Most remaining switches are small steps that track a moving receiver.
 Simulated outcomes: every "predicted clear" throw (3 scenarios) was caught when played
   out against the scripted agents. The 5 "uncertain" throws: 1 caught, 3 incomplete
   (receiver short by 0.02-0.22 m), 1 intercepted after a miss. No throw was defended
   before the catch.
 Sensitivity: re-evaluating the selected throw under 400 perturbations drawn from the
   planner's own error model keeps the verdict 97-100% of the time for the clear throws
   and 72-99% for the uncertain ones. The Gaussian P is mostly conservative versus
   sampling (e.g. 0.39 vs 0.29 contested), but in one clear scenario 0.3% of samples
   were contested while P rounded to 0.000: P is evaluated at the single most
   dangerous moment and is not a strict upper bound.

4. ASSUMPTIONS (modelled, not validated against real play)
 - All movement and catch parameters in MoverParams, the uncertainty defaults and the
   cost weights are placeholders, not measured.
 - Receiver acceleration ellipse is fixed to the heading at the throw; the real limit
   changes as the receiver turns.
 - Defender bound uses one isotropic acceleration limit; it ignores facing, hip
   turns, backpedal and body orientation, which errs toward more defender reach.
 - Errors are isotropic Gaussian and defenders independent (risk = 1 - prod(1 - P)).
 - Pre-reaction motion is coasting at the current velocity; anticipation is only
   covered through the velocity uncertainty.
 - Catch success is binary inside reach, with a fixed miss probability per catch type.
 - No drag, spin, wind, deflections, tipped balls, pass rush or sideline limits; the
   ball is a point.
 - The receiver route, if supplied, is followed at the current speed.
 - The scenario simulator's agents obey the same limits the planner assumes, so the
   outcome checks show internal consistency, not real-world accuracy.

5. INFORMATION NEEDED TO CALIBRATE
 - Tracking data (positions/velocities at the game's tick rate) for receivers and
   defenders: top speed, acceleration, braking, turning by speed, and reaction time
   distributions measured from release.
 - Jump reach heights, take-off times, dive reach and dive height limits.
 - Catch, drop and deflection rates by catch type, ball height, ball speed and contact
   distance (to replace missChest/High/Dive and the binary reach).
 - Release height and offset for the throwing motion; real speed and angle limits.
 - Observation noise of the game's state (to set Uncertainty) and how often defenders
   move before the throw (anticipation).
 - The game's units, tick rate, compute budget per frame and how many receivers and
   defenders are evaluated at once.
 - Whether the game models drag or spin: if so the closed-form launch solve must be
   replaced by a shooting method and the ulps-level accuracy above no longer applies.
 - Designer intent for the cost weights (how much risk is worth one tenth of a second).
