RE Edit
=======

Introduction
------------

RE Edit is a free and open source WYSIWYG editor for the UI of a Rack Extension, or in other words, a graphical editor for `hdgui_2D.lua` and `device_2D.lua`. Although the tool is currently limited to the UI only, it is already extremely useful and depending on feedback, it may be expanded, at a later date, to allow supporting editing ALL configuration files (motherboard_def.lua, display.lua, etc...).

> #### Note
> This README is about the _project_ itself not the _product_. If you want information about how to install and use the product, check the [RE Edit](https://pongasoft.com/re-edit/) product page.

Features implemented by the code
--------------------------------

- CMake based Dear ImGui application
- Uses raylib for the ImGui backend
- Uses custom shader for live image editing
- Render to texture for merging raylib + custom shader and ImGui
- Uses Dear ImGui docking branch to allow to dock windows (offer quick way to toggle between horizontal/vertical layouts)
- Clickable, zoomable "panel" where widgets can be freely moved around (100% ImGui code with rendered textures and mouse event handling)
- Uses a "Welcome" window which occupies the entire available space and properly resizes as the (native) window is resized 
- Native implementation preference management (load/save preferences as a lua file stored in the "default" preference location per platform)
- Native file browser (integrates [btzy/nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended))
- Remembers location/sizes when the application is closed and reopened
- Remembers the history of previously opened Rack Extensions
- Handle high resolution (4K/retina) on macOS properly (font is rendered super crisp ;) ) as well as resolution change (ex: dragging to a non retina monitor)
- Handle high resolution (4K) on Windows properly (using a scaling factor so that everything looks about right), as well as resolution change (ex: dragging to a low resolution monitor)
- Undo/Redo (Undo is challenging and requires merging events (ex: moving a widget around = one undo entry))
- Native implementation "open URL in browser"
- Native implementation network access (HTTPS GET only)
- Embedded lua parser (used by Rack Extensions as well as preferences for RE Edit)
- Asynchronous texture loading (which works out of the box on macOS/metal but is challenging on Windows/OpenGL3)
- Uses a different font (JetBrains Mono) and FontAwesome (embedded via base85 compressed encoding)
- Uses built-in icons (embedded via base85 compressed encoding)

Look & Feel
-----------

#### Dark Style
![RE Edit - Dark](https://github.com/pongasoft/re-edit/releases/download/v1.6.0/re-edit-dark.png)

#### Light Style
![RE Edit - Light](https://github.com/pongasoft/re-edit/releases/download/v1.6.0/re-edit-light.png)

Build
-----

* The project is a CMake based project and requires CMake 3.24+ to be installed.
* If you want to build the archive on Windows, you must install Wix (v3)
* All other dependencies are fetched automatically

> #### Note
> * It successfully builds on macOS 11.7.2/Xcode 13.2.1 (Intel only) as well as macOS 13.0.1/Xcode 14.1 (universal build).
> * It successfully builds on Windows 10 with Visual Studio 16 2019 build tools.


Clone this repository and run the following commands.

> #### Note
> * `configure.py` and `re-edit.py` are simple (python 3) wrappers around `cmake` invocation to make it less error-prone and build with the right set of parameters. Each command takes a `-n` option to show which actual `cmake` command is being invoked (dry-run).
> * Use `-h` option for help

### macOS
```
# will build the macOS app in release mode (-r option)
> ./configure.py -r
> cd build
> ./re-edit.py -r build
```


### Windows

```
# will build the Windows executable in release mode (-r option)
> python3 .\configure.py -r
> cd build
> python3 .\re-edit.py -r build
```

Feedback and Support
--------------------

Feedback and/or suggestions to improve the tool are more than welcomed. Feel free to open a ticket to discuss or [contact me](https://pongasoft.com/contact.html).

Sponsor
-------

pongasoft produces a variety of high quality and free/open source software. If you would like to support my work and help offset the cost of development tools, web hosting, etc. here are a couple ways to do so:

* [Send money](https://paypal.me/YanPujante) via PayPal
* [Sponsor Me](https://github.com/sponsors/ypujante) on GitHub

Release Notes
-------------
* #### 1.6.3 - 2024/02/12

- Fixed issue with contrast not being saved
 
* #### 1.6.2 - 2023/08/28

- Added "Commit All Effects", at the widget level as well as at the global level, to be able to remove all `re_edit_*` entries from `device_2D.lua` once the effects are applied (permanent/no more undo)
- Added concept of notifications to inform the user of important messages 
- Fixed instances where RE Edit would generate duplicate names
- Some performance improvements (textures are removed from the GPU when deleted)

* #### 1.6.1 - 2023/08/20

- Use a button (rather than a menu) for resizing the panel
- Added "Show Performance" menu entry to always show (resp. hide) the performance
- Added new icons, especially for widget visibility to make it clear that the user has manually changed it
- Fixed "Escape key" closes the application

* #### 1.6.0 - 2023/08/13

- Added "light" image editing/effects: resize, tint, brightness, contrast and flip (horizontal and vertical)
  * all effects are done live on the GPU
  * when the project is saved a new image with the effects applied is generated
  * `device_2D.lua` uses this image so that it works with Render2D
  * `device_2D.lua` also stores which original image + effects were used so that when reloaded in RE Edit, it works as well
  * Check it in action on [YouTube](https://youtu.be/r4xpHIiJcKM)
- Added a menu entry "File/Delete unused images" to figure out which images (under GUI2D) are used by the project and which ones are not
- Added a performance menu to tweak the frame rate
- Use `Alt` key to disable most filters (used when selecting images or properties in drop down lists)
- Internally, massive refactoring of the code to use raylib as the backend for ImGui:
  * much more common code between Windows and macOS
  * using custom OpenGL shader (for effects on GPU)
  * generate the panel (using raylib + custom shader) into a texture to be rendered as an image in ImGui

* #### 1.5.0 - 2023/05/22

- Added ability to drag and drop image(s) (png) from the Finder (resp. File Explorer) onto the main window to import them
- Use grid when adding widgets/decals
- Turned "Add Decal" into a menu showing the list of all graphics (since a decal is nothing more than a graphic)
- Shows the image in Quick View for graphics
- Added Quick View in lists of graphics (menu lists and dropdowns) to easily and quickly be able to pick the right graphic

* #### 1.4.2 - 2023/05/15

- Added Help menu: displays the keyboard shortcuts
- Uses re-mock 1.4.3 which means re-edit can be built without depending on the RE SDK

* #### 1.4.1 - 2023/05/07

- Added ability to load a project by simply dragging it from the Finder (resp. File Explorer) onto the main window
- Disable saving preferences when there are multiple instances running (to avoid conflicts)
- Fixed visibility property type and owner

* #### 1.4.0 - 2023/02/20

- Implemented Undo History / Timeline (quickly undo/redo multiple operations)
- Easily show/hide a widget even if there is no visibility property defined (eye icon)
- Added an "All" widgets tab in the Widgets window (in this tab, widgets are sorted by name). The "Widgets" and "Decals" tab remain the same and are sorted by Z-order (this is where you would go to change their respective orders).
- Major redesign of the "Properties" window to only show visibility properties currently in use
- Implemented "drag and drop" to easily add a widget to a visibility group (use "Alt" key to add to multiple groups): drag a widget from the Widgets window and drop onto a group in the Properties window (drag and drop also works from within the Properties window itself).
- Internal complete redesign of the Undo layer to be more flexible and allow "timeline"
 
* #### 1.3.0 - 2023/01/31

- Implemented Copy/Paste for bulk editing/changes
  * You can copy a single widget (copies all attributes), and it can be pasted into:
    * any panel where the type of the widget is allowed => duplicates the widget
    * another widget => copy all (possible) attributes to the widget
    * selected widgets => copy all (possible) attributes to all selected widgets
  * You can copy a single widget attribute value, and it can be pasted into:
      * another widget => copy this attribute to the widget (if possible)
      * selected widgets => copy this attribute to the selected widgets (if possible)
  * You can copy the selected widgets, and it can be pasted into:
      * any panel where the type of the widgets is allowed => duplicates the widgets
- Removed "Duplicate Widget(s)" menu entry since it is redundant with copy/paste (duplicate only allowed duplication on the same panel, copy/paste does not have this restriction)  
- Added Select All/Select By Type menu entries (use `Alt` to include hidden widgets)
- Simplified popup menu on Panel (added Widgets submenu instead of multiple menu entries)
- Added "A" keyboard shortcut to toggle between Select All/Select None
- Added "Q" keyboard shortcut to show a Quick View (while the key is being held). Essentially a tooltip on demand:
  * When hovering above a widget on the Panel
  * When hovering above a widget name in the "Widgets" window
  * When hovering above a property path
  * When hovering above a graphics attribute
  * When hovering above an entry in the "Open Recent" submenu
- Shift + click now also selects the widget under the mouse pointer (no need to move)
- Added more visible error icon in the tab bar
- Moved the notification section up to be sure it is always visible
- Various UI tweaks
- Fixed some issue with Undo/Redo

* #### 1.2.1 - 2023/01/22

- Fixed application name on macOS
- Fixed trackpad and mouse wheel sensitivity issue when zooming in/out

* #### 1.2.0 - 2023/01/20

- Major redesign of the panel: 
  - no more scrollbars
  - input shortcuts
    - click + drag to move freely (or space + click + drag if on top of a widget to disable selecting the widget)
    - arrows to move the panel
    - mouse wheel to zoom in/out (zoom focus point is wherever the mouse pointer is)
    - `X` to toggle Widget X-Ray
    - `F` for Zoom to fit
    - `C` for Center panel
    - `B` to toggle Widget borders
    - `R` to toggle Rails (+ panel X-Ray to see them)
- added "Clear Recent List" menu entry
- fixed crash on redo
- fixed improper error reporting when moving multiple widgets

* #### 1.1.0 - 2023/01/12

- Added Fold Icon / Rack Rails (thanks to @jengstrom at Reason Studios for the icons)
- Added Panel rendering (None / Border / Normal / X-Ray) (for example, to see the rails)
- Changed zoom to be common to all panels and added Zoom to Fit (now defaults when device opens)
- Added keyboard shortcuts (Save, Undo/Redo, Zoom In/Out/Fit, Quit)
- Fixes [#1](https://github.com/pongasoft/re-edit/issues/1): _RE Edit fails to load nested decals_
- Fixes [#2](https://github.com/pongasoft/re-edit/issues/2): _Decals are not rendered properly_
- Some minor performance improvements

* #### 1.0.0 - 2023/01/02

- First public release


Misc
----

This project uses the following open source projects (some of them are embedded in this source tree under `external`)

| Project                                                                                                     | License                                                                                            |
|-------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------|
| [craigsapp/midifile](https://github.com/craigsapp/midifile)                                                 | [BSD 2-Clause "Simplified" License](https://github.com/craigsapp/midifile/blob/master/LICENSE.txt) |
| [bitmask_operators](https://www.justsoftwaresolutions.co.uk/cplusplus/using-enum-classes-as-bitfields.html) | [Boost Software License](http://www.boost.org/LICENSE_1_0.txt)                                     | 
| [btzy/nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended)                         | [ZLib license](https://github.com/btzy/nativefiledialog-extended/blob/master/LICENSE)              |
| [Dear ImGui](https://github.com/ocornut/imgui)                                                              | [MIT License](https://github.com/ocornut/imgui/blob/master/LICENSE.txt)                            |
| [glfw](https://www.glfw.org)                                                                                | [ZLib license](https://www.glfw.org/license.html)                                                  |
| [libsndfile](https://github.com/libsndfile/libsndfile)                                                      | [LGPL-2.1 License](https://github.com/libsndfile/libsndfile/blob/master/COPYING)                   |
| [Lua](https://www.lua.org)                                                                                  | [MIT License](https://www.lua.org/license.html)                                                    |
| [lubgr/lua-cmake](https://github.com/lubgr/lua-cmake)                                                       | [MIT License](https://github.com/lubgr/lua-cmake/blob/master/LICENSE)                              |
| [nlohmann/json](https://github.com/nlohmann/json)                                                           | [MIT License](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT)                           |
| [nothings/stb](https://github.com/nothings/stb)                                                             | [Public Domain](https://github.com/nothings/stb/blob/master/docs/why_public_domain.md)             |
| [pongasoft/re-mock](https://github.com/pongasoft/re-mock)                                                   | [Apache 2.0](https://github.com/pongasoft/re-mock/blob/master/LICENSE.txt)                         |
| [raylib](https://www.raylib.com/)                                                                           | [ZLIB License](https://github.com/raysan5/raylib/blob/master/LICENSE)                              |
| [raylib-extras/rlImGui](https://github.com/raylib-extras/rlImGui)                                           | [ZLIB License](https://github.com/raylib-extras/rlImGui/blob/main/LICENSE)                         |
| [SpartanJ/efsw](https://github.com/SpartanJ/efsw)                                                           | [MIT License](https://github.com/SpartanJ/efsw/blob/master/LICENSE)                                |

Licensing
---------

- Apache 2.0 License. This project can be used according to the terms of the Apache 2.0 license.

