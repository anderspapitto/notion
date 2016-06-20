Notion
======

Notion Screenshot

[notion_screenshot](https://i.imgur.com/W7szWlO.png)

Here you can see dzen2 panel and compton/xcompisiting support.

Notion Kernel ChangeLog:

* xft font support. It is not compatible with bitmap ones, because of
  preprocessor-based implementation.
* Xcompositing / Compton support
* Deleted mod_xinerama, mod_query, mod_menu. Instead we have rofi and dzen2
  statusbar integration. Scripts for it you can get from my dotfiles
* Tag-based navigation support for run-or-raise. For example you may create
  a tag "www" with firefox, chromium, chrome. When you hit M-w(in my config)
  you will get cyclic linked list  behaviour: firefox -> chrome -> chromium.
  It if useful for games or similar applications
* Scratchpads are similar on awesome/xmonad ones, you can bind an application
  to it
* Scratchpads are get highest z-order priority when selected, it is useful,
  I have no idea for what it must be hidden. In practice there is no need
  to close prev scratchpad if you are need to show new one.
* Another small improvements.

Notion Scripts Changes:

* app.lua -- tag support
* named_scratchpad.lua -- Scratchpads are get highest z-order priority when selected, it is useful,
  I have no idea for what it must be hidden. In practice there is no need
  to close prev scratchpad if you are need to show new one.
* hide_tabs -- hide tabs for everything. Including floating windows.
* directions -- 2bwm wasd navigation/moving
* screenshot -- to take screenshot with notification _after_.
* cfg_dzen -- two dzen2 panels support
* functions.lua -- misc functions
* transparency-lua -- autoset transparency for empty frames
* dzen_bg -- module to print information to dzen in coroutine

Copyright (c) the Notion team 2010-2015.
Copyright (c) Tuomo Valkonen 1999-2009.

http://notion.sf.net

