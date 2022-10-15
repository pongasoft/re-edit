To add new icons:

* Open application "Font Awesome 6 Subsetter"
* Load the project `external/fontawesome/src/re-edit.yaml`
* Add new icons
* Click "Build <N> Icon Subset" -> Generates a zip file
* Open zip and copy content of `metadata` and `webfonts` to `external/fontawesome/src/re-edit-icons`
* Run `python3 gen-re-edit-font.py`
* Move generated `IconsFAReEdit.h` to `external/fonts/includes`
* Make sure `binary_to_compressed_c` is built (build it if not using the CMake target)
* Run `/mnt/vault/deployment/build/re-edit/Debug/external/ocornut/imgui/binary_to_compressed_c -base85 /Volumes/Development/github/org.pongasoft/re-edit/external/fontawesome/src/re-edit-icons/webfonts/fa-solid-900.ttf IconsFAReEdit > /Volumes/Development/github/org.pongasoft/re-edit/external/fonts/src/IconsFAReEdit.cpp`
