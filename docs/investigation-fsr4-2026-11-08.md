# FSR 4 investigation (2026-11-08)

Flights and provider findings for FSR 4 as this branch ships it: 4.1.1 on Proton.
The FSR 4.1.2 design, plan and later findings (Proton-CachyOS, ML frame generation)
are on branch `fsr4-1-2` of `pattern-state/ed-unofficial-patch`.

## Status

- **State (2026-11-08): `fsr4` runs flat on Linux/Proton from a compile-only build of
  `fsr4-1-2` at `5c5a0929` (main `5033d3c8` plus `claude/fsr-4-1-flat-vr-support-5f6ea4`),
  provider FSR 4.1.1, with `FSR4_UPGRADE=1`.** At Supersampling 1.0 it treats every frame
  on the HDR route; below 1.0 it upscales every frame on the copy route (post-tonemap,
  `hdr=0`). Release 0.21.1's `fsr` (FSR 3.1) treats nothing below 1.0 on the same rig.
- **Proton provider:** without `FSR4_UPGRADE=1` AMD's runtime offers only FSR 3.1.5 and
  2.3.4 and `fsr4` refuses with its named message; with it, 4.1.1 is offered and used.
  The shipped `amd_fidelityfx_upscaler_dx12.dll` does not by itself make 4.x available
  under Proton. The SDK 2.4 DLLs do not change what Proton offers (Flight 2).
- **4.1.2 via the user's driver file:** with the user's own `amdxcffx64.dll` 2.4.0.4541
  beside the game, 4.1.2 is offered and used with the SDK 2.3.0 runtime DLLs (Flight 3).
  With the SDK 2.4 runtime DLLs and the same file, AMD's runtime offers no FSR 4 (Flight 4).
  The cause is not known. David parked the route on 2026-11-09. The driver file is removed
  from the development game, and flights use Proton's 4.1.1.
- **Seen, not yet investigated:** the cockpit HUD with HUD anti-aliasing on (thin strokes
  break up, e.g. the left edge of the S in "SYS"); one exit while switching Supersampling
  to 2.0 on release 0.21.1, not reproduced on the second try; overlay fit (Flight 4): the
  F8 row text truncates ("FSR 4 (unavai..."), the status line overflows its box, and the
  footer's "saved" overlaps the version string when the describe string is long ("-dirty").
- **Not flown:** VR; Windows; the 4.1.2 re-pin (Phase 1).
- Design and plan: branch `fsr4-1-2`, `docs/design-fsr4-1-2-2026-11-07.md` and
  `.agents/plan-fsr4-1-2-2026-11-07.md`.

## Rig

| Item | Value |
|---|---|
| GPU | AMD Radeon RX 9070 XT (Navi 48, RADV GFX1201), 16 GB |
| CPU | AMD Ryzen 7 9800X3D |
| OS | openSUSE Tumbleweed, kernel 7.1.8-1-default, KDE Plasma X11 session |
| Mesa / RADV | Mesa 26.2.0, `libvulkan_radeon` 26.2.0 |
| Proton | Proton Experimental `experimental-11.0-20261027b` |
| Monitor | 5120x1440 |
| Game | Elite Dangerous: Odyssey build 334925, an isolated development copy with its own prefix, launched by the `ed-fsr4` harness (`tools/launch_frontier.py`) |
| Headset | none (flat) |

## Linux compile-only toolchain (plan Task 0.2)

`tools/linux_toolchain.py` ran inside an Arch Linux container (podman), because the host's
Tumbleweed snapshot could not take system Wine without removing its X11 session. The fork
is mounted at its own path, so `.toolchains/` and `build/compile-build/` land in the tree
as on a native host. Container Wine: `wine-11.21`.

- `setup --accept-license`: MSVC 14.44.35207, Windows SDK 10.0.26100.0, Windows Python
  3.12.10, CMake 4.4.4, Ninja 1.13.2, msvc-wine `514f8ea34842`; SDKs ngx, XeSS 3.0.2,
  FidelityFX 2.3.0 (`60f4ea819092`, FSR 4.1.1 runtime), FidelityFX-SDK-DX11 3.1.2
  (`9b04fa49abae`).
- `doctor`: all 14 checks ok.
- `build-dlls`: `v0.22.0-rc.7-71-g5c5a0929`, 82.7 s, no errors. `d3d11.dll` sha256
  `1b50f0c7b40b218f…`; receipt `compile_only_receipt.json` sha256 `7dd5a699d03baf39…`.
  NOT Windows validated.

## Flight 0: release 0.21.1, flat, Proton (2026-11-08)

The release's Windows installer, run in the development prefix through Proton
Experimental's `runinprefix` with `--dir` naming the development game (the Linux wrapper
only accepts games inside a Steam library). Window 3840x1080, game anti-aliasing off,
`temporal_aa = fsr`, docked in a hangar. FPS is the game's counter (David); treated
counts are the `flat hdr route 5s` lines.

| Supersampling | Render | FPS | FSR 3.1 treated |
|---|---|---|---|
| 1.0 | 3840x1080 | about 140 | all (`treated-jittered-hdr`) |
| 1.0, `temporal_aa = off` | 3840x1080 | 144 (cap) | n/a |
| 0.75 | 2880x810 | 144 (cap) | none |
| 0.5 | 1920x540 | 144 (cap) | none |
| 1.0 again | 3840x1080 | about 140 | all |
| 1.5 | 5760x1620 | 78 | all |
| 2.0 | 7680x2160 | 46 | all |
| 1.0 again | 3840x1080 | 135 | all |

Below 1.0 every trigger reads `hdr-route-needs-render-at-least-output`: the release's flat
FSR 3.1 anti-aliases or downsamples, and never upscales. Every size change reset history
cleanly. An earlier session exited while switching to 2.0 (the log stops at 22:48:18 with
no shutdown line; the game had not saved 2.0); the run above did not repeat it.

## Flight 1: `fsr4-1-2` at `5c5a0929`, flat, Proton (2026-11-08)

Installed with `tools/install_edvr.py --target <development game> --scope flat
--compile-only --fsr4`. `temporal_aa = fsr4`, game anti-aliasing off, docked.

**Without `FSR4_UPGRADE`** (log `edvr_gfx_20261108_230053.log`):

```
fsr4: D3D11/D3D12 adapter matched LUID=00000000:000003F5 vendor=0x10DE device=0x7550 wine=1
fsr4: AMD's runtime offers FSR 3.1.5, 2.3.4 for this device (2 versions)
fsr4: refused: AMD's runtime offers only FSR 3.1.5, 2.3.4 for this GPU; ... under Proton, launch Elite with FSR4_UPGRADE=1 %command% ...
```

Every frame falls back to the spatial output. The provider check is made once per process:
selecting FSR 4.1 again in F8 in the same session still falls back.

**With `FSR4_UPGRADE=1`** (log `edvr_gfx_20261108_231143.log`):

```
fsr4: AMD's runtime offers FSR 4.1.1, 3.1.5, 2.3.4 for this device (3 versions)
fsr4: using AMD's provider 4.1.1 (version id 0xF5A5CA1E81001001), the highest 4.x offered
fsr4: slot=0 context created in 6.5 ms: in=3840x1080 out=3840x1080 flags=0x39 infinite=1 hdr=1
```

The colour, motion, depth and output textures open on the D3D12 side, and dispatches
return `rc=0`.

| Window | Supersampling | Render -> output | FPS (David) | Route | FSR 4 treated | FSR 4 context |
|---|---|---|---|---|---|---|
| 3840x1080 | 1.0 | 3840x1080 -> 3840x1080 | about 133 | HDR | all | `hdr=1` |
| 5120x1440 | 1.0 | 5120x1440 -> 5120x1440 | 82 | HDR | all | `hdr=1` |
| 5120x1440 | 0.75 | 3840x1080 -> 5120x1440 | 120 | copy (admitted) | all | `hdr=0`, `flags=0x138` |
| 5120x1440 | 0.5 | 2560x720 -> 5120x1440 | 144 (cap) | copy (admitted) | all | `hdr=0`, `flags=0x138` |

Below 1.0 the HDR route declines (`hdr-route-needs-render-at-least-output`, as in 0.21.1),
but the copy structure admits the game's upscale copy (`admitted=597 of 597` at 0.75,
`720 of 720` at 0.5), and FSR 4 upscales there, after tone mapping. David: "looks pretty
good". Bridge cost, D3D11 side, median/p95: 1.01/1.50 ms at 3840x1080 native; 1.58/1.67 ms
at 0.75 on 5120x1440 (D3D12 dispatch 0.95/0.99 ms of it).

**HUD:** with HUD anti-aliasing on, the cockpit HUD's thin strokes break up (the left edge
of the S in "SYS" beside the pips); off, they stay whole but aliased. Supersampling 2.0
renders it crisp at about a third of the frame rate. Not yet compared at 0.75 or 0.5.

## Flight 2: SDK 2.4 DLLs, provider check (2026-11-08)

SDK 2.4 DLLs (upscaler 4.1.2.4555, loader 2.3.0.4555) beside the game, no driver file.
Build `5c5a0929`, 23:44. Plan Task 0.3. Logs `edvr_gfx_20261108_234436.log` and
`edvr_gfx_20261108_234515.log`.

| `FSR4_UPGRADE` | AMD's runtime offers | Outcome |
|---|---|---|
| unset | FSR 3.1.6, 2.3.4 | `fsr4` refused |
| 1 | FSR 4.1.1, 3.1.6, 2.3.4 | uses 4.1.1 |

The SDK 2.4 upscaler DLL does not supply a 4.x provider under Proton. The only change from
Flight 1 is FSR 3.1.6 where 3.1.5 was offered.

## Flight 3: user-supplied driver file, FSR 4.1.2 (2026-11-08)

SDK 2.3.0 DLLs. The user's own `amdxcffx64.dll` 2.4.0.4541 beside `EliteDangerous64.exe`.
The file is from AMD Adrenalin 26.10.2
(`Packages/Drivers/Display/WT6A_INF/B026716/`). `FSR4_UPGRADE=1`. 23:57. Log
`edvr_gfx_20261108_235720.log`.

| Item | Value |
|---|---|
| AMD's runtime offers | FSR 4.1.2, 3.1.5, 2.3.4 |
| Provider used | 4.1.2, version id `0xF5A5CA1E81001002` |
| Frames | treated |

Proton's `amdxc64` shim calls `LoadLibraryA("amdxcffx64")` by bare name, so the game-folder
copy wins over Proton's.

## Flight 4: Tasks 1.1/1.2 build, SDK 2.4 with the driver file (2026-11-09)

New uncommitted build: Task 1.1 (SDK 2.4 re-pin) and Task 1.2 (version pick and labels).
`d3d11.dll` linked 00:31. Describe `v0.22.0-rc.7-71-g5c5a0929-dirty`. SDK 2.4 DLLs and the
same driver file 2.4.0.4541. `FSR4_UPGRADE=1`, confirmed in the game process environment.
00:33. Log `edvr_gfx_20261109_003327.log`.

| Item | Value |
|---|---|
| AMD's runtime offers | FSR 3.1.6, 2.3.4 |
| `fsr4` | refused |
| F8 row | "FSR 4 (unavailable)" |
| Status line | the refusal |

The labels behave as designed: the row and status line report what is served. The cause is
not known. Proton's log was not read.

Decision (David, 2026-11-09): park the driver-file route. The driver file was removed from
the development game. Flights use Proton's 4.1.1. The SDK 2.4 re-pin stays.

Overlay fit seen in this flight: the row text truncates ("FSR 4 (unavai..."), the status
line overflows its box, and the footer's "saved" overlaps the version string. To fix (plan
Task 1.2).

## Provider combinations (RX 9070 XT, Proton Experimental, `FSR4_UPGRADE=1`)

| SDK runtime DLLs | Driver file beside the game | FSR 4 served | Flight |
|---|---|---|---|
| 2.3.0 | `amdxcffx64.dll` 2.4.0.4541 | 4.1.2 | 3 |
| 2.4 | none (Proton's provider) | 4.1.1 | 2 |
| 2.4 | `amdxcffx64.dll` 2.4.0.4541 | none (refused) | 4 |
| 2.4, Tasks 1.1/1.2 build | none (Proton's provider) | 4.1.1 | 5 |

## Flight 5: Tasks 1.1/1.2 build, SDK 2.4, Proton's provider (2026-11-09)

Same build as Flight 4, driver file removed, `FSR4_UPGRADE=1`. Log
`edvr_gfx_20261109_004650.log`.

```
fsr4: AMD's runtime offers FSR 4.1.1, 3.1.6, 2.3.4 for this device (3 versions)
fsr4: picked FSR 4.1.1, version id 0xF5A5CA1E81001001 (preferred 4.1.2 is not offered; this is the highest 4.x offered)
fsr4: slot=0 dispatch #3 rc=0 in=2560x720 out=5120x1440 hdr=0 ... provider=fsr 4.1.1 (Proton driver provider)
```

F8 (David's screenshot, Supersampling 0.75): row "FSR 4.1.1"; status line
"FSR 4.1.1: 3840x1080 -> 5120x1440", inside its box. Game counter 127 fps. The
footer's "saved" still overlaps the version string.

## Next

- Diagnose the clash between SDK 2.4 and the driver file (Flight 4) with Proton's log. Does
  the shim load the file? Compare "Replaced FSR3 with FSR4!" with "Failed to load FSR4 dll".
  The route is parked until then.
- Fix the overlay fit: row text for the unavailable state, status line within its box,
  footer overlap (plan Task 1.2).
- The copy route below 1.0 is the input the quality modes will use: check its image (HUD,
  ghosting, the station streaks Sean noted) against the HDR route at 1.0.
- The HUD with HUD anti-aliasing on, under `fsr4` and `fsr`.
