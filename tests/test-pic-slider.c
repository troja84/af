#include <gtk/gtk.h>
#include <af/af-animator.h>
#include "myslider.h"

#define DURATION 1500

static GtkWidget *my_slider = NULL;
static guint id = 0;

static guint number = 0;
static gboolean path = FALSE;

static void load_pics (GtkWidget *widget, gchar* dir)
{
  MySlider *slider;
  GFile *path;
  GFileEnumerator *iter;
  GFileInfo *file;
  GdkPixbuf *handle;
  GError *err;
  const gchar *filename;
  gchar *filepath;

  slider = MY_SLIDER (widget);

  g_assert (dir != NULL);

  handle = NULL;
  err = NULL;

  path = g_file_new_for_path (dir);
  iter = g_file_enumerate_children (path, "standard::name", 
		                    G_FILE_QUERY_INFO_NONE,
				    NULL,
				    &err);

  if (err != NULL)
    {
      g_error ("Failure while opening path '%s'", dir);
      g_error_free (err); 
    }

  file = g_file_enumerator_next_file (iter, NULL, &err);

  if (err != NULL)
    {
      g_error ("Failure while opening path '%s'", dir);
      g_error_free (err); 
    }

  while (file)
    {
      filename = g_file_info_get_name (file); 

      filepath = g_strconcat (dir, filename, NULL);

      // printf ("FILE: %s\n", filename);

      handle = gdk_pixbuf_new_from_file (filepath, &err);

      g_free (filepath);

      if (err != NULL)
        {
          g_error ("Failure while loading picture '%s' - %s'", filename,
		   err->message);
          g_error_free (err); 
        }

      if (handle)
        my_slider_add_picture (slider, handle);

      handle = NULL;
      file = g_file_enumerator_next_file (iter, NULL, &err);

      if (err != NULL)
        {
          g_error ("Failure while opening file '%s'", filename);
          g_error_free (err); 
        }
    }
}

static void
finished_cb (guint    anim_id,
	     gpointer user_data)
{
  if (id == anim_id)
    {
      id = 0;
      number = 0;
    }
}

static gboolean
forward_cb (GtkButton *button,
            gpointer   user_data)
{
  gdouble pos;

  if (id == 0)
    {
      g_object_get (my_slider, 
		    "pic", &pos,
		    NULL);

      if (my_slider_picture_count (MY_SLIDER (my_slider))
	  > pos + 1)
        number = pos + 1;
      else
	return FALSE;

      id = af_animator_tween (G_OBJECT (my_slider),
		              DURATION,
			      AF_TIMELINE_PROGRESS_LINEAR,
			      NULL, NULL, finished_cb,
			      "pic", pos + 1, my_slider_trans,
			      NULL);
    }
  else
    {

      if (my_slider_picture_count (MY_SLIDER (my_slider))
	  > number + 1)
        number++;
      else
	return FALSE;

      af_animator_remove (id);

      id = af_animator_tween (G_OBJECT (my_slider),
		              DURATION,
			      AF_TIMELINE_PROGRESS_LINEAR,
			      NULL, NULL, finished_cb,
			      "pic", (gdouble)number, my_slider_trans,
			      NULL);
    }

  return FALSE;
}

static gboolean
back_cb (GtkButton *button,
         gpointer   user_data)
{
  gdouble pos;

  if (id == 0)
    {
      g_object_get (my_slider, 
		    "pic", &pos,
		    NULL);

      if (0 <= pos - 1)
        number = pos - 1;
      else
	return FALSE;

      id = af_animator_tween (G_OBJECT (my_slider),
		              DURATION,
			      AF_TIMELINE_PROGRESS_LINEAR,
			      NULL, NULL, finished_cb,
			      "pic", pos - 1, my_slider_trans,
			      NULL);
    }
  else
    {
      if (0 <= number - 1)
        number--;
      else
	return FALSE;

      af_animator_remove (id);

      id = af_animator_tween (G_OBJECT (my_slider),
		              DURATION,
			      AF_TIMELINE_PROGRESS_LINEAR,
			      NULL, NULL, finished_cb,
			      "pic", (gdouble)number, my_slider_trans,
			      NULL);
    }

  return FALSE;
}

int main( int   argc,
          char *argv[] )
{
  GtkWidget *window, *box, *bbox, *button;
  GOptionContext *context;
  GError *err;
  gchar *dir = NULL;
    
  gtk_init (&argc, &argv);

  context = g_option_context_new ("PATH - a picture slider app");
  g_option_context_set_help_enabled (context, TRUE);

  if (!g_option_context_parse (context, &argc, &argv, &err))
    {
      g_print ("option parsing failed: %s\n", err->message);
    }
  
  if (argc > 1)
    {
      dir = argv[1];
    }
  else
    g_error ("No path to a picture directory given!");
    
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW(window), 320, 240);

  /* It's a good idea to do this for all windows. */
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect (G_OBJECT (window), "delete_event",
    		    G_CALLBACK (gtk_main_quit), NULL);

  box = gtk_vbox_new (FALSE, 0);

  my_slider = my_slider_new ();

  //gtk_widget_set_size_request (my_slider, 640, 480);

  load_pics (my_slider, dir);

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
