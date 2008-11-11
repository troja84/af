#include <gtk/gtk.h>
#include <glib-object.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <math.h>

#include "myslider.h"

#define MY_SLIDER_GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MY_TYPE_SLIDER, MySliderPriv))

typedef struct MySliderPriv MySliderPriv;
typedef struct MyPixbufContainer MyPixbufContainer;

struct MyPixbufContainer
{
  GdkPixbuf *org;
  GdkPixbuf *cur;

  gint width, height;
};

struct MySliderPriv
{
  gint   current;
  gdouble position;

  GList *pics;
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

static void my_slider_free_pic         (gpointer data,
		                        gpointer user_data);

static GdkPixbuf* resize               (GdkPixbuf *org,
	                                guint      width,
	                                guint      height);

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

  priv->pics = NULL;
  priv->position = 0;
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
  gdouble dvalue;

  slider = MY_SLIDER (object);
  priv = MY_SLIDER_GET_PRIV (slider);

  length = g_list_length (priv->pics);

  switch (prop_id)
    {
      case PROP_PIC_POSITION:
	dvalue = g_value_get_double (value);
	if (!length || dvalue > length - 1 || dvalue < 0)
	  return;

	priv->position = dvalue;

        if (GTK_WIDGET_DRAWABLE (GTK_WIDGET (slider)) == TRUE)
          { 
            gdk_window_invalidate_rect (GTK_WIDGET (slider)->window, 
 		                        NULL, TRUE);
          }

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

  g_list_foreach (priv->pics, my_slider_free_pic, NULL);
  g_list_free (priv->pics);
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
    con->cur = resize (picture, con->width, con->height);
  else
    con->cur = NULL;

  priv->pics = g_list_append (priv->pics, con);
}

guint
my_slider_picture_count (MySlider *slider)
{
  MySliderPriv *priv;

  priv = MY_SLIDER_GET_PRIV (slider);

  return g_list_length (priv->pics);
}

static gboolean
my_slider_expose (GtkWidget      *slider,
		  GdkEventExpose *event)
{
  MySliderPriv *priv;
  GdkGC *gc;
  MyPixbufContainer *con;
  GdkPixbuf *pic1, *pic2;
  gdouble pos_pic1, pos_pic2, progress, offset, scale_factor;
  gint pos1, pos2;
  gint width, height;

  priv = MY_SLIDER_GET_PRIV (slider);

  if (g_list_length (priv->pics) == 0)
    return FALSE;

  width = slider->allocation.width;
  height = slider->allocation.height;

  /* set a clip region for the expose event */
  if (event)
    {
      gc = gdk_gc_new (slider->window);
      gdk_gc_set_clip_rectangle (gc, &event->area);
    }
  else
    gc = NULL;

  pic1 = pic2 = NULL;
  pos_pic1 = pos_pic2 = 0.0;

  progress = priv->position - (gint)priv->position;

  pos1 = (gint) ceil (priv->position);
  pos2 = (gint) floor (priv->position);

  offset = (gdouble) width * progress;
  scale_factor = height / 2.0;

  if (pos1 != pos2) 
    {

      if (pos1 < 0)
        pos1 = g_list_length (priv->pics) - 1;

      con = (MyPixbufContainer *) g_list_nth (priv->pics, pos1)->data;

      if (con->width == width && con->height == height && con->cur)
        pic1 = con->cur;
      else
        {
          if (con->cur)
            {
              g_object_unref (con->cur);
              con->cur = NULL;
	    }

          con->cur = resize (con->org, width, height);
          con->width = width;
          con->height = height;

          pic1 = con->cur;
        }
    }
  
  if (pos2 > g_list_length (priv->pics) - 1)
    pos2 = 0;

  con = (MyPixbufContainer *) g_list_nth (priv->pics, pos2)->data;

  if (con->width == width && con->height == height && con->cur)
    pic2 = con->cur;
  else
    {
      if (con->cur)
        {
          g_object_unref (con->cur);
          con->cur = NULL;
	}

      con->cur = resize (con->org, width, height);
      con->width = width;
      con->height = height;

      pic2 = con->cur;
    }
  

  if (pic1)
    {
      gdouble pic_width = gdk_pixbuf_get_width (pic1);

      pos_pic1 = offset - ((gdouble) width / 2.0 + pic_width / 2.0);
    }

  if (pic2)
    {
      gdouble pic_width = gdk_pixbuf_get_width (pic2);

      pos_pic2 = offset + ((gdouble) width / 2.0 - pic_width / 2.0);
    }

  if (pic1)
    {
      gdk_draw_pixbuf (slider->window, gc, pic1,
		       0, 0,
		       pos_pic1, 0,
		       -1, -1,
		       GDK_RGB_DITHER_NONE, 0, 0);
    }

  if (pic2)
    {
      gdk_draw_pixbuf (slider->window, gc, pic2,
		       0, 0,
		       pos_pic2, 0,
		       -1, -1,
		       GDK_RGB_DITHER_NONE, 0, 0);
    }

  return FALSE;
}

static void 
my_slider_free_pic (gpointer data,
		    gpointer user_data)
{
  MyPixbufContainer *con;

  con = (MyPixbufContainer *) data;

  g_object_unref (con->org);
  
  if (con->cur)
    g_object_unref (con->cur);

  g_slice_free (MyPixbufContainer, con);
}

static GdkPixbuf*
resize (GdkPixbuf *org,
	guint      width,
	guint      height)
{
  guint n_width; 
  gdouble p_width;

  p_width = gdk_pixbuf_get_width (org);
  n_width = p_width * (gdouble) height / 2.0 / p_width;

  return gdk_pixbuf_scale_simple (org, n_width, height / 2,
				  GDK_INTERP_BILINEAR);
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

