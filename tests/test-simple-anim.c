#include <gtk/gtk.h>
#include <af/af-animator.h>

int
main (int argc, char *argv[])
{
        GtkWidget *window, *label;
        GValue v1 = { 0, };
        GValue v2 = { 0, };
        guint id;

        gtk_init (&argc, &argv);

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);
        g_signal_connect (window, "destroy",
                          G_CALLBACK (gtk_main_quit), NULL);

        label = gtk_label_new ("Sliff Sloff");
        gtk_container_add (GTK_CONTAINER (window), label);

        af_animator_tween (G_OBJECT (label),
                           1000,
                           "xalign", 0., 1.,
                           "yalign", 1., 0.,
                           "angle", 0., 90.,
                           NULL);

        gtk_widget_show_all (window);

        gtk_main ();

        return 0;
}