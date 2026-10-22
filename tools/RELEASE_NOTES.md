Fixes rendering corruption when switching between Xbox, PlayStation and Nintendo controller artwork in **The Suffering** and **The Suffering: Ties That Bind**.

Both games now reuse one native controller texture and upload the selected artwork when the icon pack changes. Individual icon packs remain embedded in the DLL. Shared upload validation handles different pack sizes and clears unused pixels.

The project owner confirmed the clean fix in both games, including the GOG and AiO-patched installations of The Suffering. The release workflow builds the confirmed source and runs the input, prompt, upload and artwork checks before publishing.

Download the ZIP for your game. Existing installations generally need only the updated game DLL. New installations include Win32 Ultimate ASI Loader 9.7.4, configuration and instructions. Preserve existing game fixes. SHA256SUMS.txt contains package checksums.

## Credits

- [ThirteenAG](https://github.com/ThirteenAG) - thank you for [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader). Its MIT licence is included.
- [JulioCacko](https://juliocacko.itch.io/) - thank you for the controller button icons.
