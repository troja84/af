#include <gtk/gtk.h>
#include <af/af-animator.h>

int
main (int argc, char *argv[])
{
  GtkWidget *window, *fixed, *label;
  guint id;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 300, 300);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  fixed = gtk_fixed_new ();

  label = gtk_label_new ("Sliff Sloff");
  gtk_fixed_put (GTK_FIXED (fixed), label, 0, 0);

  gtk_container_add (GTK_CONTAINER (window), fixed);

  id = af_animator_child_tween (GTK_CONTAINER (fixed),
                                label,
                                1000,
                                AF_TIMELINE_PROGRESS_LINEAR,
				NULL, NULL,
                                "x", 200, NULL,
                                "y", 200, NULL,
                                NULL);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
