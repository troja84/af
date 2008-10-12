#include <gtk/gtk.h>
#include "color-area.h"

enum {
  PROP_0,
  PROP_BACKGROUND_COLOR
};

static void  af_color_area_set_property  (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void  af_color_area_get_property  (GObject      *object,
                                          guint         prop_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);

static void af_color_area_realize        (GtkWidget    *widget);


G_DEFINE_TYPE (AfColorArea, af_color_area, GTK_TYPE_DRAWING_AREA)

static void
af_color_area_class_init (AfColorAreaClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = af_color_area_set_property;
  object_class->get_property = af_color_area_get_property;

  widget_class->realize = af_color_area_realize;

  g_object_class_install_property (object_class,
				   PROP_BACKGROUND_COLOR,
				   g_param_spec_boxed ("background-color",
                                                       "Background color",
                                                       "Background color",
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));
}

static void
af_color_area_init (AfColorArea *color_area)
{
  
}

static void
af_color_area_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_BACKGROUND_COLOR:
      af_color_area_set_background_color (AF_COLOR_AREA (object),
                                          (GdkColor *) g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
af_color_area_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_BACKGROUND_COLOR:
      {
        GdkColor color;

        af_color_area_get_background_color (AF_COLOR_AREA (object), &color);
        g_value_set_boxed (value, &color);
      }

      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
af_color_area_realize (GtkWidget *widget)
{
  AfColorArea *area;

  GTK_WIDGET_CLASS (af_color_area_parent_class)->realize (widget);

  area = AF_COLOR_AREA (widget);

  gdk_window_set_background (widget->window,
                             &area->background_color);
}

GtkWidget *
af_color_area_new (void)
{
  return g_object_new (AF_TYPE_COLOR_AREA, NULL);
}

void
af_color_area_set_background_color (AfColorArea    *area,
                                    const GdkColor *color)
{
  GdkColormap *colormap;

  g_return_if_fail (AF_IS_COLOR_AREA (area));
  g_return_if_fail (color != NULL);

  area->background_color = *color;

  colormap = gtk_widget_get_colormap (GTK_WIDGET (area));
  gdk_rgb_find_color (colormap, &area->background_color);

  if (GTK_WIDGET_REALIZED (area))
    gdk_window_set_background (GTK_WIDGET (area)->window,
                                 &area->background_color);

  gtk_widget_queue_draw (GTK_WIDGET (area));
  g_object_notify (G_OBJECT (area), "background-color");
}

void
af_color_area_get_background_color (AfColorArea *area,
                                    GdkColor    *color)
{
  g_return_if_fail (AF_IS_COLOR_AREA (area));
  g_return_if_fail (color != NULL);

  *color = area->background_color;
}
