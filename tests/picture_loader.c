#include <gtk/gtk.h>

#include "myadvancedslider.h"

static void error_handling (GError *err, gchar *file)
{
  const gchar *quark;

  if (err == NULL)
    return;

  quark = g_quark_to_string (err->domain);

  if (g_strcmp0 (quark, g_quark_to_string (G_FILE_ERROR)) == 0)
    {
      switch (err->code)
        {
          case G_IO_ERROR_NOT_FOUND:
            g_error ("IO error occured! '%s'\n", file);
            break;
          case G_FILE_ERROR_NOTDIR:
	    g_error ("The given path '%s' doesn't direct to a directory!\n", file);
	    break;
	  default:
	    g_error ("Unknown error occured!\n");
	}
    }

  if (g_strcmp0 (quark, g_quark_to_string (GDK_PIXBUF_ERROR)) == 0)
    {
      switch (err->code)
	{
	  case GDK_PIXBUF_ERROR_CORRUPT_IMAGE:
	    g_error ("Picture '%s' is corrupted!\n", file);
	    break;
	  case GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY:
	    g_error ("Not enough memory available!\n");
	    break;
	  case GDK_PIXBUF_ERROR_BAD_OPTION:
	    g_error ("Bad option passed to Pixbuf save module!\n");
	    break;
	  case GDK_PIXBUF_ERROR_UNKNOWN_TYPE:
	    g_error ("Unknown picture type for file '%s'!\n", file);
	    break;
	  case GDK_PIXBUF_ERROR_UNSUPPORTED_OPERATION:
	    g_error ("Unsupported operation!\n");
	    break;
	  case GDK_PIXBUF_ERROR_FAILED:
	    g_error ("Pixbuf error, no detailed information available!\n");
	    break;
	  default:
	    g_error ("Unknown error occured!\n");
	}
    }

  g_free (err);
}

void load_pics (gpointer data, 
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

  if (err != NULL)
    error_handling (err, g_file_get_path (path));

  file_info = g_file_enumerator_next_file (iter, NULL, &err);

  if (err != NULL)
    error_handling (err, g_file_get_path (path));

  while (file_info)
    {
      file = g_file_get_child (path, g_file_info_get_name (file_info));

      filepath = g_file_get_path (file);

      handle = gdk_pixbuf_new_from_file (filepath, &err);

      if (err != NULL)
        error_handling (err, filepath);

      g_free (filepath);
      g_object_unref (file);
      
      if (handle)
        {
          my_slider_add_picture (slider, handle);
	  g_object_unref (handle);
	}

      handle = NULL;

      file_info = g_file_enumerator_next_file (iter, NULL, &err);

      if (err != NULL)
        error_handling (err, filepath);
    }
}

