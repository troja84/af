#include <gtk/gtk.h>
#include <glib-object.h>
#include <math.h>

#include "myslider.h"

#define MY_SLIDER_GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MY_TYPE_SLIDER, MySliderPriv))

typedef struct MySliderPriv MySliderPriv;
typedef struct MyPixbufContainer MyPixbufContainer;

struct MyPixbufContainer
{
  GdkPixbuf *org;
  GdkPixbuf *cur;

  gint width, height, cur_width, cur_height;
};

struct MySliderPriv
{
  gint counter;
  gdouble position;

  GHashTable *pics;
};

enum 
{
  PROP_0,

  PROP_PIC_POSITION
};

static void my_slider_set_property     (GObject         *object,
                                        guint            prop_id,
                                        const GValue    *value,
                                        GParamSpec      *pspec);
static void my_slider_get_property     (GObject         *object,
                                        guint            prop_id,
                                        GValue          *value,
                                        GParamSpec      *pspec);
static void my_slider_finalize         (GObject *object);

static gboolean my_slider_expose       (GtkWidget      *chart,
		                        GdkEventExpose *event);

static void my_slider_free_pic         (gpointer data);

static void resize                     (MyPixbufContainer *con,
	                                gint              *width,
	                                gint              *height);

G_DEFINE_TYPE (MySlider, my_slider, GTK_TYPE_DRAWING_AREA);
	
static void
my_slider_class_init (MySliderClass *class)
{
  GtkWidgetClass *widget_class;
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = my_slider_set_property;
  object_class->get_property = my_slider_get_property;
  object_class->finalize = my_slider_finalize;

  widget_class = GTK_WIDGET_CLASS (class);

  widget_class->expose_event = my_slider_expose;

  g_object_class_install_property (object_class,
				   PROP_PIC_POSITION,
				   g_param_spec_double ("pic",
					                "PIC_COUNT",
						        "The amount of pictures held by the widget",
						        -G_MAXDOUBLE,
						        G_MAXDOUBLE,
						        0,
						        G_PARAM_READWRITE));

  g_type_class_add_private (class, sizeof (MySliderPriv));
}

static void
my_slider_init (MySlider *slider)
{
  MySliderPriv *priv;

  priv = MY_SLIDER_GET_PRIV (slider);

  priv->counter = -1;
  priv->pics = g_hash_table_new_full (g_direct_hash, 
		                      g_direct_equal, 
				      NULL,
				      my_slider_free_pic);
  priv->position = 0;
}

static void
define_clip_rect (MySlider     *slider,
		  GdkRectangle *rect,
		  gdouble      *old_position,
		  gdouble      *new_position)
{
  GtkWidget *widget;
  MySliderPriv *priv;
  MyPixbufContainer *con1, *con2;
  gdouble o_progress, n_progress, o_offset, n_offset, h_width, d_pos1, d_pos2;
  gint width, height, pos1, pos2;

  widget = GTK_WIDGET (slider);
  priv = MY_SLIDER_GET_PRIV (slider);

  o_progress = *old_position - (gint)*old_position;
  n_progress = *new_position - (gint)*new_position;

  width = widget->allocation.width;
  height = widget->allocation.height;

  if (n_progress == 0 || o_progress == 0)
    {
      rect->x = rect->y = 0;
      rect->width = width;
      rect->height = height;

      return;
    }

  pos1 = (gint) ceil (priv->position);
  pos2 = (gint) floor (priv->position);

  con1 = (MyPixbufContainer *) g_hash_table_lookup (priv->pics, GINT_TO_POINTER (pos1));
  con2 = (MyPixbufContainer *) g_hash_table_lookup (priv->pics, GINT_TO_POINTER (pos2));

  o_offset = (gdouble) width * o_progress;
  n_offset = (gdouble) width * n_progress;
  h_width = (gdouble) width / 2.0;

  rect->y = (gdouble) height / 4.0;

  d_pos1 = h_width - (gdouble) con1->cur_width / 2.0;
  d_pos2 = h_width - (gdouble) con2->cur_width / 2.0;

  rect->height = con1->cur_height;

  if (o_progress < n_progress)
    {
      if (o_offset + d_pos2 <= con2->width && o_offset - d_pos1 + con1->cur_width >= 0)
        {
          rect->x = o_offset - d_pos1 - con1->cur_width;
          rect->width = n_offset + d_pos2 + con2->cur_width; 

	  if (rect->x < 0)
	    rect->x = 0;
	}
      else if (o_offset + d_pos2 < con2->width)
        {
          rect->x = o_offset + d_pos2;
          rect->width = n_offset + con2->cur_width; 
	}
      else if (o_offset - d_pos1 + con1->cur_width >= 0)
        {
          rect->x = o_offset - d_pos1 - con1->cur_width;
          rect->width = n_offset + con1->cur_width; 
	}
    }
  else
    {
      if (o_offset + d_pos2 <= con2->width && o_offset - d_pos1 + con1->cur_width >= 0)
        {
          rect->x = n_offset - d_pos1 - con1->cur_width;
          rect->width = o_offset + d_pos2 + con2->cur_width; 

	  if (rect->x < 0)
	    rect->x = 0;
	}
      else if (o_offset + d_pos2 < con2->width)
        {
          rect->x = n_offset + d_pos2;
          rect->width = o_offset + d_pos2 + con2->cur_width; 
	}
      else if (o_offset - d_pos1 + con1->cur_width >= 0)
        {
          rect->x = n_offset - d_pos1 - con1->cur_width;
          rect->width = o_offset + con1->cur_width; 
	}
    }


  rect->height = (con1->cur_height > con2->cur_height) 
	         ? con1->cur_height : con2->cur_height;
}

void
my_slider_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  MySlider *slider;
  MySliderPriv *priv;
  gint length;
  gdouble d_value;

  slider = MY_SLIDER (object);
  priv = MY_SLIDER_GET_PRIV (slider);

  length = priv->counter + 1;

  switch (prop_id)
    {
      case PROP_PIC_POSITION:
	d_value = g_value_get_double (value);
	if (d_value > length - 1 || d_value <= 0)
	  return;

        if (GTK_WIDGET_DRAWABLE (GTK_WIDGET (slider)) == TRUE)
          { 
	    GdkRectangle rect;

            define_clip_rect (slider, &rect, &priv->position, &d_value);

	    priv->position = d_value;

            gdk_window_invalidate_rect (GTK_WIDGET (slider)->window, 
 		                        &rect, FALSE);
          }

	priv->position = d_value;
	break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

void
my_slider_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  MySlider *slider;
  MySliderPriv *priv;

  slider = MY_SLIDER (object);
  priv = MY_SLIDER_GET_PRIV (slider);

  switch (prop_id)
    {
      case PROP_PIC_POSITION:
        g_value_set_double (value, priv->position);
	break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
my_slider_finalize (GObject *object)
{
  MySlider *slider;
  MySliderPriv *priv;

  slider = MY_SLIDER (object);
  priv = MY_SLIDER_GET_PRIV (slider);

  g_hash_table_unref (priv->pics);
}

GtkWidget *
my_slider_new (void)
{
  return g_object_new (MY_TYPE_SLIDER, NULL);
}

void
my_slider_add_picture (MySlider  *slider, 
		       GdkPixbuf *picture)
{
  GtkWidget *widget;
  MySliderPriv *priv;
  MyPixbufContainer *con;

  priv = MY_SLIDER_GET_PRIV (slider);
  widget = GTK_WIDGET (slider);

  con = g_slice_new (MyPixbufContainer);

  g_object_ref (picture);

  con->org = picture;
  con->width = widget->allocation.width;
  con->height = widget->allocation.height;

  if (con->width > 1 && con->height > 1)
    resize (con, &con->width, &con->height);
  else
    con->cur = NULL;

  priv->counter++;
  
  g_hash_table_insert (priv->pics, GINT_TO_POINTER (priv->counter), con);
}

guint
my_slider_picture_count (MySlider *slider)
{
  MySliderPriv *priv;

  priv = MY_SLIDER_GET_PRIV (slider);

  return g_hash_table_size (priv->pics);
}

static gboolean
my_slider_expose (GtkWidget      *slider,
		  GdkEventExpose *event)
{
  MySliderPriv *priv;
  MyPixbufContainer *con;
  GdkGC * gc;
  GdkPixbuf *pic;
  gdouble h_width, f_height, pos, progress, offset;
  gint pos1, pos2;
  gint width, height;

  priv = MY_SLIDER_GET_PRIV (slider);

  if (priv->counter < 0)
    return FALSE;

  width = slider->allocation.width;
  height = slider->allocation.height;

  gc = NULL;

  progress = priv->position - (gint)priv->position;

  pos1 = (gint) ceil (priv->position);
  pos2 = (gint) floor (priv->position);

  offset = (gdouble) width * progress;
  h_width = (gdouble) width / 2.0;
  f_height = (gdouble) height / 4.0;

  if (event)
    {
      gc = gdk_gc_new (slider->window);
      gdk_gc_set_clip_rectangle (gc, &event->area);
    }

  if (pos1 != pos2) 
    {
      con = (MyPixbufContainer *) g_hash_table_lookup (priv->pics, GINT_TO_POINTER (pos1));

      if (con->width != width || con->height != height)
        {
          resize (con, &width, &height);
        }

      pos = offset - h_width - (gdouble) con->cur_width / 2.0;

      gdk_draw_pixbuf (slider->window, gc, con->cur,
		       0, 0,
		       pos, f_height,
		       -1, -1,
		       GDK_RGB_DITHER_NONE, 0, 0);
    }
  
  con = (MyPixbufContainer *) g_hash_table_lookup (priv->pics, GINT_TO_POINTER (pos2));

  if (con->width != width || con->height != height)
    {
      resize (con, &width, &height);
      pic = con->cur;
    }

  pos = offset + h_width - (gdouble) con->cur_width / 2.0;
  
  gdk_draw_pixbuf (slider->window, gc, con->cur,
		   0, 0,
		   pos, f_height,
		   -1, -1,
		   GDK_RGB_DITHER_NONE, 0, 0);

  return FALSE;
}

static void 
my_slider_free_pic (gpointer data)
{
  MyPixbufContainer *con;

  con = (MyPixbufContainer *) data;

  g_object_unref (con->org);
  
  if (con->cur)
    g_object_unref (con->cur);

  g_slice_free (MyPixbufContainer, con);
}

static void
resize (MyPixbufContainer *con,
	gint              *width,
	gint              *height)
{
  gint n_width, n_height;
  gdouble factor;

  factor = (gdouble) gdk_pixbuf_get_width (con->org) 
	  / gdk_pixbuf_get_height (con->org);

  if (con->cur)
    {
      g_object_unref (con->cur);
      con->cur = NULL;
    }

  if ((gdouble)*width / *height >= 1 || factor < 1)
    {
      n_height = *height / 2;
      n_width = ((gdouble)*height / 2.0) * factor;
    }
  else
    {
      factor = 1 / factor;

      n_width = *width;
      n_height = (gdouble) *width * factor;
    }

  con->cur = gdk_pixbuf_scale_simple (con->org,
                                      n_width, n_height,
				      GDK_INTERP_BILINEAR);

  con->cur_width = n_width;
  con->cur_height = n_height;

  con->width = *width;
  con->height = *height;
}

/* Animation stuff */

void
my_slider_trans (const GValue *from,
	         const GValue *to,
	         gdouble       progress,
		 gpointer      user_data,
		 GValue       *out_value)
{
  gdouble dfrom, dto, *offset, res;

  dfrom = g_value_get_double (from);
  dto = g_value_get_double (to);
  offset = (gdouble *) user_data;

  res = dfrom + (dto - dfrom) * progress;
  // printf ("%f - %f = %f\n", dto, dfrom, res);

  g_value_set_double (out_value, res);
}

