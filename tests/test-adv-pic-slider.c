#include <gtk/gtk.h>

#include "myadvancedslider.h"
#include "picture_loader.h"

static GtkWidget *my_slider = NULL;

static GThreadPool *load_pictures;

static gboolean
forward_cb (GtkButton *button,
            gpointer   user_data)
{
  guint pos;

  g_object_get (my_slider,
		"current_picture", &pos,
		NULL);

  pos++;

  g_object_set (my_slider,
		"current_picture", pos,
		NULL);

  return FALSE;
}

static gboolean
back_cb (GtkButton *button,
         gpointer   user_data)
{
  guint pos;
 
  g_object_get (my_slider,
		"current_picture", &pos,
		NULL);

  pos--;

  g_object_set (my_slider,
		"current_picture", pos,
		NULL);

  return FALSE;
}

int main( int   argc,
          char *argv[] )
{
  GtkWidget *window, *box, *bbox, *button;
  GOptionContext *context;
  GError *err;
  GFile *path;
    
  gtk_init (&argc, &argv);

  context = g_option_context_new ("PATH - a Picture slider app");
  g_option_context_set_help_enabled (context, TRUE);

  if (!g_option_context_parse (context, &argc, &argv, &err))
    {
      g_print ("option parsing failed: %s\n", err->message);
    }
  
  if (argc > 1)
    {
      path = g_file_new_for_commandline_arg (argv[1]);
    }
  else
    {
      printf ("Run with -h or --help to see further options!\n");
      return 0;
    }
    
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW(window), 320, 240);

  /* It's a good idea to do this for all windows. */
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect (G_OBJECT (window), "delete_event",
    		    G_CALLBACK (gtk_main_quit), NULL);

  box = gtk_vbox_new (FALSE, 0);

  my_slider = my_slider_new ();

  if (!g_thread_supported ()) 
    {
      g_thread_init (NULL);
    }
  
  load_pictures = g_thread_pool_new (load_pics,
		                     my_slider,
				     -1, FALSE, NULL);

  g_thread_pool_push (load_pictures, path, NULL);

  gtk_box_pack_start (GTK_BOX (box), my_slider, TRUE, TRUE, 0);

  bbox = gtk_hbutton_box_new ();

  button = gtk_button_new_from_stock (GTK_STOCK_GO_BACK);
  gtk_box_pack_end (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (back_cb), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_GO_FORWARD);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (forward_cb), NULL);

  gtk_box_pack_end (GTK_BOX (box), bbox, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);

  gtk_main ();
    
  return 0;
}
