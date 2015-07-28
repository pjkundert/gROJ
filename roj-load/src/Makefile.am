# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = librojload.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
librojload_la_SOURCES = rojload.c rojload.h

# compiler and linker flags used to compile this plugin, set in configure.ac
librojload_la_CFLAGS = $(GST_CFLAGS) -lm -lfftw3 -lsndfile -lasound -Wl,-rpath=/usr/local/lib/gstreamer-1.0
librojload_la_LIBADD = $(GST_LIBS)
librojload_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -L../../roj-common/ -lrojvar
librojload_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = rojload.h
