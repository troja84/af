/*
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
#include <gobject/gvaluecollector.h>
#include <string.h>

#include "af-animator.h"
#include "af-timeline.h"

static GHashTable *animators = NULL;
static GHashTable *transformable_types = NULL;
static guint id_count = 0;

typedef struct AfPropertyRange AfPropertyRange;
typedef struct AfAnimator AfAnimator;

struct AfPropertyRange
{
  GParamSpec *pspec;
  GValue from;
  GValue to;
};

struct AfTransition
{
  gdouble from;
  gdouble to;
  AfTimelineProgressType type;
  AfTypeTransformationFunc func;

  GObject *object;
  GObject *child;
  GArray *properties;
};

struct AfAnimator
{
  AfTimeline *timeline;
  GPtrArray *transitions;
  GPtrArray *finished_transitions;

  gpointer user_data;
  GDestroyNotify value_destroy_func;

  AfFinishedAnimationNotify finished_notify;
};

static void af_animator_remove_with_notification (guint id);

static AfTransition *
af_transition_new (GObject                 *object,
                   GObject                 *child,
                   gdouble                  from,
                   gdouble                  to,
                   AfTimelineProgressType   type)
{
  AfTransition *transition;

  transition = g_slice_new0 (AfTransition);
  transition->object = g_object_ref (object);
  transition->from = from;
  transition->to = to;
  transition->type = type;
  transition->properties = g_array_new (FALSE, FALSE,
                                       sizeof (AfPropertyRange));

  if (child)
    transition->child = g_object_ref (child);

  return transition;
}

static void
af_transition_free (AfTransition *transition)
{
  guint i;

  g_object_unref (transition->object);

  if (transition->child)
    g_object_unref (transition->child);

  for (i = 0; i < transition->properties->len; i++)
    {
      AfPropertyRange *property_range;

      property_range = &g_array_index (transition->properties, AfPropertyRange, i);
      g_param_spec_unref (property_range->pspec);
    }

  g_array_free (transition->properties, TRUE);
  g_slice_free (AfTransition, transition);
}

static AfAnimator *
af_animator_new (void)
{
  AfAnimator *animator;

  animator = g_slice_new0 (AfAnimator);
  animator->transitions = g_ptr_array_new ();
  animator->finished_transitions = g_ptr_array_new ();

  animator->user_data = NULL;
  animator->value_destroy_func = NULL;

  animator->finished_notify = NULL;

  return animator;
}

static void
af_animator_free (AfAnimator *animator)
{
  if (animator->timeline)
    {
      af_timeline_pause (animator->timeline);
      g_object_unref (animator->timeline);
    }

  g_ptr_array_foreach (animator->transitions,
                       (GFunc) af_transition_free,
                       NULL);
  g_ptr_array_free (animator->transitions, TRUE);


  g_ptr_array_foreach (animator->finished_transitions,
                       (GFunc) af_transition_free,
                       NULL);
  g_ptr_array_free (animator->finished_transitions, TRUE);

  if (animator->value_destroy_func)
    (animator->value_destroy_func) (animator->user_data);

  g_slice_free (AfAnimator, animator);
}

static void
af_transition_set_progress (AfTransition *transition,
                            gdouble       progress,
			    gpointer      user_data)
{
  GValue value = { 0, };
  GArray *properties;
  guint i;
  AfTypeTransformationFunc func;

  properties = transition->properties;
  progress = af_timeline_calculate_progress (progress, transition->type);

  for (i = 0; i < properties->len; i++)
    {
      AfPropertyRange *property_range;
      gboolean handled = FALSE;
      GType type;

      property_range = &g_array_index (properties, AfPropertyRange, i);

      if (G_UNLIKELY (G_VALUE_TYPE (&property_range->from) == G_TYPE_INVALID))
        {
          g_value_init (&property_range->from,
                        property_range->pspec->value_type);

          if (!transition->child)
            g_object_get_property (transition->object,
                                   property_range->pspec->name,
                                   &property_range->from);
          else
            gtk_container_child_get_property (GTK_CONTAINER (transition->object),
                                              GTK_WIDGET (transition->child),
                                              property_range->pspec->name,
                                              &property_range->from);
        }

      g_value_init (&value, property_range->pspec->value_type);
      type = property_range->pspec->value_type;
      
      func = transition->func;

      if (!func && transformable_types)
        func = g_hash_table_lookup (transformable_types, GSIZE_TO_POINTER (type));

      if (!func)
        {
          switch (type)
            {
              case G_TYPE_INT:
                {
                  gint from, to, val;

                  from = g_value_get_int (&property_range->from);
                  to = g_value_get_int (&property_range->to);

                  val = from + ((to - from) * progress);
                  g_value_set_int (&value, val);
                  handled = TRUE;
                }

                break;
              case G_TYPE_DOUBLE:
                {
                  gdouble from, to, val;

                  from = g_value_get_double (&property_range->from);
                  to = g_value_get_double (&property_range->to);

                  val = from + ((to - from) * progress);
                  g_value_set_double (&value, val);
                  handled = TRUE;
		}

                break;
              case G_TYPE_FLOAT:
                {
                  gfloat from, to, val;

                  from = g_value_get_float (&property_range->from);
                  to = g_value_get_float (&property_range->to);

                  val = from + ((to - from) * progress);
                  g_value_set_float (&value, val);
                  handled = TRUE;
                }

                break;
              default:
                g_warning ("Property of type '%s' not handled", g_type_name (type));
            }
        }
      else
        {
          (func) (&property_range->from,
                  &property_range->to,
                  progress,
	          user_data,
                  &value);
           handled = TRUE;
        }


      if (handled)
        {
          if (!transition->child)
            g_object_set_property (transition->object,
                                   property_range->pspec->name,
                                   &value);
          else
            gtk_container_child_set_property (GTK_CONTAINER (transition->object),
                                              GTK_WIDGET (transition->child),
                                              property_range->pspec->name,
                                              &value);
        }

      g_value_unset (&value);
    }
}

static void
animator_frame_cb (AfTimeline *timeline,
                   gdouble     progress,
                   gpointer    user_data)
{
  AfAnimator *animator;
  AfTransition *transition;
  guint i = 0;

  animator = (AfAnimator *) user_data;

  while (i < animator->transitions->len)
    {
      gdouble transition_progress;

      transition = g_ptr_array_index (animator->transitions, i);

      if (progress <= transition->from)
        {
          i++;
          continue;
        }

      transition_progress = progress - transition->from;
      transition_progress /= (transition->to - transition->from);
      transition_progress = CLAMP (transition_progress, 0.0, 1.0);

      af_transition_set_progress (transition, 
		                  transition_progress,
				  animator->user_data);

      if (progress >= transition->to)
        {
          g_ptr_array_remove_index_fast (animator->transitions, i);
          g_ptr_array_add (animator->finished_transitions, transition);
        }
      else
        i++;
    }

  if (progress == 1.0 &&
      af_timeline_get_loop (timeline))
    {
      /* Animation is about to begin again,
       * dump all finished transitions back.
       */
      while (animator->finished_transitions->len > 0)
        {
          transition = g_ptr_array_remove_index_fast (animator->finished_transitions, 0);
          g_ptr_array_add (animator->transitions, transition);
        }
    }
}

static gboolean
transition_add_property (AfTransition            *transition,
                         GParamSpec              *pspec,
                         GValue                  *to,
			 AfTypeTransformationFunc func)
{
  AfPropertyRange property_range = { 0, };

  if (G_VALUE_TYPE (to) != pspec->value_type)
    {
      g_warning ("Target value type (%s) do not match property type (%s)",
                 G_VALUE_TYPE_NAME (to),
                 g_type_name (pspec->value_type));
      return FALSE;
    }

  property_range.pspec = g_param_spec_ref (pspec);

  g_value_init (&property_range.to, pspec->value_type);
  g_value_copy (to, &property_range.to);

  g_array_append_val (transition->properties, property_range);

  transition->func = func;

  return TRUE;
}

static void
transition_add_properties (AfTransition *transition,
                           va_list       args)
{
  const gchar *property_name;
  GObject *object, *child;

  property_name = va_arg (args, const gchar *);
  object = transition->object;
  child = transition->child;

  while (property_name)
    {
      GParamSpec *pspec;
      GValue to = { 0, };
      GValue func = { 0, };
      gchar *error = NULL;

      if (!child)
        pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                              property_name);
      else
        pspec = gtk_container_class_find_child_property (G_OBJECT_GET_CLASS (object),
                                                         property_name);

      if (G_UNLIKELY (!pspec))
        {
          g_warning ("Property '%s' does not exist on object of class '%s'",
                     property_name, G_OBJECT_TYPE_NAME (object));
          break;
        }

      g_value_init (&to, pspec->value_type);
      G_VALUE_COLLECT (&to, args, 0, &error);

      if (error)
        {
          g_warning (error);
          g_free (error);
          break;
        }

      g_value_init (&func, G_TYPE_POINTER);
      G_VALUE_COLLECT (&func, args, 0, &error);

      if (error)
        {
          g_warning (error);
          g_free (error);
          break;
        }

      transition_add_property (transition, pspec, &to, 
		               (AfTypeTransformationFunc)g_value_get_pointer (&func));

      g_value_unset (&to);
      g_value_unset (&func);

      property_name = va_arg (args, const gchar *);
    }
}

void
af_animator_register_type_transformation (GType                    type,
                                          AfTypeTransformationFunc trans_func)
{
  if (!transformable_types)
    transformable_types = g_hash_table_new (g_direct_hash, g_direct_equal);

  if (!trans_func)
    g_hash_table_remove (transformable_types, GSIZE_TO_POINTER (type));
  else
    g_hash_table_insert (transformable_types,
                         GSIZE_TO_POINTER (type),
                         trans_func);
}

guint
af_animator_add (void)
{
  AfAnimator *animator;
  guint id;

  animator = af_animator_new ();
  id = ++id_count;

  if (G_UNLIKELY (!animators))
    animators = g_hash_table_new_full (g_direct_hash,
                                       g_direct_equal,
                                       NULL,
                                       (GDestroyNotify) af_animator_free);

  g_hash_table_insert (animators,
                       GUINT_TO_POINTER (id),
                       animator);
  return id;
}

gboolean
af_animator_set_user_data (guint          anim_id,
		           gpointer       user_data,
		           GDestroyNotify value_destroy_func)
{
  AfAnimator *animator;

  g_return_val_if_fail (animators != NULL, FALSE);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (anim_id));

  g_return_val_if_fail (animator != NULL, FALSE);

  if (animator->value_destroy_func)
    (animator->value_destroy_func) (animator->user_data);

  animator->user_data = user_data;
  animator->value_destroy_func = value_destroy_func;

  return TRUE;
}

gboolean
af_animator_set_finished_notify (guint anim_id,
		                 AfFinishedAnimationNotify finished_notify)
{
  AfAnimator *animator;

  g_return_val_if_fail (animators != NULL, FALSE);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (anim_id));

  g_return_val_if_fail (animator != NULL, FALSE);

  animator->finished_notify = finished_notify;

  return TRUE;
}

AfTransition*
af_animator_add_transition_valist (guint                   anim_id,
                                   gdouble                 from,
                                   gdouble                 to,
                                   AfTimelineProgressType  type,
                                   GObject                *object,
                                   va_list                 args)
{
  AfAnimator *animator;
  AfTransition *transition;

  g_return_val_if_fail (animators != NULL, FALSE);
  g_return_val_if_fail (anim_id != 0, FALSE);
  g_return_val_if_fail (from >= 0.0 && from <= 1.0, FALSE);
  g_return_val_if_fail (to >= 0.0 && to <= 1.0, FALSE);
  g_return_val_if_fail (from <= to, FALSE);
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (anim_id));

  g_return_val_if_fail (animator != NULL, FALSE);

  transition = af_transition_new (object, NULL, from, to, type);

  transition_add_properties (transition, args);

  g_ptr_array_add (animator->transitions, transition);

  return transition;
}

AfTransition*
af_animator_add_transition (guint                   anim_id,
                            gdouble                 from,
                            gdouble                 to,
                            AfTimelineProgressType  type,
                            GObject                *object,
                            ...)
{
  AfTransition *result;
  va_list args;

  va_start (args, object);
  result = af_animator_add_transition_valist (anim_id,
                                              from, to,
                                              type,
                                              object, args);
  va_end (args);

  va_start (args, object);
  return result;
}

AfTransition*
af_animator_add_child_transition_valist (guint                    anim_id,
                                         gdouble                  from,
                                         gdouble                  to,
                                         AfTimelineProgressType   type,
                                         GtkContainer            *container,
                                         GtkWidget               *child,
                                         va_list                  args)
{
  AfAnimator *animator;
  AfTransition *transition;

  g_return_val_if_fail (animators != NULL, FALSE);
  g_return_val_if_fail (anim_id != 0, FALSE);
  g_return_val_if_fail (from >= 0.0 && from <= 1.0, FALSE);
  g_return_val_if_fail (to >= 0.0 && to <= 1.0, FALSE);
  g_return_val_if_fail (from <= to, FALSE);
  g_return_val_if_fail (GTK_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (GTK_IS_WIDGET (child), FALSE);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (anim_id));

  g_return_val_if_fail (animator != NULL, FALSE);

  transition = af_transition_new (G_OBJECT (container),
                                  G_OBJECT (child),
                                  from, to,
                                  type);

  transition_add_properties (transition, args);

  g_ptr_array_add (animator->transitions, transition);

  return transition;
}

AfTransition*
af_animator_add_child_transition (guint                   anim_id,
                                  gdouble                 from,
                                  gdouble                 to,
                                  AfTimelineProgressType  type,
                                  GtkContainer           *container,
                                  GtkWidget              *child,
                                  ...)
{
  AfTransition *result;
  va_list args;

  va_start (args, child);
  result = af_animator_add_child_transition_valist (anim_id,
                                                    from, to,
                                                    type,
                                                    container, child, args);
  va_end (args);

  return result;
}

gboolean
af_animator_remove_transition (guint         id,
		               AfTransition *transition)
{
  AfAnimator *animator;

  g_return_val_if_fail (animators != NULL, FALSE);
  g_return_val_if_fail (transition != NULL, FALSE);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  g_return_val_if_fail (animator != NULL, FALSE);

  if (g_ptr_array_remove (animator->transitions, transition) == FALSE)
    return g_ptr_array_remove (animator->finished_transitions, transition);

  return TRUE;
}

gboolean
af_animator_start (guint id,
                   guint duration)
{
  AfAnimator *animator;

  g_return_val_if_fail (animators != NULL, FALSE);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  g_return_val_if_fail (animator != NULL, FALSE);
  g_return_val_if_fail (animator->timeline == NULL, FALSE);

  animator->timeline = af_timeline_new (duration);

  g_signal_connect (animator->timeline, "frame",
                    G_CALLBACK (animator_frame_cb), animator);
  g_signal_connect_swapped (animator->timeline, "finished",
                            G_CALLBACK (af_animator_remove_with_notification),
                            GUINT_TO_POINTER (id));

  af_timeline_start (animator->timeline);

  return TRUE;
}

gdouble
af_animator_pause (guint id)
{
  AfAnimator *animator;

  g_return_val_if_fail (animators != NULL, -1);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  g_return_val_if_fail (animator != NULL, -1);

  af_timeline_pause (animator->timeline);

  return af_timeline_get_progress(animator->timeline);
}

void
af_animator_resume (guint id)
{
  AfAnimator *animator;

  g_return_if_fail (animators != NULL);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  g_return_if_fail (animator != NULL);

  af_timeline_start (animator->timeline);
}

void
af_animator_resume_with_progress (guint   id,
		                  gdouble progress)
{
  AfAnimator *animator;

  g_return_if_fail (progress >= 0 && progress <= 1);

  g_return_if_fail (animators != NULL);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  g_return_if_fail (animator != NULL);

  af_timeline_set_progress (animator->timeline, progress);

  af_timeline_start (animator->timeline);
}

void
af_animator_reverse (guint id)
{
  AfAnimator *animator;
  AfTimelineDirection direction;

  g_return_if_fail (animators != NULL);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  g_return_if_fail (animator != NULL);

  direction = af_timeline_get_direction(animator->timeline);

  if (direction == AF_TIMELINE_DIRECTION_FORWARD)
    af_timeline_set_direction(animator->timeline, 
                              AF_TIMELINE_DIRECTION_BACKWARD);
  else
    af_timeline_set_direction(animator->timeline, 
                              AF_TIMELINE_DIRECTION_FORWARD);
}

void
af_animator_advance (guint   id,
		     gdouble progress)
{
  AfAnimator *animator;

  g_return_if_fail (animators != NULL);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  g_return_if_fail (animator != NULL);

  af_timeline_advance (animator->timeline, progress);
}

void
af_animator_remove (guint id)
{
  g_return_if_fail (animators != NULL);

  g_hash_table_remove (animators, GUINT_TO_POINTER (id));
}

static void 
af_animator_remove_with_notification (guint id)
{
  AfAnimator *animator;

  g_return_if_fail (animators != NULL);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  g_return_if_fail (animator != NULL);

  if (animator->finished_notify)
    (animator->finished_notify) (id, animator->user_data);

  g_hash_table_remove (animators, GUINT_TO_POINTER (id));
}

void
af_animator_set_loop (guint    id,
                      gboolean loop)
{
  AfAnimator *animator;

  g_return_if_fail (animators != NULL);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  g_return_if_fail (animator != NULL);

  af_timeline_set_loop (animator->timeline, loop);
}

guint
af_animator_tween (GObject                  *object,
                   guint                     duration,
                   AfTimelineProgressType    type,
		   gpointer                  user_data,
		   GDestroyNotify            value_destroy_func,
		   AfFinishedAnimationNotify finished_notify,
                   ...)
{
  va_list args;
  guint anim_id;

  g_return_val_if_fail (G_IS_OBJECT (object), 0);

  anim_id = af_animator_add ();

  if (user_data)
    af_animator_set_user_data (anim_id, 
		               user_data,
			       value_destroy_func);

  if (finished_notify)
    af_animator_set_finished_notify (anim_id,
		                     finished_notify);

  va_start (args, finished_notify);

  af_animator_add_transition_valist (anim_id,
                                     0.0, 1.0,
                                     type,
                                     object,
                                     args);
  va_end (args);

  af_animator_start (anim_id, duration);

  return anim_id;

}

guint
af_animator_child_tween (GtkContainer             *container,
                         GtkWidget                *child,
                         guint                     duration,
                         AfTimelineProgressType    type,
                         ...)
{
  va_list args;
  guint anim_id;

  g_return_val_if_fail (GTK_IS_CONTAINER (container), 0);
  g_return_val_if_fail (GTK_IS_WIDGET (child), 0);

  anim_id = af_animator_add ();

  va_start (args, type);

  af_animator_add_child_transition_valist (anim_id,
                                           0.0, 1.0,
                                           type,
                                           container,
                                           child,
                                           args);
  va_end (args);

  af_animator_start (anim_id, duration);

  return anim_id;
}

