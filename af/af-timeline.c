/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* gtktimeline.c
 *
 * Copyright (C) 2007 Carlos Garnacho <carlos@imendio.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <glib-object.h>
#include <math.h>
#include "af-timeline.h"

#define AF_TIMELINE_GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AF_TYPE_TIMELINE, AfTimelinePriv))
#define MSECS_PER_SEC 1000
#define FRAME_INTERVAL(nframes) (MSECS_PER_SEC / nframes)
#define DEFAULT_FPS 30

typedef struct AfTimelinePriv AfTimelinePriv;

struct AfTimelinePriv
{
  guint duration;
  guint fps;
  guint source_id;
  guint delay;

  GTimer *timer;

  GdkScreen *screen;

  guint animations_enabled : 1;
  guint loop               : 1;
  guint direction          : 1;
  guint delay_past         : 1;

  gdouble last_progress;

  GStaticMutex progress_mutex;
};

enum {
  PROP_0,
  PROP_FPS,
  PROP_DURATION,
  PROP_DELAY,
  PROP_LOOP,
  PROP_DIRECTION,
  PROP_SCREEN
};

enum {
  STARTED,
  PAUSED,
  FINISHED,
  FRAME,
  LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };


static void  af_timeline_set_property  (GObject         *object,
                                        guint            prop_id,
                                        const GValue    *value,
                                        GParamSpec      *pspec);
static void  af_timeline_get_property  (GObject         *object,
                                        guint            prop_id,
                                        GValue          *value,
                                        GParamSpec      *pspec);
static void  af_timeline_finalize      (GObject *object);


G_DEFINE_TYPE (AfTimeline, af_timeline, G_TYPE_OBJECT)


static void
af_timeline_class_init (AfTimelineClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = af_timeline_set_property;
  object_class->get_property = af_timeline_get_property;
  object_class->finalize = af_timeline_finalize;

  g_object_class_install_property (object_class,
				   PROP_FPS,
				   g_param_spec_uint ("fps",
						      "FPS",
						      "Frames per second for the timeline",
						      1,
						      G_MAXUINT,
						      DEFAULT_FPS,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_DURATION,
				   g_param_spec_uint ("duration",
						      "Animation Duration",
						      "Animation Duration",
						      0,
						      G_MAXUINT,
						      0,
						      G_PARAM_READWRITE));
   g_object_class_install_property (object_class,
				   PROP_DELAY,
				   g_param_spec_uint ("delay",
						      "Animation Delay",
						      "Animation Delay",
						      0,
						      G_MAXUINT,
						      0,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_LOOP,
				   g_param_spec_boolean ("loop",
							 "Loop",
							 "Whether the timeline loops or not",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_DIRECTION,
				   g_param_spec_enum ("direction",
						      "Direction",
						      "Whether the timeline moves forward or backward in time",
						      AF_TYPE_TIMELINE_DIRECTION,
						      AF_TIMELINE_DIRECTION_FORWARD,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_SCREEN,
				   g_param_spec_object ("screen",
							"Screen",
							"Screen to get the settings from",
							GDK_TYPE_SCREEN,
							G_PARAM_READWRITE));

  signals[STARTED] =
    g_signal_new ("started",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AfTimelineClass, started),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  signals[PAUSED] =
    g_signal_new ("paused",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AfTimelineClass, paused),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  signals[FINISHED] =
    g_signal_new ("finished",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AfTimelineClass, finished),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  signals[FRAME] =
    g_signal_new ("frame",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AfTimelineClass, frame),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__DOUBLE,
		  G_TYPE_NONE, 1,
		  G_TYPE_DOUBLE);

  g_type_class_add_private (class, sizeof (AfTimelinePriv));
}

static void
af_timeline_init (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  priv = AF_TIMELINE_GET_PRIV (timeline);

  priv->fps = DEFAULT_FPS;
  priv->duration = 0.0;
  priv->delay = 0.0;
  priv->direction = AF_TIMELINE_DIRECTION_FORWARD;
  priv->screen = gdk_screen_get_default ();

  priv->delay_past = FALSE;
  priv->last_progress = 0;

  g_static_mutex_init (&priv->progress_mutex);
}

static void
af_timeline_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  AfTimeline *timeline;
  AfTimelinePriv *priv;

  timeline = AF_TIMELINE (object);
  priv = AF_TIMELINE_GET_PRIV (timeline);

  switch (prop_id)
    {
    case PROP_FPS:
      af_timeline_set_fps (timeline, g_value_get_uint (value));
      break;
    case PROP_DURATION:
      af_timeline_set_duration (timeline, g_value_get_uint (value));
      break;
    case PROP_DELAY:
      af_timeline_set_delay (timeline, g_value_get_uint (value));
      break;
    case PROP_LOOP:
      af_timeline_set_loop (timeline, g_value_get_boolean (value));
      break;
    case PROP_DIRECTION:
      af_timeline_set_direction (timeline, g_value_get_enum (value));
      break;
    case PROP_SCREEN:
      af_timeline_set_screen (timeline,
                              GDK_SCREEN (g_value_get_object (value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
af_timeline_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  AfTimeline *timeline;
  AfTimelinePriv *priv;

  timeline = AF_TIMELINE (object);
  priv = AF_TIMELINE_GET_PRIV (timeline);

  switch (prop_id)
    {
    case PROP_FPS:
      g_value_set_uint (value, priv->fps);
      break;
    case PROP_DURATION:
      g_value_set_uint (value, priv->duration);
      break;
    case PROP_DELAY:
      g_value_set_uint (value, priv->delay);
      break;
    case PROP_LOOP:
      g_value_set_boolean (value, priv->loop);
      break;
    case PROP_DIRECTION:
      g_value_set_enum (value, priv->direction);
      break;
    case PROP_SCREEN:
      g_value_set_object (value, priv->screen);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
af_timeline_finalize (GObject *object)
{
  AfTimelinePriv *priv;

  priv = AF_TIMELINE_GET_PRIV (object);

  if (priv->source_id)
    {
      g_source_remove (priv->source_id);
      priv->source_id = 0;
    }

  if (priv->timer)
    g_timer_destroy (priv->timer);

  g_static_mutex_free (&priv->progress_mutex);

  G_OBJECT_CLASS (af_timeline_parent_class)->finalize (object);
}

static gboolean
af_timeline_run_frame (AfTimeline *timeline)
{
  AfTimelinePriv *priv;
  gdouble delta_progress, progress;
  guint elapsed_time;

  priv = AF_TIMELINE_GET_PRIV (timeline);

  elapsed_time = (guint) (g_timer_elapsed (priv->timer, NULL) * 1000);
  g_timer_start (priv->timer);

  if (priv->animations_enabled)
    {
      delta_progress = (gdouble) elapsed_time / priv->duration;

      /* enter critical section */
      g_static_mutex_lock (&priv->progress_mutex);
      progress = priv->last_progress;

      if (priv->direction == AF_TIMELINE_DIRECTION_BACKWARD)
	progress -= delta_progress;
      else
	progress += delta_progress;

      priv->last_progress = progress;
      g_static_mutex_unlock (&priv->progress_mutex);
      /* leave critical section */

      progress = CLAMP (progress, 0., 1.);
    }
  else
    progress = (priv->direction == AF_TIMELINE_DIRECTION_FORWARD) ? 1.0 : 0.0;

  g_signal_emit (timeline, signals [FRAME], 0, progress);

  if ((priv->direction == AF_TIMELINE_DIRECTION_FORWARD && progress >= 1.0) ||
      (priv->direction == AF_TIMELINE_DIRECTION_BACKWARD && progress <= 0.0))
    {
      if (!priv->loop)
	{
	  if (priv->source_id)
	    {
	      g_source_remove (priv->source_id);
	      priv->source_id = 0;
	    }
          g_timer_stop (priv->timer);
	  g_signal_emit (timeline, signals [FINISHED], 0);
	  return FALSE;
	}
      else
        {
          /* reset timer */
          if (priv->timer)
            {
              if (af_timeline_get_direction(timeline) != AF_TIMELINE_DIRECTION_FORWARD)
      	        priv->last_progress = 1.0 + progress;
              else
	        priv->last_progress = progress - 1.0;
	    }
	}
    }

  return TRUE;
}

/**
 * af_timeline_new:
 * @duration: duration in milliseconds for the timeline
 *
 * Creates a new #AfTimeline with the specified number of frames.
 *
 * Return Value: the newly created #AfTimeline
 **/
AfTimeline *
af_timeline_new (guint duration)
{
  return g_object_new (AF_TYPE_TIMELINE,
		       "duration", duration,
		       NULL);
}

AfTimeline *
af_timeline_new_n_frames (guint n_frames,
		 	  guint fps)
{
  return g_object_new (AF_TYPE_TIMELINE,
		       "duration", ((gdouble) (n_frames * 1000) / fps),
		       NULL);
}

AfTimeline *
af_timeline_new_for_screen (guint      duration,
                            GdkScreen *screen)
{
  return g_object_new (AF_TYPE_TIMELINE,
		       "duration", duration,
		       "screen", screen,
		       NULL);
}

AfTimeline *
af_timeline_clone (AfTimeline *timeline)
{
  return g_object_new (AF_TYPE_TIMELINE,
		       "fps", af_timeline_get_fps (timeline),
		       "direction", af_timeline_get_direction (timeline),
		       "loop", af_timeline_get_loop (timeline),
		       "duration", af_timeline_get_duration (timeline),
		       "screen", af_timeline_get_screen (timeline),
		       NULL);
}

void
af_timeline_start_init (AfTimeline *timeline)
{
  AfTimelinePriv *priv;
  GtkSettings *settings;
  gboolean enable_animations = FALSE;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  if (priv->timer)
    g_timer_continue (priv->timer);
  else
    priv->timer = g_timer_new ();

  /* sanity check */
  g_assert (priv->fps > 0);

  if (priv->screen)
    {
      settings = gtk_settings_get_for_screen (priv->screen);
      g_object_get (settings, "gtk-enable-animations", &enable_animations, NULL);
    }

  priv->animations_enabled = (enable_animations == TRUE);

  g_signal_emit (timeline, signals [STARTED], 0);

  if (enable_animations)
    priv->source_id = gdk_threads_add_timeout (FRAME_INTERVAL (priv->fps),
                                               (GSourceFunc) af_timeline_run_frame,
                                               timeline);
  else
    priv->source_id = gdk_threads_add_idle ((GSourceFunc) af_timeline_run_frame,
                                            timeline);
}

gboolean
af_timeline_start_delay (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  priv = AF_TIMELINE_GET_PRIV (timeline);

  if (priv->source_id && priv->timer
        && (g_timer_elapsed (priv->timer, NULL) * 1000) >= priv->delay)
    {
      priv->delay_past = TRUE;

      g_timer_destroy (priv->timer);
      priv->timer = NULL;

      g_source_remove (priv->source_id);

      af_timeline_start_init (timeline);

      return FALSE;
    }
  else
    return TRUE;
}

/**
 * af_timeline_start:
 * @timeline: A #AfTimeline
 *
 * Runs the timeline from the current frame.
 **/
void
af_timeline_start (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  if (!priv->source_id)
    {
      if (priv->delay > 0 && priv->delay_past == FALSE)
        {
	  if (!priv->timer)
	    priv->timer = g_timer_new ();
	  else
	    g_timer_continue (priv->timer);
            
	  priv->source_id = gdk_threads_add_timeout ((priv->delay - g_timer_elapsed (priv->timer, NULL) * 1000),
                                                     (GSourceFunc) af_timeline_start_delay,
                                                     timeline);
	}
      else
        af_timeline_start_init (timeline);      

    }
}

/**
 * af_timeline_pause:
 * @timeline: A #AfTimeline
 *
 * Pauses the timeline.
 **/
void
af_timeline_pause (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  if (priv->source_id)
    {
      if (priv->timer)
        g_timer_stop (priv->timer);
      
      g_source_remove (priv->source_id);
      priv->source_id = 0;
      g_signal_emit (timeline, signals [PAUSED], 0);
    }
}

/**
 * af_timeline_stop:
 * @timeline: A #AfTimeline
 *
 * Stopes the timeline and moves to frame 0.
 **/
void
af_timeline_stop (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  if (priv->source_id)
    {
      if (priv->timer)
        g_timer_stop (priv->timer);
      
      g_source_remove (priv->source_id);
      priv->source_id = 0;
    }

  priv->last_progress = 0.0;
}
/**
 * af_timeline_rewind:
 * @timeline: A #AfTimeline
 *
 * Rewinds the timeline.
 **/
void
af_timeline_rewind (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  priv->delay_past = FALSE;

  /* reset timer */
  if (priv->timer)
    {
      if (af_timeline_get_direction(timeline) != AF_TIMELINE_DIRECTION_FORWARD)
      	priv->last_progress = 1.0;
      else
	priv->last_progress = 0;

      g_timer_start (priv->timer);
      if (!priv->source_id)
        g_timer_stop (priv->timer);
    }
}

/**
 * af_timeline_skip:
 * @timeline: A #AfTimeline
 *
 * Skips the requested number of frames.
 **/
void
af_timeline_skip (AfTimeline *timeline,
		  guint       n_frames)
{
  AfTimelinePriv *priv;
  gdouble skip_msec;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  skip_msec = ((gdouble) n_frames * 1000) / priv->fps / priv->duration;

  /* enter critical section */
  g_static_mutex_lock (&priv->progress_mutex);

  if (priv->direction == AF_TIMELINE_DIRECTION_BACKWARD)
    priv->last_progress -= skip_msec;
  else
    priv->last_progress += skip_msec;

  g_static_mutex_unlock (&priv->progress_mutex);
  /* leave critical section */
}

/**
 * af_timeline_advance:
 * @timeline: A #AfTimeline
 *
 * Advance timeline to the requested frame number.
 **/
void
af_timeline_advance (AfTimeline *timeline,
		     guint       frame_num)
{
  AfTimelinePriv *priv;
  guint current_frame;
  gint skip_frames;
  gdouble skip_msec;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  /* enter critical section */
  g_static_mutex_lock (&priv->progress_mutex);

  current_frame = (priv->last_progress * priv->fps * priv->duration) / 1000;

  if ((current_frame < frame_num && priv->direction == AF_TIMELINE_DIRECTION_FORWARD)
      || (current_frame > frame_num && priv->direction == AF_TIMELINE_DIRECTION_BACKWARD))
    {
      /* substract 1 so the next handled frame is the one requested */
      skip_frames = frame_num - current_frame;
      skip_frames = ABS(skip_frames) - 1;

      skip_msec = ((gdouble) skip_frames * 1000) / priv->fps / priv->duration;

      if (priv->direction == AF_TIMELINE_DIRECTION_BACKWARD)
        priv->last_progress -= skip_msec;
      else
        priv->last_progress += skip_msec;
    }

  g_static_mutex_unlock (&priv->progress_mutex);
  /* leave critical section */
}

/**
 * af_timeline_get_current_frame:
 * @timeline: A #AfTimeline
 *
 * Request the current frame number of the timeline.
 **/
guint
af_timeline_get_current_frame (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_val_if_fail (AF_IS_TIMELINE (timeline), 0);

  priv = AF_TIMELINE_GET_PRIV (timeline);

  return (priv->last_progress * priv->fps * priv->duration) / 1000;
}

/**
 * af_timeline_get_progress:
 * @timeline: A #AfTimeline
 *
 * The position of the timeline in a [0, 1] interval.
 **/
gdouble
af_timeline_get_progress (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_val_if_fail (AF_IS_TIMELINE (timeline), 0);

  priv = AF_TIMELINE_GET_PRIV (timeline);

  return priv->last_progress;
}
/**
 * af_timeline_is_running:
 * @timeline: A #AfTimeline
 *
 * Returns whether the timeline is running or not.
 *
 * Return Value: %TRUE if the timeline is running
 **/
gboolean
af_timeline_is_running (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_val_if_fail (AF_IS_TIMELINE (timeline), FALSE);

  priv = AF_TIMELINE_GET_PRIV (timeline);

  return (priv->source_id != 0);
}

/**
 * af_timeline_get_fps:
 * @timeline: A #AfTimeline
 *
 * Returns the number of frames per second.
 *
 * Return Value: frames per second
 **/
guint
af_timeline_get_fps (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_val_if_fail (AF_IS_TIMELINE (timeline), 1);

  priv = AF_TIMELINE_GET_PRIV (timeline);
  return priv->fps;
}

/**
 * af_timeline_set_fps:
 * @timeline: A #AfTimeline
 * @fps: frames per second
 *
 * Sets the number of frames per second that
 * the timeline will play.
 **/
void
af_timeline_set_fps (AfTimeline *timeline,
                     guint       fps)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));
  g_return_if_fail (fps > 0);

  priv = AF_TIMELINE_GET_PRIV (timeline);

  priv->fps = fps;

  if (af_timeline_is_running (timeline))
    {
      g_source_remove (priv->source_id);
      priv->source_id = gdk_threads_add_timeout (FRAME_INTERVAL (priv->fps),
						 (GSourceFunc) af_timeline_run_frame,
						 timeline);
    }

  g_object_notify (G_OBJECT (timeline), "fps");
}

/**
 * af_timeline_get_loop:
 * @timeline: A #AfTimeline
 *
 * Returns whether the timeline loops to the
 * beginning when it has reached the end.
 *
 * Return Value: %TRUE if the timeline loops
 **/
gboolean
af_timeline_get_loop (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_val_if_fail (AF_IS_TIMELINE (timeline), FALSE);

  priv = AF_TIMELINE_GET_PRIV (timeline);
  return priv->loop;
}

/**
 * af_timeline_set_loop:
 * @timeline: A #AfTimeline
 * @loop: %TRUE to make the timeline loop
 *
 * Sets whether the timeline loops to the beginning
 * when it has reached the end.
 **/
void
af_timeline_set_loop (AfTimeline *timeline,
                      gboolean    loop)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);
  priv->loop = loop;

  g_object_notify (G_OBJECT (timeline), "loop");
}

void
af_timeline_set_duration (AfTimeline *timeline,
                          guint       duration)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  priv->duration = duration;

  g_object_notify (G_OBJECT (timeline), "duration");
}

guint
af_timeline_get_duration (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_val_if_fail (AF_IS_TIMELINE (timeline), 0);

  priv = AF_TIMELINE_GET_PRIV (timeline);

  return priv->duration;
}

void
af_timeline_set_delay (AfTimeline *timeline,
                       guint       delay)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  priv->delay = delay;

  g_object_notify (G_OBJECT (timeline), "delay");
}

guint
af_timeline_get_delay (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_val_if_fail (AF_IS_TIMELINE (timeline), 0);

  priv = AF_TIMELINE_GET_PRIV (timeline);

  return priv->delay;
}

void
af_timeline_set_n_frames (AfTimeline *timeline,
			  guint	     n_frames)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  priv->duration = ((gdouble) n_frames * 1000) / priv->fps;

  g_object_notify (G_OBJECT (timeline), "duration");

}

guint
af_timeline_get_n_frames (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_val_if_fail (AF_IS_TIMELINE (timeline), 0);

  priv = AF_TIMELINE_GET_PRIV (timeline);

  return (((gdouble) priv->duration * priv->fps) / 1000);
}

/**
 * af_timeline_get_direction:
 * @timeline: A #AfTimeline
 *
 * Returns the direction of the timeline.
 *
 * Return Value: direction
 **/
AfTimelineDirection
af_timeline_get_direction (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_val_if_fail (AF_IS_TIMELINE (timeline), AF_TIMELINE_DIRECTION_FORWARD);

  priv = AF_TIMELINE_GET_PRIV (timeline);
  return priv->direction;
}

/**
 * af_timeline_set_direction:
 * @timeline: A #AfTimeline
 * @direction: direction
 *
 * Sets the direction of the timeline.
 **/
void
af_timeline_set_direction (AfTimeline          *timeline,
                           AfTimelineDirection  direction)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);
  priv->direction = direction;

  g_object_notify (G_OBJECT (timeline), "direction");
}

GdkScreen *
af_timeline_get_screen (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_val_if_fail (AF_IS_TIMELINE (timeline), NULL);

  priv = AF_TIMELINE_GET_PRIV (timeline);
  return priv->screen;
}

void
af_timeline_set_screen (AfTimeline *timeline,
                        GdkScreen  *screen)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));
  g_return_if_fail (GDK_IS_SCREEN (screen));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  if (priv->screen)
    g_object_unref (priv->screen);

  priv->screen = g_object_ref (screen);

  g_object_notify (G_OBJECT (timeline), "screen");
}

gdouble
af_timeline_calculate_progress (gdouble                linear_progress,
                                AfTimelineProgressType progress_type)
{
  gdouble progress;

  progress = linear_progress;

  switch (progress_type)
    {
    case AF_TIMELINE_PROGRESS_LINEAR:
      break;
    case AF_TIMELINE_PROGRESS_SINUSOIDAL:
      progress = sinf ((progress * G_PI) / 2);
      break;
    case AF_TIMELINE_PROGRESS_EXPONENTIAL:
      progress *= progress;
      break;
    case AF_TIMELINE_PROGRESS_EASE_IN_EASE_OUT:
      {
        progress *= 2;

        if (progress < 1)
          progress = pow (progress, 3) / 2;
        else
          progress = (pow (progress - 2, 3) + 2) / 2;
      }
    }

  return progress;
}
