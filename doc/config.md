# Pademelon Config File

## Section: `daemons`

These keys require the id of their respective daemon files:
* `window-manager`: X11 Window Manager
* `compositor`: X11 Compositor (e.g. `picom`)
* `hotkeys`: Hotkey Daemon
* `notifications`: Notification Daemon
* `polkit`: Polkit GUI Provider
* `power`: Power Manager

These values should be specified as a boolean:
* `no-window-manager`: don't launch any window manager

## Section: `appearance`

These values should be specified as a boolean:
* `set-wallpaper`: let pademelon set wallpaper


