
SUBDIRS = roj-common roj-gener roj-load roj-pointer roj-line roj-noise roj-filter roj-olsa roj-parcel roj-stft roj-reass roj-raster roj-convert roj-kadr roj-screen roj-debug
MAINDIR = $(shell pwd)

all:	$(SUBDIRS) 

$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: $(SUBDIRS)


INSTALLDIR = /usr/local/lib/gstreamer-1.0
groj:
	install -m 0755 test-python/groj.py $(INSTALLDIR)

install: groj
	for DIR in $(SUBDIRS); do \
        $(MAKE) -C $$DIR $@; \
        done

autogen:
	for DIR in $(SUBDIRS); do \
	cd $(MAINDIR); \
	cd $$DIR; pwd; \
	./$@.sh; \
	done

autoclean:
	for DIR in $(SUBDIRS); do \
	cd $(MAINDIR); \
	cd $$DIR; pwd; \
	./$@.sh; \
	done

common:
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install

gener: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
load: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
pointer: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
line: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
noise: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
filter: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
olsa: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
parcel: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
stft: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
reass: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
raster: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
convert: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
kadr: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
screen: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install
debug: common
	$(MAKE) -C roj-$@
	$(MAKE) -C roj-$@ install


clean:	
	${RM} *.o *~ \#*  
	for dir in $(SUBDIRS); do \
	$(MAKE) -C $$dir -f Makefile $@; \
	done

