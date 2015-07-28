# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = librojstft.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
librojstft_la_SOURCES = rojstft.c rojstft.h

# compiler and linker flags used to compile this plugin, set in configure.ac
librojstft_la_CFLAGS = $(GST_CFLAGS) -lm -lfftw3 -Wl,-rpath=/usr/local/lib/gstreamer-1.0
librojstft_la_LIBADD = $(GST_LIBS)  
librojstft_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -L../../roj-common/ -lrojfft -lrojvar
librojstft_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = rojstft.h
