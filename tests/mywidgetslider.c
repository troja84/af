#include <gtk/gtk.h>
#include <glib-object.h>
#include <math.h>
#include <af/af-timeline.h>

#include "mywidgetslider.h"

#define PROG_TYPE AF_TIMELINE_PROGRESS_LINEAR

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
  gdouble position, old_position;

  guint          default_expand : 1;
  guint          spacing_set    : 1;
};

enum 
{
  PROP_0,

  PROP_WIDGET_INDEX
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_EXPAND,
  CHILD_PROP_FILL,
  CHILD_PROP_PADDING,
  CHILD_PROP_PACK_TYPE,
  CHILD_PROP_POSITION
};

static void my_slider_set_property     (GObject         *object,
                                        guint            prop_id,
                                        const GValue    *value,
                                        GParamSpec      *pspec);
static void my_slider_get_property     (GObject         *object,
                                        guint            prop_id,
                                        GValue          *value,
                                        GParamSpec      *pspec);

static void my_slider_size_request     (GtkWidget      *widget,
		                        GtkRequisition *requisition);
static void my_slider_size_allocate    (GtkWidget      *widget,
		                        GtkAllocation  *allocation);

static void my_slider_animation_frame_cb    (AfTimeline *timeline,
		                             gdouble     progress,
		                             gpointer    user_data);
static void my_slider_animation_finished_cb (AfTimeline *timeline,
		                             gpointer    user_data);


G_DEFINE_TYPE (MySlider, my_slider, GTK_TYPE_BOX);

static void
my_slider_class_init (MySliderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class;

  object_class->set_property = my_slider_set_property;
  object_class->get_property = my_slider_get_property;

  widget_class = GTK_WIDGET_CLASS (class);

  widget_class->size_request = my_slider_size_request;
  widget_class->size_allocate = my_slider_size_allocate;

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
      priv->timeline = af_timeline_new (650);

      g_signal_connect (priv->timeline, "frame",
		        G_CALLBACK (my_slider_animation_frame_cb), slider);
      
      g_signal_connect (priv->timeline, "finished",
		        G_CALLBACK (my_slider_animation_finished_cb), 
			slider);

      af_timeline_start (priv->timeline);
    }
  else
    {
      priv->old_position = priv->position;

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
  GtkBox *box;
  gint length;
  guint index;

  slider = MY_SLIDER (object);
  priv = MY_SLIDER_GET_PRIV (slider);
  box = GTK_BOX (slider);

  length = g_list_length (box->children);

  switch (prop_id)
    {
      case PROP_WIDGET_INDEX:
	index = g_value_get_uint (value);
	if (index < 0 || index >= length)
	  return;

	priv->old_position = (gdouble)priv->widget_index;
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
my_slider_size_request (GtkWidget      *widget,
		        GtkRequisition *requisition)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;

  box = GTK_BOX (widget);

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
        {
          GtkRequisition req;

	  gtk_widget_size_request (child->widget, &req);

          requisition->width = MAX (requisition->width, req.width);
          requisition->height = MAX (requisition->height, req.height);
	}
    }
}

static void 
my_slider_size_allocate (GtkWidget      *widget,
		         GtkAllocation  *allocation)
{
  GtkBox *box;
  GtkBoxChild *child;
  MySliderPriv *priv;
  GList *children;
  GtkRequisition req;
  GtkAllocation all;
  gint width, height, x, vis_l, vis_r;
  gdouble progress, h_width;
  gboolean visible;

  box = GTK_BOX (widget);
  priv = MY_SLIDER_GET_PRIV (widget);

  widget->allocation = *allocation;

  width = allocation->width;
  height = allocation->height;

  h_width = (gdouble)width / 2.0;

  progress = priv->position - (gint)priv->position;

  vis_l = (gint) floor (priv->position);
  vis_r = (gint) ceil (priv->position);

   x = 0;

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      gtk_widget_size_request (child->widget, &req);

      if (req.width <= width)
        all.width = req.width;
      else
        all.width = width;

      if (req.height <= height)
	all.height = req.height;
      else
	all.height = height;

      if (x == vis_l)
        {
          all.x = progress * (gdouble)allocation->width + h_width - (gdouble)all.width / 2.0;

	  visible = TRUE;
	}
      else if (x == vis_r)
        {
          all.x = progress * (gdouble)allocation->width - h_width - (gdouble)all.width / 2.0;

	  visible = TRUE;
	}
      else
        visible = FALSE;

      all.y = height / 2 - req.height / 2;

      if (visible)
        gtk_widget_set_child_visible (child->widget, TRUE);
      else
        gtk_widget_set_child_visible (child->widget, FALSE);

      /*
      printf ("ALLOCATION NR %d: X: %d Y: %d WIDTH: %d HEIGHT: %d POSITION: %f\n",
	      x, all.x, all.y, all.width, all.height, priv->position);
      */

      gtk_widget_size_allocate (child->widget, &all);

      x++;
    }
}

GtkWidget *
my_slider_new (void)
{
  return g_object_new (MY_TYPE_SLIDER, NULL);
}

/* Animation stuff */

static void
my_slider_animation_frame_cb (AfTimeline *timeline,
		              gdouble     progress,
		              gpointer    user_data)
{
  MySlider *slider;
  MySliderPriv *priv;
  gdouble new_position;
  gdouble from, to;

  slider = MY_SLIDER (user_data);
  priv = MY_SLIDER_GET_PRIV (slider);

  from = priv->old_position;
  to = (gdouble)priv->widget_index;
  
  new_position = from + (to - from) * progress;
  /*
  printf ("FRAME: FROM: %f TO: %f PROGRESS: %f = %f\n",
	  from, to, progress, new_position);
  */

  priv->position = new_position;

  gtk_widget_queue_resize (GTK_WIDGET (slider));
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
