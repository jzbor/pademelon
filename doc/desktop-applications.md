# Desktop Application Files

* Desktop application files end on `.dapp`
* The section name is the id-name of the application
    * It must not contain spaces or "special" characters

### Attributes

* `name`: Name to be displayed
* `description`: Description of what the application is/does
* `category`: Application category (see below)
* `command`: Shell command to launch application
* `test`: Command to test whether the application is installed/available
* `settings`: Settings application or file. Can be one of the following:
    * with prefix `config://`: config file relative to `~/.config`
    * with prefix `file://`: config file with absolute path (not as useful)
    * else: path to settings executable
* `default`: Whether the application serves as default/fallback for its category

### Categories

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

## Example
```ini
[xfce4-power-manager]
name = XFCE4 Power Manager
description = Power manager for the Xfce desktop
category = power
command = /usr/bin/xfce4-power-manager
test = test -x /usr/bin/xfce4-power-manager
settings = /usr/bin/xfce4-power-manager-settings
default = True
```
