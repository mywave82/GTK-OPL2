CC=gcc
CXX=g++
CFLAGS=-g `pkg-config gtk+-3.0 cairo libpulse-mainloop-glib adplug --cflags`
CXXFLAGS=-g
LD=g++
LIBS=-g `pkg-config gtk+-3.0 cairo libpulse-mainloop-glib adplug --libs` -lm

all: mainwindow

clean:
	rm -f *.o mainwindow

mainwindow.o: \
	mainwindow.c \
	pulse.h \
	adplug-instance.h
	$(CC) $(CFLAGS) $< -o $@ -c

pulse.o: \
	pulse.c \
	pulse.h
	$(CC) $(CFLAGS) $< -o $@ -c

adplug-instance.o: \
	adplug-instance.c \
	adplug-instance.h
	$(CC) $(CXXFLAGS) $< -o $@ -c

mainwindow: \
	mainwindow.o \
	pulse.o \
	adplug-instance.o
	$(LD) $^ $(LIBS) -o $@
