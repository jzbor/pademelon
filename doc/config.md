# Pademelon Config File

## Section: `daemons`

These keys require the id of their respective application files:
* `window-manager`: X11 Window Manager
* `compositor`: X11 Compositor (e.g. `picom`)
* `hotkeys`: Hotkey Daemon
* `notifications`: Notification Daemon
* `polkit`: Polkit GUI Provider
* `power`: Power Manager
* `status`: Status script or status bar

These values should be specified as a boolean:
* `no-window-manager`: don't launch any window manager

## Section: `input`

* `keyboard-layout`: keyboard layout as defined by `setxkbmap(1)` and `xkeyboard-config(7)`


