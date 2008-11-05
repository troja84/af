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

#ifndef __AF_ANIMATOR_H__
#define __AF_ANIMATOR_H__

#include <glib.h>
#include <gtk/gtk.h>
#include "af-enums.h"

G_BEGIN_DECLS

typedef void (*AfTypeTransformationFunc) (const GValue *from,
                                          const GValue *to,
                                          gdouble       progress,
					  gpointer      user_data,
                                          GValue       *out_value);

void  af_animator_register_type_transformation (GType                    type,
                                                AfTypeTransformationFunc trans_func);

guint    af_animator_add                         (void);

gboolean af_animator_add_transition_valist       (guint                   anim_id,
                                                  gdouble                 from,
                                                  gdouble                 to,
                                                  AfTimelineProgressType  type,
                                                  GObject                *object,
                                                  va_list                 var_args);
gboolean af_animator_add_child_transition_valist (guint                   anim_id,
                                                  gdouble                 from,
                                                  gdouble                 to,
                                                  AfTimelineProgressType  type,
                                                  GtkContainer           *container,
                                                  GtkWidget              *child,
                                                  va_list                 var_args);

gboolean af_animator_add_transition              (guint                   anim_id,
                                                  gdouble                 from,
                                                  gdouble                 to,
                                                  AfTimelineProgressType  type,
                                                  GObject                *object,
                                                  ...);
gboolean af_animator_add_child_transition        (guint                   anim_id,
                                                  gdouble                 from,
                                                  gdouble                 to,
                                                  AfTimelineProgressType  type,
                                                  GtkContainer           *container,
                                                  GtkWidget              *child,
                                                  ...);

gboolean af_animator_start                       (guint         id,
                                                  guint         duration);

void     af_animator_remove                      (guint         id);
void     af_animator_pause                       (guint         id);
void     af_animator_resume                      (guint         id);
void     af_animator_reverse                     (guint         id);
void     af_animator_skip                        (guint         id,
		                                  gdouble       delta_progress);
void     af_animator_advance                     (guint         id,
		                                  gdouble       progress);

void     af_animator_set_loop                    (guint         id,
                                                  gboolean      loop);

/* Helper functions */
guint    af_animator_tween                       (GObject                *object,
                                                  guint                   duration,
                                                  AfTimelineProgressType  type,
						  gpointer                user_data,
		                                  GDestroyNotify          value_destroy_func,
                                                  ...);
guint    af_animator_child_tween                 (GtkContainer           *container,
                                                  GtkWidget              *child,
                                                  guint                   duration,
                                                  AfTimelineProgressType  type,
						  gpointer                user_data,
		                                  GDestroyNotify          value_destroy_func,
                                                  ...);

G_END_DECLS

#endif /* __AF_ANIMATOR_H__ */
