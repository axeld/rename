# rename
version 0.2.0 (23.3.2024)

### introduction.
It can be used as a Tracker add-on and as a command line utility.

### requirements.
Any Haiku release after Beta 1 will do.

### installation.
You can copy the application "rename" wherever you want to. If you want to use it frequently, you should place it within your path, e.g. /boot/home/config/bin.
The Tracker add-on "Rename files" should be in /boot/home/config/add-ons/Tracker/. Since there is only one file, you can either copy it to both locations, or create a symlink from one to the other.
The "Install" script part of this archive will it install it in the former way, so that you can safely rename the Tracker add-on to suit your needs.

### usage.
If you run "rename" without any arguments, a short help message is printed.Command line mode has not been implemented yet; it's a GUI application only at this point.

```sh
rename [-vr] <list of directories/files>
	-v	verbose mode
	-u	show UI
```

### history.
version 0.1.0 (22.3.2019)
 - initial release.

version 0.2.0 (23.3.2024)
 - automatically detects when launched by Tracker, and shows UI

### author.
"rename" is written by Axel Dörfler <axeld@pinc-software.de>.
visit: www.pinc-software.de

Have fun.
