### New stable release: 0.12.3
Changes: https://gitlab.com/o9000/tint2/blob/0.12.3/ChangeLog

Documentation: https://gitlab.com/o9000/tint2/wikis/Configure

Try it out with (see also [dependencies](https://gitlab.com/o9000/tint2/wikis/Install#dependencies)):
```
git clone https://gitlab.com/o9000/tint2.git
cd tint2
<<<<<<< HEAD
git checkout 0.12.2
=======
git checkout 0.12.3
>>>>>>> 0.12.3
mkdir build
cd build
cmake ..
make -j4
./tint2 &
./src/tint2conf/tint2conf &
```

To install from source, also run (as root):
```
make install
```

Please report any problems to https://gitlab.com/o9000/tint2/issues. Your feedback is much appreciated.

Known issues:
* [System tray resize loop with GTK applications](https://gitlab.com/o9000/tint2/issues/509), see also the [GTK bug report](https://bugzilla.gnome.org/show_bug.cgi?id=710375). Fix landed in 0.12.1, if there are still problems please let me know.

P.S. GitLab is now the official location of the tint2 project, migrated from Google Code, which is shutting down. In case you are wondering why not GitHub, BitBucket etc., we chose GitLab because it is open source, it is mature and works well, looks cool and has a very nice team.

### What is tint2?

tint2 is a simple panel/taskbar made for modern X window managers. It was specifically made for Openbox but it should also work with other window managers (GNOME, KDE, XFCE etc.). It is based on ttm http://code.google.com/p/ttm/.

### Features

  * Panel with taskbar, system tray, clock and launcher icons;
  * Easy to customize: color/transparency on fonts, icons, borders and backgrounds;
  * Pager like capability: move tasks between workspaces (virtual desktops), switch between workspaces;
  * Multi-monitor capability: create one panel per monitor, showing only the tasks from the current monitor;
  * Customizable mouse events.

### Goals

  * Be unintrusive and light (in terms of memory, CPU and aesthetic);
  * Follow the freedesktop.org specifications;
  * Make certain workflows, such as multi-desktop and multi-monitor, easy to use.

### I want it!

  * [Install tint2](https://gitlab.com/o9000/tint2/wikis/Install)

### How do I ...

  * [Install](https://gitlab.com/o9000/tint2/wikis/Install)
  * [Configure](https://gitlab.com/o9000/tint2/wikis/Configure)
  * [Add applet not supported by tint2](https://gitlab.com/o9000/tint2/wikis/ThirdPartyApplets)
  * [Other frequently asked questions](https://gitlab.com/o9000/tint2/wikis/FAQ)
  * [Debug](https://gitlab.com/o9000/tint2/wikis/Debug)

### How can I help out?

  * Report bugs and ask questions on the [issue tracker](https://gitlab.com/o9000/tint2/issues);
  * Contribute to the development by helping us fix bugs and suggesting new features.

### Links
  * Home page: https://gitlab.com/o9000/tint2
  * Git repository: https://gitlab.com/o9000/tint2.git
  * Documentation: https://gitlab.com/o9000/tint2/wikis/home
  * Downloads: https://gitlab.com/o9000/tint2-archive/tree/master or https://code.google.com/p/tint2/downloads/list
  * Old project location (inactive): https://code.google.com/p/tint2

### Releases
<<<<<<< HEAD
  * Latest stable release: tint2 0.12.2 (August 2015)
=======
  * Latest stable release: tint2 0.12.3 (November 2015)
>>>>>>> 0.12.3

### Screenshots
![screenshot](https://gitlab.com/o9000/tint2/wikis/screenshot.png)
