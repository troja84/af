#include <gtk/gtk.h>
#include <af/af-animator.h>

static void
finished_cb (guint id,
	     gpointer user_data)
{
    GtkLabel *label;

    label = GTK_LABEL (user_data);

    gtk_label_set_label (label, "finished");
}

int
main (int argc, char *argv[])
{
        GtkWidget *window, *label;
        guint id;

        gtk_init (&argc, &argv);

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);
        g_signal_connect (window, "destroy",
                          G_CALLBACK (gtk_main_quit), NULL);

        label = gtk_label_new ("Sliff Sloff");
        gtk_container_add (GTK_CONTAINER (window), label);

        id = af_animator_tween (G_OBJECT (label),
                                1000,
                                AF_TIMELINE_PROGRESS_LINEAR,
				label, NULL, finished_cb,
                                "xalign", 1., NULL,
                                "yalign", 0., NULL,
                                "angle", 90., NULL,
                                NULL);

        gtk_widget_show_all (window);

        gtk_main ();

        return 0;
}
