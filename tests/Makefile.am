INCLUDES = \
	-I$(top_srcdir) \
	$(AF_CFLAGS)
LDADDS = \
	$(top_srcdir)/af/libanimation-framework-1.la\
	$(AF_LIBS)

noinst_PROGRAMS = \
	test-custom-transform \
	test-simple-anim

test_simple_anim_SOURCES = \
	test-simple-anim.c

test_simple_anim_LDADD = $(LDADDS)

test_custom_transform_SOURCES = \
	color-area.c \
	color-area.h \
	test-custom-transform.c

test_custom_transform_LDADD = $(LDADDS)
