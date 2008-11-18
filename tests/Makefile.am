INCLUDES = \
	-Wall \
	-g \
	-I$(top_srcdir) \
	$(AF_CFLAGS)
LDADDS = \
	$(top_srcdir)/af/libanimation-framework-1.la\
	$(AF_LIBS) \
	-lgthread-2.0

noinst_PROGRAMS = \
	test-widget-slider \
	test-adv-pic-slider\
	test-pic-slider\
	test-graph\
	test-timeline-marker \
	test-anim-control \
	test-chained-transitions \
	test-child-anim \
	test-custom-transform \
	test-overlapping-transitions \
	test-simple-anim

test_simple_anim_SOURCES = \
	test-simple-anim.c

test_simple_anim_LDADD = $(LDADDS)

test_custom_transform_SOURCES = \
	color-area.c \
	color-area.h \
	test-custom-transform.c

test_custom_transform_LDADD = $(LDADDS)

test_child_anim_SOURCES = \
	test-child-anim.c

test_child_anim_LDADD = $(LDADDS)

test_anim_control_SOURCES = \
	test-anim-control.c

test_anim_control_LDADD = $(LDADDS)

test_timeline_marker_SOURCES = \
	test-timeline-marker.c

test_timeline_marker_LDADD = $(LDADDS)

test_graph_SOURCES = \
        mychart.c \
	mychart.h \
	test-graph.c

test_graph_LDADD = $(LDADDS)

test_pic_slider_SOURCES = \
	myslider.c \
	myslider.h \
	picture_loader.h \
	picture_loader.c \
	test-pic-slider.c

test_pic_slider_LDADD = $(LDADDS)

test_adv_pic_slider_SOURCES = \
	myadvancedslider.c \
	myadvancedslider.h \
	picture_loader.h \
	picture_loader.c \
	test-adv-pic-slider.c

test_adv_pic_slider_LDADD = $(LDADDS)

test_widget_slider_SOURCES = \
	mywidgetslider.c \
	mywidgetslider.h \
	test-widget-slider.c

test_widget_slider_LDADD = $(LDADDS)

test_chained_transitions_SOURCES = \
	test-chained-transitions.c

test_chained_transitions_LDADD = $(LDADDS)

test_overlapping_transitions_SOURCES = \
	test-overlapping-transitions.c

test_overlapping_transitions_LDADD = $(LDADDS)
