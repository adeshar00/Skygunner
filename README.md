#About

This is a game that I began working on in 2014.  It's written in C, and can be compiled for the web with emscripten (the web version of it can be played [here](http://adeshar00.github.io/Skygunner/)).

The player pilots a helicopter, and has to shoot incoming enemies before they reach the player's base.  This game is still in the early stages of development: the game models are just ellipsoids, there is no collision detection between the player and anything else, and there are no victory/defeat conditions.

Build instructions are at the bottom of this file.  The game's controls are above the build instructions.

#Development

A lot of the code is experimental.  This was my first time making a non-trivial multithreaded application, I played with different ways to encapsulate my code with header files, and I also tried implementing a few data-oriented design techniques (like avoiding branch instructions where possible, and organizing my data in ways to minimize cache misses).

The majority of the game's data are stored as fixed-point integers instead of floating point numbers, and I ended up writing my own functions and macros to perform math operations on fixed-point integers instead of using the default float operations (these can be found in [intmath.h](https://github.com/adeshar00/Skygunner/blob/master/src/intmath.h)).  There are macros for doing multiplication and trig operations, but for other functions that I needed I just cheated and wrote functions which convert the integers to floats, do the operations with the C math library on the floats, and then convert the floats back to fixed-point integers.

Physical relativity is obeyed in the game's gun mechanics: bullet escape velocity is affected by the velocity of the player.  The rotation of the player's gun is automatically adjusted to account for velocity, to make aiming at stationary targets easier; the larger mouse cursor indicates bullet escape velocity relative to the ground, while the smaller trailing cursor indicates bullet escape velocity relative to the helicopter.

The collision detection between bullets and enemies is achieved by checking if spherical zones around the bullets overlap with ellipsoidal zones around the enemies.  With some linear algebra, this ellipsoid/sphere check is turned into a sphere/sphere check, which is computationally efficient, and is accurate regardless of bullet or enemy velocity.

#Controls

|Action                 |Button                    |
|-----------------------|--------------------------|
|Horizontal Movement    |WASD                      |
|Fire Cannon            |Left mouse button         |
|Rotate Camera          |Right mouse button (hold) |
|Vertical Movement      |Spacebar & C              |
|Change Weapons         |1 & 2 keys                |
|Spawn Enemies Rapidly  |T (hold)                  |

Your copter has two weapons: a machine gun and a cannon.  Note that the big dark green enemies (which are supposed to be tanks) are immune to machine gun bullets, so to kill them you'll need to hit '2' to switch to the cannon.

#To Build

Dependencies: OpenGL ES 2.0, GLEW, and SDL2.  On Debian systems these can be downloaded with the following commands:

```
sudo apt-get install libgles2-mesa-dev
sudo apt-get install libglew-dev
sudo apt-get install libsdl2-dev
```

To build natively, just run `make`.  To compile for the web, download [emscripten](https://kripken.github.io/emscripten-site/docs/getting_started/downloads.html), and then run the `webmake` script, which will dump it's output into the 'web' folder.
