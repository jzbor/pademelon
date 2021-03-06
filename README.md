# Pademelon

A desktop manager for modular Linux desktop setups.

## What does this do?
Pademelon ties together different software and tools to get a working desktop out of the box.
It aims to be completely modular, so you can swap out everything as you go.
Still one important goal of this software is to provide a desktop that works out of the box -
especially with window managers like DWM or i3.

## What is a pademelon?
[This.](https://en.wikipedia.org/wiki/Pademelon)

## Features
* Launch different daemons required to get a fully working desktop
* Graphical settings application for basic setup and default applications
* Simple, human-readable configuration files
* Modular and easily extendable concept
* Bindings for wallpaper setting, volume and backlight
* Save xrandr configuration and restore on restart

## Default software
This is a curated set of applications that work well together and
provide a user-friendly tiling WM experience.
Pademelon provides a default config using these applications, which is also available
in the [AUR](https://aur.archlinux.org/packages/pademelon-desktop).

* **Window Manager:** MoonWM
* **Compositor:** Picom
* **Notifications:** xfce4-notifyd
* **Power Management:** xfce4-power-manager
* **Polkit:** gnome-polkit
* **Terminal:** XFCE4 Terminal
* **Applets:** nm-applet

## Dependencies

### Dependencies on external executables
* **ARandR:** Configure display layouts
* **LXAppearance**: Customize Look and Feel
* **setxkbmap:** Set the keyboard map
* **xbacklight** or **acpilight:** Control display backlight
* **libpulse (`pactl`):** Volume control

### Libraries
* **Imlib2**
* **XLib**
* **Xrandr**
* **libcanberra**
* **libinih**
* **pkg-config** (only at build time)
* **python-gobject**
