# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = librojfilter.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
librojfilter_la_SOURCES = rojfilter.c rojfilter.h

# compiler and linker flags used to compile this plugin, set in configure.ac
librojfilter_la_CFLAGS = $(GST_CFLAGS) -lm -lfftw3 -Wl,-rpath=/usr/local/lib/gstreamer-1.0
librojfilter_la_LIBADD = $(GST_LIBS)
librojfilter_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -L../../roj-common/ -lrojvar
librojfilter_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = rojfilter.h
