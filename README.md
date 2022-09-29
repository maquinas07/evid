evid
---

Screen recorder for X with a minimal interface. Supports mp4 (outputs h264 encoded files) and GIFs. The heavy lifting is done by ffmpeg.   

Build requirements
---
 - libx11   
 - libnotify - Optional (enabled by default, modify the Makefile to disable)
 - libXfixes - Optional (enabled by default, modify the Makefile to disable)

Runtime requirements
---
 - ffmpeg 
 - zenity - Optional (disabled by default, modify the Makefile to enable)

Compile from source
---

### Install compile dependencies

#### Ubuntu/Debian:
```bash
sudo apt install libx11-dev libxext-dev libxfixes-dev libnotify-dev
```

  
#### Then:
```bash
make
make install
```
`make install` might need to be run as `sudo` as it tries to write into the /usr/local/bin folder.

Usage
---
evid has preconfigured default values so you can just execute the binary, select the area to record and when you're done just press CTRL+s to save or CTRL+c to copy to clipboard. If no area is selected and evid is compiled with HAVE_XEXTENSIONS, evid will record the entire window that was clicked. evid doesn't have any config files so to change the default shortcuts you will need to modify [src/actions.h](./src/actions.h) and recompile.

By default evid outputs the recordings as mp4 without any audio and saves them in `$XDG_VIDEOS_DIR/evid/` or `$HOME/Videos/evid/`.   
You can change the output directory by setting the XDG_VIDEOS_DIR variable, the HOME variable or specify it via the argument `-o` (E.G. `-o/some/path`).   
To output gifs you can use the `-g` argument, or `-gg` if you want higher quality gifs.   
To record audio you can use `-a`, by default it uses the default source in pulseaudio. You can also use alsa and optionally specify the source. `-apulse,somesource` or `-aalsa` or `-aalsa,somealsadevice`.   
