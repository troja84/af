#include <gtk/gtk.h>
#include <af/af-animator.h>
#include "mychart.h"

#define WIDTH 320
#define HEIGHT 240

static GtkWidget *my_chart = NULL;
static guint id = 0;
static gchar *test_data = "Hallo people!";

static MyChartPoint start, end;

static void
free_id (guint    anim_id,
         gpointer user_data)
{
  if (id == anim_id)
    {
      id = 0;
    }
}

static gboolean
play_cb (GtkButton *button,
         gpointer   user_data)
{
  gchar *test_data_new = NULL;

  if (id == 0)
    {
      start.x = start.y = 0;

      my_chart_remove_all_points (MY_CHART (my_chart));

      g_object_set (my_chart,
                    "point", &start,
		    NULL);

      end.x = 0;
      end.y = HEIGHT - 10;

      test_data_new = g_strdup (test_data);
      
      id = af_animator_tween (G_OBJECT (my_chart),
		              2000,
			      AF_TIMELINE_PROGRESS_LINEAR,
			      test_data_new, g_free, free_id,
			      "point", &end, my_chart_trans,
			      NULL);
    }
  else
    af_animator_resume (id);

  return FALSE;
}

static gboolean
reverse_cb (GtkButton *button,
         gpointer   user_data)
{
  if (id)
    af_animator_reverse (id);

  return FALSE;
}

int main( int   argc,
          char *argv[] )
{
  GtkWidget *window, *box, *bbox, *button;
    
  gtk_init (&argc, &argv);
    
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW(window), 320, 240);

  /* It's a good idea to do this for all windows. */
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect (G_OBJECT (window), "delete_event",
    		    G_CALLBACK (gtk_main_quit), NULL);

  box = gtk_vbox_new (FALSE, 0);

  my_chart = my_chart_new ();
  gtk_widget_set_size_request (my_chart, 320, 240);
  gtk_box_pack_start (GTK_BOX (box), my_chart, TRUE, TRUE, 0);

  bbox = gtk_hbutton_box_new ();

  button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_PLAY);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (play_cb), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_REFRESH);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (reverse_cb), NULL);

  gtk_box_pack_end (GTK_BOX (box), bbox, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);

  /*
  af_animator_register_type_transformation (MY_CHART_TYPE_POINT,
		                            my_chart_trans);
  */

  gtk_main ();
    
  return 0;
}
