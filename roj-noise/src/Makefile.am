# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = librojnoise.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
librojnoise_la_SOURCES = rojnoise.c rojnoise.h

# compiler and linker flags used to compile this plugin, set in configure.ac
librojnoise_la_CFLAGS = $(GST_CFLAGS) -lm -lfftw3 -Wl,-rpath=/usr/local/lib/gstreamer-1.0
librojnoise_la_LIBADD = $(GST_LIBS)
librojnoise_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -L../../roj-common/ -lrojvar
librojnoise_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = rojnoise.h
