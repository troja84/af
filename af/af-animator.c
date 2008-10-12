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

typedef struct AfPropertyRange AfPropertyRange;
typedef struct AfAnimator AfAnimator;

struct AfPropertyRange
{
  GParamSpec *pspec;
  GValue from;
  GValue to;
};

struct AfAnimator
{
  GObject *object;
  GArray *properties;
};

static void
animator_frame_cb (AfTimeline *timeline,
                   gdouble     progress,
                   gpointer    user_data)
{
  AfAnimator *animator;
  GValue value = { 0, };
  guint i;

  animator = (AfAnimator *) user_data;

  for (i = 0; i < animator->properties->len; i++)
    {
      AfPropertyRange *property_range;
      gboolean handled = FALSE;
      GType type;

      property_range = &g_array_index (animator->properties, AfPropertyRange, i);
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
        g_object_set_property (animator->object,
                               property_range->pspec->name,
                               &value);

      g_value_unset (&value);
    }
}

static gboolean
animator_add_tween_property (AfAnimator  *animator,
                             const gchar *property_name,
                             GValue      *from,
                             GValue      *to)
{
  AfPropertyRange property_range = { 0, };
  GParamSpec *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (animator->object),
                                        property_name);

  if (G_UNLIKELY (!pspec))
    {
      g_warning ("Property '%s' does not exist on object of class '%s'",
                 property_name, G_OBJECT_TYPE_NAME (animator->object));
      return FALSE;
    }

  if (G_VALUE_TYPE (from) != pspec->value_type ||
      G_VALUE_TYPE (to) != pspec->value_type)
    {
      g_warning ("Value types (%s, %s) do not match property type (%s)",
                 G_VALUE_TYPE_NAME (from),
                 G_VALUE_TYPE_NAME (to),
                 g_type_name (pspec->value_type));
      return FALSE;
    }

  property_range.pspec = g_param_spec_ref (pspec);

  g_value_init (&property_range.from, pspec->value_type);
  g_value_copy (from, &property_range.from);

  g_value_init (&property_range.to, pspec->value_type);
  g_value_copy (to, &property_range.to);

  g_array_append_val (animator->properties, property_range);

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
af_animator_tween_property (gpointer     object,
                            guint        duration,
                            const gchar *property_name,
                            GValue      *from,
                            GValue      *to)
{
  AfTimeline *timeline;
  AfAnimator *animator = NULL;
  static guint id_count = 0;
  guint id;

  g_return_val_if_fail (G_IS_OBJECT (object), 0);
  g_return_val_if_fail (property_name != NULL, 0);
  g_return_val_if_fail (from != NULL, 0);
  g_return_val_if_fail (to != NULL, 0);

  if (G_UNLIKELY (!animators))
    animators = g_hash_table_new (g_direct_hash, g_direct_equal);

  animator = g_slice_new0 (AfAnimator);
  animator->object = g_object_ref (object);
  animator->properties = g_array_new (FALSE, FALSE, sizeof (AfPropertyRange));

  if (!animator_add_tween_property (animator, property_name, from, to))
    {
      /* FIXME: Free animator */
      return 0;
    }

  id = ++id_count;

  g_hash_table_insert (animators,
                       GUINT_TO_POINTER (id),
                       animator);

  timeline = af_timeline_new (duration);
  g_signal_connect (timeline, "frame",
                    G_CALLBACK (animator_frame_cb), animator);

  af_timeline_start (timeline);

  return id;
}

void
af_animator_add_tween_property (guint        id,
                                const gchar *property_name,
                                GValue      *from,
                                GValue      *to)
{
  AfAnimator *animator;

  g_return_if_fail (animators != NULL);
  g_return_if_fail (from != NULL);
  g_return_if_fail (to != NULL);

  animator = g_hash_table_lookup (animators, GUINT_TO_POINTER (id));

  g_return_if_fail (animator != NULL);

  animator_add_tween_property (animator, property_name, from, to);
}

guint
af_animator_tween_valist (gpointer object,
                          guint    duration,
                          va_list  args)
{
  const gchar *property_name;
  guint id = 0;

  g_return_val_if_fail (G_IS_OBJECT (object), 0);
  g_return_val_if_fail (args != NULL, 0);

  property_name = va_arg (args, const gchar *);

  while (property_name)
    {
      GParamSpec *pspec;
      GValue from = { 0, };
      GValue to = { 0, };
      gchar *error = NULL;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                            property_name);

      if (G_UNLIKELY (!pspec))
        {
          g_warning ("Property '%s' does not exist on object of class '%s'",
                     property_name, G_OBJECT_TYPE_NAME (object));
          break;
        }

      g_value_init (&from, pspec->value_type);
      G_VALUE_COLLECT (&from, args, 0, &error);

      if (error)
        {
          g_warning (error);
          g_free (error);
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
        id = af_animator_tween_property (object, duration,
                                         property_name,
                                         &from, &to);
      else
        af_animator_add_tween_property (id,
                                        property_name,
                                        &from, &to);

      g_value_unset (&from);
      g_value_unset (&to);

      property_name = va_arg (args, const gchar *);
    }

  return id;
}

guint
af_animator_tween (gpointer object,
                   guint    duration,
                   ...)
{
  guint id;
  va_list args;

  g_return_val_if_fail (G_IS_OBJECT (object), 0);

  va_start (args, duration);
  id = af_animator_tween_valist (object, duration, args);
  va_end (args);

  return id;
}