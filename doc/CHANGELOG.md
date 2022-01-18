# Changelog

<!-- START doctoc.sh generated TOC please keep comment here to allow auto update -->
<!-- DO NOT EDIT THIS SECTION, INSTEAD RE-RUN doctoc.sh TO UPDATE -->
**Table of Contents**

- [0.3.0](#030)
- [0.2.0](#020)

<!-- END doctoc.sh generated TOC please keep comment here to allow auto update -->

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
