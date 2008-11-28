/* mywidgetslider.h
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

#ifndef __MY_SLIDER_H__
#define __MY_SLIDER_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#define MY_TYPE_SLIDER                        (my_slider_get_type ())
#define MY_SLIDER(obj)                        (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_SLIDER, MySlider))
#define MY_IS_SLIDER(obj)                     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_SLIDER))
#define MY_SLIDER_CLASS(obj)                  (G_TYPE_CHECK_CLASS_CAST ((obj), MY_TYPE_SLIDER,  MySliderClass))
#define MY_IS_SLIDER_CLASS(obj)               (G_TYPE_CHECK_CLASS_TYPE ((obj), MY_TYPE_SLIDER))
#define MY_SLIDER_GET_CLASS                   (G_TYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_SLIDER, MySliderClass))

typedef struct _MySlider      MySlider;
typedef struct _MySliderClass MySliderClass;

struct _MySlider
{
  GtkBox parent_instance;
};

struct _MySliderClass
{
  GtkBoxClass parent_class;
};

GType my_slider_get_type (void);

GtkWidget *my_slider_new (void);

/*
 * Method definitions.
 */

#endif /* __MY_SLIDER_H__ */
