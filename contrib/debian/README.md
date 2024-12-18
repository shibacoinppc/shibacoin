
Debian
====================
This directory contains files used to package shibacoind/shibacoin-qt
for Debian-based Linux systems. If you compile shibacoind/shibacoin-qt yourself, there are some useful files here.

## shibacoin: URI support ##


shibacoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install shibacoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your shibacoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/shibacoin128.png` to `/usr/share/pixmaps`

shibacoin-qt.protocol (KDE)

