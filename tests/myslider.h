#ifndef __MY_SLIDER_H__
#define __MY_SLIDER_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include <librsvg/rsvg.h>

#define MY_TYPE_SLIDER                        (my_slider_get_type ())
#define MY_SLIDER(obj)                        (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_SLIDER, MySlider))
#define MY_IS_SLIDER(obj)                      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_SLIDER))
#define MY_SLIDER_CLASS(obj)                   (G_TYPE_CHECK_CLASS_CAST ((obj), MY_TYPE_SLIDER,  MySliderClass))
#define MY_IS_SLIDER_CLASS(obj)                (G_TYPE_CHECK_CLASS_TYPE ((obj), IS_TYPE_SLIDER))
#define MY_SLIDER_GET_CLASS                    (G_TYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_SLIDER, MySliderClass))

typedef struct _MySlider      MySlider;
typedef struct _MySliderClass MySliderClass;

struct _MySlider
{
  GtkDrawingArea parent;
};

struct _MySliderClass
{
  GtkDrawingAreaClass parent_class;
};

GType my_slider_get_type (void);

GtkWidget *my_slider_new (void);

void my_slider_add_picture (MySlider  *slider,
		            GdkPixbuf *picture);

guint my_slider_picture_count (MySlider *slider);

/*
 * Method definitions.
 */
void
my_slider_trans (const GValue *from,
	         const GValue *to,
	         gdouble       progress,
		 gpointer      user_data,
		 GValue       *out_value);

#endif /* __MY_SLIDER_H__ */
