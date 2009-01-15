FLAGS=-Wall -g `pkg-config --cflags --libs cairo pangocairo x11 xinerama imlib2 glib-2.0`
PROGNAME=tint
FILES=tint.c server.c window.c task.c launcher.c visual.c config.c

ifndef DESTDIR
	ifndef PREFIX
		BINDIR=/usr/bin
		XDG_CONFIG_DIR=/etc/xdg
	else
		BINDIR=$(PREFIX)/bin
		XDG_CONFIG_DIR=/etc/xdg
	endif
else
	BINDIR=$(DESTDIR)/usr/bin
	XDG_CONFIG_DIR=$(DESTDIR)/etc/xdg
endif


$(PROGNAME): $(FILES)
	$(CC) $(CFLAGS) $(FLAGS) -o $(PROGNAME) $(FILES)
	strip $(PROGNAME)

install:
	mkdir -p $(BINDIR)
	mkdir -p $(XDG_CONFIG_DIR)/tint
	install $(PROGNAME) $(BINDIR)
	cp -f ../tintrc1 $(XDG_CONFIG_DIR)/tint/tintrc

uninstall:
	rm -f $(BINDIR)/$(PROGNAME)

clean:
	rm -f $(PROGNAME)

