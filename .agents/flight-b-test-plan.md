# Flight B test plan: Tasks 2, 3 and 4 (FSR 4.1.1)

Tests the `fsr4-1-1` build carrying Task 2 (debug view), Task 3 (camera planes in AMD's order), Task 4 (fixed exposure on the flat HDR route) and the jitter revert, against the baseline. Plan: [4.1.1-iteration-plan.md](4.1.1-iteration-plan.md).

## Setup (agent, game closed)

- [ ] Build `fsr4-1-1` at the commit after Task 4 and its two diagnostics keys. The tree must be clean, so the log's build id matches the commit.
- [ ] Install into the development game: `install_edvr.py --target <dev game product dir> --scope flat --compile-only --fsr4 --root <fork>`. Record the d3d11.dll sha256.
- [ ] `edvr-flat.ini`:
  - `temporal_aa = fsr4`, `ui_quality = 200`, `hud_antialiasing = off`.
  - Remove the temporary `temporal_aa_jitter_phases` line; the default 8 applies.
  - Under `[advanced]`: `flat_refusal_view = off`, `fsr4_debug_view = off`, `fsr4_auto_exposure = off`, `fsr4_debug_checker = on`.
- [ ] Elite's Supersampling: 1.0 for Part 1. `Custom.4.4.fxcfg` in the development prefix; backup in `evidence/flight-a-20261109/`.
- [ ] Launch: `launch_frontier.py --run --fsr4-upgrade`. Docked, Solo.

All `[advanced]` keys below are live: the agent edits the ini during the flight and a change applies within about a second. A change to the debug view, auto exposure or debug checker key makes a new FSR 4 context, so expect one history reset.

## Part 1: Supersampling 1.0, the HDR route (Task 4, Task 3)

Log checks (agent):
- [ ] `fsr4: picked FSR 4.1.1`, `provider=fsr 4.1.1 (Proton driver provider)`, dispatch `rc=0`, `hdr=1`.
- [ ] The HDR route's create flags no longer include auto exposure (no 0x… AUTO_EXPOSURE bit on the create line), and the log names the fixed exposure.
- [ ] AMD's debug checker (Task 3): no "cameraFar value is very low" warning. "cameraNear value is very low" may remain if Elite's near plane is under 0.075. That is the game's near plane, not a regression; record its value.
- [ ] The context line reads `exposure=fixed-1.0` on the HDR route and `debug_checker=1`.
- [ ] No refusals, no context-creation failures.
- [ ] AMD's message relay stops after 20 messages per session, in total. First-frame warnings arrive within that; a warning repeated every frame can use up the rest. Read the first 20.

Exposure A/B (David flying, agent toggling `fsr4_auto_exposure`):
- [ ] **Dark hangar, docked.** Captures with auto exposure off (fixed), on, then off again. Same view, headlook off.
- [ ] **Open space with a star field.** David undocks and holds still. The same A/B/A. Look for star loss, dimming, flicker and hull highlight detail.
- [ ] **A bright scene:** planet limb or the star, if convenient. The same A/B/A.
- [ ] Frame rate at each: F8 overlay or the log's `flat cpu 5s` present median.

Pass: fixed exposure shows stars at least as well as auto, no dimming, no new flicker, and no frame-rate cost (within 2%).

## Part 2: FSR 4's debug view (Task 2)

- [ ] `fsr4_debug_view = on`, Supersampling 1.0, then 0.5 (David changes it in Elite's options).
- [ ] What AMD documents: motion vectors top left, blend factor top right (bright purple is a low blend factor).
- [ ] Still, docked: the blend factor should be high (little purple) on static geometry.
- [ ] Headlook sweep left and right: the motion vectors' colour should flip with direction and be uniform across static geometry. Any patch moving the wrong way, or a cockpit edge flagged as moving while the cockpit is still, is a jitter-sign or MV-scale problem.
- [ ] A moving ship or the hangar platform, if one is in view: its motion should differ from the background.
- [ ] Captures of each. Then `fsr4_debug_view = off`.

Pass: motion vectors match the motion seen, and the blend factor is high on still geometry. A fail names the region and the motion that disagrees.

## Part 3: Supersampling 0.5, the copy route (regression and visuals)

- [ ] Log: `in=2560x720 out=5120x1440 hdr=0`. The copy route keeps auto exposure: its create flags are unchanged at 0x138, apart from the debug-checker bit.
- [ ] `flat jitter: … phases=8` (the revert).
- [ ] Frame rate within a few percent of Flight A at the same settings (about 130 fps at UI quality 200).
- [ ] The refusal view in motion: `flat_refusal_view = on` during a headlook sweep, and on foot if David disembarks. This feeds the Task 5 decision. Then off.
- [ ] David's verdict on the overall picture against his memory of the baseline: a short note.

## Close

- [ ] Quit normally. Restore Supersampling to 0.75 from the backup unless David says otherwise. Set `fsr4_debug_checker = off`.
- [ ] Investigation entry "Flight 8: Tasks 2–4", with log file names, flags, captures and verdicts. Dev log line.
- [ ] Failures go back to the plan as findings with evidence. Nothing is fixed during the flight.
