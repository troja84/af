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
typedef struct AfTransition AfTransition;
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

  GObject *object;
  GObject *child;
  GArray *properties;
};

struct AfAnimator
{
  AfTimeline *timeline;
  GPtrArray *transitions;
};

static AfTransition *
af_transition_new (GObject *object,
                   GObject *child,
                   gdouble  from,
                   gdouble  to);
{
  AfTransition *transition;

  transition = g_slice_new0 (AfTransition);
  transition->object = g_object_ref (object);
  transition->from = from;
  transition->to = to;
  transition->properties = g_array_new (FALSE, FALSE,
                                       sizeof (AfPropertyRange));

  if (child)
    transition->child = g_object_ref (child);

  return transition;
}

static void
af_transition_free (AfTransition *transition)
{
  g_object_unref (transition->object);

  if (transition->child)
    g_object_unref (transition->child);

  for (i = 0; i < transition->properties->len; i++)
    {
      AfPropertyRange *property_range;

      property_range = &g_array_index (transition->properties, AfPropertyRange, i);
      g_param_spec_unref (property_range->pspec);
    }

  g_slice_free (AfTransition, transition);
}

static AfAnimator *
af_animator_new (void)
{
  AfAnimator *animator;

  animator = g_slice_new0 (AfAnimator);
  animator->transitions = g_ptr_array_new ();

  return animator;
}

static void
af_animator_free (AfAnimator *animator)
{
  guint i;

  if (animator->timeline)
    {
      af_timeline_stop (animator->timeline);
      g_object_unref (animator->timeline);
    }

  for (i = 0; i < animator->transitions->len; i++)
    {
      AfTransition *transition;

      transition = g_ptr_array_index (animator->transitions, i);
      af_transition_free (transition);
    }

  g_ptr_array_free (animator->transitions, TRUE);
  g_slice_free (AfAnimator, animator);
}

static void
af_transition_set_progress (AfTransition *transition,
                           gdouble        progress)
{
  GValue value = { 0, };
  GArray *properties;
  guint i;

  properties = transition->properties;

  for (i = 0; i < properties->len; i++)
    {
      AfPropertyRange *property_range;
      gboolean handled = FALSE;
      GType type;

      property_range = &g_array_index (properties, AfPropertyRange, i);
      g_value_init (&value, property_range->pspec->value_type);
      type = property_range->pspec->value_type;

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
          {
            AfTypeTransformationFunc func = NULL;

            if (transformable_types)
              func = g_hash_table_lookup (transformable_types, GSIZE_TO_POINTER (type));

            if (!func)
              g_warning ("Property of type '%s' not handled", g_type_name (type));
            else
              {
                (func) (&property_range->from,
                        &property_range->to,
                        progress,
                        &value);
                handled = TRUE;
              }
          }
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

  animator = (AfAnimator *) user_data;
}

static gboolean
transition_add_property (AfTransition *transition,
                         const gchar  *property_name,
                         GValue       *to)
{
  AfPropertyRange property_range = { 0, };
  GParamSpec *pspec;

  if (!transition->child)
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (transition->object),
                                          property_name);
  else
    pspec = gtk_container_class_find_child_property (G_OBJECT_GET_CLASS (transition->object),
                                                     property_name);

  if (G_UNLIKELY (!pspec))
    {
      g_warning ("Property '%s' does not exist on object of class '%s'",
                 property_name, G_OBJECT_TYPE_NAME (transition->object));
      return FALSE;
    }

  if (G_VALUE_TYPE (to) != pspec->value_type)
    {
      g_warning ("Target value type (%s) do not match property type (%s)",
                 G_VALUE_TYPE_NAME (to),
                 g_type_name (pspec->value_type));
      return FALSE;
    }

  property_range.pspec = g_param_spec_ref (pspec);

  g_value_init (&property_range.from, pspec->value_type);

  if (!transition->child)
    g_object_get_property (transition->object, pspec->name, &property_range.from);
  else
    gtk_container_child_get_property (GTK_CONTAINER (transition->object),
                                      GTK_WIDGET (transition->child),
                                      pspec->name, &property_range.from);

  g_value_init (&property_range.to, pspec->value_type);
  g_value_copy (to, &property_range.to);

  g_array_append_val (transition->properties, property_range);

  return TRUE;
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
af_animator_add_transition_valist (guint    anim_id,
                                   gdouble  from,
                                   gdouble  to,
                                   GObject *object,
                                   va_list  args)
{
  AfAnimator *animator;
  AfTransition *transition;
  const gchar *property_name;

  g_return_val_if_fail (animators != NULL);
  g_return_val_if_fail (anim_id != 0, FALSE);
  g_return_val_if_fail (from >= 0.0 && from <= 1.0, FALSE);
  g_return_val_if_fail (to >= 0.0 && to <= 1.0, FALSE);
  g_return_val_if_fail (from <= to, FALSE);
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (anim_id));

  g_return_val_if_fail (animator != NULL);

  transition = af_transition_new (object, NULL, from, to);

  /* FIXME: Add property ranges */

  g_ptr_array_add (animator->transitions, transition);

  return TRUE;
}

gboolean
af_animator_add_transition (guint    anim_id,
                            gdouble  from,
                            gdouble  to,
                            GObject *object,
                            ...)
{
  gboolean result;
  va_list args;

  va_start (args, object);
  result = af_animator_add_transition_valist (anim_id, from, to, object, args);
  va_end (args);

  return result;
}

gboolean
af_animator_add_child_transition_valist (guint         anim_id,
                                         gdouble       from,
                                         gdouble       to,
                                         GtkContainer *container,
                                         GtkWidget    *child,
                                         va_list       args)
{
  AfAnimator *animator;
  AfTransition *transition;

  g_return_val_if_fail (animators != NULL);
  g_return_val_if_fail (anim_id != 0, FALSE);
  g_return_val_if_fail (from >= 0.0 && from <= 1.0, FALSE);
  g_return_val_if_fail (to >= 0.0 && to <= 1.0, FALSE);
  g_return_val_if_fail (from <= to, FALSE);
  g_return_val_if_fail (GTK_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (GTK_IS_WIDGET (child), FALSE);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (anim_id));

  g_return_val_if_fail (animator != NULL);

  transition = af_transition_new (container, child, from, to);

  /* FIXME: Add property ranges */

  return TRUE;
}

gboolean
af_animator_add_child_transition (guint         anim_id,
                                  gdouble       from,
                                  gdouble       to,
                                  GtkContainer *container,
                                  GtkWidget    *child,
                                  ...)
{
  gboolean result;
  va_list args;

  va_start (args, child);
  result = af_animator_add_child_transition_valist (anim_id, from, to,
                                                    container, child, args);
  va_end (args);

  return result;
}

static guint
af_animator_tween_property_internal (gpointer     object,
                                     gpointer     child,
                                     const gchar *property_name,
                                     GValue      *to)
{
  AfAnimator *animator = NULL;
  guint id;

  if (G_UNLIKELY (!animators))
    animators = g_hash_table_new (g_direct_hash, g_direct_equal);

  animator = af_animator_simple_new (object, child);

  if (!animator_add_tween_property ((AfAnimatorSimple *) animator,
                                    property_name, to))
    {
      af_animator_free (animator);
      return 0;
    }

  id = ++id_count;

  g_hash_table_insert (animators,
                       GUINT_TO_POINTER (id),
                       animator);
  return id;
}

guint
af_animator_tween_property (gpointer     object,
                            const gchar *property_name,
                            GValue      *to)
{
  g_return_val_if_fail (G_IS_OBJECT (object), 0);
  g_return_val_if_fail (property_name != NULL, 0);
  g_return_val_if_fail (to != NULL, 0);

  return af_animator_tween_property_internal (object, NULL,
                                              property_name,
                                              to);
}

guint
af_animator_tween_child_property (GtkContainer *container,
                                  GtkWidget    *child,
                                  const gchar  *property_name,
                                  GValue       *to)
{
  g_return_val_if_fail (GTK_IS_CONTAINER (container), 0);
  g_return_val_if_fail (GTK_IS_WIDGET (child), 0);
  g_return_val_if_fail (property_name != NULL, 0);
  g_return_val_if_fail (to != NULL, 0);

  return af_animator_tween_property_internal (container, child,
                                              property_name,
                                              to);
}

void
af_animator_add_tween_property (guint        id,
                                const gchar *property_name,
                                GValue      *to)
{
  AfAnimator *animator;

  g_return_if_fail (animators != NULL);
  g_return_if_fail (to != NULL);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  /* FIXME: Improve error handling here */
  g_return_if_fail (animator != NULL);
  g_return_if_fail (animator->type == ANIMATOR_TYPE_SIMPLE);

  animator_add_tween_property ((AfAnimatorSimple *) animator,
                               property_name, to);
}

static guint
af_animator_tween_valist_internal (gpointer object,
                                   gpointer child,
                                   va_list  args)
{
  const gchar *property_name;
  guint id = 0;

  property_name = va_arg (args, const gchar *);

  while (property_name)
    {
      GParamSpec *pspec;
      GValue to = { 0, };
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

      if (id == 0)
        {
          if (!child)
            id = af_animator_tween_property (object, property_name, &to);
          else
            id = af_animator_tween_child_property (object, child,
                                                   property_name,
                                                   &to);
        }
      else
        af_animator_add_tween_property (id,
                                        property_name,
                                        &to);

      g_value_unset (&to);

      property_name = va_arg (args, const gchar *);
    }

  return id;
}

guint
af_animator_tween_valist (gpointer object,
                          va_list  args)
{
  g_return_val_if_fail (G_IS_OBJECT (object), 0);
  g_return_val_if_fail (args != NULL, 0);

  return af_animator_tween_valist_internal (object, NULL, args);
}

guint
af_animator_child_tween_valist (GtkContainer *container,
                                GtkWidget    *child,
                                va_list       args)
{
  g_return_val_if_fail (GTK_IS_CONTAINER (container), 0);
  g_return_val_if_fail (GTK_IS_WIDGET (child), 0);
  g_return_val_if_fail (args != NULL, 0);

  return af_animator_tween_valist_internal (container, child, args);
}

guint
af_animator_tween (gpointer     object,
                   ...)
{
  guint id;
  va_list args;

  g_return_val_if_fail (G_IS_OBJECT (object), 0);

  va_start (args, object);
  id = af_animator_tween_valist (object, args);
  va_end (args);

  return id;
}

guint
af_animator_child_tween (GtkContainer *container,
                         GtkWidget    *child,
                         ...)
{
  guint id;
  va_list args;

  g_return_val_if_fail (GTK_IS_CONTAINER (container), 0);
  g_return_val_if_fail (GTK_IS_WIDGET (child), 0);

  va_start (args, child);
  id = af_animator_child_tween_valist (container, child, args);
  va_end (args);

  return id;
}

gboolean
af_animator_start (guint id,
                   guint duration)
{
  AfAnimator *animator;

  g_return_val_if_fail (animators != NULL, FALSE);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  g_return_val_if_fail (animator != NULL, FALSE);
  g_return_val_if_fail (animator->timeline != NULL, FALSE);

  animator->timeline = af_timeline_new (duration);

  g_signal_connect (animator->timeline, "frame",
                    G_CALLBACK (animator_frame_cb), animator);

  af_timeline_start (animator->timeline);

  return TRUE;
}

void
af_animator_pause (guint id)
{
  AfAnimator *animator;

  g_return_if_fail (animators != NULL);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  g_return_if_fail (animator != NULL);

  af_timeline_pause (animator->timeline);
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
af_animator_remove (guint id)
{
  g_return_if_fail (animators != NULL);

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
