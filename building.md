# Building

The instructions below are only for Windows 64bit using Visual Studio, if you are on some other operating system, you are on your own to figure out how to build this.

## The actual steps

First clone the git repository

```
git clone https://github.com/username121546434/SquareSyncTest.git
```

Then make a folder called `ext` and within that folder, make three folders: `Capnp`, `GameNetworkingSockets`, and `SDL3`

Then inside both the `Capnp` and `GameNetworkingSockets` folders, make a `Release` and `Debug` folder. In the `GameNetworkingSockets` folder, make a folder called `include`

### Setting up SDL3

[Download SDL 3.6](https://github.com/libsdl-org/SDL/releases/tag/preview-3.1.6), you want to download `SDL3-devel-3.1.6-mingw` (either zip or tar.gz is fine) and then extract the contents.
Now copy the contents of the `x86_64-w64-mingw32` folder to the SDL3 subfolder from earlier.

### Setting up Capnproto

More details about the the following instructions can be found on the [Capnproto installation website](https://capnproto.org/install.html)
[Download Capnp 1.0.2](https://capnproto.org/capnproto-c++-win32-1.0.2.zip) and in the `capnproto-c++-1.0.2` folder run this command:
```
cmake -G "Visual Studio 16 2019"
```
Open the "Cap'n Proto" solution and build the ALL_BUILD project in x64 using Release and Debug.
In the `Capnp\Release` and `Capnp\Debug` folder from earlier, make a `lib` and `include` directory. Copy the contents of `capnproto-c++-1.0.2\src` into both the `Capnp\Release\include` and `Capnp\Debug\include`.
Then copy the `*.lib` files from `capnproto-c++-1.0.2\src\{capnp,kj}\{Debug,Release}` into their respective `Capnp\{Release,Debug}\lib` directory.

### Setting up Steam Game Networking Sockets

In the `ext\GameNetworkingSockets\{Release,Debug}` folders from earlier, make a `bin` and `lib` folder.
After doing cloning into the [github repo](https://github.com/ValveSoftware/GameNetworkingSockets/tree/master) from a completely seperate folder, you can for the most part just follow the instructions on their [github repo](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/BUILDING.md#windows--visual-studio), the only difference is that when it tells you to run `cmake -S . -B build -G Ninja` you should actually run `cmake -G "Visual Studio 17 2022" -A x64`.
Once done, open the solution and build using x64 and Release and Debug. Then put the contents of the `bin\{Release,Debug}` folders into the respective `ext\GameNetworkingSockets\{Release,Debug}\bin` folder.
Next, put the contents of `src\{Release,Debug}` into the respective `ext\GameNetworkingSockets\{Release,Debug}\lib` folder.
Finally, put the contents of the `include` folder into the `ext\GameNetworkingSockets\include` folder.

## You're done!

You should now be able to open the project in Visual Studio and build it!

