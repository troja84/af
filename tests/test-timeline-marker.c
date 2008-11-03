#include <gtk/gtk.h>
#include <af/af-animator.h>
#include <af/af-timeline.h>

#define DURATION 3000
#define DELAY    3000
#define SKIP     (DURATION * (1/3))
#define ADVANCE  (DURATION / 2)

static GtkWidget *label, *combo_box = NULL;

static AfTimeline *timeline = NULL;

static gint length = 7;
static guint msecs[] = {500, 2000, 150, 150, 2500, 310, 0};
static gchar *markers[] = {"Near middle", "near end", "near start", 
			   "also near start", "far of", "middle start", 
			   "the real start"};


static AfTimeline *
init_timeline (AfTimeline *timeline, 
	       guint duration,
	       guint *frames,
	       gchar **markers,
	       gsize size)
{
  gint x;	

  timeline = af_timeline_new (duration);

  for (x=0; x < size; x++)
    {
      af_timeline_add_marker_at_time (timeline, markers[x], msecs[x]);
    }

  return timeline;
}

static void
init_gtk_combo_box_text (AfTimeline *timeline,
		         GtkWidget *combo_box)
		         
{
  gchar **list;
  gsize size;
  gint y;

  g_assert (timeline);

  list = af_timeline_list_markers (timeline, -1, &size);

  for (y = 0; y < size; y++)
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), list[y]);
    }

  g_strfreev (list);
}

static void
free_timeline (AfTimeline *timeline)
{
  if (!timeline)
    return;

  af_timeline_pause (timeline);
  g_object_unref (timeline);
}

static void
anim_cb (AfTimeline   *timeline,
         gdouble      progress,
	 const gchar* marker_name,
         gpointer     user_data)
{
  gtk_label_set_label (GTK_LABEL (label), marker_name);
}

static void
play_cb (GtkButton *button,
         gpointer   user_data)
{
  if (!timeline)
    init_timeline (timeline, 4000, msecs, markers, length);

  af_timeline_set_loop (timeline, TRUE);

  g_signal_connect (timeline, "marker",
		    G_CALLBACK (anim_cb), label);
    
  af_timeline_start (timeline);
}

static void
play_delay_cb (GtkButton *button,
               gpointer   user_data)
{
  if (!timeline)
    init_timeline (timeline, 4000, msecs, markers, length);

  af_timeline_set_delay (timeline, DELAY);
    
  af_timeline_start (timeline);
}

static void
pause_cb (GtkButton *button,
          gpointer   user_data)
{
  g_assert (timeline);
    
  af_timeline_pause (timeline);
}

static void
reverse_cb (GtkButton *button,
          gpointer   user_data)
{
  AfTimelineDirection direction;

  g_assert (timeline);
  
  direction = af_timeline_get_direction(timeline);

  if (direction == AF_TIMELINE_DIRECTION_FORWARD)
    af_timeline_set_direction(timeline, 
                              AF_TIMELINE_DIRECTION_BACKWARD);
  else
    af_timeline_set_direction(timeline, 
                              AF_TIMELINE_DIRECTION_FORWARD);
 
}

static void
stop_cb (GtkButton *button,
         gpointer   user_data)
{
  if (timeline)
    af_timeline_stop (timeline);
}

static void
skip_cb (GtkButton *button,
         gpointer   user_data)
{
  if (timeline)
    af_timeline_skip (timeline, SKIP);
}

static void
advance_cb (GtkButton *button,
            gpointer   user_data)
{
  if (timeline)
    af_timeline_advance (timeline, ADVANCE);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *box, *hbox, *bbox, *button;

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

  button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_FORWARD);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (skip_cb), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_NEXT);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (advance_cb), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_REFRESH);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (play_delay_cb), NULL);

  gtk_box_pack_end (GTK_BOX (box), bbox, FALSE, FALSE, 0);

  /* COMBOX BOX */
  timeline = init_timeline (timeline, DURATION, msecs, markers, length);

  combo_box = gtk_combo_box_new_text ();

  g_assert (timeline);

  init_gtk_combo_box_text (timeline, combo_box); 
  label = gtk_label_new ("...");

  hbox = gtk_hbox_new (TRUE, 6);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), combo_box, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  /* END */

  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);

  gtk_main ();

  free_timeline (timeline);

  return 0;
}
