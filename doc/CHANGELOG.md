# Changelog

<!-- START doctoc.sh generated TOC please keep comment here to allow auto update -->
<!-- DO NOT EDIT THIS SECTION, INSTEAD RE-RUN doctoc.sh TO UPDATE -->
**Table of Contents**

- [1.0.0](#100)
- [0.3.0](#030)
- [0.2.0](#020)

<!-- END doctoc.sh generated TOC please keep comment here to allow auto update -->

## 1.0.0
* Monitor plugged in keyboards to update keymap
* Adding muting to volume tool
* Change `SIGUSR1` to reloading everything
* Ignore `SIGUSR2` for now
* Adding `xdg-xmenu`
* Adding filemanager category
* Improved responsiveness on canberra audio feedback

## 0.3.0
* Adding volume and backlight control via `pactl` and `xbacklight` (use *acpilight* to get acpi backlight support)
* Argument parser and help option for `pademelon-tools`
* Improved display handling of wallpaper setter

## 0.2.0
* Improved wallpaper setter
* Application Menu category (`dmenu`)
* Dock category (`dock`)
* Restart daemons from settings app or with signal (`SIGUSR1`)
* Restart window manager from settings app or with signal (`SIGUSR2`)
* Adding more window manager options: Fluxbox, Openbox, DWM
