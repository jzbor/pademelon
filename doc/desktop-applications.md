# Categories

These are daemons - they are started once and run in the background:

* `window-manager`: X11 Window Manager
* `compositor`: X11 Compositor (e.g. `picom`)
* `hotkeys`: Hotkey Daemon
* `notifications`: Notification Daemon
* `polkit`: Polkit GUI Provider
* `power`: Power Manager
* `status`: status script or bar

These are regular applications - currently pademelon only exports them to their environment variables and only if they are not set already (for example through `~/.profile`):

* `browser`: Web Browser (exported to `$BROWSER`)
* `terminal`: Terminal Emulator (exported to `$TERMINAL`)
