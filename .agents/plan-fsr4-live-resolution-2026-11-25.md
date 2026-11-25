# FSR4 live render-resolution selector: delivery plan

## Status and authority

**Latest execution constraint:** no commits and no pushes until David confirms
in-game confidence. Native validation and development-game retesting are
authorised. The first flat candidate was installed and flew with FSR 4.1.2,
but live quality changes failed because the request publisher used the
scope-filtered AA accessor. Duplicate status, unused-slot pending text and
footer overlap also failed live acceptance. Correct these with production-path
regressions, rebuild, then retest flat before VR. This supersedes all later
commit/push and overnight installation-deferral wording.

**Date:** 25 November 2026. **Owner:** David / pattern-state. **Revision:** 4.
**Status:** implementation and targeted native validation in progress; final
combined validation and local commits pending. Live acceptance deferred.

Objective: select an FSR4 quality preset in ED-UP's settings window or F8
overlay, and have Elite change its own internal rendering resolution during
play, in flat and VR, while keeping the selected output resolution fixed.

David authorised execution on 25 November 2026: “kick the work off with you
supervising. stop when it's complete”. Implement, validate, commit and push
according to this plan. Live acceptance still requires David to have finished
his current session and be ready to test. Earlier planning-only wording records
the preparation phase and is superseded by this execution authority.

Canonical implementation repository:
`/home/davidmoore/src/ed-unofficial-patch`, branch `fsr4-1-2`, inspected commit
`f0b40e6c9b0a7ad14e92fec97af15b863e00d993`. Its worktree was clean during
inspection. The native build snapshot is `C:\Dev\EDUP-Build`; it is a build
copy, not the source of truth. `/home/davidmoore/src/ed-fsr4` is the harness
and has numerous pre-existing changes. This feature does not require staging
those changes.

Plan: `.agents/plan-fsr4-live-resolution-2026-11-25.md` in the implementation
repository. Future evidence: `C:\Dev\EDUP-Validation\fsr4-quality-2026-11-25\`,
outside the source checkouts, with a compact human-readable record in
`docs/fsr4-windows-flight-2026-11-25.txt`. Do not commit full private flight
logs. New test fixtures belong under `tools/fsr4_quality_test/fixtures/`.

## Execution clarifications before combined validation

The external delivery-status and task evidence files record actual completion,
red/green results, source hashes and corrected failed attempts. They are the
live completion ledger; this plan is frozen before receipt generation.

- T08 was advanced alongside T04 because it consumes accepted T01/T02 contracts
  without editing the controller. T05 pure policy preceded final T04 runtime
  acceptance; runtime integration waited for the checked hook gate. T07's pure
  output policy and T09's menu work also overlapped disjoint integration work.
- Invalid quality configuration is a refused request, not an implicit Game
  selection: no new scale writes, with previous actual values and ownership
  receipt retained. Explicit Game, AA-off or profile release must still restore
  conditionally even while the new request is invalid.
- The 120 mismatch limit counts unique terminal attempts per required slot;
  wrong-size failure is a mismatch, matching-size failure restores immediately.
  A validated callback awaiting UI geometry is pending, not an absent callback.
- Recovery versus release is captured in each action. A release arriving during
  recovery remains queued; owner replacement invalidates the old in-flight
  action. Unchanged submission sequences do not dirty UI presentation every
  frame; visible state changes do.
- T09 reserves at most five flat status lines and increases line storage to 26
  for ten settings, the existing warning and status. Flat entry pages use the
  existing compact pitch; VR visible row limits stay unchanged. Native GDI
  layout checks prove the combined card fits the raster budget. Overflow is
  explicitly directed to detailed status/logs, never presented as complete.
- Shared helper files added for specific production/test seams include
  `fsr4_dispatch_status.h`, `menu_fsr4_quality.h`,
  `game_render_scale_status.h`, `ui_game_resize.h` and
  `native_temporal_output.h`; they are not alternate implementations of their
  production behaviour.
- Native UI fixtures match the installer's per-monitor DPI awareness and use
  scratch settings only. Baseline stale test expectations for the retained
  jitter row and a temporal rig's seventeenth channel were corrected without
  removing the feature or increasing the production pool.
- No remote push is authorised. T11 ends with local commits and matching build
  identity. T12 and live-flight evidence in T13 remain for testing with David
  in the morning; no installation or game launch is part of overnight work.

## Requirements and boundaries

| ID | Requirement / reason | Observable acceptance |
|---|---|---|
| R1 | One quality selection shared by ED-UP's native settings window and F8; avoid competing configuration owners. | The same profile file yields the same preset and custom scale in both UIs. |
| R2 | Change Elite's scene resolution live in both directions. | Low-to-high and high-to-low requests cause Elite's normal target rebuild without a restart or Graphics Apply. |
| R3 | Preserve output resolution, FOV and headset/display settings. | Measured FSR output remains unchanged during a quality-only transition; no display-resolution or OpenXR-resolution writes. |
| R4 | Use defined FSR ratios and actual game arithmetic. | Presets use full-precision scalar ratios, game float multiplication and truncation; actual sizes are displayed. |
| R5 | Preserve manual game control and separate flat/VR settings. | Default `game` writes no scale; releasing managed control restores the latest genuine game selection where ownership is still valid; flat edits never modify VR settings. |
| R6 | Fail visibly and safely. | Bad inputs, unsupported game build, invalid pointers, impossible game bounds and failed FSR submission cannot become an applied claim or repeated rebuild loop. |
| R7 | Distinguish desired settings from successful FSR work. | Current-epoch successful submission at expected dimensions is required; both VR eyes are checked independently; world-route evidence is separate. |
| R8 | Preserve current VR SS hold, UI sizing, history/resize handling and non-FSR engines. | VR ordinary SS remains 1; HUD/panel sizing follows the effective scene scale; existing regression rigs and controlled flights pass. |
| R9 | Respect game/render-thread ownership and lifecycle. | Only validated game callbacks change game fields; no GPU work in that callback; failed/TEST/foreign-thread Presents cannot acknowledge work. |
| R10 | Keep the flown baseline and live Steam installation safe. | Candidate stays on `fsr4-1-2`; only the verified development copy is installed later; no Steam files, registration or launch settings change. |
| R11 | Deliver reviewable, tested source. | Full native validation precedes granular commits; exact candidate and receipts are recorded; fork push is verified without rewriting flown history. |

The control changes **internal scene rendering**, not the Windows desktop size,
the headset's refresh rate, SteamVR's resolution, or ED-UP's existing
`fix.openxr_resolution` control. That existing overlay row fixes base per-eye
output through `NativeRuntimeHost::captureRenderSettings/applyRenderScale` when
the VR session/swapchains start, which explains its restart requirement. Mip
bias is a separate limitation. The proposed FSR control changes input
resolution against that output. MinEdLauncher continues to authenticate and
start Elite; “launcher settings” here means ED-UP's native settings window.

Excluded: frame generation, a Vulkan/D3D12 renderer replacement, VR startup
error 124, changing FSR provider/version policy, changing the jitter
experiment, a new installer architecture, and changing other engines' quality
policy. Existing startup-latched texture LOD bias is documented as a
limitation, not silently represented as a live sampler update. This plan
delivers a development candidate and its source, not a public release. The
selector adds no manual end-user setup step; public FSR-runtime packaging
remains subject to the existing one-click installation requirement.

## Evidence and inspection

All source findings below refer to the inspected commit. Source inspection is
not proof that the proposed hook has executed in a game session.

| Owner / symbols | Established fact |
|---|---|
| `src/common/config.h/.cpp`: `chooseSettingsPath`, `switchProfile`, `parse`, `generation`, `getFloat`, `requestedTemporalMode` | One whole active-profile file, hot reload, shared-lock getters, generation counters and runtime AA overrides already exist. Do not add another configuration store. |
| `edvr.ini`, `tools/gen_settings_schema.py`, `flat_profile_keys.h`, `menu_flat_rows.h` | INI annotations and real runtime readers generate both UI schemas. Flat rows and allowed keys are explicit. |
| `menu.cpp`: `applyChange`, `enqueueChange`, `writerMain`, `menuIniWritePairs`, `menuProfileSwitch`; `menu_write.h` | Writes are asynchronous and optimistic. Jobs currently lack immutable profile/path identity; execution resolves the then-current Config path. Flat fallback can therefore write the shared VR file. |
| `settings.cpp`: `SettingRow::parseTyped`, `SettingsModel::set/currentText` | Native settings reread and atomically merge the file; the first flat write seeds its own file from the shared file. Keep those semantics. |
| `settings_view.cpp`: `paintRow`, `onClick`, `applyValue`, `commitEdit` | Choice rows use narrow segmented controls. Seven preset labels need a full-label popup. Dependencies must be checked on write, not only when laying out the list. |
| `menu_schema.h`: `menuFileNumber`; `SettingRow::parseTyped` | Existing two-/four-decimal formatting cannot round-trip one-third. New custom-scale values need their own shared parser/serializer. |
| Preserved build-334925 executable | Static registration, callback wrapper, requester entry, dirty check and configure chain were traced. The wrapper passes renderer in RCX and a float in XMM1. The requester consumes the dirty flag before resizing. Exact pins belong in production source and `docs/build-334925.md`, not duplicated here. |
| `vr_window_trampolines.cpp::copyReplacement`, `vr_ssaa_hold.cpp`, `ui_panel_scale.cpp::hookedSetter` | Settings-copy and SS-setter observation already exist. The VR hold must receive the genuine requested SS value before any flat override. |
| `native_temporal.cpp::treat`, `nativeTemporalAsked`, `nativeTemporalRecommended` | VR's FSR output uses treated geometry, including settled FOV trim. Existing 98% threshold suppresses near-native upscale. Requested and treated recommendations can differ during adoption. |
| `fsr4_engine.cpp::makeContext/fsr4Evaluate/fsr4ReleaseFeatures/fsr4StatusLine` | Current status records context size before dispatch. `ffxDispatch` success can still be followed by bridge failures. Only final `fsr4Evaluate == true` proves submission/output-copy queuing. |
| `device_hook.cpp::hookedPresent/presentFrameBoundary`, `render_boundary.cpp`, `mode_watch.cpp::commitPosted` | The ordinary frame-boundary function runs even for failed/TEST Presents. Profile teardown, Config switch and reload have a defined order. Existing owner records can establish the correct thread without acquiring another OpenXR lease. |

Complete bounded reads were performed across the responsible reviewers for:
`config.h/.cpp`; `menu.cpp`; `menu_schema.h`, `menu_write.h`,
`menu_edit_hold.h`; `flat_profile_keys.h`, `menu_flat_rows.h`; installer
`settings.h/.cpp` and `settings_view.h/.cpp`; `gen_settings_schema.py`;
`fsr4_engine.h/.cpp`, `fsr4_version.h`; `device_hook.cpp`;
`render_boundary.h/.cpp`; `mode_watch.h/.cpp`; `native_temporal.cpp` and
`src/common/native_temporal.h`; `vr_window_trampolines.h/.cpp`;
`vr_ssaa_hold.h/.cpp`; `ui_panel_scale.h/.cpp`; `ui_surfaces.h/.cpp`;
`code_hook.h/.cpp`; the existing `weapon_ui_getter_probe.asm`; and the engine,
flat-panel, menu-write and actual Present test rigs. The large INI, build
recipe and existing installer test file were inspected at their relevant
declarations, callsites and test sections; this is not a claim of a
whole-repository review. Generated schema, vendor implementation, unrelated
shaders and unrelated installer scenarios are excluded from hand editing. SDK
quality constants were checked in the pinned local
`third_party/fsr4/include/FidelityFX/upscalers/include/ffx_upscale.h`.

The original local executable is not redistributed. Its SHA-256 is
`ed5a8f8cd10f9a80d3bda5badd7ba1d4fcfc15996c8c7e018852cb170e264b63`. The earlier
provisional report that the requester had no references was corrected during
inspection: a scan had not completed. Completed inspection established
registration and the wrapper's tail call. No unresolved xref choice is deferred
to delivery.

## Frozen design decisions

### D1 — Settings, values and precedence

Add `[fix] fsr4_quality = game` with values `game`, `native`, `quality`,
`balanced`, `performance`, `ultra_performance`, `custom`, and `[fix]
fsr4_render_scale = 0.75`. Existing `fix.temporal_aa = fsr4` selects the
engine; these settings select its input scale. Missing preset means Game.

Both keys are per profile. Flat reads `edvr-flat.ini`, falling back to the
whole VR file only while the flat file is absent. A flat write creates the flat
file, preserving the VR file. Native settings and F8 use the same value
contract.

`src/common/fsr4_quality.h` owns `Preset`, `Request`, `parsePreset`,
`parseCustomScale`, `formatCustomScale`, `steppedCustomScale`, `scaleFor`,
`scaledDimension`, `controlsEnabled` and the templated `readRequest`. Custom
scale is a raw fraction in `[1.0f/3.0f, 1.0f]`; UI arrows add `direction * 0.01
* multiplier` then clamp to those endpoints (multiplier 1 normally, 5 with
Shift); do not use generic grid snapping. Persist with `max_digits10` float
precision, independent of display rounding. Reject suffixes, NaN, infinity and
out-of-range typed values without writing. Missing/empty persisted custom uses
0.75; empty typed input refuses. Malformed remembered custom refuses Custom
activation but does not block Game or named presets. Unknown preset is
unmanaged and visibly invalid, not silently mapped to Quality.

Runtime reading uses `getString("fix.fsr4_render_scale", "0.75")` and the same
strict parser as both UIs; Config's numeric fallback must not hide malformed
Custom input. Add the explicit INI annotation `| type number` to this key.
`gen_settings_schema.py::Setting/apply_annotation/run` gains a validated
numeric presentation override for a real string reader: permit String-to-Number
or an already numeric reader, reject Boolean/Hotkey/choice conflicts, require
finite numeric bounds with `math.isfinite(float(value))`, set floating
precision 2 rather than String precision 0, and preserve inference for every
unannotated key. It changes presentation metadata, not runtime parsing. Do not
add a dummy numeric getter. Read Config generation before/after the related
values; accept a request only from one unchanged generation. Retry once; if it
changes again, retain current control and report pending until the next
boundary.

| Preset | Per-axis scale | 5120×1440 example | 2528×2704 example |
|---|---:|---:|---:|
| Native AA | 1 | 5120×1440 | 2528×2704 |
| Quality | `1.0f / 1.5f` | 3413×960 | 1685×1802 |
| Balanced | `1.0f / 1.7f` | 3011×847 | 1487×1590 |
| Performance | 0.5 | 2560×720 | 1264×1352 |
| Ultra Performance | `1.0f / 3.0f` | 1706×480 | 842×901 |
| Custom 0.75 | 0.75 | 3840×1080 | 1896×2028 |
| Custom 0.99 | 0.99 | 5068×1425 | 2502×2676 |

These examples use float multiplication followed by truncation, matching the
game's sizing instructions. They are predicted input sizes, not proof of
successful SDK operation. Do not replace ratios with 0.67/0.59/0.33, force even
inputs, or invent divisibility restrictions from the Discord hypothesis.

### D2 — One CPU controller, with two existing scale paths

Create `game_render_scale.h/.cpp` plus pure `game_render_scale_policy.h`,
`game_render_scale_pins.h` and checked `game_render_scale_binding.h/.cpp`. The
binding module owns `GameImageView`, `GameScaleBinding`, `resolveBindings`,
`readLiveOwner` and `applyScaleAction`: explicit external game image/object
inputs, production identity/pin validation and guarded memory operations. It
has no Config, UI, FSR or GPU dependency. The runtime facade supplies real
image views and calls it after panel preparation; the native rig supplies a
synthetic external game image and executes exactly those checks/writes. This
separation makes the game boundary testable without replacing internal
collaborators. The controller owns desired requests, saved genuine game values,
effective writes, renderer identity, observation epochs and outcome. It owns
**flat SS** and **VR HMD quality** only. The existing VR ordinary-SS hold
continues to own SS=1 and retains all install, profile and shutdown calls.

Flat application validates the live context and its SS min/max, sets its SS
field using the game's verified clamp semantics, and marks the renderer dirty.
Valid bounds are finite, ordered and nonnegative; `0..FLT_MAX` is valid and was
observed in existing flights. Do not invent a maximum-bound cap such as 4 or 8.
The exact current-build bound initialisers are not assumed: runtime checks are
part of the fixed adapter contract. If the requested scale is outside the
game's valid bounds, refuse before writing; never silently run Performance
under an Ultra label. VR application updates the context HMD value and calls
the verified manager setter, then marks the same dirty flag. Determine mode
from the live copied game state; mismatch with the active ED-UP profile defers
the request. Unknown modes refuse.

Use the registered requester entry immediately before the game's dirty check.
Do not call the full configure routine from Present, from a UI thread, or
through a cached renderer pointer. Revalidate current renderer/context/manager
each callback. New object identity invalidates old snapshots and
acknowledgement.

Initial support is the verified game build only. A different executable
identity or any failed instruction/vtable/field witness refuses this feature
with a specific reason while ordinary game control remains available. Do not
expand signature support opportunistically during implementation.

### D3 — ABI, ownership and locking

Use new `game_render_scale_entry.asm`, not a guessed integer-only C++ function
signature. It preserves original flags, volatile integer registers and FP/SSE
state, invokes a CPU-only callback, restores the incoming machine state, then
tail-jumps through the prepared original trampoline. Preserve flags before any
flag-changing stack arithmetic; use valid unwind metadata, aligned FXSAVE
storage and an ABI-preserving epilogue. `CodeHook::Prepare` publishes the
original target before the patch becomes callable. The callback is `noexcept`;
its guarded memory access does not encompass execution of the original game
routine.

The settings/UI owner publishes immutable intent. The game callback alone
executes checked game-field actions. The D3D render owner alone probes FSR,
consumes submission evidence and performs any existing engine lifecycle work.
Small state locks protect snapshots only; never hold one across game functions,
GPU calls, file IO or logging. Coalesce requests to the newest sequence. A
result from a superseded request may describe history but cannot overwrite
current state.

Add `renderBoundaryOwnsPresentThread(device)` using the existing
canonical-device and first-Present thread records under their existing mutex.
Do not create another thread owner or take the single OpenXR boundary lease.
Controller acknowledgement requires `hr == S_OK`, no `DXGI_PRESENT_TEST`, the
canonical device and its thread. An occluded, failed or foreign Present does
not advance acceptance.

### D4 — Baselines, manual changes and UI sizing

In `copyReplacement`, observe fresh game settings after the original copy and
before `vrSsaaHoldAfterCopy`. Record genuine manual values for the current
owner. In `ui_panel_scale.cpp::hookedSetter`, retain `requested`, pass that raw
value to `vrSsaaHoldSetterValue`, then pass `(context, requested, held)` to
`gameRenderScaleFlatSetterValue`. Only a verified managed-flat context may
replace the held result. The UI preparation and original setter receive the
effective value. Passing the override to the VR hold first would corrupt its
saved value.

Prepare UI panel sizing **before** Elite rebuilds. Add
`uiPanelScalePrepareGameResize(plan)`, reusing the existing flat/VR panel
formula, budgets, factor lock and flat-settle epoch. It accepts intended scene
dimensions and verified effective HMD/SS, rather than fabricating a setter
object. If UI resizing is inactive it is a no-op success. If active and
required geometry is not available, keep the resolution request pending. Do not
resize with a known stale UI factor. The HMD value consumed by UI sizing must
prefer the controller's validated live snapshot; the .fxcfg cache remains the
fallback when no live snapshot exists. Specifically, rename the file-cache
accessor to `hmdFileCached` and add `effectiveHmdQuality`; both
`ui_surfaces.cpp::readBasis` and `uiSurfacesHmdQuality` use it. Keep the
background file cache separate from the controller's atomic observed HMD value.
Publish after actual checked writes and restoration, and also observe genuine
unmanaged values; invalidate on owner/mode loss and shutdown. CreateTexture2D
readers perform no IO or pointer dereference.

`uiPanelScalePrepareGameResize` uses `UiFlatPanelInputs/uiFlatPanelPlanFor` for
flat intended render dimensions versus measured swapchain output. VR reuses
`gatherInputs/uiPanelPlanFor` with explicit intended HMD/SS: game-facing
intended sizes supply the UI scene basis, and raw runtime output supplies the
existing panel-output basis, including the existing frustum terms. Do not
confuse that raw panel-output basis with treated FSR output. Use `writeFactors`
under the existing `g_floatLock`, and the existing `PanelEnv::noteMove`/flat
setter epoch to hold factors until measured dimensions settle. Add a transition
generation to VR's frame-boundary factor write: sample before gathering,
recheck under the same float lock, and skip a stale or in-progress transition.
Start the transition before preparation, finish after game stores and
observed-HMD publication; retain a receipt containing previous panel divisors
and generation. If no game field was written, conditionally revert only this
transition's factors under the same lock. If some writes succeeded, publish
verified actual state, invalidate acknowledgement, queue checked restoration
and report partial failure; never just clear the transition and revive a stale
snapshot. Frame-owned bookkeeping remains frame-owned.

Selecting Game, disabling FSR4 or changing profile releases ownership at the
next validated game callback. Restore only fields still equal to our last write
and belonging to the current verified object; a newer genuine game selection
wins. Observation invalidation must retain one bounded outgoing release-only
receipt. At the next callback restore only through the currently validated
matching object and fields still equal to our writes; if replaced, discard the
receipt without touching old addresses. New managed writes wait until release
is handled. Never restore through stale addresses. No direct XML/.fxcfg edits
are needed. Elite may itself save its effective graphics settings when the
player presses Apply; this feature does not promise byte-identical game
settings after that.

### D5 — Resize completion and failure

States: `Game`, `PendingGame`, `PendingBoundary`, `AwaitingFsr`, `Applied`,
`Refused`, `Restoring`. A saved UI value is only a request. A game field write
is only a rebuild handoff. A successful matching FSR submission is a separate
fact.

After a write, immediately invalidate old acknowledgement. Arm the new engine
observation epoch only at the next eligible owned render boundary, after the
handoff, so an in-flight old frame cannot be accepted merely because the
selected preset changed. Required observations must match expected input and
output, request, profile and owner generation. Old-epoch results never
acknowledge a new request, including same-size A→B→A changes.

On preflight refusal write nothing. On a failed matching FSR attempt, queue one
restoration to the last confirmed scale for this owner, or the saved game scale
if no managed scale was confirmed. Keep the selected request visible as
refused. Do not retry it every frame. A new user request, a new valid
output/owner identity, or explicit Game→preset selection starts a new attempt.
Restoring is not marked complete until the game callback has performed the
checked restoration.

Do not use a wall-clock timeout to misclassify a paused game, loading screen or
map with no FSR dispatch. After 120 eligible owned boundaries without the game
callback consuming a queued request, show `Refused: game resize callback not
observed`; no game write occurred. After a handoff with no FSR work, show
`Awaiting FSR frames`. Repeated eligible FSR attempts at unchanged wrong sizes
are a mismatch: after 120 such attempts, restore once and report the mismatch.
Consume each `(epoch,slot,attemptSequence)` once; rereading a snapshot is not
another attempt. These are fixed control limits, not new user-facing settings.

### D6 — FSR observation is submission evidence, not a label

Keep provider identity in the existing served-provider code. Add neutral
`fsr4SetObservationEpoch(uint64_t)` and per-slot `Fsr4DispatchStatus` in
`fsr4_engine.h/.cpp`. Status includes epoch, attempt sequence, attempted sizes,
latest outcome/reason and separately labelled last successful submission.

The public `fsr4Evaluate` becomes an observation wrapper around the current
implementation. Begin captures epoch/sequence under the status lock. Every
return records success/failure, even when caller's `why` is null. Success is
recorded only after the existing final true return path, including queued
output copy, consumer signal and flush. This proves submitted work, not GPU
completion. Epoch changes, release and shutdown invalidate current success. A
stale completion cannot overwrite a newer attempt or epoch. Do not hold the
status lock across GPU operations or invoke controller callbacks while holding
it.

Flat needs slot 0. VR eyes need slots 0 and 1 independently. Slot 2 is reported
as the VR screen/world route and cannot substitute for either eye. World-only
scenes may therefore say “VR screen submitted; awaiting eye dispatch” without
claiming both eyes were upscaled. `layerOnlyTreat`, fallback TAA and successful
context creation never acknowledge FSR. Same-size preset changes invalidate
observation but need not reset unchanged temporal history; dimension changes
retain the existing reset/recreation path and profile switches retain feature
teardown.

### D7 — Output and near-native policy

VR ratios refer to the current treated FSR output recommendation, including
settled FOV trim, which may differ from the headset swapchain. Keep those
separate in evidence. Compare `nativeTemporalAsked` and
`nativeTemporalRecommended` during adoption; do not announce a settled ratio
while they disagree. Actual dispatch dimensions are the final authority for
each slot.

In `native_temporal.cpp::treat`, FSR4 selects recommended output whenever
upscaling is enabled, both input axes are no larger than the recommendation and
at least one is smaller. Equal size uses the existing 1:1 convention. Other
engines keep the existing 98% threshold and NVIDIA floor handling; XeSS's
separate `jitterCycleFor` condition is unchanged. Downsampling is not
introduced.

### D8 — UI and persistence contracts

Both rows remain visible. Quality is editable only with FSR4 selected; Custom
scale additionally requires Custom. Every mutation route, including reset and
typed commit after an external change, rechecks the same shared predicate. The
native settings window uses a full-label popup, not seven tiny segments. F8
shows a short requested/outcome line and actual dimensions; flat has no hover
tooltips, so refusal reasons must be visible without a tooltip. Refresh on
status revision in both menu paths, without requiring navigation.

Add common `SettingsWriteTarget` in `src/common/settings_write_target.h` with
profile, profile epoch, destination path, fallback source path, destination
leaf and seed permission. `Config::captureWriteTarget()` captures coherent
identity under one shared lock; Config stores that profile identity and
increments a separate profile epoch on every `switchProfile`, including
Flat→VR→Flat. When flat is reading the shared fallback, the destination is
`edvr-flat.ini` in that selected source directory. Extend every WriteJob
destination with this target, including pairs, plugin settings and addon
approvals; preserve their existing already-active side-effect/failure
semantics. Coalesce only jobs with the same target and row identity.
`readWriteSource(target)` rereads the destination if it exists; only absence
plus explicit seed permission allows reading the captured whole-file source. An
existing unreadable destination is a failure, not permission to fall back. Put
`readWriteSource` and `writeSettingsPairs(target,pairs,sideEffects)` in new
`src/common/settings_write_target.cpp` beside the value header. The real worker
and real-file tests use this implementation, reusing iniedit parsing, merge and
atomic write. An explicit side-effects interface supplies existing
backup/mirror operations using test-owned directories in tests; it never
replaces source selection or the writer. Preserve unrelated edits, back up an
existing destination once per destination and mirror the captured leaf. Worker
code must not query live Config/profile identity. An unbound target refuses.
`Config::completeWriteTarget(target)` may repick/reload only when its profile
and epoch still match. Successful seeding repicks the path within the same
profile, without clearing runtime AA overrides or incrementing the profile
epoch. An older failure cannot undo a newer row edit. The paired-setting
`aimPromptWriteDone` completion must also use originating identity. New-key
rejected edits carry explicit UI error state: the flat footer cannot infer
saved merely from nonempty lastWrite. Native invalid typing stays visibly
invalid. SettingsModel::set revalidates the new keys/dependencies against its
freshly reread document before backup/merge. Native dependencies use persisted
AA; F8/controller also honour runtime overrides such as F7. Do not wait for
disk IO on the render thread during a profile switch. Apply this destination
correction to built-in writes through the existing writer, not a second
FSR-only persistence system.

### D9 — Lifecycle and compatibility

Install once in the normal device-hook path after normal vScreen installation,
before flat temporal startup; do not install in sentinel recovery, graphics-off
or probe-only paths. Installation does not change game scale. Before publishing
any permanent entry patch, require `graphicsModulePinned()` from
`src/common/module_pin.h` to be true. A failed pin refuses this feature without
modifying game code; retaining a trampoline alone cannot keep an unloaded DLL
mapped.

Add a controller lifecycle tick immediately after mode-watch commit and before
the graphics-disabled early return. Add config/observation processing at the
end of `presentFrameBoundary`, after its existing config refresh. Pass real
Present eligibility from `hookedPresent`; do not reinterpret its unconditional
frame counter as proof of an eligible frame. In `mode_watch::commitPosted`,
invalidate the outgoing controller alongside the existing teardown and then
publish the new profile after Config switches. Keep every existing VR hold
call.

Shutdown may run under loader lock on another thread. Close write admission and
clear observation/HMD snapshots without GPU waits, IO, game rebuilds or
cached-object writes. Retain entry patch, relay, forwarding pointer and
trampoline for process lifetime in intentionally retained hook storage. Do not
call CodeHook::uninstall or its destructor: it frees a trampoline an in-flight
bridge may still use. The pinned module safely continues forwarding. Do not
promise restoration during abrupt process termination. Ordinary Game/AA/profile
release is the supported restoration path.

## Fixtures and verification layers

| Fixture | Provenance / destination | Purpose and limits |
|---|---|---|
| Three original dispatch lines | Source `edvr_gfx_20261125_012645.log`, SHA-256 `d1fe76b57a83dcfd115e9294aed51560ad8fd43227b18ea2c43eb19d913266c9`. Future `tools/fsr4_quality_test/fixtures/quest3-scales.log`. Select first `fsr4: slot=... dispatch` line with rc=0 for each input 2528×2704, 1264×1352, 1896×2028, in that order, all output 2528×2704; UTF-8, LF, terminal newline. Derived-subset SHA-256 `a3247a48050449f9d08432d6d4f2fe25045cd3e098601a4d2b27f4f62d7d5000`. | Real dimensions and provider strings. Sparse logging does not prove both eyes at every scale or bridge completion. Do not use it as such. No account, journal, commander or launch-token data. |
| Ratio/boundary cases | Synthetic cases in `fsr4_quality_test.cpp`; include 2529×2705 odd base, 0.99 and tiny/invalid extents. | Exercise float/truncation, validation and serializer endpoints; not a hardware qualification claim. |
| Game image/object fixture | Synthetic executable allocation and object graph in `game_render_scale_test.cpp`, using production pin descriptors and controlled stub functions. Never commit the game executable. | Exercise actual pin checking, guarded access, writes, dirty marking and ABI forwarding. A fake external game adapter is allowed; monkey-patching internal production functions or bypassing guards is not. |
| Profile documents | Temporary real `edvr.ini` / `edvr-flat.ini` with comments, unrelated values and independent scales. | Exercise actual parsing, atomic replacement, fallback seeding and delayed writes. |
| Epoch/failure cases | Deterministic production policy state driven with explicit boundary and submission events. | Delayed success, failed resize, restoration, owner change and both-eye requirements. No mocked assertions that merely count expected method calls. |

No fixture files were created during planning. The proposed subset hash was
computed in memory. An initial attempt to select six lines, one per eye per
scale, found no sampled slot-0 dispatch at the half-size transition; the
fixture is deliberately three lines and does not manufacture that evidence.

Verification layers are distinct: CPU policy/parser tests; real file/Windows
ABI and Present integration tests; isolated real-Radeon FSR tests; full native
build; then the development game's flat and VR acceptance. Offline tests do not
certify visual quality or headset behaviour. No live test or full build was run
during planning. Only source inspection, CLI help, arithmetic checks and hashes
ran.

## Source review and complexity budget

Revision 3 was reviewed against unchanged source commit
`f0b40e6c9b0a7ad14e92fec97af15b863e00d993` and `CODING_STANDARDS.md`. Two
concrete gaps were corrected: T02 now exercises real Config epochs and the
production coalescing predicate, and T04/D9 require successful module pinning
before publishing a permanent hook. This is source/design review, not passing
implementation, ABI, GPU or live-flight evidence. Prospective new rig commands
remain delivery contracts, not claims that those executables already exist.

Keep the value parser, pure controller policy, checked external-game binding,
runtime coordinator and UI presentation separate. Reuse existing Config,
CodeHook, INI merge, panel math and build owners. Add no generic event bus,
plugin system, dependency-injection container or test-only alternate renderer.
The external game-image fixture and file side-effects seam serve concrete
unsafe boundaries; they must remain small and specific.

Tests must distinguish a plausible failure from correct behaviour. Extend
existing rigs wherever practical; no new test framework or duplicated state
machine in assertions. Keep exhaustive arithmetic/state edge cases on CPU. The
GPU matrix covers the specified dimensions, ratios, directions and slots; do
not cross-product every parser/profile/failure case with HDR, format and
hardware permutations. Retain existing representative GPU format/error cases.
Synthetic witnesses do not independently establish real game addresses, and
nonzero GPU readback does not prove visual quality. T12 retains those live
acceptance responsibilities. No new visual golden-image framework is required.

## OpenAI delivery assignments

David's 25 November instruction sets GPT-6.1-Sol at **medium** reasoning effort
as the delivery default, with the current parent assistant as orchestrator.
This revision assigns work only; execution remains unstarted and requires a
later instruction to proceed. Do not switch or respawn the parent merely to
match a historical model name.

Adapted from the mounted openSUSE skills:

- Codex:
  `/mnt/wsl/opensuse-source/@/home/davidmoore/.codex/skills/multi-agent-orchestration/SKILL.md`.
- Claude:
  `/mnt/wsl/opensuse-source/@/home/davidmoore/.claude/skills/orchestrating-multi-model-delivery/SKILL.md`.

These are role mappings, not claims that models have identical capabilities.
David's explicit model preference overrides the skills' generic tier defaults.
The source skills remain unchanged.

| Original role | OpenAI assignment | Use in this plan |
|---|---|---|
| Haiku: mechanical delivery | `gpt-6.1-sol`, `medium` | Keep one predictable worker default, including documentation and validation support; no Luna workers scheduled. |
| Sonnet: implementation and tests | `gpt-6.1-sol`, `medium` | All implementation tasks and delegated validation work. |
| Fable / Astra: orchestration and difficult judgement | Current parent assistant | Own design decisions, dependency admission, integration, review, escalation and final verification. |

| Task | Delivery owner / effort | Why this fits; orchestrator review focus |
|---|---|---|
| T01 — Values and arithmetic | GPT-6.1-Sol / medium | Exact ratios, parser contract and independent expected values are specified; review float endpoints and round trips. |
| T02 — Profile-owned writes | GPT-6.1-Sol / medium | Destination and epoch contracts are frozen; review races, fallback seeding, atomic persistence and old completions. |
| T03 — FSR submission evidence | GPT-6.1-Sol / medium | Observation fields and success boundary are explicit; review every return path, stale epochs and eye separation. |
| T04 — Checked game callback and ABI | GPT-6.1-Sol / medium | Binding and ABI requirements are specified; mandatory parent review of pins, assembly, unwind behaviour and retained hook lifetime before T05. |
| T05 — Controller and restoration | GPT-6.1-Sol / medium | States, ownership and failure rules are frozen; mandatory parent review of transitions, release receipts and one-shot restoration before T06. |
| T06 — Lifecycle, UI and frame ownership | GPT-6.1-Sol / medium | Call ordering and integration points are named; mandatory parent review of thread admission, partial writes, profile changes and shutdown before Phase C. |
| T07 — Near-native output | GPT-6.1-Sol / medium | Bounded policy change with a regression matrix; review isolation from other engines. |
| T08 — Native settings controls | GPT-6.1-Sol / medium | Shared parsing, schema and UI behaviour are defined; review fresh-document guards and readable controls. |
| T09 — Overlay and live status | GPT-6.1-Sol / medium | Editing routes and evidence requirements are explicit; review every mutation guard and requested-versus-applied wording. |
| T10 — Native validation | GPT-6.1-Sol / medium, parent gate acceptance | Worker runs the specified build and strict GPU gates; parent checks actual logs, receipts, source identity and skips. |
| T11 — Review, commits and promotion | Current parent assistant; Sol medium may collect evidence | Cross-task review, commit boundaries and exact-candidate promotion are integration responsibilities; parent owns staging, commits and push. |
| T12 — Development-game acceptance | GPT-6.1-Sol / medium, parent coordination and David's live observations | Fixed install/test sequence; parent admits target and readiness checks, announces rendering changes and assesses evidence with David. |
| T13 — Acceptance record | GPT-6.1-Sol / medium, parent final verification | Worker records observed results and limitations; parent verifies receipts, SHAs, remaining gaps and final fork state. |

### Delegation and review protocol

- Use `model="gpt-6.1-sol"`, `reasoning_effort="medium"` and
  `fork_turns="none"` for delivery agents. Supply the complete task record,
  relevant requirements/design decisions, canonical plan path, applicable
  workspace/repository rules, prerequisite evidence and owned file paths. A
  fresh agent must not depend on inherited conversation to know these rules.
- Use the parallel waves below, respecting prerequisite gates and exclusive
  file ownership. The parent keeps the dependency graph and file claims
  current. Up to three delivery workers may run beside the parent. Do not make
  every task sequential merely because some shared files need serial
  integration.
- Every worker writes the specified tests first, records the expected failing
  result, then implements and runs its gates. Report changed paths, actual
  commands/exit status, evidence paths, deviations and unresolved failures.
  Tests exercise production code against independently known outcomes.
- The parent reads the returned diff and gate evidence, runs the applicable
  verification and admits the next task. Agent assertions alone are not proof.
  Reviews focus on the risks named above; no additional test flight is added
  just because a different model reviewed a task.
- Workers do not independently commit, push, promote, install or launch. T11
  owns commits after native validation; T12 explicitly delegates the approved
  development installation and launches when David is ready. This preserves the
  plan's build-before-commit rule over the Claude skill's generic instruction
  for each worker to commit its own files.
- If a worker cannot satisfy the frozen contract, stop its affected work and
  return the evidence to the parent. The parent diagnoses and records the
  reason before any higher-effort/model assignment. Do not silently increase
  effort or substitute a larger worker; no larger worker is preassigned. Design
  changes follow the existing deviation policy.
- Keep per-task completion, red/green records and any escalation in
  `C:\Dev\EDUP-Validation\fsr4-quality-2026-11-25\delivery-status.md` and
  deviations in that directory's `deviations.md`, created only on execution.
  Before T10, reconcile the plan with accepted design changes and freeze its
  receipt-covered content. Subsequent routine completion records stay outside
  the checkout, as T10/T13 require. A material plan change requires matching
  revalidation; never silently invalidate a build receipt to update status.

### Parallel delivery waves and file ownership

David explicitly requests as much manageable parallel work as dependencies and
disjoint files allow. Phase labels describe acceptance groups; they do not
impose an extra barrier on independent tasks. In particular, T08 may run while
Phase B finishes because it consumes the already accepted T01/T02/T05
contracts. T09 still requires all of T03/T06/T07/T08. This changes scheduling
only, not the feature design or validation requirements.

| Wave | Tasks | Entry / exit and ownership |
|---|---|---|
| 1 | T01 | Establish shared values/tests before consumers start. |
| 2 | T02 + T03 | Both depend on accepted T01. T02 owns Config, settings persistence, menu writer/tests and its build recipe edits. T03 owns FSR engine/status and engine rig. Accept each result independently; complete Phase A before T04. |
| 3 | T04 | Checked binding/ABI and its rig; sole writer to controller skeleton and build recipe. Parent accepts ABI/lifetime evidence before T05. |
| 4 | T05 | Controller policy/facade and controller rig; freeze its public interface before the next wave. |
| 5 | T06 + T07 + T08 | All stated prerequisites are met. T06 owns lifecycle/game integration, UI sizing, Present/hold rigs and build recipe edits. T07 owns native temporal output policy and temporal rigs. T08 owns INI/schema generator, settings UI/model, flat key/row declarations and installer/flat settings tests. Each worker is GPT-6.1-Sol medium. |
| 6 | T09 | Begins only after parent acceptance of T06/T07/T08. Owns F8 editing/status and any remaining controller status formatter changes. |
| 7 | T10 | Freeze the combined candidate; one native full build and strict GPU qualification sequence. |
| 8 | T11 | Parent reviews, stages granular commits, promotes and verifies the fork push. |
| 9 | T12 | Coordinated development install, flat acceptance, then VR with David ready. |
| 10 | T13 | Sol records evidence; parent verifies final source/build/flight identities and completion. |

Before dispatching a wave, include exact file claims in each brief. The task's
listed Files/functions and TDD paths define the initial claims. Reading another
worker's files is allowed; editing them requires returning the change request
to the parent. Shared headers, generated files and build recipes count as files
for ownership, even for a one-line change. If a necessary edit crosses a claim,
queue that edit with the owning worker or parent and continue independent work;
do not allow competing writers or silently redesign the interface.

In wave 5, T05's controller header/API is frozen. T06 may implement the planned
integration definitions inside `game_render_scale.cpp`; T07/T08 consume the
accepted interface without editing it. T08's generator outputs have one writer
and must not race another task's schema generation. Workers use unique
task-owned temporary directories for test output and fixtures. Targeted tests
may run concurrently only when their output paths and resources are
independent.

Serialise shared native snapshot synchronisation and all native builds under
one parent-coordinated slot, in addition to `build.bat --wait-lock`. That build
lock does not protect source synchronisation. Do not copy a changing shared
worktree into the native snapshot: briefly quiesce writers when capturing a
wave snapshot, then validate that immutable snapshot and its recorded source
identity. Preserve each task's red/green evidence; T10 validates the integrated
final tree regardless of earlier per-task passes. GPU gates wait for David's
flight to finish. Commits, promotion, installation and game launches remain
serialised under T11/T12.

The parent can review a finished worker's diff while another disjoint task
continues. Use a free worker slot for read-only review when useful, with the
same Sol medium default. If all three slots are delivering, the parent reviews
directly rather than interrupting useful work. A failing task blocks its
dependants; unrelated authorised work may continue without weakening gates.

## Phase A — Contracts and independently testable foundations

Exit gate: values, persistence destinations and submission evidence have one
owner each and failing tests for the missing behaviour have been captured.

### T01 — Define quality values and exact scale arithmetic

- **Delivery owner:** GPT-6.1-Sol, reasoning effort **medium**; current parent
  assistant reviews and accepts the gate. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** create one value contract so launcher, overlay and renderer
  cannot disagree on Quality, custom bounds or decimal persistence. R1/R4/R6;
  D1. Prerequisite: inspected baseline unchanged, execution authorised.
- **Files/functions:** new `src/common/fsr4_quality.h` symbols listed in D1;
  new `tools/fsr4_quality_test/fsr4_quality_test.cpp`; new fixture and
  `fixtures/README.md` containing the selection/hash above. Add its CPU rig
  stanza to `build.bat`; do not edit generated schema by hand.
- **BDD:** Given Quality and 2528×2704, when dimensions are predicted, then the
  result is 1685×1802, with no forced even width. Given Custom at its minimum,
  when saved/read repeatedly, then it remains in bounds and identical as a
  float. Given NaN, a suffix or 1.1, when typed, then it is refused and no
  value is saved.
- **TDD:** write cases for every table row, unknown/missing preset, missing
  custom, malformed raw input despite Config's fallback, finite endpoints,
  lower-endpoint step/clamp/roundtrip, odd dimensions, zero and overflow
  rejection. Exercise the real shared parser and Config adapter, including
  generation change between reads (in the existing Config-linked
  `tools/config_test/config_test.cpp`, owned by T01 until its gate passes),
  empty typed/stored input, named presets with malformed unused custom, and
  endpoint/Shift stepping. Capture red results, implement only the specified
  helper, then rerun.
- **Verification:** `build\fsr4_quality_test.exe --self-test`; schema/config
  contract checks once readers exist. Evidence `T01-policy.txt` plus fixture
  hash.
- **Gate / failure:** all numerical and malformed-input cases pass. No
  rendering change exists yet; failed checks leave this task incomplete, not
  worked around.

### T02 — Make queued settings writes belong to their originating profile

- **Delivery owner:** GPT-6.1-Sol, reasoning effort **medium**; current parent
  assistant reviews and accepts the gate. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** a quality change queued in Flat must not land in VR after a
  switch; missing flat configuration must not overwrite the shared VR source.
  R1/R5/R9; D8. Depends on T01.
- **Files/functions:** new
  `src/common/settings_write_target.h/.cpp::SettingsWriteTarget/readWriteSource/writeSettingsPairs/sameWriteTarget`;
  `config.h/.cpp::init/switchProfile/captureWriteTarget/completeWriteTarget`
  and locked profile identity/epoch;
  `src/d3d11/menu_write.h::WriteJob/WriteDone`,
  `runMenuWrite/completeMenuWrite`; `menu.cpp::enqueueChange/enqueueWrite`,
  `aimPromptPick/aimPromptWriteDone`, `stepPluginSetting`,
  `approveAddonCandidate`, queue coalescing, `writerMain`, `menuIniWritePairs`,
  `drainWrites`, `menuProfileSwitch`, `menuNoteConfigReloaded`; existing
  `tools/menu_edit_hold_test/menu_edit_hold_test.cpp`; add the new common .cpp
  to both production and menu-write rig source lists in `build.bat`. Keep
  `SettingsModel::currentText/set` as the reference semantics, not another
  writer.
- **Real integration coverage:** extend `tools/config_test/config_test.cpp`
  using its existing `iniNameFixturesIsolated` child-process fixture route.
  Call real Config capture/completion across Flat→VR→Flat, old success/failure
  and fallback seeding; build-directory INIs must not contaminate that premise.
  Link `settings_write_target.cpp` and `src/common/iniedit.cpp` into both the
  Config and menu-write rigs where the real writer is used, preserving their
  existing Config/log linkage. Extract only the existing later-job coalescing
  predicate into `menu_write.h::menuWritesCoalesce`; production `writerMain`
  and the menu rig both call it. Cover same-key/different-target and paired
  writes without copying queue logic into tests. These files belong to T02.
- **BDD:** Given a Flat edit delayed on disk, when the active profile becomes
  VR, then only the captured flat destination changes and the VR display is not
  rolled back by that completion. Given no flat file, the first Flat edit
  creates one from the shared whole document and leaves shared bytes unchanged.
- **TDD:** use temporary files and the production job/persistence interfaces.
  Force deterministic queue ordering with an explicit delayed external
  persistence adapter, not patched methods; selection and file writes still
  execute the production .cpp. Test same-key different targets do not coalesce,
  concurrent unrelated edit survives, old failure cannot undo a newer edit, and
  failure after replacement is reported as unconfirmed. Add red tests before
  adding immutable `WriteTarget` data and source/target selection. Preserve
  held-edit batching and generation-aware rollback.
- **Verification:** `build\obj\menueditHold\menu_edit_hold_test.exe --self-test
  C:\Dev\EDUP-Build`; existing installer/config tests through full validation.
  Evidence `T02-profile-writes.txt` and before/after fixture hashes.
- **Gate / failure:** all profile and rollback assertions pass without blocking
  the render thread. No game files are involved; delete only test-owned temp
  data.

### T03 — Record successful FSR submissions and reject stale completions

- **Delivery owner:** GPT-6.1-Sol, reasoning effort **medium**; current parent
  assistant reviews and accepts the gate. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** context creation and provider selection cannot acknowledge a
  quality change. R6/R7/R9; D6. Depends on T01.
- **Files/functions:** `src/d3d11/fsr4_engine.h` add observation API/snapshot;
  `fsr4_engine.cpp::fsr4Evaluate` wrapper and internal implementation,
  `makeContext`, `fsr4StatusLine`, `fsr4ReleaseFeatures`, `fsr4Shutdown`;
  `tools/fsr4_engine_test/fsr4_engine_test.cpp`. Keep `fsr4_version.h` provider
  selection policy unchanged.
- **BDD:** Given a successful old frame, when a new-size attempt fails after
  context creation, then current status is failed with attempted size/reason
  and old size explicitly labelled historical. Given an old-epoch success
  arriving after a new request, then it cannot replace pending/current state.
- **TDD:** add `dispatchStatusSelfTest` for context-only pending, final
  success, same-size and resized failure, null `why`, epoch and per-slot
  ordering, release/shutdown invalidation and both-eye separation. Extend
  existing `flight`, `depthStencilRefusal`, `releaseAndRecreate` and rebind
  tests with snapshot assertions. Extend FlightSpec/gpuTest so strict
  --gpu-test runs all presets and Custom 0.75/0.99 at fixed 5120×1440 and
  2528×2704 output, descending/ascending inputs, and separate eye slots 0/1.
  Include odd inputs and existing output readback assertions. Introduce the
  wrapper only after capturing failing assertions. Test state transitions
  through production reducers; real GPU error paths use invalid resource
  inputs, not fabricated SDK success.
- **Verification:** engine rig `--self-test`, later strict `--gpu-test` on
  Radeon. Evidence `T03-dispatch-status.txt`; inspect every evaluate return
  path.
- **Gate / failure:** no status path claims GPU completion; current failure
  beats historical success; no lock spans GPU work. Engine behaviour beyond
  observation remains unchanged.

## Phase B — Checked game integration and controller lifecycle

Exit gate: native synthetic-game, ABI, ownership and state tests pass before
any candidate is offered for a real flight.

### T04 — Bind the verified game resize callback without guessing its ABI

- **Delivery owner:** GPT-6.1-Sol, reasoning effort **medium**; current parent
  assistant reviews and accepts the gate. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** invoke the game's own resize on its own callback, with no
  guessed direct configure call. R2/R6/R9; D2/D3. Depends on T01/T03.
- **Files/functions:** new `game_render_scale_pins.h` with named build
  identity, requester/registration/field witnesses and descriptors; new
  `game_render_scale_entry.asm`; `game_render_scale.cpp::installCheckedHook`,
  `validateLiveOwner`, `onGameUpdate`, `prepareOriginal`; declarations in
  `game_render_scale.h`; `docs/build-334925.md` records binding and retargeting
  evidence. New `tools/game_render_scale_test/game_render_scale_test.cpp` and
  `game_render_scale_entry_test.asm`. Link the binding module, policy and real
  CodeHook/bridge into this standalone rig below
  `build\obj\game_render_scale_test`, preventing accidental import of the
  production proxy. It does not replace the runtime controller's UI/FSR
  collaborators; integrated glue is exercised by the proxy/Present rigs and
  final game acceptance. Build recipe assembles the bridge and keeps its object
  through compile-cache pruning for production and its rig.
- **Pin admission:** `installCheckedHook` checks the existing production
  `graphicsModulePinned()` result before patch publication. Test the admission
  predicate with false/true inputs and exercise refusal before patching in the
  binding rig; do not add a runtime override to bypass pinning. Synthetic pin
  descriptors prove validator consistency only: retain independent disassembly
  provenance and corrupt individual witnesses to prove rejection.
- **BDD:** Given exact supported pins and a valid synthetic owner, when the
  requester is called, then arguments, stack, flags, FP state and return
  behaviour are preserved around the callback. Given one corrupted witness or
  unknown identity, then no entry patch or scale write occurs.
- **TDD:** first test the actual CodeHook prepare-before-publish and refusal
  paths, volatile/nonvolatile registers, XMM state/MXCSR, stack arguments,
  return value, alignment and stack unwinding. Include reentrancy, null/invalid
  object graph, changed manager vtable and lost ownership. Then implement the
  bridge and checked adapter. Pin validation and synthetic objects use the same
  production functions as runtime; no test-only switch disables game
  validation.
- **Verification:**
  `build\obj\game_render_scale_test\game_render_scale_test.exe --self-test` and
  `--abi-test`; pin fixture provenance in `T04-binding.txt`. Real-game
  invocation is reserved for T12.
- **Gate / failure:** unknown build passes through safely; no dereference of a
  cached object is used for writes. Hook preparation failure leaves game bytes
  unchanged; shutdown during an in-flight bridge retains valid forwarding
  storage, without destructing the hook or freeing a later chain's original
  target.

### T05 — Implement the request, confirmation and restoration state machine

- **Delivery owner:** GPT-6.1-Sol, reasoning effort **medium**; current parent
  assistant reviews and accepts the gate. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** make repeated changes, failed attempts and release of control
  deterministic. R2/R5/R6/R7/R9; D2/D5/D6. Depends on T03/T04.
- **Files/functions:** new `game_render_scale_policy.h::ScaleController`,
  `publishRequest`, `planGameAction`, `recordGameAction`, `onOwnedBoundary`,
  `observeSubmission`, `requestRelease`, `snapshot`; runtime facade
  `gameRenderScalePublish/FrameBoundary/Status/ProfileChanged/Shutdown` in
  `game_render_scale.h/.cpp`; extend `game_render_scale_test.cpp`.
- **BDD:** Given confirmed 0.75, when Performance fails, then one restoration
  to the confirmed scale is queued and the requested preset remains visibly
  refused. Given no scene dispatch while paused, then state remains awaiting
  frames rather than declaring failure. Given right-eye success only, then
  both-eye applied is false. Given Game selection, then the latest genuine
  baseline is restored only if the current field still belongs to this
  controller.
- **TDD:** write transition tables using explicit owner/profile/request IDs and
  deterministic boundary events. Cover up/down, no-op repeated choice, rapid
  A→B→A, stale results, frame admission, one-eye failure, world-only evidence,
  copy/settings changes, new owner, AA off and back on, invalid config and both
  fixed 120-count limits. Test one restoration, no rebuild storm and no
  old-owner write. Implement the policy and then connect the real checked game
  adapter.
- **Verification:** policy cases through `game_render_scale_test --self-test`;
  evidence `T05-state-machine.txt`, including expected/actual event traces.
- **Gate / failure:** there is no transition from saved request or
  context-created directly to Applied. Refusal preserves genuine manual state
  and its reason.

### T06 — Preserve manual settings, UI scale and frame ownership

- **Delivery owner:** GPT-6.1-Sol, reasoning effort **medium**; current parent
  assistant reviews and accepts the gate. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** connect the new owner without disrupting the VR SS hold,
  panel-quality logic, profile reload or shutdown. R2/R5/R8/R9; D3/D4/D9.
  Depends on T05.
- **Files/functions:** `vr_window_trampolines.cpp::copyReplacement` adds
  `gameRenderScaleAfterSettingsCopy`; `ui_panel_scale.cpp::hookedSetter` adds
  ordered `gameRenderScaleFlatSetterValue` and `uiPanelScalePrepareGameResize`;
  corresponding declarations in `ui_panel_scale.h`;
  `uiPanelScaleFrameBoundary/gatherInputs/writeFactors` gain the D4 preparation
  and transition-generation rules;
  `ui_surfaces.cpp::hmdFileCached/effectiveHmdQuality/readBasis/`
  `uiSurfacesHmdQuality` prefer the validated live HMD snapshot;
  `device_hook.cpp::hookDevice/hookedPresent/`
  `presentFrameBoundary/shutdownDeviceHooks`; `mode_watch.cpp::commitPosted`;
  `render_boundary.h/.cpp::renderBoundaryOwnsPresentThread`.
- **BDD:** Given a managed flat scene, when the player changes SS in Elite,
  then the raw game request is remembered but the managed value remains
  effective; choosing Game restores that raw value. Given a VR resize, then
  ordinary SS stays 1 and panel preparation sees intended HMD quality before
  any target rebuild. Given TEST or foreign-thread Present, then no new
  observation epoch is armed.
- **TDD:** extend `tools/vr_ssaa_hold_test/vr_ssaa_hold_test.cpp` and
  `tools/ui_quality_test/ui_flat_panel_settle_test.h`,
  `tools/ui_quality_test/ui_panel_budget_test.h`, plus new
  `tools/ui_quality_test/ui_game_resize_test.h` called from
  `tools/ui_quality_test/ui_quality_test.cpp`. Add ordered raw/held/effective
  tests and the controller rig; assert the saved raw value is not replaced by
  the managed scale. Test missing UI geometry defers, panel-budget handling and
  live-HMD/file fallback invalidation. Extend
  `tools/openxr_present_test/openxr_present_test.cpp` at its actual owner/TEST
  coverage; add first-Present, wrong device, wrong thread and terminal
  foreign-thread Present cases. Expose read-only
  `edvr_selftest_render_boundary_owner(ID3D11Device*)` in render_boundary.cpp,
  add its --extra-export in build.bat and GetProcAddress lookup in this
  dynamically loaded proxy rig. It calls the real query and never bypasses
  admission. Do not use renderBoundaryValidateOwner as thread proof. Add
  reused/replaced-owner VR→flat→VR, shutdown during callback, panel rollback
  before game writes and partial-write restoration tests.
- **Verification:** new rigs plus existing VR hold, UI sizing, Present and
  profile switch rigs in the full build. Evidence `T06-lifecycle.txt` with an
  ordering table. No GPU operations or file IO may originate from the game
  callback.
- **Gate / failure:** existing hold calls stay intact; nonblocking shutdown
  closes admission without attempting arbitrary-thread restoration. Any missing
  prerequisite yields pending/refused, never an unsafe fallback write.

### T07 — Keep FSR4 output fixed for near-native custom input

- **Delivery owner:** GPT-6.1-Sol, reasoning effort **medium**; current parent
  assistant reviews and accepts the gate. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** Custom 0.99 must mean a slightly lower input at the same
  output, rather than accidentally making the output smaller. R3/R4/R8; D7.
  Depends on T01/T05.
- **Files/functions:** `src/d3d11/native_temporal.cpp::treat` output-selection
  condition; extract only its pure condition into
  `src/d3d11/native_temporal_output.h::useRecommendedOutput` for direct
  testing; `tools/native_temporal_test/native_temporal_test.cpp` and its
  existing GPU companion. Keep `jitterCycleFor`, NVIDIA floor and non-FSR
  thresholds unchanged.
- **BDD:** Given FSR4 and input 2502×2676 at output 2528×2704, then output
  remains 2528×2704. Given NativeAA equal sizes, then it remains 1:1. Given
  input larger on one axis, then this change does not introduce downsampling.
- **TDD:** red cases for 0.99, equality, one-axis-only shrink, oversized input,
  every other engine's existing threshold and transient recommendation changes.
  Implement the pure policy and call it at the one production selection site.
  Confirm existing dimension-change history reset and fallback behaviour.
- **Verification:** native temporal CPU/GPU rig through the full build;
  evidence `T07-output-policy.txt`. No generic-success path acknowledges FSR
  submission.
- **Gate / failure:** other engines' output sizes are unchanged for the test
  matrix; output never claims a ratio during unsettled FOV adoption.

## Phase C — User controls and observable outcomes

Exit gate: both UIs produce identical requests and explain actual renderer
state without relying on hidden tooltips or manual configuration editing.

### T08 — Add annotated settings and native launcher controls

- **Delivery owner:** GPT-6.1-Sol, reasoning effort **medium**; current parent
  assistant reviews and accepts the gate. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** users need a readable preset selector and safe custom entry
  using the existing settings store. R1/R4/R5/R6; D1/D8. Depends on
  T01/T02/T05.
- **Files/functions:** `edvr.ini` annotations beside temporal AA;
  `flat_profile_keys.h`, `menu_flat_rows.h` additions;
  `tools/gen_settings_schema.py::Setting/apply_annotation/run` implements the
  D1 explicit numeric annotation and its existing `self_test` adds real
  mini-repo fixtures; `settings.cpp::` `SettingRow::parseTyped/shown`;
  `settings_view.cpp::paintRow/onClick/` `applyValue/commitEdit` with
  full-label preset popup and shared dependencies. SettingsModel::set adds
  fresh-document validation of new keys before backup/merge. Existing
  `tools/installer_test/installer_test.cpp::testSettings/`
  `testFlatSettingsWindow`; `flat_sharpen_test.cpp::panelTable` expects ten
  Performance rows with existing order preserved. Generator outputs are
  derived.
- **BDD:** Given FSR4, selecting Quality persists `quality` while leaving the
  custom value intact. Given another AA engine, both controls explain why they
  are inactive. Given Custom, entering 0.75 saves a fractional 0.75 in the
  selected profile; cancelling the popup writes nothing.
- **TDD:** use real SettingsModel with temporary files; cover flat fallback,
  external unrelated edit, failed save, invalid typing, reset and mid-edit AA
  changes. Generator tests cover numeric-string reader, malformed type
  annotation, missing real reader, incompatible Boolean/Hotkey/choices,
  absent/nonfinite bounds and unannotated strings remaining Text. Generate the
  schemas and assert actual rows/allowed keys and defaults, not just source
  text containing a label. Add native UI acceptance at supported DPI for
  readable full labels and immediate dependency updates.
- **Verification:** `python tools\gen_settings_schema.py --root . --check`,
  generator/config self-tests and installer/flat-panel rigs. Evidence
  `T08-settings.txt` and temporary-root UI captures without account data.
- **Gate / failure:** no MinEdLauncher or Steam settings change; no generic
  parser change alters unrelated number settings. Invalid save leaves prior
  state clear.

### T09 — Connect F8 editing and honest live status

- **Delivery owner:** GPT-6.1-Sol, reasoning effort **medium**; current parent
  assistant reviews and accepts the gate. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** change quality in-game and see whether Elite/FSR actually
  used it. R1/R2/R6/R7; D5/D6/D8. Depends on T03/T06/T07/T08.
- **Files/functions:** `menu.cpp::applyChange` central new-key guard;
  `stepRow/beginEdit/commitEdit/dispatchNav` use the shared parser and guard,
  including reset; `buildContent` adds requested/outcome/actual-size text and
  FSR-specific inactive messages and explicit edit-error/footer state so
  refused edits cannot show saved; `menuTick` marks content dirty when outcome
  revision changes in both flat and VR paths. `gameRenderScaleStatus` formats
  copied snapshots, not engine/game reads from the UI thread.
- **BDD:** Given a highlighted quality row, when game resizing is pending and
  then both eyes submit, the row updates without navigating away. Given a
  failure, the flat panel shows the reason without a tooltip. Given AA changes
  while a custom text edit is open, committing cannot write a newly inactive
  setting.
- **TDD:** extend shared guard/format tests and production menu-edit helper
  tests for step/reset/type/hold/reload interactions. Test statuses for
  left-only, right-only, world-only, stale-epoch, failed-after-success and
  restoration. Preserve generic menu edit-generation rollback and input
  handling.
- **Verification:** quality/menu rigs plus full-build UI tests; evidence
  `T09-overlay.txt`. Actual controller logs use one stable `render quality:`
  prefix with request/epoch/profile, requested scale, outcome,
  attempted/observed sizes and refusal reason. Log transitions, not every
  frame. Existing
  `tools/edvr_log.py --grep 'render quality:|fsr4:'` reads them; no new log tool.
- **Gate / failure:** saved/live wording never becomes Applied without required
  current-epoch FSR evidence. A refusal remains visible after further redraws.

## Phase D — Native validation and reviewable candidate

Exit gate: source, native build receipt, strict Radeon tests and commits
identify the same candidate. No game installation happens in this phase.

### T10 — Run full native validation and strict FSR tests

- **Delivery owner:** GPT-6.1-Sol, reasoning effort **medium**; current parent
  assistant reviews and accepts the gate. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** Linux syntax checks or a DLL-only compile cannot qualify the
  Windows hooks and bridge. R8/R9/R11; D3/D6/D9. Depends on T01–T09.
- **Files/functions:** finish `build.bat` production/ASM/rig lists and cache
  keep entries; no recipe bypass. Reuse `tools/build_receipt.py`,
  `C:\Dev\EDUP-BuildTools\edup-sync-native.py`, native environment/build
  wrappers. Every new executable supports `--dry-run` and `--self-test` and
  joins the full build gate. Tool versions already prepared: MSVC 14.44.35207,
  SDK 26100, Python 3.12.10; pinned FSR SDK 2.4.0/runtime 4.1.2.
- **BDD:** Given the final candidate and matching dependencies, native full
  validation produces a matching full-build receipt. Given an optional GPU rig
  that skips FSR, the strict Radeon gate still refuses qualification.
- **TDD / verification:** retain evidence outside checkout. Freeze plan and
  receipt-covered inputs: .agents and unignored validation files affect
  fingerprints, unlike docs. Changed receipt inputs require revalidation. Then
  sync the final source with Linux `python3
  /mnt/c/Dev/EDUP-BuildTools/edup-sync-native.py`. Run native
  `C:\Dev\EDUP-BuildTools\edup-native-build.cmd`, which calls absolute
  `C:\Dev\EDUP-Build\build.bat --wait-lock`. Capture the entire log and verify
  exit status and `build\full_build_receipt.json`; compare canonical/snapshot
  fingerprints. Run the built engine rig with `--gpu-test`, recording selected
  RX 9070 XT, driver 32.0.32029.2010, provider, and all ratio fixtures both
  increasing and decreasing. Read the actual executable location from the build
  stanza (`%OBJ%\fsr4_engine_test\fsr4_engine_test.exe`), not an assumed
  build-root path.
- **Gate / failure:** all mandatory gates pass without skipped new coverage.
  Optional hardware refusal is not success. Record baseline failures and replan
  affected work instead of weakening tests. Full GPU tests wait until David is
  finished with his running flight so they do not disrupt it.

### T11 — Review and commit the validated source in granular units

- **Delivery owner:** current parent assistant (orchestrator); GPT-6.1-Sol
  medium may collect review evidence. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** preserve the flown baseline and make the feature reviewable.
  R10/R11. Depends on T10 and later execution authority.
- **Files:** only the feature's explicit source/test/build/docs paths. Inspect
  `git diff --check`, changed regions and `CODING_STANDARDS.md`; never `git add
  .` in the dirty harness. No merge from main, rebase, squash or amend of flown
  history.
- **BDD:** Given complete validation of the final source, each commit contains
  one coherent implementation/test change and names what it achieves. The final
  tree matches the validated source; no attribution trailer is added.
- **Steps / tests:** stage explicit paths into logical units after the full
  build: `added FSR quality values to share render scale settings`; `fixed
  queued settings writes to keep their original profile`; `fixed FSR status to
  report completed submissions`; `added game resize control to apply FSR
  quality live`; `fixed near native FSR output to keep its target size`; `added
  FSR quality selectors to the launcher and overlay`; `added resolution control
  coverage to the native build`. Adjust split boundaries to keep each commit
  internally coherent without changing validated content. Write subjects to
  files and use `git commit -F`. After commits rerun the sync helper, including
  Git HEAD; verify native snapshot HEAD equals canonical candidate. Then run
  absolute receipt-guarded build.bat --dll-only through the wrapper and verify
  receipt. Without the second sync promotion would stamp the earlier dirty
  revision.
- **Gate / failure:** canonical worktree clean, frozen plan included in
  candidate, changing evidence outside checkout, exact HEAD/receipt recorded.
  Push only `origin/fsr4-1-2` normally to `pattern-state/ed-unofficial-patch`
  when delivery is resumed; verify remote SHA. A rejected push is reported;
  never force-push.

## Phase E — Development-game acceptance and evidence

Exit gate: actual flat and VR behaviour, UI quality and failures match the
specified contract. A passing build alone does not complete this phase.

### T12 — Install the candidate and test flat, then VR

- **Delivery owner:** GPT-6.1-Sol, reasoning effort **medium**; current parent
  assistant reviews and accepts the gate. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** demonstrate the new hook and actual render-target changes on
  the same hardware that flew the baseline. R2/R3/R6/R7/R8/R10; all design
  decisions. Depends on T10/T11 and David being ready to end his current
  session. Announce each rendering change before applying it. Do not force quit
  as a hidden step.
- **Target/prerequisites:** verify C: NTFS on WPBSN4M8-1TGP, GPT GUID
  `50454792-57ac-41e0-a069-20850d54fa3a`, resolved development path and
  reparse/ symlink/hard-link safety. Target exactly
  `C:\Games\ED-FSR4-Dev\Products\elite-dangerous-odyssey-64`; no Steam aliases.
  Verify Elite is closed and save the current installation/settings receipt.
- **Install:** native Python runs `tools\install_edvr.py --root
  C:\Dev\EDUP-Build --target
  C:\Games\ED-FSR4-Dev\Products\elite-dangerous-odyssey-64 --scope both --fsr4
  --dry-run`, then the same command without `--dry-run`, then `--verify-only`.
  Preserve tuned INIs: do not use `--ini`, `--force` or `--no-backup`. Check
  dry-run resolves the intended files and runtime paths. Use existing
  development Flat/VR shortcuts; verify the running executable path.
- **BDD / flat sequence:** at a repeatable stationary scene, select Game,
  Custom 0.75, Quality, Performance, Quality, Native, Custom 0.99, Custom 0.75,
  then Game. Check actual input/output, no output/window-resolution change, no
  restart or Graphics Apply, and restoration. Exercise resize/alt-tab, external
  profile-file edit, invalid custom typing, AA off/on and a real in-game SS
  change while managed. Unsupported game-bound presets must refuse visibly
  without changing scale.
- **BDD / VR sequence:** connect Quest3 through Steam Link/SteamVR before the
  VR launch. Verify actual VR startup rather than accepting `/vr` as proof.
  Repeat Native→0.75→0.5→0.75→0.99→Native and Game. Require fresh matching
  observations for both eyes in a cockpit. Test FOV adoption without changing
  trim as part of quality selection; exercise the VR screen/on-foot route with
  its separate status. Check HUD/panel crispness, cursor/panel size, both-eye
  consistency, transitions, maps, and ordinary SS held at 1.
- **Evidence:** `edvr_log.py --expect-build <candidate SHA> --version` then
  exact
  log reads for `render quality:|fsr4:` and relevant UI-sizing lines. Store source
  revision, build/install receipts, driver/GPU/runtime, headset and FSR-output
  sizes separately, requested/observed scales, per-slot outcomes and David's
  observations. The Samsung monitor's 144Hz and headset's 90Hz are recorded
  limits, not proof of achieved FPS. No promised framerate gain is an
  acceptance shortcut.
- **Gate / rollback:** complete each applicable scenario; report untested ones.
  On crash, wrong-eye size, persistent corruption or failed restoration, stop
  this candidate and restore the previously verified build using the install
  tool's receipt/backup flow. Do not patch the live install or use raw
  overwrites. Returning the overlay to Game is the normal non-crash rollback.

### T13 — Record acceptance and leave the branch ready to review

- **Delivery owner:** GPT-6.1-Sol, reasoning effort **medium**; current parent
  assistant reviews and accepts the gate. See the task-specific rationale and
  review focus in OpenAI delivery assignments.
- **What / why:** others must be able to distinguish compiled, submitted and
  visually flown behaviour. R7/R10/R11. Depends on T12.
- **Files/functions:** update `docs/fsr4-windows-flight-2026-11-25.txt` with
  the compact results/limitations; keep completion/deviation records in the
  external evidence directory without changing the receipt-covered frozen plan.
  Major deviation requires revised plan and matching validation. Keep private
  logs local. Record any supported-build restriction and the existing
  texture-bias limitation plainly.
- **BDD:** Given completed acceptance, the final report identifies the exact
  source and installed build, both profile results and any refused/unverified
  paths. A missing VR test remains missing, not inferred from flat success.
- **Verification:** verify clean intended worktree, receipt hashes and fork
  ref; commit the evidence separately (`added flight evidence to record live
  quality changes`) and push the fork branch. A final docs-only commit may move
  HEAD past the flown candidate; record both SHAs explicitly.
  Documentation-only changes do not justify repeating unrelated GPU tests
  unless the receipt/source inputs changed.
- **Gate / failure:** final report links the branch and evidence, distinguishes
  locally verified Windows from untested Proton, and makes no public-release or
  upstream-merge claim.

## Traceability

| Requirement | Decisions | Tasks | Acceptance evidence |
|---|---|---|---|
| R1 | D1,D8 | T01,T02,T08,T09 | Shared parser, schema, temporary-profile and UI cases |
| R2 | D2–D5,D9 | T04–T06,T09,T12 | Actual callback/dirty rebuild and live up/down observations |
| R3 | D7 | T07,T12 | Fixed output and unchanged FOV/display/headset settings |
| R4 | D1,D7 | T01,T07,T12 | Exact ratio table, odd-input and near-native tests |
| R5 | D1,D4,D8 | T02,T05,T06,T12 | Flat isolation, raw-value retention, Game restore |
| R6 | D2,D5,D6 | T01,T03–T06,T09,T12 | Refusal, no repeated rebuild, recovery and invalid-input cases |
| R7 | D5,D6,D8 | T03,T05,T09,T12,T13 | Epoch/slot-specific submitted evidence and honest text |
| R8 | D4,D7,D9 | T06,T07,T10,T12 | VR hold, panel sizing, history and other-engine regression |
| R9 | D3,D9 | T02–T06,T10 | ABI, thread, TEST/failed Present, lifecycle and shutdown tests |
| R10 | D9 | T11–T13 | Clean baseline, approved target checks and rollback receipt |
| R11 | All | T01–T13 | Red/green records, native receipt, granular history and remote SHA |

## Deviation policy

> Delivery agents must follow the recorded requirements, design decisions, task
> boundaries and acceptance criteria. They must not introduce new design
> choices or silently deviate. If an unexpected condition requires a deviation,
> record it with justification, impact, affected tasks and validation before
> proceeding where practical. Do not use deviation logging as permission to
> exceed user authorization.

**Minor:** a necessary implementation adjustment preserving the recorded
contracts, behaviour, scope and operational guarantees. Log it, update affected
task details and validate; it does not automatically require another approval.
**Major:** changes architecture, scope, public/data contracts, defaults,
permissions, lifecycle, retention, external effects or the test strategy. Stop
affected delivery, review requirements and amend the plan before resuming;
independent work may proceed only if it cannot prejudice that decision.
Repeated minor exceptions indicating a wrong assumption are a major planning
issue.

Before each task check prerequisite evidence and relevant source drift. Do not
revert someone else's work to make the plan fit. A future unsupported binary,
unmatched field witness or callback-order contradiction is a major deviation,
not permission to invent a different hook during delivery.

| ID/date | Task | Condition | Departure and justification | Class | Impact | Resolution/authority | Validation | Revision |
|---|---|---|---|---|---|---|---|---|
| None | — | Planning only | No implementation deviations yet | — | — | — | — | 1 |

## Handoff

The deliverable of this turn is this plan only. No feature code, build, commit,
push, fixture installation or live rendering change has been performed under
the planning request. Native toolchain preparation predates it and is not
full-build evidence. On a later instruction to execute, start with T01 and
follow the parallel waves and their prerequisite gates. There is no delivery
task that asks its implementer to discover the architecture or choose a
different resize path.
