#include <gtk/gtk.h>
#include <glib-object.h>
#include <math.h>
#include <af/af-timeline.h>

#include "myadvancedslider.h"

#define PROG_TYPE AF_TIMELINE_PROGRESS_EASE_IN_EASE_OUT

#define MY_SLIDER_GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MY_TYPE_SLIDER, MySliderPriv))

typedef struct MySliderPriv MySliderPriv;
typedef struct MyWidgetContainer MyWidgetContainer;

struct MyWidgetContainer
{
  GtkWidget *widget;

  GtkRequisition *requisition;
};

struct MySliderPriv
{
  guint widget_index;

  AfTimeline *timeline;
  gdouble position;

  gint counter;
  GHashTable *widgets;
};

enum 
{
  PROP_0,

  PROP_WIDGET_INDEX
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

static void my_slider_size_request     (GtkWidget      *widget,
		                        GtkRequisition *requisition);
static gboolean my_slider_expose       (GtkWidget      *chart,
		                        GdkEventExpose *event);

static void my_slider_free_widget      (gpointer data);

/*
static void resize                     (MyPixbufContainer *con,
	                                gint              *width,
	                                gint              *height);

static void define_clip_rect           (MySlider     *slider,
		                        GdkRectangle *rect,
		                        gdouble      *old_position,
		                        gdouble      *new_position);
*/

static void my_slider_animation_frame_cb    (AfTimeline *timeline,
		                             gdouble     progress,
		                             gpointer    user_data);
static void my_slider_animation_finished_cb (AfTimeline *timeline,
		                             gpointer    user_data);


G_DEFINE_TYPE (MySlider, my_slider, GTK_TYPE_CONTAINER);
	
static void
my_slider_class_init (MySliderClass *class)
{
  GtkWidgetClass *widget_class;
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = my_slider_set_property;
  object_class->get_property = my_slider_get_property;
  object_class->finalize = my_slider_finalize;

  widget_class = GTK_WIDGET_CLASS (class);

  widget_class->size_request = my_slider_size_request;
  widget_class->expose_event = my_slider_expose;

  g_object_class_install_property (object_class,
				   PROP_WIDGET_INDEX,
				   g_param_spec_uint ("current_widget",
					             "CURRENT_widget",
						     "The currently choosen widget.",
						     0,
						     G_MAXUINT,
						     0,
						     G_PARAM_READWRITE));


  g_type_class_add_private (class, sizeof (MySliderPriv));
}

static void
my_slider_init (MySlider *slider)
{
  MySliderPriv *priv;

  priv = MY_SLIDER_GET_PRIV (slider);

  priv->widget_index = 0;
  priv->counter = -1;
  priv->widgets = g_hash_table_new_full (g_direct_hash, 
		                         g_direct_equal, 
				         NULL,
				         my_slider_free_widget);
  priv->position = 0;
}

static void
my_slider_handle_animation (MySlider *slider)
{
  MySliderPriv *priv;

  if (GTK_WIDGET_DRAWABLE (GTK_WIDGET (slider)) == FALSE)
    return;
      
  priv = MY_SLIDER_GET_PRIV (slider);

  if (!priv->timeline)
    {
      priv->timeline = af_timeline_new (2000);

      g_signal_connect (priv->timeline, "frame",
		        G_CALLBACK (my_slider_animation_frame_cb), slider);
      
      g_signal_connect (priv->timeline, "finished",
		        G_CALLBACK (my_slider_animation_finished_cb), 
			slider);

      af_timeline_start (priv->timeline);
    }
  else
    {
      af_timeline_rewind (priv->timeline);
    }
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
  guint index;

  slider = MY_SLIDER (object);
  priv = MY_SLIDER_GET_PRIV (slider);

  length = priv->counter + 1;

  switch (prop_id)
    {
      case PROP_WIDGET_INDEX:
	index = g_value_get_uint (value);
	if (index <= 0 || index > length)
	  return;

	priv->widget_index = index;

	my_slider_handle_animation (slider);

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
      case PROP_WIDGET_INDEX:
        g_value_set_uint (value, priv->widget_index);
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

  g_hash_table_unref (priv->widgets);
}

static void
my_slider_size_request (GtkWidget      *widget,
		        GtkRequisition *requisition)
{
  MySlider *slider;
  MySliderPriv *priv;
  MyWidgetContainer *con;
  GList *value;

  slider = MY_SLIDER (widget);
  priv = MY_SLIDER_GET_PRIV (slider);

  /*
  if (priv->counter < 0)
    {
      return;
    }
  */

  value = g_hash_table_get_values (priv->widgets);
  while (value)
    {
      GtkRequisition *req;

      con = (MyWidgetContainer *)value;

      req = g_slice_new (GtkRequisition);

      gtk_widget_size_request (GTK_WIDGET (con->widget), req); 

      requisition->width = MAX (requisition->width, req->width);
      requisition->height = MAX (requisition->height, req->height);

      value = g_list_next (value);
    }

  g_list_free (value);
}

static gboolean
my_slider_expose (GtkWidget      *slider,
		  GdkEventExpose *event)
{
  MySliderPriv *priv;
  MyWidgetContainer *con;
  GdkGC * gc;
  GtkAllocation allocation;
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
      con = (MyWidgetContainer *) g_hash_table_lookup (priv->widgets, GINT_TO_POINTER (pos1));

      allocation.width = con->requisition->width;
      allocation.height = con->requisition->height;

      pos = offset - h_width - (gdouble) con->requisition->width / 2.0;

      allocation.x = pos;
      allocation.y = height;

      gtk_container_propagate_expose (GTK_CONTAINER (slider), con->widget, event);
    }
  
  con = (MyWidgetContainer *) g_hash_table_lookup (priv->widgets, GINT_TO_POINTER (pos2));

  pos = offset - h_width - (gdouble) con->requisition->width / 2.0;
  
  allocation.width = con->requisition->width;
  allocation.height = con->requisition->height;

  pos = offset - h_width - (gdouble) con->requisition->width / 2.0;

  allocation.x = pos;
  allocation.y = height;

  gtk_container_propagate_expose (GTK_CONTAINER (slider), con->widget, event);

  return FALSE;
}

GtkWidget *
my_slider_new (void)
{
  return g_object_new (MY_TYPE_SLIDER, NULL);
}

void
my_slider_add_widget (MySlider  *slider, 
		      GtkWidget *widget)
{
  MySliderPriv *priv;
  MyWidgetContainer *con;

  priv = MY_SLIDER_GET_PRIV (slider);

  con = g_slice_new (MyWidgetContainer);

  g_object_ref (widget);

  con->widget = widget;
  con->requisition = NULL;

  gtk_widget_set_parent_window (widget, GTK_WIDGET (slider)->window);

  /* Resize? */

  if (priv->widget_index == 0)
    priv->widget_index++;
  
  priv->counter++;

  g_hash_table_insert (priv->widgets, GINT_TO_POINTER (priv->counter), con);

  gtk_widget_queue_resize (GTK_WIDGET (slider));
}

guint
my_slider_widget_count (MySlider *slider)
{
  MySliderPriv *priv;

  priv = MY_SLIDER_GET_PRIV (slider);

  return priv->counter + 1;
}

static void 
my_slider_free_widget (gpointer data)
{
  MyWidgetContainer *con;

  con = (MyWidgetContainer *) data;

  g_object_unref (con->widget);

  if (!con->requisition)
    g_slice_free (GtkRequisition, con->requisition);

  g_slice_free (MyWidgetContainer, con);
}

/*
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
*/

/* Animation stuff */

static void
my_slider_animation_frame_cb (AfTimeline *timeline,
		              gdouble     progress,
		              gpointer    user_data)
{
  MySlider *slider;
  MySliderPriv *priv;
  // GdkRectangle rect;
  gdouble new_position;
  gdouble from, to;

  slider = MY_SLIDER (user_data);
  priv = MY_SLIDER_GET_PRIV (slider);

  from = priv->position;
  to = (gdouble)priv->widget_index - 1;
  
  progress = af_timeline_calculate_progress (progress, PROG_TYPE);
  new_position = from + (to - from) * progress;

  /*
  define_clip_rect (slider, &rect, &priv->position, &new_position);

  priv->position = new_position;
  */

  gdk_window_invalidate_rect (GTK_WIDGET (slider)->window, 
 	                      NULL, FALSE);
}

static void
my_slider_animation_finished_cb (AfTimeline *timeline,
		                 gpointer    user_data)
{
  MySlider *slider;
  MySliderPriv *priv;

  slider = MY_SLIDER (user_data);
  priv = MY_SLIDER_GET_PRIV (slider);

  af_timeline_pause (priv->timeline);
  g_object_unref (priv->timeline);

  priv->timeline = NULL;
}

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

