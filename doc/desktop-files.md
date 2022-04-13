# Pademelon Desktop Files

To get a list of available applications pademelon uses `.desktop` files as specified in the
[XDG Desktop Entry Specification](https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html) and the [XDG Desktop Menu Specification](https://specifications.freedesktop.org/menu-spec/menu-spec-latest.html#desktop-entry-extensions-examples).

Pademelon does **not** look in the default application directories.
Instead looks for desktop files provided in `PREFIX/pademelon/applications` and
`$XDG_DATA_HOME/pademelon/applications`.

Window Managers are also read from files in these directories in contrast to the `xsessions`
directory, which display managers normally use.
The rationale behind this is that most Window Managers specify some sort of wrapper in their
`xsessions` entry, which is often not compatible with Pademelon.

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
`Applet` (`TrayIcon`), `FileManager`, `Optional`, `TerminalEmulator`, `WebBrowser`

Additionally pademelon introduces the following categories:
`AppLauncher`, `Autostart`, `Dock`, `HotkeyDaemon`, `NotificationDaemon`, `Polkit`, `PowerManager`,
`Status`, (`Panel`), `X11Compositor`, `X11WindowManager`
