#include <gtk/gtk.h>
#include <af/af-animator.h>

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

        /* Set initial values */
        g_object_set (label,
                      "xalign", 0.,
                      "yalign", 0.,
                      NULL);

        /* Create animation */
        id = af_animator_add ();

        af_animator_add_transition (id,
                                    0., 0.375,
                                    G_OBJECT (label),
                                    "xalign", 1.,
                                    NULL);
        af_animator_add_transition (id,
                                    0.125, 0.5,
                                    G_OBJECT (label),
                                    "yalign", 1.,
                                    NULL);
        af_animator_add_transition (id,
                                    0.5, 0.875,
                                    G_OBJECT (label),
                                    "xalign", 0.,
                                    NULL);
        af_animator_add_transition (id,
                                    0.625, 1.,
                                    G_OBJECT (label),
                                    "yalign", 0.,
                                    NULL);
        af_animator_start (id, 3000);
        af_animator_set_loop (id, TRUE);

        gtk_widget_show_all (window);

        gtk_main ();

        return 0;
}
