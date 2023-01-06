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
- Uses glfw/metal on macOS (uses `metal-cpp` so that `main.cpp` is a cpp file, not Objective-C like in the example provided by Dear ImGui)
- Uses glfw/OpenGL3 on Windows
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
![RE Edit - Dark](https://github.com/pongasoft/re-edit/releases/download/v1.0.0/re-edit-dark.png)

#### Light Style
![RE Edit - Light](https://github.com/pongasoft/re-edit/releases/download/v1.0.0/re-edit-light.png)

Build
-----

* The project is a CMake based project and requires CMake 3.24+ to be installed.
* The Rack Extension SDK must be installed. Please follow the instructions provided with [re-mock](https://github.com/pongasoft/re-mock) for details.
* If you want to build the archive on Windows, you must install Wix (v3)
* All other dependencies are fetched automatically

> #### Note
> * It successfully builds on macOS 11.7.2/Xcode 13.2.1(Intel only) as well as macOS 13.0.1/Xcode 14.1 (universal build).
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
| [SpartanJ/efsw](https://github.com/SpartanJ/efsw)                                                           | [MIT License](https://github.com/SpartanJ/efsw/blob/master/LICENSE)                                |

Licensing
---------

- Apache 2.0 License. This project can be used according to the terms of the Apache 2.0 license.
