#include <gtk/gtk.h>
#include <af/af-animator.h>

static GtkWidget *label = NULL;
static guint id = 0;

/*
void
my_float_trans (const GValue *from,
	        const GValue *to,
	        gdouble       progress,
		gpointer      user_data,
		GValue       *out_value)
{
  gfloat pfrom, pto, res;

  pfrom = g_value_get_float (from);
  pto = g_value_get_float (to);

  res = pfrom + ((pto - pfrom) * progress);

  g_value_set_float (out_value, res);

  printf ("RUNS!\n");
}
*/

static void
play_cb (GtkButton *button,
         gpointer   user_data)
{
  if (id == 0)
    {
      g_object_set (label,
                "xalign", 0.0,
                NULL);

      id = af_animator_tween (G_OBJECT (label),
                              3000,
                              AF_TIMELINE_PROGRESS_LINEAR,
			      NULL, NULL, NULL,
                              "xalign", 1.0, NULL,
                              NULL);
      af_animator_set_loop (id, TRUE);
    }
  else
    af_animator_resume (id);
}

static void
pause_cb (GtkButton *button,
          gpointer   user_data)
{
  if (id != 0)
    af_animator_pause (id);
}

static void
reverse_cb (GtkButton *button,
          gpointer   user_data)
{
  if (id != 0)
    af_animator_reverse (id);
}

static void
stop_cb (GtkButton *button,
         gpointer   user_data)
{
  if (id != 0)
    {
      af_animator_remove (id);
      id = 0;
    }
}

static void
advance_cb (GtkButton *button,
            gpointer   user_data)
{
  if (id != 0)
    af_animator_advance (id, (gdouble) 1200 / 3000);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *box, *bbox, *button;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  box = gtk_vbox_new (FALSE, 6);

  label = gtk_label_new ("Sliff sloff");
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);

  bbox = gtk_hbutton_box_new ();

  button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_PLAY);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (play_cb), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_PAUSE);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (pause_cb), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_STOP);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (stop_cb), NULL);

  button = gtk_button_new_with_label ("Reverse");
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (reverse_cb), NULL);

  gtk_box_pack_end (GTK_BOX (box), bbox, FALSE, FALSE, 0);

  bbox = gtk_hbutton_box_new ();

  button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_NEXT);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (advance_cb), NULL);

  gtk_box_pack_end (GTK_BOX (box), bbox, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
