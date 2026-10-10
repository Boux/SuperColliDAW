# SuperColliDAW

It's SuperCollider, but directly in your DAW. It's a scsynth server sandboxed directly into the plugin. `SoundIn.ar` takes the direct input of the plugin on your track, and `Out.ar` sends it back out to the track. The same is true for MIDI, you can create your own custom instrument tracks, with built-in MPE support.

It's currently only a CLAP plugin, but I will probably also add a VST3 version eventually for DAWs without CLAP support (ableton, cubase, etc). CLAP has much better MPE support and it's easier to define dynamic parameters for stuff like `In.kr`.

This is a very early BETA version, there's probably gonna be bugs, jank, and random issues specific to different DAWs. I have not tested mac OS, I don't have a mac and I have no idea if it works, I just let github compile it for me. I have been developing it for linux first, and I did some minimal testing on the windows version, only in Bitwig and Reaper. If something goes wrong, please [open an issue](https://github.com/Boux/SuperColliDAW/issues).

## Install

1. Install [SuperCollider 3.14](https://supercollider.github.io/downloads) if you haven't yet. On a Mac, put it in your Applications folder.
2. Download SuperColliDAW for your OS from the [Releases page](https://github.com/Boux/SuperColliDAW/releases).
3. Unzip it. You get a file called `SuperColliDAW.clap` and a folder called `SuperColliDAW`. Keep these two together.
4. Move both into your CLAP plugin folder. Create the folder if it doesn't exist.
   - **Windows:** `C:\Program Files\Common Files\CLAP`
   - **Mac:** `~/Library/Audio/Plug-Ins/CLAP`
   - **Linux:** `~/.clap`
5. **Mac only:** macOS blocks plugins downloaded from the internet. To allow it, open the Terminal app, paste this line and press Enter:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/CLAP/SuperColliDAW*
   ```
6. You should then see it in any DAW that supports CLAP plugins.

## License

SuperColliDAW is free software under the GNU General Public License, version 3. See [LICENSE](LICENSE). It includes parts of [SuperCollider](https://supercollider.github.io), which uses the same license.
