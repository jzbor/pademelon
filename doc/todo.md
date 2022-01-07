# Tasks

## Before 0.1.0
* ~~Add documentation for config and daemon files~~
* ~~Implement category overwrite in `pademelon-daemon`~~
* ~~Improve test implementation in `pademelon-daemon` with `system()`~~
* ~~Save last display layout with unxrandr~~
* ~~`pademelon-settings`: display warning when selecting daemon that is not installed~~
* ~~Support for setting the keyboard-layout via `setxkbmap`~~
* ~~**Generalize Daemon Concept for Applications:**~~
* ~~Fix settings implementation in `pademelon-settings`~~
* ~~Check for freeing in ro segements (`free_ddaemon()`)~~
* ~~Verify `no-window-manager` functionality~~
* ~~introduce `none` value for application settings~~
* ~~Make x11 and imlib2 support optional~~
* ~~use last *working* application instead of just last~~
* add `free_applications()`
* `tl_application_select()` - load user preferences

## Before 0.2.0
* Implement basic external support for volume, backlight and ~~keyboard layout~~
* Proper help screens for tools
* Add `print_config()` to tools
* Timeout for test command

## Before 1.0.0
* Add "optional" daemon category for stuff like redshift
* Add some more options for window-managers
* Support for selecting ARandr profiles
* Config template loading
* Implement SIGUSR1 to restart daemons
* Rework and verify options for `pademelon-daemon`
