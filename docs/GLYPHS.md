# Controls and controller prompts

Where supported for a game, button images are enabled by default. Xbox, PS4, PS5, SHIELD, Backbone and VX-2
select their artwork automatically; see [supported controllers](SUPPORTED_CONTROLLERS.md).
Visible icon prompts change with the most recently used controller. Keyboard
input restores keyboard prompts. The controller activity window is 30 seconds.

## Game layouts

Controller layouts are hardcoded to follow their console counterparts, primarily
the Xbox versions. Keep the game's keyboard and mouse bindings at their defaults.

### The Suffering (2004)

| Action | PlayStation | Xbox-style |
|---|---|---|
| Movement / camera | Left / right stick | Left / right stick |
| Jump | Cross | A |
| Reload | Circle | B |
| Use / save / load | Square | X |
| Insanity | Triangle | Y |
| Attack / throw or alt fire | R2 / L2 | RT / LT |
| First person / health | L1 / R1 | LB / RB |
| Flashlight / inventory | D-pad Down / Up | D-pad Down / Up |
| Inventory navigation | D-pad Left / Right | D-pad Left / Right |
| Cancel | Options / Start | Start / Menu |
| Map | Create / Share / Select | View / Back |
| Keyboard V / B actions | L3 / R3 | Left / right stick click |

VX-2 uses Start and Select text labels with the current PS4 artwork pack.

### The Suffering: Ties That Bind

Controller prompts use the shared platform image packs and switch artwork with
the active controller. Keyboard input retains the game's keyboard prompts.
Left stick uses arrow-key movement and right stick controls the camera.

| PlayStation / Xbox | Game input | Action |
|---|---|---|
| L3 / LS click | Left Ctrl | Crouch |
| R3 / RS click | F | Flashlight |
| Square / X | Enter | Interact |
| Cross / A | Space | Jump |
| Circle / B | Middle mouse | Secondary attack |
| Triangle / Y / Nintendo X (tap) | Q | Cycle weapons |
| Triangle / Y / Nintendo X (hold) | Z | Pick up |
| L1 / LB | C | Insanity |
| R1 / RB | X | Cycle thrown weapons |
| L2 / LT | Right mouse | Throw |
| R2 / RT | Left mouse | Attack |
| Options / Menu / Start | Escape | Pause |
| Create / Share / View / Back | 2 | Map |
| D-pad Up | 1 | First person |
| D-pad Down | R | Reload |
| D-pad Left | Numpad 4 | - |
| D-pad Right | Numpad 6 | - |

Tap the top face button to cycle weapons on release. Holding it for at least
300 milliseconds instead holds Z for pickup, without also cycling weapons.
Pickup instructions say "Hold". Menu Select uses Square / Xbox X / Nintendo Y;
Cancel uses Options / Start / Plus. Delete-save prompts remain keyboard-only.

## Stick walking

Both games use Left Shift for stick walking.

Left-stick distance is measured radially from the centre, before deadzone
rescaling. At 15% or less, it contributes no movement. Movement
starts walking with Left Shift held, unless the initial tilt already exceeds
48%. Walking switches to running above 48%; running switches back to walking
below 42%. Between these thresholds, the current mode is retained around the
nominal 45% boundary.

When stopping from a walk, Shift remains held for 250 milliseconds to allow the
walk-to-idle transition to finish. Idle drift cannot start or extend this hold.
Entering the run range cancels it immediately. Controller-selection reset also
clears the hold; normal idle contributes no Shift once it expires.

The shared activity readers use the same 15% radial threshold in both games.
Physical keyboard state is preserved when adding controller contributions.

## Settings

The following image settings and display details apply to games with
controller button-image rendering.

Set overrides in the same cmd window used to launch the game:

```cmd
set RETROINPUT_INLINE_GLYPHS=0
Suffering.exe
```

Clear `RETROINPUT_INLINE_GLYPHS` to restore images. To override artwork:

```cmd
set RETROINPUT_ICON_PACK=dualshock4
Suffering.exe
```

Choices: `auto`, `dualsense`, `dualshock4` (or `ps4`), `xboxseries` (or `xbox-series`),
`nintendo` (or `switch`), `xbox` (legacy) and `playstation` (legacy). Clear the variable for automatic selection.
Artwork selection does not change input bindings.

## Display and fallback

The approved baseline is 1.5 times the original icon size. Movement-stick images
use 110% of that baseline; trigger images use 75%. Other images use the baseline.
Spacing, square proportions, centring, text fading and clipping are retained.
Shared source tiles remain 128x128; display sizing is set separately for each game.

Failed hook setup retains text prompts. A failed texture upload after expansion
uses an X fallback. The Suffering writes its renderer log to
`%TEMP%\RetroInputTextureDiagnostic.log`. Ties That Bind writes
`RetroInputTTBImages.log` beside its DLL, falling back to `%TEMP%` if that folder
is not writable.
See [editing artwork](../assets/buttons/README.md).
