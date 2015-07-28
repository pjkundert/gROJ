
CC	= gcc
CFLAGS  = -g -Wall -lfftw3 -lm -fPIC  `pkg-config --cflags --libs gstreamer-1.0`
LDFLAGS = -shared

all: rojfft rojrgb rojvar

rojrgb: rojrgb.o rojrgb.c rojrgb.h
	$(CC) -o librojrgb.so rojrgb.o $(LDFLAGS) $(CFLAGS)

rojvar: rojvar.o rojvar.c rojvar.h
	$(CC) -o librojvar.so rojvar.o $(LDFLAGS) $(CFLAGS)

rojfft: rojfft.o rojfft.c rojfft.h rojvar.o 
	$(CC) -o librojfft.so rojfft.o  rojvar.o $(LDFLAGS) $(CFLAGS)


INSTALLDIR = /usr/local/lib/gstreamer-1.0
install:
	install -m 0755 librojfft.so $(INSTALLDIR)
	install -m 0755 librojrgb.so $(INSTALLDIR)
	install -m 0755 librojvar.so $(INSTALLDIR)

clean: 
	${RM} *.o *.so *~