# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = librojdebug.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
librojdebug_la_SOURCES = rojdebug.c rojdebug.h

# compiler and linker flags used to compile this plugin, set in configure.ac
librojdebug_la_CFLAGS = $(GST_CFLAGS) -lfftw3 -Wl,-rpath=/usr/local/lib/gstreamer-1.0
librojdebug_la_LIBADD = $(GST_LIBS)
librojdebug_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -L../../roj-common/ -lrojvar
librojdebug_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = rojdebug.h
