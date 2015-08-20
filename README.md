# citrus

citrus is a simplified 3DS homebrew library, originally known as ctrcommon. It is split into various distinct "modules":
 * core - Main module, initializes everything else and contains the most basic functions.
 * app - Application management functions.
 * battery - Battery status functions.
 * err - Error management functions.
 * fs - Filesystem functions that aren't covered by the standard library.
 * gpu - OpenGL-esque GPU functions.
 * gput - GPU Tools; provides a default shader, a matrix stack, and higher-level drawing functions.
 * hid - Input functions.
 * ir - Infrared communication functions.
 * news - Notification management functions.
 * nor - NVRAM access functions.
 * snd - Sound playback functions.
 * soc - Internal module for initializing and cleaning up sockets.
 * wifi - WiFi status functions.
 
Core functions:
 * ctr::core::init() - Must be called before using any citrus functions.
 * ctr::core::cleanup() - Must be called after you're finished using citrus functions, usually on exit.
 * ctr::core::running() - Performs the standard APT main loop and returns whether or not the application should terminate.
 * ctr::core::launcher() - Returns whether the application was launched from the homebrew launcher.
 * ctr::core::time() - Returns the current time in milliseconds.

citrus also comes with a small set of tools, including bannertool, makerom, template RSFs, and a template Makefile.

An example of citrus and its tools in use can be found [here](https://github.com/Steveice10/3DSHomebrewTemplate/).

Requires [devkitARM](http://sourceforge.net/projects/devkitpro/files/devkitARM/), [ctrulib](https://github.com/smealum/ctrulib), and [aemstro](https://github.com/smealum/aemstro) to build. Run 'make' to build, and run 'make install' to install it to your devkitPro directory.

# TODO
 * Standard UI module, to make the creation of UIs easy.
 * Figure out and add easy to use accelerometer/gyroscope functions.
