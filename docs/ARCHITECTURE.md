# Code structure

RetroInput normalizes controller input to `XINPUT_GAMEPAD`. That structure is
the shared state format even for controllers read through native HID.

| Component | Responsibility |
|---|---|
| `ControllerSelector.h` | Latest-activity selection, keyboard takeover and timeout |
| `Core/ControllerTypes.h` | Device identities and controller families |
| `Core/ControllerActivity.h` | Shared activity thresholds |
| `Core/StickWalk.h`, `Core/TapHold.h` | Reusable stick-walk and tap/hold state |
| `Core/XInputAggregator.h` | Four-slot Xbox/XInput aggregation (Xbox Controller Assist)|
| `DualSense`, `DualShock4`, `Shield`, `Backbone`, `Vx2`, `JoyCon` readers | HID discovery, decoding and reconnects |
| `Core/*Report.h` | Device-specific report/descriptor decoding |
| `Core/ButtonImages.*` | Logical button requests, pack selection and CPU pixels |
| `Controller.cpp` | Pass selected input to the game-specific code |
| `Games/<game>/GameHooks.cpp` | x86 input hook |
| `Games/<game>/InputMapping.h` | Game keyboard/mouse bindings |
| `Games/<game>/Prompts.*` | Prompt substitutions (Console Controller Icons)|
| `Games/<game>/ButtonRenderer.h` | Font metrics, texture uploads and drawing |
| `Games/TheSufferingTTB/RenderingSupport.h` | Upload geometry checks and renderer logging |
| `dllmain.cpp` | Start initialization on a worker |

Fixed addresses, engine/GPU objects, font metrics, bindings and display sizing
stay in the game-specific code. Shared artwork is requested by logical control through
`GetButtonImage(controller, button)`; the code for each game owns its GPU resources.

HID reads use advertised buffer sizes and cancel/drain pending I/O before closing
handles. The Joy-Con reader sends temporary report-mode commands and reads stored
stick calibration through SPI-read commands; it does not write calibration or
flash memory. Other native readers make no controller output/feature writes. The game-specific
executable and hook-signature checks remain in place. Unloading the DLL from a
running game is not supported.

`tests/` contains protocol, selection, mapping and artwork checks. PNG importers
and legacy-image packing live in `tools/`; normal Visual Studio builds embed the
checked-in images directly. Each game has its own project in
`RetroInput.slnx` and produces a separate DLL. Shared installation and controls
guides cover the supported games without a separate Markdown page for each game.

