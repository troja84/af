#include <gtk/gtk.h>
#include <af/af-animator.h>
#include "myslider.h"

#define DURATION 2500
#define PROG_LIMIT 0.35
#define PROG_TYPE AF_TIMELINE_PROGRESS_EASE_IN_EASE_OUT

static GtkWidget *my_slider = NULL;
static guint id = 0;
static AfTransition *trans = NULL;

static GThreadPool *load_pictures;

static gint number = 0;
static gboolean forward = TRUE;

static void error_handling (GError *err)
{
  if (err == NULL)
    return;

  switch (err->code)
    {
      case G_IO_ERROR_NOT_FOUND:
	g_error ("IO error occured!\n");
        break;
      case G_FILE_ERROR_NOTDIR:
	g_error ("The given path doesn't direct to a directory!\n");
	break;
      default:
	g_error ("Unknown error occured!\n");
    }

  g_free (err);
}

static void load_pics (gpointer data, 
		       gpointer user_data)
{
  MySlider *slider;
  GFileEnumerator *iter;
  GFileInfo *file_info;
  GFile *path, *file;
  GdkPixbuf *handle;
  GError *err;
  gchar *filepath;

  slider = MY_SLIDER (user_data);
  path = (GFile*)data;

  g_assert (path != NULL);

  handle = NULL;
  err = NULL;

  iter = g_file_enumerate_children (path, "standard::name, standard::type", 
		                    G_FILE_QUERY_INFO_NONE,
				    NULL,
				    &err);

  error_handling (err);

  file_info = g_file_enumerator_next_file (iter, NULL, &err);

  error_handling (err);

  while (file_info)
    {
      file = g_file_get_child (path, g_file_info_get_name (file_info));

      filepath = g_file_get_path (file);

      /*
      printf ("FILE: %s - PATH: %s\n", 
	      g_file_info_get_name (file_info), filepath);
      */

      handle = gdk_pixbuf_new_from_file (filepath, &err);

      g_free (filepath);
      g_object_unref (file);
      
      error_handling (err);

      if (handle)
        {
          my_slider_add_picture (slider, handle);
	  g_object_unref (handle);
	}

      handle = NULL;

      file_info = g_file_enumerator_next_file (iter, NULL, &err);

      error_handling (err);
    }
}

static void
finished_cb (guint    anim_id,
	     gpointer user_data)
{
  if (id == anim_id)
    {
      id = number = 0;
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
        {
          number = (gint) pos + 1;
	}
      else
	return FALSE;

      id = af_animator_add ();

      af_animator_set_finished_notify (id, finished_cb);

      trans = af_animator_add_transition (id, 0, 1,
					  PROG_TYPE,
					  G_OBJECT (my_slider),
					  "pic", pos + 1, my_slider_trans,
					  NULL);

      af_animator_start (id, DURATION);

      forward = TRUE;
    }
  else
    {
      gdouble progress;

      progress = 0;

      if (my_slider_picture_count (MY_SLIDER (my_slider))
	  > number + 1)
        number++;
      else
	return FALSE;

      progress = af_animator_pause (id);

      af_animator_remove_transition (id, trans);

      trans = af_animator_add_transition (id, 0, 1,
					  PROG_TYPE,
					  G_OBJECT (my_slider),
					  "pic", (gdouble)number, my_slider_trans,
					  NULL);

      if (forward == TRUE)
        {
          if (progress > 1 - PROG_LIMIT)
            progress = PROG_LIMIT;
	}
      else
	progress = 1 - progress; 

      af_animator_resume_with_progress (id, progress);

      forward = TRUE;
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

      id = af_animator_add ();

      af_animator_set_finished_notify (id, finished_cb);

      trans = af_animator_add_transition (id, 0, 1,
					  PROG_TYPE,
					  G_OBJECT (my_slider),
					  "pic", pos - 1, my_slider_trans,
					  NULL);

      af_animator_start (id, DURATION);

      forward = FALSE;
    }
  else
    {
      gdouble progress;

      progress = 0;

      if (0 <= number - 1)
	number--;
      else
	return FALSE;

      progress = af_animator_pause (id);

      af_animator_remove_transition (id, trans);

      trans = af_animator_add_transition (id, 0, 1,
					  PROG_TYPE,
					  G_OBJECT (my_slider),
					  "pic", (gdouble)number, my_slider_trans,
					  NULL);

      if (forward == FALSE)
        {
          if (progress > 1 - PROG_LIMIT)
            progress = PROG_LIMIT;
	}
      else
	progress = 1 - progress; 

      af_animator_resume_with_progress (id, progress);

      forward = FALSE;
    }

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
