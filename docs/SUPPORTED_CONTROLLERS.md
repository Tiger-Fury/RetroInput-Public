# Supported controllers

The controllers marked Working below have been tested in The Suffering by the project owner.
Development primarily focuses on **DualSense and Xbox/XInput controllers**,
while the additional controllers listed below are also supported.

| Controller | Reader | Connection / mode | Status | Automatic artwork |
|---|---|---|---|---|
| Xbox / XInput controllers | Xbox/XInput | XInput | Working | Xbox Series |
| PowerA Spectra Controller | Xbox/XInput | XInput | Working | Xbox Series |
| GioTeck VX4 | Xbox/XInput | XInput | Working | Xbox Series |
| Mayflash Magic-S Pro 2 | Xbox/XInput | XInput | Partial support | Xbox Series |
| Sony DualSense (PS5) | Native DualSense | USB / Bluetooth | Working | DualSense |
| Sony DualShock 4 (PS4) | Native DualShock 4 | USB / Bluetooth | Working | PS4 |
| NVIDIA SHIELD Controller (2017) | Native SHIELD | USB / Bluetooth | Working | Xbox Series |
| Backbone One PlayStation Edition Gen 2 | Native Backbone | USB | Working | DualSense |
| Gioteck VX-2 | Native VX-2 | USB / receiver | Working | PS4 |
| Original Switch Joy-Con (L + R pair) | Native Joy-Con | Bluetooth | Working | Nintendo Switch |

PowerA Spectra, GioTeck VX4 and Mayflash use the existing dedicated Xbox/XInput
reader. Mayflash remains partially supported; the table does not imply complete
support for every controller connected through the adapter.

Native device IDs: DualSense `054C:0CE6`, DualShock 4 `054C:05C4` / `054C:09CC`,
SHIELD `0955:7214`, Backbone `358A:0304`, VX-2 `25F0:83C3`,
Joy-Con L `057E:2006` and R `057E:2007`. DualSense Edge
`054C:0DF2` is recognized but has not been separately confirmed in this list.
Other revisions and clones require their own checks.

Joy-Con support combines one left and one right controller into a single full
controller. Both must be connected and sending input; losing either half clears
pair input. Face buttons follow physical positions: Nintendo B/A/Y/X correspond
to Xbox A/B/X/Y. Nintendo artwork is selected automatically. Single sideways Joy-Cons,
Joy-Con 2, rumble and motion controls are not covered by this reader.

## Controller Assist

Nine controllers have been tested connected and working seamlessly together:
**four Xbox/XInput controllers**, a DualSense, a DualShock 4, an NVIDIA SHIELD
controller, a Gioteck VX-2 and a Backbone. This mixed-controller setup is a useful
capability, although expanding Controller Assist is not a primary project focus.

A maximum of **four Xbox/XInput controllers** can participate at once, across
all models using the Xbox/XInput reader. Player assignment and multiplayer
support depend on the game.
See [Controller Assist behaviour](MULTIPLE_CONTROLLERS.md) for input aggregation
and selection details. Each native reader currently selects one matching device.

## Limitations

VX-2 triggers are
digital. Native support covers gameplay controls; rumble, motion, voice, touch
gestures and device lighting are not implemented.

See [installation](INSTALLATION.md) and [prompt settings](GLYPHS.md).

