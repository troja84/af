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

  GTimer *timer;

  GdkScreen *screen;
  AfTimelineProgressType progress_type;
  AfTimelineProgressFunc progress_func;

  guint animations_enabled : 1;
  guint loop               : 1;
  guint direction          : 1;

  gdouble last_progress;
};

enum {
  PROP_0,
  PROP_FPS,
  PROP_DURATION,
  PROP_LOOP,
  PROP_DIRECTION,
  PROP_SCREEN,
  PROP_PROGRESS_TYPE,
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
				   PROP_DIRECTION,
				   g_param_spec_enum ("progress-type",
						      "Progress type",
						      "Type of progress through the timeline",
						      AF_TYPE_TIMELINE_PROGRESS_TYPE,
						      AF_TIMELINE_PROGRESS_LINEAR,
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
  priv->direction = AF_TIMELINE_DIRECTION_FORWARD;
  priv->screen = gdk_screen_get_default ();

  priv->last_progress = 0;
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
    case PROP_PROGRESS_TYPE:
      af_timeline_set_progress_type (timeline, g_value_get_enum (value));
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
    case PROP_LOOP:
      g_value_set_boolean (value, priv->loop);
      break;
    case PROP_DIRECTION:
      g_value_set_enum (value, priv->direction);
      break;
    case PROP_SCREEN:
      g_value_set_object (value, priv->screen);
      break;
    case PROP_PROGRESS_TYPE:
      g_value_set_enum (value, priv->progress_type);
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

  G_OBJECT_CLASS (af_timeline_parent_class)->finalize (object);
}

/* Sinusoidal progress */
static gdouble
sinusoidal_progress (gdouble progress)
{
  return (sinf ((progress * G_PI) / 2));
}

static gdouble
exponential_progress (gdouble progress)
{
  return progress * progress;
}

static gdouble
ease_in_ease_out_progress (gdouble progress)
{
  progress *= 2;

   if (progress < 1)
     return pow (progress, 3) / 2;

   return (pow (progress - 2, 3) + 2) / 2;
}

static AfTimelineProgressFunc
progress_type_to_func (AfTimelineProgressType type)
{
  if (type == AF_TIMELINE_PROGRESS_SINUSOIDAL)
    return sinusoidal_progress;
  else if (type == AF_TIMELINE_PROGRESS_EXPONENTIAL)
    return exponential_progress;
  else if (type == AF_TIMELINE_PROGRESS_EASE_IN_EASE_OUT)
    return ease_in_ease_out_progress;

  return NULL;
}

static gboolean
af_timeline_run_frame (AfTimeline *timeline)
{
  AfTimelinePriv *priv;
  gdouble linear_progress, delta_progress, progress;
  guint elapsed_time;
  AfTimelineProgressFunc progress_func = NULL;

  priv = AF_TIMELINE_GET_PRIV (timeline);

  elapsed_time = (guint) (g_timer_elapsed (priv->timer, NULL) * 1000);
  g_timer_start (priv->timer);
  linear_progress = priv->last_progress;
  delta_progress = (gdouble) elapsed_time / priv->duration;

  if (priv->animations_enabled)
    {
      if (priv->direction == AF_TIMELINE_DIRECTION_BACKWARD)
	linear_progress -= delta_progress;
      else
	linear_progress += delta_progress;
      
      priv->last_progress = linear_progress;

      linear_progress = CLAMP (linear_progress, 0., 1.);

      if (priv->progress_func)
	progress_func = priv->progress_func;
      else if (priv->progress_type)
	progress_func = progress_type_to_func (priv->progress_type);

      if (progress_func)
	progress = (progress_func) (linear_progress);
      else
	progress = linear_progress;
    }
  else
    progress = (priv->direction == AF_TIMELINE_DIRECTION_FORWARD) ? 1.0 : 0.0;

  g_signal_emit (timeline, signals [FRAME], 0,
		 CLAMP (progress, 0.0, 1.0));

  if ((priv->direction == AF_TIMELINE_DIRECTION_FORWARD && linear_progress >= 1.0) ||
      (priv->direction == AF_TIMELINE_DIRECTION_BACKWARD && linear_progress <= 0.0))
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
	af_timeline_rewind (timeline);
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
af_timeline_new_for_screen (guint      duration,
                            GdkScreen *screen)
{
  return g_object_new (AF_TYPE_TIMELINE,
		       "duration", duration,
		       "screen", screen,
		       NULL);
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
  GtkSettings *settings;
  gboolean enable_animations = FALSE;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  if (!priv->source_id)
    {
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
      g_timer_stop (priv->timer);
      g_source_remove (priv->source_id);
      priv->source_id = 0;
      g_signal_emit (timeline, signals [PAUSED], 0);
    }
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

void
af_timeline_set_progress_type (AfTimeline             *timeline,
                               AfTimelineProgressType  type)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);

  priv->progress_type = type;

  g_object_notify (G_OBJECT (timeline), "progress-type");
}

AfTimelineProgressType
af_timeline_get_progress_type (AfTimeline *timeline)
{
  AfTimelinePriv *priv;

  g_return_val_if_fail (AF_IS_TIMELINE (timeline), AF_TIMELINE_PROGRESS_LINEAR);

  priv = AF_TIMELINE_GET_PRIV (timeline);

  if (priv->progress_func)
    return AF_TIMELINE_PROGRESS_LINEAR;

  return priv->progress_type;
}

/**
 * af_timeline_set_progress_func:
 * @timeline: A #AfTimeline
 * @progress_func: progress function
 *
 * Sets the progress function. This function will be used to calculate
 * a different progress to pass to the ::frame signal based on the
 * linear progress through the timeline. Setting progress_func
 * to %NULL will make the timeline use the default function,
 * which is just a linear progress.
 *
 * All progresses are in the [0.0, 1.0] range.
 **/
void
af_timeline_set_progress_func (AfTimeline             *timeline,
                               AfTimelineProgressFunc  progress_func)
{
  AfTimelinePriv *priv;

  g_return_if_fail (AF_IS_TIMELINE (timeline));

  priv = AF_TIMELINE_GET_PRIV (timeline);
  priv->progress_func = progress_func;
}
