#ifndef __MY_BOX_H__
#define __MY_BOX_H__

/* mybox_a.h
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MY_TYPE_BOX		 (my_box_get_type ())
#define MY_BOX(obj)		 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_BOX, MyBox))
#define MY_BOX_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), MY_TYPE_BOX, MyBoxClass))
#define MY_IS_BOX(obj)	         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_BOX))
#define MY_IS_BOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MY_TYPE_BOX))
#define MY_BOX_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_BOX, MyBoxClass))


typedef struct _MyBox	    MyBox;
typedef struct _MyBoxClass  MyBoxClass;
typedef enum   MyBoxAnimationStyle MyBoxAnimationStyle;

struct _MyBox
{
  GtkBox box;
};

struct _MyBoxClass
{
  GtkBoxClass parent_class;
};

enum MyBoxAnimationStyle
{
  MY_BOX_ANIMATION_STYLE_MOVE,
  MY_BOX_ANIMATION_STYLE_RESIZE
};

GType       my_box_get_type (void) G_GNUC_CONST;

GtkWidget * my_box_new          (gboolean homogeneous,
                                 gint     spacing);

void        my_box_pack_start_n (GtkBox    *box,
		                 GtkWidget *child,
				 gboolean   expand,
				 gboolean   fill,
				 guint      padding,
				 gint       position);

MyBoxAnimationStyle my_box_get_animation_style (MyBox               *box);
void                my_box_set_animation_style (MyBox               *box,
		                                MyBoxAnimationStyle  style);

MyBoxAnimationStyle my_box_get_animation_style (MyBox               *vbox);
void                my_box_set_animation_style (MyBox               *vbox,
		                                MyBoxAnimationStyle  style);


G_END_DECLS

#endif /* __MY_BOX_H__ */
