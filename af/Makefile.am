INCLUDES = \
	-Wall \
	-DG_LOG_DOMAIN=\"Animation-Framework\" \
	$(AF_CFLAGS)

libtool_opts = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-export-dynamic -export-symbols-regex "^af_.*"

afincludedir=$(includedir)/animation-framework-1.0/af/

lib_LTLIBRARIES= libanimation-framework-1.la

afinclude_HEADERS = \
	af-animator.h \
	af-enums.h \
	af-timeline.h

libanimation_framework_1_la_SOURCES = \
	$(BUILT_SOURCES) \
	af-animator.c \
	af-animator.h \
	af-enums.h \
	af-timeline.c \
	af-timeline.h

libanimation_framework_1_la_LIBADD = $(AF_LIBS)
libanimation_framework_1_la_LDFLAGS = $(libtool_opts)

af-enumtypes.h: af-enums.h Makefile af-enumtypes.h.in
	@( cd $(srcdir) && $(GLIB_MKENUMS) --template af-enumtypes.h.in \
	af-enums.h ) >> xgen-gtbh \
	&& (cmp -s xgen-gtbh $@ || (echo "Creating $@..."; cp xgen-gtbh $@) ) \
	&& rm -f xgen-gtbh

af-enumtypes.c: af-enums.h Makefile af-enumtypes.c.in
	@( cd $(srcdir) && $(GLIB_MKENUMS) --template af-enumtypes.c.in \
	af-enums.h ) > xgen-gtbc \
	&& (cmp -s xgen-gtbc $@ || (echo "Creating $@..."; cp xgen-gtbc $@) ) \
	&& rm -f xgen-gtbc

BUILT_SOURCES = \
	af-enumtypes.c \
	af-enumtypes.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	af-enumtypes.c.in \
	af-enumtypes.h.in
