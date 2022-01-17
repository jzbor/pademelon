# Pademelon

A desktop manager for modular Linux desktop setups.

## What does this do?
Pademelon ties together different software and tools to get a working desktop out of the box.
It aims to be completely modular, so you can swap out everything as you go.
Still one important goal of this software is to provide a desktop that works out of the box - especially with window managers like DWM or i3.

## Features
* Launch different daemons required to get a fully working desktop
* Graphical application for basic settings
* Simple, human-readable configuration files
* Modular and easily extendable concept
* Setting and updating the wallpaper

[Planned Features](doc/todo.md)

## Dependencies

### Default software
* **Window Manager:** MoonWM
* **Compositor:** Picom
* **Hotkeys:** sxhkd
* **Notifications:** xfce4-notifyd
* **Power Management:** xfce4-power-manager
* **Polkit:** gnome-polkit
* **Display Manager:** LightDM

### Dependencies on external executables
* **ARandR:** Configure display layouts
* **LXAppearance**: Customize Look and Feel
* **setxkbmap:** Set the keyboard map
* **xbacklight** or **acpilight:** Control display backlight
* **libpulse (`pactl`):** Volume control

### Libraries
* **XLib**
* **Xrandr**
* **Imlib2**
* **libinih**
* **pkg-config** (only at build time)
* **python-gobject**
