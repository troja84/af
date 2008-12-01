#include <gtk/gtk.h>

#include "mybox_a.h"

static GtkWidget *my_box = NULL;

static GtkWidget *widget1 = NULL;
static GtkWidget *widget2 = NULL;
static GtkWidget *widget3 = NULL;

static gint offset = 0;

static GtkTextBuffer *text_buffer = NULL;
static gchar *undo = NULL;

static void
show_simple_dialog (const gchar *title,
		    const gchar *text,
		    gboolean     with_textbox)
{
  GtkWidget *dialog, *label, *content_area;
  
  dialog = gtk_dialog_new_with_buttons (title,
		                        NULL,
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_OK,
                                        NULL);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  label = gtk_label_new (text);

  g_signal_connect_swapped (dialog,
                             "response", 
                             G_CALLBACK (gtk_widget_destroy),
                             dialog);

  gtk_container_add (GTK_CONTAINER (content_area), label);

  if (with_textbox)
    {
      GtkWidget *entry, *frame, *vbox;

      vbox = gtk_vbox_new (FALSE, 0);

      gtk_container_add (GTK_CONTAINER (vbox), 
		         gtk_label_new ("filename: "));

      entry = gtk_entry_new ();
      frame = gtk_frame_new (NULL);

      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

      gtk_container_add (GTK_CONTAINER (frame), entry);
      gtk_container_add (GTK_CONTAINER (vbox), frame);
      gtk_container_add (GTK_CONTAINER (content_area), vbox);
    }

  gtk_widget_show_all (dialog);
}

static gboolean
delete_cb (GtkButton *button,
	   gpointer user_data)
{
  GtkTextIter start, end;

  if (undo)
    g_free (undo);

  gtk_text_buffer_get_start_iter (text_buffer, &start);
  gtk_text_buffer_get_end_iter (text_buffer, &end);

  undo = gtk_text_buffer_get_text (text_buffer,
		                   &start,
				   &end,
				   FALSE);

  gtk_text_buffer_set_text (text_buffer, "", -1);

  return FALSE;
}

static gboolean
undo_cb (GtkButton *button,
	 gpointer user_data)
{
  gtk_text_buffer_set_text (text_buffer, undo, -1);

  return FALSE;
}

static gboolean
save_cb (GtkButton *button,
	 gpointer user_data)
{
  const gchar *mode = (const gchar *)user_data;

  if (g_strcmp0 ("save", mode) == 0)
    {
      show_simple_dialog ("Save", "Your file is saved!", FALSE);
    }
  else if (g_strcmp0 ("save as", mode) == 0)
    {
      show_simple_dialog ("Save as", "Please enter a filename!", TRUE);
    }

  return FALSE;
}

static gboolean
button_cb (GtkCheckButton *button,
	   gpointer user_data)
{
  GtkWidget *widget;
  gint pos = GPOINTER_TO_INT (user_data);
  gint shift = 0;

  switch (pos)
    {
      case 1:
        widget = widget1;
	break;
      case 2:
	widget = widget2;

	if (offset == 4)
	  shift = 1;
	break;
      case 4:
	widget = widget3;
	break;
      default:
	widget = widget1;
    }

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      my_box_pack_start_n (GTK_BOX (my_box),
		           widget,
			   TRUE, TRUE,
			   0, pos - 1 - shift); 
      offset += pos;
    }
  else
    {
      gtk_container_remove (GTK_CONTAINER (my_box), widget);
      offset -= pos;
    }

  return FALSE;
}

static gboolean
select_animation_cb (GtkRadioButton *button,
		     gpointer user_data)
{
  const gchar *mode = (const gchar *)user_data;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    return FALSE;

  if (g_strcmp0 ("move", mode) == 0)
    my_box_set_animation_style (MY_BOX (my_box),
		                MY_BOX_ANIMATION_STYLE_MOVE);
  else if (g_strcmp0 ("resize", mode) == 0)
    my_box_set_animation_style (MY_BOX (my_box),
		                MY_BOX_ANIMATION_STYLE_RESIZE);

  return FALSE;
}

static gboolean
select_orientation_cb (GtkRadioButton *button,
		       gpointer user_data)
{
  const gchar *mode = (const gchar *)user_data;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    return FALSE;

  if (g_strcmp0 ("vertical", mode) == 0)
    my_box_set_orientation (MY_BOX (my_box),
		            GTK_ORIENTATION_VERTICAL);
  else if (g_strcmp0 ("horizontal", mode) == 0)
    my_box_set_orientation (MY_BOX (my_box),
		            GTK_ORIENTATION_HORIZONTAL);

  return FALSE;
}

int main( int   argc,
          char *argv[] )
{
  GtkWidget *window, *hbox, *vbox, *bbox, *checkbox, *text_view, *button; 
  GtkWidget *radio1, *radio2, *rbbox, *frame;
    
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW(window), 320, 240);

  /* It's a good idea to do this for all windows. */
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect (G_OBJECT (window), "delete_event",
    		    G_CALLBACK (gtk_main_quit), NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  my_box = my_box_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 0);

  gtk_container_add (GTK_CONTAINER (hbox), my_box);

  /* Some widgets to test with MyVBox */
  widget1 = gtk_hbutton_box_new ();
  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (save_cb), "save");
  gtk_container_add (GTK_CONTAINER (widget1), button);

  button = gtk_button_new_from_stock (GTK_STOCK_SAVE_AS);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (save_cb), "save as");
  gtk_container_add (GTK_CONTAINER (widget1), button);

  widget2 = gtk_frame_new ("Text");
  text_buffer = gtk_text_buffer_new (NULL);
  text_view = gtk_text_view_new_with_buffer (text_buffer);
  gtk_container_add (GTK_CONTAINER (widget2), text_view);

  widget3 = gtk_vbutton_box_new ();
  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (delete_cb), NULL);
  gtk_container_add (GTK_CONTAINER (widget3), button);

  button = gtk_button_new_from_stock (GTK_STOCK_UNDELETE);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (undo_cb), NULL);
  gtk_container_add (GTK_CONTAINER (widget3), button);

  /* Show the widgets and attach them later */
  gtk_widget_show_all (widget1);
  gtk_widget_show_all (widget2);
  gtk_widget_show_all (widget3);

  /* Add checkboxes to the vbox */
  bbox = gtk_vbutton_box_new ();
  checkbox = gtk_check_button_new_with_label ("Save");
  g_signal_connect (checkbox, "toggled",
		    G_CALLBACK (button_cb), GINT_TO_POINTER (1));
  gtk_container_add (GTK_CONTAINER (bbox),
		     checkbox);
  checkbox = gtk_check_button_new_with_label ("Text");
  g_signal_connect (checkbox, "toggled",
		    G_CALLBACK (button_cb), GINT_TO_POINTER (2));
  gtk_container_add (GTK_CONTAINER (bbox),
		     checkbox);
  checkbox = gtk_check_button_new_with_label ("Edit");
  g_signal_connect (checkbox, "toggled",
		    G_CALLBACK (button_cb), GINT_TO_POINTER (4));
  gtk_container_add (GTK_CONTAINER (bbox),
		     checkbox);

  /* Add radio buttons to select the animation style */
  frame = gtk_frame_new ("Animation mode");
  rbbox = gtk_hbutton_box_new ();
  radio1 = gtk_radio_button_new_with_label (NULL, "Move");
  g_signal_connect (radio1, "toggled",
		    G_CALLBACK (select_animation_cb), "move");
  gtk_container_add (GTK_CONTAINER (rbbox),
		     radio1);
  radio2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1), "Resize");
  g_signal_connect (radio2, "toggled",
		    G_CALLBACK (select_animation_cb), "resize");
  gtk_container_add (GTK_CONTAINER (rbbox),
		     radio2);
  
  gtk_container_add (GTK_CONTAINER (frame), rbbox);

  gtk_container_add (GTK_CONTAINER (bbox), frame);

  /* Add radio buttons to select the orientation */
  frame = gtk_frame_new ("Orientation");
  rbbox = gtk_hbutton_box_new ();
  radio1 = gtk_radio_button_new_with_label (NULL, "Vertical");
  g_signal_connect (radio1, "toggled",
		    G_CALLBACK (select_orientation_cb), "vertical");
  gtk_container_add (GTK_CONTAINER (rbbox),
		     radio1);
  radio2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1), "Horizontal");
  g_signal_connect (radio2, "toggled",
		    G_CALLBACK (select_orientation_cb), "horizontal");
  gtk_container_add (GTK_CONTAINER (rbbox),
		     radio2);
  
  gtk_container_add (GTK_CONTAINER (frame), rbbox);

  gtk_container_add (GTK_CONTAINER (bbox), frame);


  gtk_container_add (GTK_CONTAINER (vbox), bbox);

  /* Add vbox to the hbox */
  gtk_container_add (GTK_CONTAINER (hbox), vbox);

  gtk_container_add (GTK_CONTAINER (window), hbox);

  gtk_widget_show_all (window);

  gtk_main ();
    
  return 0;
}
