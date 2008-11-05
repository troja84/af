#ifndef __MY_CHART_H__
#define __MY_CHART_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#define MY_TYPE_CHART                         (my_chart_get_type ())
#define MY_CHART(obj)                         (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_CHART, MyChart))
#define MY_IS_CHART(obj)                      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_CHART))
#define MY_CHART_CLASS(obj)                   (G_TYPE_CHECK_CLASS_CAST ((obj), MY_TYPE_CHART,  MyChartClass))
#define MY_IS_CHART_CLASS(obj)                (G_TYPE_CHECK_CLASS_TYPE ((obj), IS_TYPE_CHART))
#define MY_CHART_GET_CLASS                    (G_TYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_CHART, MyChartClass))

#define MY_CHART_TYPE_POINT                   (my_chart_point_get_type ())

typedef struct _MyChart      MyChart;
typedef struct _MyChartClass MyChartClass;
typedef struct _MyChartPoint MyChartPoint;

struct _MyChart
{
  GtkDrawingArea parent;
};

struct _MyChartClass
{
  GtkDrawingAreaClass parent_class;
};

struct _MyChartPoint
{
  gdouble x;
  gdouble y;
};

GType my_chart_get_type (void);

GtkWidget *my_chart_new (void);

void my_chart_add_point (MyChart      *chart,
		         MyChartPoint *point);

GType my_chart_point_get_type (void) G_GNUC_CONST;
/*
 * Method definitions.
 */
void my_chart_trans (const GValue *from,
 	             const GValue *to,
	             gdouble       progress,
		     gpointer      user_data,
		     GValue       *out_value);

#endif /* __MY_CHART_H__ */
