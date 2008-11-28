#ifndef __MY_VBOX_H__
#define __MY_VBOX_H__

/* myvbox_a.h
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

#define MY_TYPE_VBOX		 (my_vbox_get_type ())
#define MY_VBOX(obj)		 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_VBOX, MyVBox))
#define MY_VBOX_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), MY_TYPE_VBOX, MyVBoxClass))
#define MY_IS_VBOX(obj)	         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_VBOX))
#define MY_IS_VBOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MY_TYPE_VBOX))
#define MY_VBOX_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_VBOX, MyVBoxClass))


typedef struct _MyVBox	     MyVBox;
typedef struct _MyVBoxClass  MyVBoxClass;
typedef enum   MyVBoxAnimationStyle MyVBoxAnimationStyle;

struct _MyVBox
{
  GtkBox box;
};

struct _MyVBoxClass
{
  GtkBoxClass parent_class;
};

enum MyVBoxAnimationStyle
{
  MY_VBOX_ANIMATION_STYLE_MOVE,
  MY_VBOX_ANIMATION_STYLE_RESIZE
};

GType       my_vbox_get_type (void) G_GNUC_CONST;

GtkWidget * my_vbox_new          (gboolean homogeneous,
                                  gint     spacing);

void        my_vbox_pack_start_n (GtkBox    *box,
		                  GtkWidget *child,
				  gboolean   expand,
				  gboolean   fill,
				  guint      padding,
				  gint       position);

MyVBoxAnimationStyle my_vbox_get_animation_style (MyVBox               *vbox);
void                 my_vbox_set_animation_style (MyVBox               *vbox,
		                                  MyVBoxAnimationStyle  style);


G_END_DECLS

#endif /* __MY_VBOX_H__ */
