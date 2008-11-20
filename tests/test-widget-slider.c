#include <gtk/gtk.h>

#include "mywidgetslider.h"

static GtkWidget *my_slider = NULL;
static GtkTextBuffer *buffer = NULL;

static gboolean
clear_cb (GtkButton *button,
          gpointer   user_data)
{
  gtk_text_buffer_set_text (buffer, "", -1);

  return FALSE;
}

static gboolean
remove_cb (GtkButton *button,
	   gpointer   user_data)
{
  GtkContainer *container;

  container = GTK_CONTAINER (my_slider);

  gtk_container_remove (container, GTK_WIDGET (button));

  return FALSE;
}

static gboolean
to_pos_cb (GtkButton *button,
	   gpointer   user_data)
{
  guint pos;

  pos = GPOINTER_TO_INT (user_data);

  g_object_set (my_slider,
		"current_widget", pos,
		NULL);

  return FALSE;
}

static gboolean
forward_cb (GtkButton *button,
            gpointer   user_data)
{
  guint pos;

  g_object_get (my_slider,
		"current_widget", &pos,
		NULL);

  pos++;

  g_object_set (my_slider,
		"current_widget", pos,
		NULL);

  return FALSE;
}

static gboolean
back_cb (GtkButton *button,
         gpointer   user_data)
{
  guint pos;
 
  g_object_get (my_slider,
		"current_widget", &pos,
		NULL);

  pos--;

  g_object_set (my_slider,
		"current_widget", pos,
		NULL);

  return FALSE;
}

int main( int   argc,
          char *argv[] )
{
  GtkWidget *window, *box, *bbox, *hbox, *scroll_win, *button;
    
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW(window), 320, 240);

  /* It's a good idea to do this for all windows. */
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect (G_OBJECT (window), "delete_event",
    		    G_CALLBACK (gtk_main_quit), NULL);

  box = gtk_vbox_new (FALSE, 0);

  my_slider = my_slider_new ();

  gtk_box_pack_start (GTK_BOX (box), my_slider, TRUE, TRUE, 0);

  /* Add something to the slider - START */

  /* I know, bad behaviour! */
  button = gtk_image_new_from_file ("uncle_gnu.jpg");
  gtk_box_pack_start (GTK_BOX (my_slider), 
		      button, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS);
  gtk_box_pack_start (GTK_BOX (my_slider), 
		      button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (to_pos_cb), GINT_TO_POINTER (4));

  hbox = gtk_hbox_new (FALSE, 15);
  scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll_win), 
		                       GTK_SHADOW_IN);
  gtk_widget_set_size_request (scroll_win, 180, 200);

  buffer = gtk_text_buffer_new (NULL);
  gtk_text_buffer_set_text (buffer, "Put some text in!", -1);
  button = gtk_text_view_new_with_buffer (buffer);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (button), TRUE);
  gtk_container_add (GTK_CONTAINER (scroll_win), button);
  gtk_box_pack_start (GTK_BOX (hbox),
		      scroll_win, TRUE, TRUE, 0);

  bbox = gtk_hbutton_box_new ();
  button = gtk_button_new_from_stock (GTK_STOCK_CLEAR); 
  gtk_box_pack_end (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (clear_cb), NULL);

  gtk_box_pack_start (GTK_BOX (hbox),
		      bbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (my_slider),
		      hbox, TRUE, TRUE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_DELETE); 
  gtk_box_pack_start (GTK_BOX (my_slider), 
		      button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (remove_cb), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_NEXT); 
  gtk_box_pack_start (GTK_BOX (my_slider), 
		      button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (to_pos_cb), GINT_TO_POINTER (1));

  /* I know, bad behaviour! */
  button = gtk_image_new_from_file ("graf_zahl.jpg");
  gtk_box_pack_start (GTK_BOX (my_slider), 
		      button, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_DELETE); 
  gtk_box_pack_start (GTK_BOX (my_slider), 
		      button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (remove_cb), NULL);

  /* END */

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
