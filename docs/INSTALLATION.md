# Installation and building

## Install a release

1. Choose your game from [Supported games and downloads](SUPPORTED_GAMES.md).
2. Download its release ZIP and close the game.
3. Read the included **RetroInput-README.md**, then follow its installation instructions. Each package includes the correct game DLL, loader and configuration.
4. Preserve existing game fixes and check any existing loader files before replacing them.

## Build a game DLL

1. Download or clone the repository.
2. Open **RetroInput.slnx** in Visual Studio.
3. Select **Release | x86**.
4. In Solution Explorer, right-click the required game project and select **Build**. Alternatively, select **Build → Build Solution** to build all game DLLs.
5. Locate the DLL in the output path shown by Visual Studio.

Each game has a separate DLL. Use the DLL intended for your game.

## Automatic loading

RetroInput uses **[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)** to load the appropriate game DLL when the game starts.

1. Close the game.
2. Copy the game DLL into the game folder beside its executable.
3. Install the **32-bit Ultimate ASI Loader** using a proxy DLL filename supported by the game.
4. Create the matching loader configuration file and set `LoadExtraPlugins` to the game DLL’s filename.

For a setup using `winmm.dll`, create **`winmm.ini`** containing:

```ini
[GlobalSets]
LoadPlugins=1
LoadExtraPlugins=YourGame.dll
DontLoadFromDllMain=1
DebugLog=1
```

Replace `YourGame.dll` with the actual DLL filename, then launch the game normally.

The loader filename must suit the game. Preserve existing fixes and check the purpose of any existing proxy DLL before replacing it.

## Controller setup

Connect a [supported controller](SUPPORTED_CONTROLLERS.md) before or during play. Native controller readers work directly with supported devices; avoid exposing the same controller through an additional virtual-controller tool.

Controller layouts follow their console counterparts, primarily the Xbox versions. Keep the game’s keyboard and mouse bindings at their defaults.

Feature availability, including controller button images, depends on the game.

## Updating

Close the game and download the updated package for your game, or build the updated game project. Follow the package instructions when replacing files. The loader files can remain in place unless the loading configuration changes.

## Troubleshooting

- Confirm that you built **Release | x86**.
- Check that the correct game DLL is beside the game executable.
- Confirm that `LoadExtraPlugins` matches its filename.
- For the configuration above, check `winmm.log` in the game folder for loading errors.

## Uninstalling

Remove the game DLL and the loader files added during installation. Preserve other game fixes and shared components.
