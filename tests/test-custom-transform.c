#include <gtk/gtk.h>
#include <af/af-animator.h>
#include "color-area.h"

static void
color_transform_func (const GValue *from,
                      const GValue *to,
                      gdouble       progress,
                      GValue       *out_value)
{
  GdkColor *from_color, *to_color;
  GdkColor value = { 0, };

  from_color = (GdkColor *) g_value_get_boxed (from);
  to_color = (GdkColor *) g_value_get_boxed (to);

  value.red = from_color->red + ((to_color->red - from_color->red) * progress);
  value.green = from_color->green + ((to_color->green - from_color->green) * progress);
  value.blue = from_color->blue + ((to_color->blue - from_color->blue) * progress);

  g_value_set_boxed (out_value, &value);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *color_area;
  GdkColor from = { 0, 65535, 0, 0 };
  GdkColor to = { 0, 0, 0, 65535 };

  gtk_init (&argc, &argv);

  af_animator_register_type_transformation (GDK_TYPE_COLOR, color_transform_func);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  color_area = af_color_area_new ();
  af_color_area_set_background_color (AF_COLOR_AREA (color_area), &from);
  gtk_container_add (GTK_CONTAINER (window), color_area);

  af_animator_tween (G_OBJECT (color_area), 500,
                     "background-color", &to,
                     NULL);

  gtk_widget_show_all (window);

  gtk_main ();
}
