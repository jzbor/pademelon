# Changelog

<!-- START doctoc.sh generated TOC please keep comment here to allow auto update -->
<!-- DO NOT EDIT THIS SECTION, INSTEAD RE-RUN doctoc.sh TO UPDATE -->
**Table of Contents**

- [1.1.0](#110)
- [1.0.4](#104)
- [1.0.3](#103)
- [1.0.2](#102)
- [1.0.1](#101)
- [1.0.0](#100)
- [0.3.0](#030)
- [0.2.0](#020)

<!-- END doctoc.sh generated TOC please keep comment here to allow auto update -->

## 1.1.0
* Switching application file format to `.desktop` files (**BREAKING CHANGE!!**)
* Adding `pademelon-widgets` for controlling stuff like poweroff dialogs or switching power profiles
* Adding various applications to application files

## 1.0.4
* Fixing permissions for saved display configuration
* Speeding up launching by setting wallpaper after launching WM

## 1.0.3
* Fixing data dir not getting created for tl_save_display_conf

## 1.0.2
* Properly terminating applications on exit
* Reduce memory consumption by dropping image cache
* Fixing bug in xdg-xmenu

## 1.0.1
* Improving desktop entries
* Re-export settings if set through pademelon-daemon

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
