# Support for .desktop files (WIP)

_This is a work-in-progess proposal and not yet implemented in any release of pademelon._

To get a list of available applications pademelon uses `.desktop` files as specified in the
[XDG Desktop Entry Specification](https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html) and the [XDG Desktop Menu Specification](https://specifications.freedesktop.org/menu-spec/menu-spec-latest.html#desktop-entry-extensions-examples).
In addition to the paths in `$XDG_DATA_DIRS/applications` and `$XDG_DATA_HOME/applications` pademelon
shall also provide overwrite paths in `PREFIX/pademelon/applications` and `$XDG_DATA_HOME/pademelon/applications`.

## Fields

Pademelon uses the following fields from the XDG Desktop Entry Specification:

* `Name`: display name
* `Comment`: application description
* `Exec`: command to execute application
* `TryExec`: test if the application is available
* `Categories`: category of application (see below for extension)
    * this is a required field in the context of pademelon

Pademelon also extends the specification by the following values to fit its needs:

* `X-Pademelon-Settings`: settings executable or file for the application
    * with prefix `config://`: config file relative to `$XDG_CONFIG_HOME`
    * with prefix `file://`: config file with absolute path (not as useful)
    * else: path to settings executable

## Categories

Pademelon makes use of the following categories as listed in the XDG Desktop Menu Specification:
`WebBrowser`, `TerminalEmulator`, `FileManager`, `Applet` (`TrayIcon`)

Additionally pademelon introduces the following categories:
`HotkeyDaemon`, `X11Compositor`, `NotificationDaemon`, `Polkit`, `PowerManager`, `Dock`, `Status`, (`Panel`), `AppLauncher`, `Autostart`

## Todo
* How to handle window managers?
