# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = librojreass.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
librojreass_la_SOURCES = rojreass.c rojreass.h

# compiler and linker flags used to compile this plugin, set in configure.ac
librojreass_la_CFLAGS = $(GST_CFLAGS) -lm -lfftw3 -Wl,-rpath=/usr/local/lib/gstreamer-1.0
librojreass_la_LIBADD = $(GST_LIBS)
librojreass_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -L../../roj-common/ -lrojfft -lrojvar
librojreass_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = rojreass.h
