## Version: 7.2
* Add cmake rules to build fbpanel
* Get rid of previous python2-bsed build system
* Update documetation
* Extend max amount of buttons 20->40 on launchbar


## Version: 7.1

Date: 2020-06-07 20:20:00

* Reimplement 'Xinerama support' patch for 7.0
* Fix some deprecation warnings
* Added plugin to display battery usage in text form
* Added Italian translation
* Added icon drawing support to the pager plugin
* Some other assorted fixes


## Version: 7.0

Date: 2015-12-05 08:25:36

* [#12] fix menu position for top panel
* [#11] new plugin: user menu with gravatar icon
* [#8] Fix for issue #5 (make battery plugin work with /sys)
* [#6] Rounded corners don't work with widthtype=request
* [#5] make battery plugin work with /sys
* [#4] update README
* [#2] Include option for vertical (y) and horizontal (x) margin

[#12]: https://github.com/aanatoly/fbpanel/issues/12
[#11]: https://github.com/aanatoly/fbpanel/issues/11
[#8]: https://github.com/aanatoly/fbpanel/pull/8
[#6]: https://github.com/aanatoly/fbpanel/issues/6
[#5]: https://github.com/aanatoly/fbpanel/issues/5
[#4]: https://github.com/aanatoly/fbpanel/issues/4
[#2]: https://github.com/aanatoly/fbpanel/issues/2


## Version: 6.2

* 3367953: 'move to desktop' item in taskbar menu


## Version: 6.1

New Features:

* 2977832: meter plugin  - base plugin for icons slide show
* 2977833: battery plugin
* 2981332: volume plugin
* 2981313: Enhancements to 'tclock' plugin - calendar and transparency
* multiline taskbar: new config MinTaskHeight was added to set minimal
  task/row height
* multiline launchbar: row height is MaxIconSize
* scrolling on panel changes desktops
* dclock vertical layout was implemented. It still draws digits
  horizontaly if there is enough space
* new global config MaxEelemHeight was added to limit plugin elements
  (eg icons, messages) height
* 993836: add GTK frame to non-transparent chart plugins

Fixed Bugs:

* 2990621: add charging icons to battery plugin
* 2993878: set menu icons size from panel config not from gtk rc
* fixed locale/NLS issues with configure
* chart class struct was made static
* 2979388: configure broken - problems in busybox environments
* fixing variable name check in configure
* 2985792: Menu disappears too quickly
* 2990610: panel with autohide disappears when positioned at (0,0)
* 2990620: fix dclock for vertical orientation
* 2991081: do not autohide panel when menu is open
* 3002021: reduce sensitive area of hidden panel


## Version: 6.0

* adding xlogout script
* fixing cpu and net plugins to recover after /proc read errors
* menu: new code to build system menu
* GUI configurator code was reworked
* common API to run external programs was added
* new configuration system - xconf - was introduced
* adding png icons for reboot and shutdown. They may be missing in some icon themes.
* all svg icons were removed
* automatic profile created
* show calendar as default action in dclock
* fixed 'toggle iconfig all' algorithm
* 2863566: Allow seconds in dclock plugin
* 2972256: rebuild code upon makefile changes
* 2965428: fbpanel dissapears when configuring via GUI
* 2958238: 5.6 has bugs in configure script and fails to load plugins
* 2953357: Crashes when staring Opera


## Version: 5.8

* moving config dir ~/.config
* automatic new profile creation
* removing app categories default icons
* dclock plugin pops up calendar if no action was set
* net plugin got detailed tooltip and color configs
* cpu plugin got detailed tooltip and color configs
* mem plugin was made
* allocating plugin's private section as part of instance malloc
* drag and drop fix in launchbar
* Fixed "2891558: widthtype=request does not work"


## Version: 5.7

* XRandR support (dynamic desktop geometry changes)
* Fixed "2891558: widthtype=request does not work"
* configurator redraws panel on global changes
* fixing 'toggle iconify all' algorithm


## Version: 5.6

* genmon plugin - displays command output in a panel
* CFLAGS propagation fix

## Version: 5.5

* adding static build option for debugin purposes e.g to use with valgrind
* ability to set CFLAGS from command line was added. 
  make CFLAGS=bla-bla works correctly
* fixing memory leaks in taskbar, menu and icons plugin

## Version: 5.4

* fb_image and icon loading code refactoring
* chart: making frame around a chart more distinguishable
* taskbar: enable tooltips in IconsOnly mode
* taskbar: build tooltips from text rather then from markup


## Version: 5.3

* when no icon exists in a theme, missing-image icon is substituted. theme-avare
* prevent duplicate entries in menu
* menu plugin uses simple icons, and rebuild entire menu upon theme change, rather then creating many heavy theme-aware icons, and let them update
* cpu, net plugins: linux specific code was put into ifdefs and stub for another case was created
* system menu icon was renamed to logo.png from star.png
* strip moved to separete target and not done automatically after compile
* by default make leaves output as is, to see summaries only run 'make Q=1'
* enbling dependency checking by default
* adding svn ebuild fbpanel-2009.ebuild
* adding tooltips to cpu and net plugins
* BgBox use BG_STYLE by default
* close_profile function was added to group relevant stuff
* autohide was simplified. Now it hides completly and ignoress heightWhenHidden


## Version: 5.2

* fixing segfault in menu plugin
* extra spaces in lunchbar plugin were removed
* replacing obsolete GtkTooltips with GtkTooltip
* plugins' install path is set to LIBDIR/fbpanel instead of LIBEXECDIR/fbpanel
* fixing short flash of wrong background on startup


## Version: 5.1

* Tooltips can have mark-uped text, like '<b>T</b>erminal'
* Cpu plugin is fixed and working
* Added general chart plugin (used by cpu and net monitors)
* Code layout was changed, new configure system and new makefiles set was adopted
* fixed segfault in taskbar plugin
* background pixmap drawing speed ups and bugfixes

## Version: 4.13

New Features:

* support for "above all" and "below all" layering states. Global section
  was added string variable
      Layer = None | Above | Below
* to speed start-up, panel does not have window icon, only configuator window has
* Control-Button3 click launches configureation dialog
* taskbar was changed to propagate Control-Button3 clicks to parent window i.e to panel
* launchbar was changed to propagate Control-Button3 clicks to parent window
* pager was changed to propagate Control-Button3 clicks to parent window
* dclock was changed to propagate Control-Button3 clicks to parent window
* menu was changed to propagate Control-Button3 clicks to parent window
* normal support for round corners. Config file gets new global integer option - RoundCornersRadius
* system tray transparency fix
* clock startup delay was removed
* menu: fixed segfault caused by timeout func that used stale pointer


## Version: 4.12

New Features:

* smooth icon theme change without panel reload
* autohide. Config section is part of 'Global' section

```ini
    autoHide = false
    heightWhenHidden = 2
```

* 3 sec delayed menu creation to improve start-up time

Fixed Bugs:

* icons, taskbar do not free all tasks when destroyed


## Version: 4.11

Fixed Bugs:

* black background when no bg pixmap and transparency is unset


## Version: 4.10

New Features:

* tclock: dclock was renamed to tclock = text clock
* dclock: digital blue clock. adopted from blueclock by Jochen Baier <email@Jochen-Baier.de>
* dclock: custom clock color can be set with 'color' option

```json
  Plugin {
     type = dclock
     config {
         TooltipFmt = %A %x
         Action = xterm &
         color = wheat
     }
  }
```

* menu: items are sorted by name
* menu: icon size set to 22
* launchbar: drag-n-drop now accepts urls draged from web browsers
* style changes are grouped and only last of them is processed

Fixed Bugs:

* menu: forgoten g_free's were added
* 1723786: linkage problems with --as-needed flag
* 1724852: crash if root bg is not set
* WM_STATE usage is dropped. NET_WM_STATE is used instead. affected plugins are
  taskbar and pager
* fixed bug where pager used unupdated panel->desknum instead of pager->desknum
* all Cardinal vars were changed to guint from int
* bug in Makefile.common that generated wrong names in *.dep files
* style changes are grouped and only last of them is processed


## Version: 4.9

* new menu placement to not cover panel; used in menu and taskbar
* taskbar: icons were added to task's menu (raise, iconify, close)
* access to WM_HINTS is done via XGetWMHints only and not via get_xa_property; in taskbar it fixes failure to see existing icon pixmap
* 1704709: config checks for installed devel packages


## Version: 4.8

* help text in configurator was made selectable
* pager shows desktop wallpaper
* expanding tilda (~) in action field in config files
* menu icons size was set to 24 from 22 to avoid scaling
* avoid re-moving panel to same position
* plugins section in configurator dialog suggests to edit config manually
* taskbar vertical layout was fixed
* taskbar 'icons only' mode was optimized
* fbpanel config window has nice "star" icon


## Version: 4.7

New Feature

* Build application menu from *.desktop files
* Using themed icons. Change icon theme and see fbpanel updates itself
* default config files were updates to use new functionality


## Version: 4.6

New Features

* [ 1295234 ] Detect Window "Urgency".
* Raise window when drag target is over its name in taskbar
* fixing meory leaks from XGetWindowProperty.
* fix urgency code to catch up urgency of new windows
* taskbar: correct position of task's label
* taskbar: remove extra spaces
* taskbar: do not create event box beneath task button
* taskbar: use default expose method in gtk_bar
* taskbar; use default expose method in task button
* taskbar: cleaning up dnd code
* launchbar: visual feedback on button press


## Version: 4.5

Fixed bugs

* Makefile.common overwrite/ignore CFLAGS and LDFLAGS env. variables
* rebuild dependancy Makefiles (*.dep) if their prerequisits were changed
* fixing gcc-4.1 compile warnings about signess
* removing tar from make's recursive goals
* fixing NET_WM_STRUT code to work on 64 bit platforms

New features

* porting plugins/taskbar to 64 bit
* porting plugins/icons to 64 bit
* adding LDFLAGS=-Wl,-O1 to Makefile
* adding deskno2 plugin; it shows current desktop name and allow to scroll over available desktops
* applying patch [ 1062173 ] NET_ACTIVE_WINDOW support
* hiding tray when there are no tray icons
* remove extra space around tray
* using new icons from etiquette theme. droping old ones


## Version: 4.4

New Feature

* 64-bit awarenes


## Version: 4.3

New Feature

* [1208377] raise and iconify windows with mouse wheel
* [1210550] makefile option to compile plugins statically
* makefile help was added. run 'make help' to get it
* deskno gui changes

Fixed Bugs

* deskno can't be staticaly compiled
* typo fixes
* Makefile errors for shared and static plugin build

## Version: 4.2

Fixed Bugs

* [1161921] menu image is too small
* [1106944] ERROR used before int declaration breaks build
* [1106946] -isystem needs space?
* [1206383] makefile fails if CFLAGS set on command line
* [1206385] DnD in launchbar fails if url has a space
* fixed typos in error messages

New Feature

* New code for panel's buttons. Affected plugins are wincmd, launchbar and menu
* Depreceted option menu widget was replaced by combo box
* sys tray is packed into shadowed in frame
* pad is inserted betwean tasks in a taskbar
* clock was made flat

## Version: 4.1

New Feature

* gui configuration utility
* transparency bug fixes

## Version: 4.0

New Feature

* plugins get root events via panel's proxy rather then directly
* added configure option to disable cpu plugin compilation

## Version: 3.18

New Feature

* [ 1071997 ] deskno - plugin that displays current workspace number

Fixed Bugs

* [ 1067515 ] Fixed bug with cpu monitor plugin


## Version: 3.17

Fixed Bugs

* [ 1063620 ] 3.16 crashes with gaim 1.0.2 sys tray applet

New Feature

* [ 1062524 ] CPU usage monitor


## Version: 3.16
New Feature
* taskbar does not change window icons anymore.
* invisible (no-gui) plugin type was introduced
* icons plugin was implemented. it is invisible plugin used to  changes
  window icons with desktop-wide effect.


## Version: 3.15

Fixed Bugs

* [ 1061036 ] segfault if tray restarted


## Version: 3.14

New Feature
* [ 1010699 ] A space-filler plugin
* [ 1057046 ] transparency support
* all static plugins were converted to dlls
* added -verbose command line option

Fixed Bugs

* dynamic module load fix

## Version: 3.13

New Feature

* [ 953451 ] Add include functionality for menu config file.

Fixed Bugs

* [ 1055257 ] crash with nautilus+openbox

## Version: 3.12

New Features

* [ 976592 ] Right-click Context menu for the taskbar


## Version: 3.11

* fixed [ 940441 ] pager loose track of windows


## Version: 3.10

* fix for "996174: dclock's 'WARNING **: Invalid UTF8 string'"
* config file fix


## Version: 3.9

* fix bg change in non transparent mode
* enable icon only in taskbar
* ensure all-desktop presence if starting before wm (eg openbox)
* wincmd segfault fix


## Version: 3.8

* warnings clean-up
* X11 memory leacher was fixed
* taskbar can be set to show only mapped/iconified and wins from other desktops
* transparency initial support
* gtkbar was ported to gtk2, so fbpanel is compiled with GTK_DISABLE_DEPRECETED
* initial dll support


## Version: 3.7

* rounded corners (optional)
* taskbar view fix


## Version: 3.6

* taskbar icon size fix
* menu icon size fix
* pager checks for drawable pixmap


## Version: 3.5

* Drag-n-Drop for launchbar
* menu plugin
* removed limith for max task size in taskbar


## Version: 3.4

* gtk2.2 linkage fix
* strut fix
* launchbar segfault on wrong config fix
* '&' at the end of action var in launchbar config is depreciated


## Version: 3.3

* taskbar icon size fix


## Version: 3.2

* scroll mouse in pager changes desktops
* packaging and makefiles now are ready for system wide install additionally ./configure was implemented
* systray checks for another tray already running


## Version: 3.1

* improving icon quility in taskbar
* system tray (aka notification area) support
* NET_WM_STRUT_PARTIAL and NET_WM_STRUT were implmented
* taskbar update icon image on every icon change


## Version: 3.0

* official version bump :-)


## Version: 3.0-rc-1

* porting to GTK2+. port is based on phako's patch "[ 678749 ] make it compile and work with gtk2"


## Version: 2.2

* support for XEmbed docklets via gtktray utility


## Version: 2.1

* tray plugin was written
* documentation update
* web site update


## Version: 2.0

* complete engine rewrite
* new plugin API
* pager fixes


## Version: 1.4

* bug-fixes for pager plugin


## Version: 1.3

* middle-click in taskbar will toggle shaded state of a window
* added image plugin - this is simple plugin that just shows an image
* pager eye-candy fixes
* close_module function update


## Version: 1.2

* we've got new module - pager! Yeeaa-Haa!!
* segfault on wrong config file was fixed


## Version: 1.1

* parsing engine was rewritten
* modules' static variables were converted to mallocs
* configurable size and postion of a panel
* ability to specify what modules to load
* '~' is accepted in config files


## Version: 1.0

* 1.0-rc2 was released as 1.0


## Version: 1.0-rc2

* taskbar config file was added an option to switch tooltips on/off
* added tooltips to taskbar (thanks to Joe MacDonald joe@deserted.net)


## Version: 1.0-rc1

* copyright comments were changed


## Version: 1.0-rc0

* added _NET_WM_STRUT support
* panel now is unfocusable. this fixes iconify bug under sawfish
* panel's height is calculated at run-time, instead of fixed 22


## Version: 0.11

* improved EWMH/NETWM support
* added openbox support
* added clock customization (thanks to Tooar tooar@gmx.net)
* README was rewrited
* bug fixes
