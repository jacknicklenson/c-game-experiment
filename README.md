# c-game-experiment
Simple C game.

# Dependencies

 - [Meson](https://mesonbuild.com/Getting-meson.html)
 - pkg-config
 - [Conan](https://conan.io/downloads)
 - [Ninja](https://ninja-build.org/) and C99 compliant compiler. For Windows -> [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
 - [Git](https://git-scm.com/downloads)

 **Make sure these are installed and on your PATH.**
 
If you are on Windows 10 version 1709 and later, you can install this dependencies easily with this command (open cmd.exe or powershell.exe and type):
```shell
winget install --id Microsoft.VisualStudio.2022.BuildTools
winget install --id Git.Git
winget install --id mesonbuild.meson
winget install --id bloodrock.pkg-config-lite
winget install --id JFrog.Conan
```
Then:
```
git clone https://https://github.com/CODESOLE/c-game-experiment.git
```

# BUILD

First before runing build script;

If you installed conan first time, it needs a Conan profile to build the project. Conan profiles allow users to define a configuration set for things like the compiler, build configuration, architecture, shared or static libraries, etc. Conan, by default, will not try to detect a profile automatically, so we need to create one. To let Conan try to guess the profile, based on the current operating system and installed tools, please run:

```shell
conan profile detect --force
```

Then we can continue for build.

## Build Instructions For WINDOWS

Just Run:
```shell
.\build-windows.bat
```


## Build Instructions For LINUX

For Linux install build dependencies, for Ubuntu/Debian and its derivatives command is:

```shell
sudo apt update && sudo apt install libx11-xcb-dev libfontenc-dev libice-dev libsm-dev libxaw7-dev libxcomposite-dev libxcursor-dev libxdamage-dev libxfixes-dev libxi-dev libxinerama-dev libxkbfile-dev libxmu-dev libxmuu-dev libxpm-dev libxrandr-dev libxrender-dev libxres-dev libxss-dev libxt-dev libxtst-dev libxv-dev libxvmc-dev libxxf86vm-dev libxcb-render0-dev libxcb-render-util0-dev libxcb-xkb-dev libxcb-icccm4-dev libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev libxcb-shape0-dev libxcb-sync-dev libxcb-xfixes0-dev libxcb-xinerama0-dev libxcb-dri3-dev libxcb-cursor-dev libxcb-util-dev libxcb-util0-dev libgl-dev libgl1-mesa-dev libxkbcommon-dev
```

Just Run:
```shell
./build-linux.sh
```


# LICENSE

MIT
