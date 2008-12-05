/* vim: set tabstop=2:softtabstop=2:shiftwidth=2:noexpandtab */

#include <gtk/gtk.h>

#include "myexpander.h"
#include "myvbox_a.h"

static GtkWidget *my_expander = NULL;

static gboolean
select_animation_cb (GtkRadioButton *button,
											 gpointer user_data)
{
  const gchar *mode = (const gchar *)user_data;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    return FALSE;

  if (g_strcmp0 ("move", mode) == 0)
    my_expander_set_animation_style (MY_EXPANDER (my_expander),
																		 MY_EXPANDER_ANIMATION_STYLE_MOVE);
  else if (g_strcmp0 ("resize", mode) == 0)
    my_expander_set_animation_style (MY_EXPANDER (my_expander),
																		 MY_EXPANDER_ANIMATION_STYLE_RESIZE);

  return FALSE;
}

int main( int   argc,
          char *argv[] )
{
  GtkWidget *window, *box, *hbox_i, *frame, *bbox, *radio, *widget;
    
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW(window), 320, 240);

  /* It's a good idea to do this for all windows. */
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect (G_OBJECT (window), "delete_event",
    		    G_CALLBACK (gtk_main_quit), NULL);

  box = gtk_vbox_new (FALSE, 0);
	hbox_i = gtk_hbox_new (FALSE, FALSE);
  frame = gtk_frame_new ("Let's count together");
  my_expander = my_expander_new ("Test of the function...");

	/* some widgets above the expander */
  bbox = gtk_vbutton_box_new ();
  widget = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_container_add (GTK_CONTAINER (bbox), widget);
  gtk_container_add (GTK_CONTAINER (box), bbox);

  bbox = gtk_vbutton_box_new ();

  radio = gtk_radio_button_new_with_label (NULL, "Count even numbers");
	g_signal_connect (radio, "toggled",
										G_CALLBACK (select_animation_cb), "move");
  gtk_container_add (GTK_CONTAINER (bbox), radio);
  widget = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio), "Count odd numbers");
	g_signal_connect (widget, "toggled",
										G_CALLBACK (select_animation_cb), "resize");
  gtk_container_add (GTK_CONTAINER (bbox), widget);
  widget = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio), "Count all numbers");
  gtk_container_add (GTK_CONTAINER (bbox), widget);
  widget = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_container_add (GTK_CONTAINER (bbox), widget);

  /* button box into the frame */
  gtk_container_add (GTK_CONTAINER (frame), bbox);

  /* picture into the vbox */
  gtk_container_add (GTK_CONTAINER (hbox_i), 
										 gtk_image_new_from_file ("./graf_zahl.jpg"));

	/* frame into the vbox */
  gtk_container_add (GTK_CONTAINER (hbox_i), frame);

	/* box into expander */
  gtk_container_add (GTK_CONTAINER (my_expander), hbox_i);

	/*
  gtk_container_add (GTK_CONTAINER (my_expander), 
										 gtk_image_new_from_file ("./graf_zahl.jpg"));
	*/

  gtk_container_add (GTK_CONTAINER (box), my_expander);

  gtk_container_add (GTK_CONTAINER (window), box);

	gtk_widget_show_all (window);

  gtk_main ();
    
  return 0;
}
