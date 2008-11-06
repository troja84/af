#include <gtk/gtk.h>
#include <glib-object.h>
#include "mychart.h"

#define ORIGIN_X 5
#define ORIGIN_Y 5
#define ABSZISSE -5
#define ORDINATE -5

#define MY_CHART_GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MY_TYPE_CHART, MyChartPriv))

typedef struct MyChartPriv MyChartPriv;

struct MyChartPriv
{
  GList *points;

  MyChartPoint *latest;
};

enum 
{
  PROP_0,

  PROP_POINTS
};

static void  my_chart_set_property     (GObject         *object,
                                        guint            prop_id,
                                        const GValue    *value,
                                        GParamSpec      *pspec);
static void  my_chart_get_property     (GObject         *object,
                                        guint            prop_id,
                                        GValue          *value,
                                        GParamSpec      *pspec);
static void  my_chart_finalize         (GObject *object);

static gboolean my_chart_expose        (GtkWidget      *chart,
		                        GdkEventExpose *event);

static void my_chart_free_points       (gpointer data,
	                                gpointer user_data);

static gpointer my_chart_point_copy    (gpointer boxed);

static gint my_chart_find_latest (gconstpointer a,
		                  gconstpointer b);

G_DEFINE_TYPE (MyChart, my_chart, GTK_TYPE_DRAWING_AREA);
	
static void
my_chart_class_init (MyChartClass *class)
{
  GtkWidgetClass *widget_class;
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = my_chart_set_property;
  object_class->get_property = my_chart_get_property;
  object_class->finalize = my_chart_finalize;

  widget_class = GTK_WIDGET_CLASS (class);

  widget_class->expose_event = my_chart_expose;

  g_object_class_install_property (object_class,
				   PROP_POINTS,
				   g_param_spec_boxed ("point",
					               "MYPOINT",
						       "Point in a Cartesian coordinate system",
						       MY_CHART_TYPE_POINT,
						       G_PARAM_READWRITE));

  g_type_class_add_private (class, sizeof (MyChartPriv));
}

static void
my_chart_init (MyChart *chart)
{
  MyChartPriv *priv;

  priv = MY_CHART_GET_PRIV (chart);

  priv->points = NULL;
}

void
my_chart_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  MyChart *chart;
  MyChartPriv *priv;
  MyChartPoint *copy;
  GList *latest, *next;

  chart = MY_CHART (object);
  priv = MY_CHART_GET_PRIV (chart);

  latest = NULL;

  switch (prop_id)
    {
      case PROP_POINTS:
        copy = my_chart_point_copy (g_value_get_boxed (value));

	if (priv->latest && (copy->x < priv->latest->x))
          latest = g_list_find_custom (priv->points, priv->latest,
			               my_chart_find_latest);

        if (latest)
	  {
	    while (latest)
            {
              priv->points = g_list_remove_link (priv->points, latest);
	      my_chart_free_points (latest->data, NULL);
	      next = g_list_next (latest);
	      g_list_free_1 (latest);

	      latest = next;
	    }

	    priv->latest = (MyChartPoint*) (g_list_last (priv->points))->data;
	  }
	else
	  {
	   priv->points = g_list_append (priv->points, copy);
	   priv->latest = copy;
	  }
	
	if (GTK_WIDGET_DRAWABLE (GTK_WIDGET (chart)) == TRUE)
	  { 
	    gdk_window_invalidate_rect (GTK_WIDGET (chart)->window, 
			                NULL, TRUE);
	  }
	break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

void
my_chart_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  MyChart *chart;
  MyChartPriv *priv;

  chart = MY_CHART (object);
  priv = MY_CHART_GET_PRIV (chart);

  switch (prop_id)
    {
      case PROP_POINTS:
        g_value_set_boxed (value, g_list_last (priv->points)->data);
	break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
my_chart_finalize (GObject *object)
{
  MyChart *chart;
  MyChartPriv *priv;

  chart = MY_CHART (object);
  priv = MY_CHART_GET_PRIV (chart);

  g_list_foreach (priv->points, my_chart_free_points, NULL);
  g_list_free (priv->points);
}

GtkWidget *
my_chart_new (void)
{
  return g_object_new (MY_TYPE_CHART, NULL);
}

void
my_chart_add_point (MyChart      *chart, 
		    MyChartPoint *point)
{
  MyChartPriv *priv;
  MyChartPoint *copy;

  priv = MY_CHART_GET_PRIV (chart);

  copy = my_chart_point_copy (point);

  priv->points = g_list_append (priv->points, copy);
}

static gboolean
my_chart_expose (GtkWidget      *chart,
		 GdkEventExpose *event)
{
  MyChartPriv *priv;
  MyChartPoint *point;

  GList *element;

  cairo_t *cr;
  gint width, height;

  priv = MY_CHART_GET_PRIV (chart);

  gtk_widget_get_size_request (chart, &width, &height);

  cr = gdk_cairo_create (chart->window); 

  g_assert (cr);
  /* set a clip region for the expose event */
  if (event)
    cairo_rectangle (cr,
                     event->area.x, event->area.y,
                     event->area.width, event->area.height);

  cairo_move_to (cr, ORIGIN_X, ORIGIN_Y);
  cairo_line_to (cr, ORIGIN_X, height + ORDINATE);
  cairo_line_to (cr, width + ABSZISSE, height + ORDINATE);

  if (priv->points)
    {
      element = priv->points;
      point = (MyChartPoint *)element->data;
      
      cairo_move_to (cr, point->x + ORIGIN_X, height - ORIGIN_Y - point->y);

      element = g_list_next(element);
      
      while (element)
        {
          point = (MyChartPoint *)element->data;

          cairo_line_to (cr, point->x + ORIGIN_X, height - ORIGIN_Y - point->y);

          element = g_list_next (element);
	}
    }

  cairo_stroke (cr);

  cairo_destroy(cr);

  return FALSE;
}

static gint
my_chart_find_latest (gconstpointer a,
		      gconstpointer b)
{
  MyChartPoint *data, *cmp;

  data = (MyChartPoint *)a;
  cmp = (MyChartPoint *)b;

  if (data->x == cmp->x)
    return 0;

  if (data->x < cmp->x)
    return -1;

  return 1;
}

static void
my_chart_free_points (gpointer data,
	              gpointer user_data)
{
  MyChartPoint *point;

  point = (MyChartPoint *)data;

  g_slice_free (MyChartPoint, point);
}

/* Animation stuff */
void
my_chart_trans (const GValue *from,
	        const GValue *to,
	        gdouble       progress,
		gpointer      user_data,
		GValue       *out_value)
{
  MyChartPoint *pfrom, *pto, *res;
  gdouble d, dl, m, t_square_end;

  d = 5.0;
  m = 1000.0;
  t_square_end = (m / d) * 4;

  pfrom = g_value_get_boxed (from);
  pto = g_value_get_boxed (to);

  res = g_slice_new (MyChartPoint);

  dl = (pto->y - pfrom->y) / 2;

  res->x = 140 * progress;

  if (progress < 0.5)
      res->y = ABS((-d * dl * t_square_end * progress * progress) / m);
  else
    {
      progress = 1 - progress;
      
      res->y = pto->y - ABS((-d * dl * t_square_end * progress * progress) / m);
    }  
      
  g_value_set_boxed (out_value, res);
}

/* box MyChartPoint */
static gpointer
my_chart_point_copy (gpointer boxed)
{
  MyChartPoint *point, *copy;

  point = boxed;

  copy = g_slice_new (MyChartPoint);

  copy->x = point->x;
  copy->y = point->y;

  return copy;
}

static void
my_chart_point_free (gpointer boxed)
{
  g_slice_free (MyChartPoint, boxed);
}

GType
my_chart_point_get_type (void)
{
  static GType type_id = 0;
  if (!type_id)
    type_id = g_boxed_type_register_static (g_intern_static_string ("MyChartPoint"),
		                            my_chart_point_copy,
					    my_chart_point_free);

  return type_id;
}
