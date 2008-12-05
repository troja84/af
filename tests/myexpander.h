/* vim: set tabstop=2:softtabstop=2:shiftwidth=2:noexpandtab */

/* myexpander.h
 *
 * Copyright (C) 2008  Hagen Schink <hagen@imendio.com> 
 *
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * Lesser General Public License for more details. 
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA 02110, USA
 */

#ifndef __MY_EXPANDER_H__
#define __MY_EXPANDER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MY_TYPE_EXPANDER            (my_expander_get_type ())
#define MY_EXPANDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_EXPANDER, MyExpander))
#define MY_EXPANDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MY_TYPE_EXPANDER, MyExpanderClass))
#define MY_IS_EXPANDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_EXPANDER))
#define MY_IS_EXPANDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MY_TYPE_EXPANDER))
#define MY_EXPANDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_EXPANDER, MyExpanderClass))

typedef struct _MyExpander        MyExpander;
typedef struct _MyExpanderClass   MyExpanderClass;
typedef enum   MyExpanderAnimationStyle MyExpanderAnimationStyle;

struct _MyExpander
{
  GtkExpander expander;
};

struct _MyExpanderClass
{
  GtkExpanderClass    parent_class;
};

enum MyExpanderAnimationStyle
{
  MY_EXPANDER_ANIMATION_STYLE_MOVE,
  MY_EXPANDER_ANIMATION_STYLE_RESIZE
};

GType                 my_expander_get_type          (void) G_GNUC_CONST;

GtkWidget            *my_expander_new               (const gchar *label);

MyExpanderAnimationStyle my_expander_get_animation_style (MyExpander *expander);
void                     my_expander_set_animation_style (MyExpander *expander,
																																 MyExpanderAnimationStyle style);

G_END_DECLS

#endif /* __MY_EXPANDER_H__ */
