
CFLAGS =

# Uncomment if you have gcc
# CC = gcc

# NOTE: if you get errors when linking qstat (missing symbols or
# libraries), then switch the LIBS macro being used.

SRC = qstat.c hcache.c template.c
O = qstat.o hcache.o template.o
OBJ = qstat.obj hcache.obj template.obj

NO_LIBS =
SOLARIS_LIBS = -lsocket -lnsl
WINDOWS_LIBS = wsock32.lib
OS2_LIBS = so32dll.lib tcp32dll.lib

# Irix 5.3, Linux, FreeBSD, many other Unixes
LIBS = $(NO_LIBS)

# Solaris 2
# LIBS = $(SOLARIS_LIBS)

# Windows 95/NT
# LIBS = $(WINDOWS_LIBS)

# OS/2
# LIBS = $(OS2_LIBS)

# The first line is for Unix.  Switch the comment character for Windows.
# (there should be no need to compile on Windows, but in case you care ...)
all: qstat
#all: qstat.exe

qstat: $(SRC)
	$(CC) $(CFLAGS) -o qstat $(SRC) $(LIBS)

solaris: $(SRC)
	$(CC) $(CFLAGS) -o qstat $(SRC) $(SOLARIS_LIBS)

aix sgi freebsd irix linux: $(SRC)
	$(CC) $(CFLAGS) -o qstat $(SRC) $(NO_LIBS)

hp hpux: $(SRC)
	$(CC) $(CFLAGS) -Ae -o qstat $(SRC) $(NO_LIBS)

windows: $(SRC)
	$(CC) $(CFLAGS) /ML /c $(SRC)
	link $(LDFLAGS) $(OBJ) /out:qstat.exe $(WINDOWS_LIBS)

os2: $(SRC)
	$(CC) /Q /W0 /C+ $(SRC)
	ilink /PM:VIO $(OBJ) /out:qstat.exe $(OS2_LIBS)

clean:
	rm -f qstat core qstat.exe $(O) $(OBJ)

VERSION = 23g
WVERSION = 23g
CP_FILES = Makefile qstat.txt qstatdoc.html qstat.c qstat.h \
        hcache.c template.c \
        CHANGES.txt COMPILE.txt win32 template

tar: always
	rm -rf tar qstat$(VERSION)
	rm -f qstat$(VERSION).tar qstat$(VERSION).tar.Z qstat$(VERSION).tar.gz
	mkdir qstat$(VERSION)
	-cp -rp $(CP_FILES) qstat$(VERSION)
	tar cvf qstat$(VERSION).tar qstat$(VERSION)
	gzip qstat$(VERSION).tar

zip: always
	rm -rf zip qsta$(WVERSION) qstat$(WVERSION)
	rm -f qstat$(WVERSION).zip qstat$(WVERSION).zip
	mkdir qstat$(WVERSION)
	-cp -rp $(CP_FILES) qstat$(WVERSION)
	mv qstat$(WVERSION)/qstatdoc.html qstat$(WVERSION)/qstatdoc.htm
	mv qstat$(WVERSION)/template/brocTh.html \
		qstat$(WVERSION)/template/brocTh.htm
	mv qstat$(WVERSION)/template/brocTp.html \
		qstat$(WVERSION)/template/brocTp.htm
	mv qstat$(WVERSION)/template/brocTs.html \
		qstat$(WVERSION)/template/brocTs.htm
	mv qstat$(WVERSION)/template/brocTt.html \
		qstat$(WVERSION)/template/brocTt.htm
	mv qstat$(WVERSION)/template/unrealTh.html \
		qstat$(WVERSION)/template/unrealTh.htm
	mv qstat$(WVERSION)/template/unrealTp.html \
		qstat$(WVERSION)/template/unrealTp.htm
	mv qstat$(WVERSION)/template/unrealTs.html \
		qstat$(WVERSION)/template/unrealTs.htm
	mv qstat$(WVERSION)/template/unrealTt.html \
		qstat$(WVERSION)/template/unrealTt.htm
	cd qstat$(WVERSION) ; crtocrlf * ; cd ..
	cd qstat$(WVERSION)/template ; crtocrlf * ; cd ../..
	zip -r qstat$(WVERSION).zip qstat$(WVERSION)

always:
