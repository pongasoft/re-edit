To add new icons:

* Open application "Font Awesome 6 Subsetter"
* Load the project `external/fontawesome/src/re-edit.yaml`
* Add new icons
* Click "Build <N> Icon Subset" -> Generates a zip file
* Open zip and copy `metadata/icons.yml` and `webfonts/fa-solid-900.ttf` to `external/fontawesome/src/re-edit-icons`
* Run `python3 gen-re-edit-font.py`
* Move generated `IconsFAReEdit.h` and `IconsFAReEditCustom.h` to `external/fonts/includes`
* Make sure `binary_to_compressed_c` is built (build it if not using the CMake target)
* Run `/Volumes/Vault/deployment/build/re-edit/Debug/external/ocornut/imgui/binary_to_compressed_c -base85 /Volumes/Development/github/org.pongasoft/re-edit/external/fontawesome/src/re-edit-icons/webfonts/fa-solid-900.ttf IconsFAReEdit > /Volumes/Development/github/org.pongasoft/re-edit/external/fonts/src/IconsFAReEdit.cpp`
* Run `/Volumes/Vault/deployment/build/re-edit/Debug/external/ocornut/imgui/binary_to_compressed_c -base85 /Volumes/Development/github/org.pongasoft/re-edit/external/fontawesome/src/re-edit-icons/webfonts/custom-icons.ttf IconsFAReEditCustom > /Volumes/Development/github/org.pongasoft/re-edit/external/fonts/src/IconsFAReEditCustom.cpp`


As of 2023/08/19, due to issues with the newly introduced Font Awesome subsetter on the web:
* regular icons are still coming from the "Font Awesome 6 Subsetter" application. 
* `webfonts/custom-icons.ttf` is coming from the re-edit kit on Font Awesome website (downloading the web zip file)
* `metadata/custom-icons.yml` is manually generated
