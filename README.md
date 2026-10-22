# RetroInput

A controller-support framework for older PC games, providing native controller input, automatic controller detection and console-style button images.

Controller readers, device selection and artwork are shared across supported games. Each game has its own bindings and hooks, built into a separate DLL. Available features depend on the game.

- [Supported games and downloads](docs/SUPPORTED_GAMES.md)
- [Installation and building](docs/INSTALLATION.md)
- [Supported controllers](docs/SUPPORTED_CONTROLLERS.md)
- [Controls and prompt settings](docs/GLYPHS.md)
- [Multiple Xbox controllers](docs/MULTIPLE_CONTROLLERS.md)
- [Editing button artwork](assets/buttons/README.md)
- [Code structure](docs/ARCHITECTURE.md)

## Building

Open **RetroInput.slnx** in Visual Studio and select **Release | x86**.

Right-click the required game project in Solution Explorer and select **Build**, or choose **Build Solution** to build all game DLLs. Each project produces its own DLL.

## Controller layouts

Controller button layouts are hardcoded to follow their console counterparts, primarily the Xbox versions. PlayStation controllers use the equivalent buttons for the same actions.

For games without a console counterpart, a comparable layout will be provided based on a similar game or a later entry in the same series. For example, The Witcher would use a layout based on The Witcher 2 or 3, while Hitman: Codename 47 would use Hitman 2: Silent Assassin as a reference.

PC keyboard and mouse bindings remain at their default settings.

## Credits

- **[ThirteenAG](https://github.com/ThirteenAG)** — thank you for creating **[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)**, used for automatic DLL loading.
- **[JulioCacko](https://juliocacko.itch.io/)** — thank you for the controller button icons used in RetroInput’s platform artwork packs.
