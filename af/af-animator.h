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

G_BEGIN_DECLS

typedef void (*AfTypeTransformationFunc) (const GValue *from,
                                          const GValue *to,
                                          gdouble       progress,
                                          GValue       *out_value);

void  af_animator_register_type_transformation (GType                    type,
                                                AfTypeTransformationFunc trans_func);

guint af_animator_tween_property (gpointer     object,
                                  guint        duration,
                                  const gchar *property_name,
                                  GValue      *from,
                                  GValue      *to);

void  af_animator_add_tween_property (guint        id,
                                      const gchar *property_name,
                                      GValue      *from,
                                      GValue      *to);


guint af_animator_tween_valist (gpointer object,
                                guint    duration,
                                va_list  args);

guint af_animator_tween (gpointer object,
                         guint    duration,
                         ...);

G_END_DECLS

#endif /* __AF_ANIMATOR_H__ */
