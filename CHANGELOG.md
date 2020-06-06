## Version: 7.1
Date: 2020-06-06 22:20:00

Group many other author's efforts in single place.

 * Remove former homebrew build scrits in favour of cmake (Alexandr Kozlinskiy <akozlins@gmail.com> and me)
 * g_type_class_add_private() was deprecated, use other methods in this place (Mark Mohr <markm11@charter.net>)
 * Fixed GLib-GObject-CRITICAL assertions showing when loading fbpanel (Mark Mohr <markm11@charter.net>)
 * Added plugin to display battery usage in text form (Fred Stober <stober@cern.ch>)
 * Allow taskbar item to have height up to 64 (Frederic Guilbault <fred@0464.ca>)
 * Add pango support genmon (borrowed from openbsd ports)
 * Added Italian translation (Man from Mars <martianpostbox@gmail.com> via  Eugenio Paolantonio (g7) <me@medesimo.eu>)
 * Some other minor changes, thanks to openbsd community and clang

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
