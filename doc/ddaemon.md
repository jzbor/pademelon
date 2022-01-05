# Desktop Daemon Files

* Desktop daemon files end on `.ddaemon`
* The section name is the id-name of the daemon
    * It must not contain spaces or "special" characters

### Attributes
* `name`: Name to be displayed
* `description`: Description of what the daemon is/does
* `category`: Daemon category (see below)
* `command`: Shell command to launch daemon
* `test`: Command to test whether the daemon is installed/available
* `settings`: Settings application or file. Can be one of the following:
    * with prefix `config://`: config file relative to `~/.config`
    * with prefix `file://`: config file with absolute path (not as useful)
    * else: path to settings executable
* `default`: Whether the daemon serves as default/fallback for its category

### Categories
* `window-manager`: X11 Window Manager
* `compositor`: X11 Compositor (e.g. `picom`)
* `hotkeys`: Hotkey Daemon
* `notifications`: Notification Daemon
* `polkit`: Polkit GUI Provider
* `power`: Power Manager
* `status`: status script or bar

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
