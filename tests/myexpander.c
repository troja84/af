/* vim: set tabstop=2:softtabstop=2:shiftwidth=2:noexpandtab */

/* myexpander.c
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
#include <af/af-timeline.h>
#include <math.h>
#include "myexpander.h"

typedef struct MyExpanderPriv MyExpanderPriv;
typedef struct MyAllocatedWidget MyAllocatedWidget;

struct MyAllocatedWidget
{
  GtkWidget *widget;

  GtkAllocation allocation, allocation_mod;
};

struct MyExpanderPriv
{
  AfTimeline *timeline;
  gdouble progress;

  MyAllocatedWidget alloc;
	MyExpanderAnimationStyle animation_style;

  gboolean expanded;
};

struct _GtkExpanderPrivate
{
  GtkWidget        *label_widget;
  GdkWindow        *event_window;
  gint              spacing;

  GtkExpanderStyle  expander_style;
  guint             animation_timeout;
  guint             expand_timer;

  guint             expanded : 1;
  guint             use_underline : 1;
  guint             use_markup : 1; 
  guint             button_down : 1;
  guint             prelight : 1;
};

static void my_expander_size_allocate           (GtkWidget      *widget,
                                                 GtkAllocation  *allocation);

static gint my_expander_expose									(GtkWidget			*widget,
																								 GdkEventExpose	*event);

/* ANIMATION*/
static void my_expander_handle_animation        (MyExpander     *expander);
static void my_expander_animation_frame_cb      (AfTimeline     *timeline,
		                                 gdouble         progress,
		                                 gpointer        user_data);
static void my_expander_animation_finished_cb   (AfTimeline     *timeline,
                                                 gpointer        user_data);

#define MY_EXPANDER_GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MY_TYPE_EXPANDER, MyExpanderPriv))

G_DEFINE_TYPE (MyExpander, my_expander, GTK_TYPE_EXPANDER)

static void
my_expander_class_init (MyExpanderClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  widget_class->size_allocate = my_expander_size_allocate;
	widget_class->expose_event = my_expander_expose;

  g_type_class_add_private (class, sizeof (MyExpanderPriv));
}

static void
my_expander_init (MyExpander *expander)
{
  MyExpanderPriv *priv;

  priv = MY_EXPANDER_GET_PRIV (expander);

	priv->animation_style = MY_EXPANDER_ANIMATION_STYLE_MOVE;

  priv->expanded = FALSE;
}

static void
get_expander_bounds (GtkExpander  *expander,
										 GdkRectangle *rect)
{
  GtkWidget *widget;
  GtkExpanderPrivate *priv;
  gint border_width;
  gint expander_size;
  gint expander_spacing;
  gboolean interior_focus;
  gint focus_width;
  gint focus_pad;
  gboolean ltr;

  widget = GTK_WIDGET (expander);
  priv = expander->priv;

  border_width = GTK_CONTAINER (expander)->border_width;

  gtk_widget_style_get (widget,
			"interior-focus", &interior_focus,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"expander-size", &expander_size,
			"expander-spacing", &expander_spacing,
			NULL);

  ltr = gtk_widget_get_direction (widget) != GTK_TEXT_DIR_RTL;

  rect->x = widget->allocation.x + border_width;
  rect->y = widget->allocation.y + border_width;

  if (ltr)
    rect->x += expander_spacing;
  else
    rect->x += widget->allocation.width - 2 * border_width -
               expander_spacing - expander_size;

  if (priv->label_widget && GTK_WIDGET_VISIBLE (priv->label_widget))
    {
      GtkAllocation label_allocation;

      label_allocation = priv->label_widget->allocation;

      if (expander_size < label_allocation.height)
	rect->y += focus_width + focus_pad + (label_allocation.height - expander_size) / 2;
      else
	rect->y += expander_spacing;
    }
  else
    {
      rect->y += expander_spacing;
    }

  if (!interior_focus)
    {
      if (ltr)
	rect->x += focus_width + focus_pad;
      else
	rect->x -= focus_width + focus_pad;
      rect->y += focus_width + focus_pad;
    }

  rect->width = rect->height = expander_size;
}

static void
my_expander_size_allocate (GtkWidget     *widget,
													 GtkAllocation *allocation)
{
  GtkExpander *expander;
  GtkBin *bin;
  GtkExpanderPrivate *priv;
	MyExpanderPriv *private;
  GtkRequisition child_requisition;
  gboolean child_visible = FALSE;
  gint border_width;
  gint expander_size;
  gint expander_spacing;
  gboolean interior_focus;
  gint focus_width;
  gint focus_pad;
  gint label_height;
  GtkWidget *label;

  expander = GTK_EXPANDER (widget);
  bin = GTK_BIN (widget);
  priv = expander->priv;

	private = MY_EXPANDER_GET_PRIV (MY_EXPANDER (expander));

  border_width = GTK_CONTAINER (widget)->border_width;

  gtk_widget_style_get (widget,
			"interior-focus", &interior_focus,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"expander-size", &expander_size,
			"expander-spacing", &expander_spacing,
			NULL);

  child_requisition.width = 0;
  child_requisition.height = 0;
  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      child_visible = TRUE;
      gtk_widget_get_child_requisition (bin->child, &child_requisition);
    }

  widget->allocation = *allocation;

  label = gtk_expander_get_label_widget (expander);
  if (label && GTK_WIDGET_VISIBLE (label))
    {
      GtkAllocation label_allocation;
      GtkRequisition label_requisition;
      gboolean ltr;

      gtk_widget_get_child_requisition (label, &label_requisition);

      ltr = gtk_widget_get_direction (widget) != GTK_TEXT_DIR_RTL;

      if (ltr)
	label_allocation.x = (widget->allocation.x +
                              border_width + focus_width + focus_pad +
                              expander_size + 2 * expander_spacing);
      else
        label_allocation.x = (widget->allocation.x + widget->allocation.width -
                              (label_requisition.width +
                               border_width + focus_width + focus_pad +
                               expander_size + 2 * expander_spacing));

      label_allocation.y = widget->allocation.y + border_width + focus_width + focus_pad;

      label_allocation.width = MIN (label_requisition.width,
				    allocation->width - 2 * border_width -
				    expander_size - 2 * expander_spacing -
				    2 * focus_width - 2 * focus_pad);
      label_allocation.width = MAX (label_allocation.width, 1);

      label_allocation.height = MIN (label_requisition.height,
				     allocation->height - 2 * border_width -
				     2 * focus_width - 2 * focus_pad -
				     (child_visible ? gtk_expander_get_spacing (expander) : 0));
      label_allocation.height = MAX (label_allocation.height, 1);

      gtk_widget_size_allocate (label, &label_allocation);

      label_height = label_allocation.height;
    }
  else
    {
      label_height = 0;
    }

  if (GTK_WIDGET_REALIZED (widget))
    {
      GdkRectangle rect;

      get_expander_bounds (expander, &rect);

      gdk_window_move_resize (priv->event_window,
			      allocation->x + border_width, 
			      allocation->y + border_width, 
			      MAX (allocation->width - 2 * border_width, 1), 
			      MAX (rect.height, label_height - 2 * border_width));
    }

  if (child_visible)
    {
      GtkAllocation child_allocation;
      gint top_height;

      top_height = MAX (2 * expander_spacing + expander_size,
			label_height +
			(interior_focus ? 2 * focus_width + 2 * focus_pad : 0));

      child_allocation.x = widget->allocation.x + border_width;
      child_allocation.y = widget->allocation.y + border_width + top_height + gtk_expander_get_spacing (expander);

      if (!interior_focus)
	child_allocation.y += 2 * focus_width + 2 * focus_pad;

      child_allocation.width = MAX (allocation->width - 2 * border_width, 1);

      child_allocation.height = allocation->height - top_height -
				2 * border_width - gtk_expander_get_spacing (expander) -
				(!interior_focus ? 2 * focus_width + 2 * focus_pad : 0);
      child_allocation.height = MAX (child_allocation.height, 1);
			
			private->alloc.widget = bin->child;
			private->alloc.allocation_mod.width = bin->child->allocation.width;
			private->alloc.allocation_mod.height = bin->child->allocation.height;
			private->alloc.allocation_mod.x = bin->child->allocation.x;
			private->alloc.allocation_mod.y = bin->child->allocation.y;

			private->alloc.allocation.width = child_allocation.width;
			private->alloc.allocation.height = child_allocation.height;
			private->alloc.allocation.x = child_allocation.x;
			private->alloc.allocation.y = child_allocation.y;

      my_expander_handle_animation (MY_EXPANDER (widget));
    }
}

static void
my_expander_expose_child (GtkWidget *child,
													gpointer   client_data)
{
  struct {
    GtkWidget *container;
    GdkEventExpose *event;
  } *data = client_data;
  
  gtk_container_propagate_expose (GTK_CONTAINER (data->container),
																 child,
																 data->event);
}

static gint 
my_expander_expose_ext (GtkWidget      *widget,
												GdkEventExpose *event)
{
	MyExpanderPriv *priv = MY_EXPANDER_GET_PRIV (MY_EXPANDER (widget));

  struct {
    GtkWidget *container;
    GdkEventExpose *event;
  } data;

  g_return_val_if_fail (GTK_IS_CONTAINER (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  
  if (GTK_WIDGET_DRAWABLE (widget)) 
    {
			GdkRectangle rect;
			MyAllocatedWidget *alloc = &priv->alloc;

			if (priv->expanded && !gtk_expander_get_expanded (GTK_EXPANDER (widget)))
				{
					rect.x = alloc->allocation_mod.x;
					rect.y = alloc->allocation_mod.y;
					rect.width = alloc->allocation_mod.width;
					rect.height = alloc->allocation_mod.height;
				}
			else
				{
					rect.x = alloc->allocation.x;
					rect.y = alloc->allocation.y;
					rect.width = alloc->allocation.width;
					rect.height = alloc->allocation.height;
				}

      data.container = widget;
      data.event = event;

			data.event->region = gdk_region_rectangle (&rect);
      
      gtk_container_forall (GTK_CONTAINER (widget),
			    my_expander_expose_child,
			    &data);
    }   
  
  return FALSE;
}

static void
gtk_expander_paint_prelight (GtkExpander *expander)
{
  GtkWidget *widget;
  GtkContainer *container;
  GtkExpanderPrivate *priv;
  GdkRectangle area;
  gboolean interior_focus;
  int focus_width;
  int focus_pad;
  int expander_size;
  int expander_spacing;

  priv = expander->priv;
  widget = GTK_WIDGET (expander);
  container = GTK_CONTAINER (expander);

  gtk_widget_style_get (widget,
			"interior-focus", &interior_focus,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"expander-size", &expander_size,
			"expander-spacing", &expander_spacing,
			NULL);

  area.x = widget->allocation.x + container->border_width;
  area.y = widget->allocation.y + container->border_width;
  area.width = widget->allocation.width - (2 * container->border_width);

  if (priv->label_widget && GTK_WIDGET_VISIBLE (priv->label_widget))
    area.height = priv->label_widget->allocation.height;
  else
    area.height = 0;

  area.height += interior_focus ? (focus_width + focus_pad) * 2 : 0;
  area.height = MAX (area.height, expander_size + 2 * expander_spacing);
  area.height += !interior_focus ? (focus_width + focus_pad) * 2 : 0;

  gtk_paint_flat_box (widget->style, widget->window,
		      GTK_STATE_PRELIGHT,
		      GTK_SHADOW_ETCHED_OUT,
		      &area, widget, "expander",
		      area.x, area.y,
		      area.width, area.height);
}

static void
gtk_expander_paint (GtkExpander *expander)
{
  GtkWidget *widget;
  GdkRectangle clip;
  GtkStateType state;

  widget = GTK_WIDGET (expander);

  get_expander_bounds (expander, &clip);

  state = widget->state;
  if (expander->priv->prelight)
    {
      state = GTK_STATE_PRELIGHT;

      gtk_expander_paint_prelight (expander);
    }

  gtk_paint_expander (widget->style,
		      widget->window,
		      state,
		      &clip,
		      widget,
		      "expander",
		      clip.x + clip.width / 2,
		      clip.y + clip.height / 2,
		      expander->priv->expander_style);
}

static void
gtk_expander_paint_focus (GtkExpander  *expander,
			  GdkRectangle *area)
{
  GtkWidget *widget;
  GtkExpanderPrivate *priv;
  GdkRectangle rect;
  gint x, y, width, height;
  gboolean interior_focus;
  gint border_width;
  gint focus_width;
  gint focus_pad;
  gint expander_size;
  gint expander_spacing;
  gboolean ltr;

  widget = GTK_WIDGET (expander);
  priv = expander->priv;

  border_width = GTK_CONTAINER (widget)->border_width;

  gtk_widget_style_get (widget,
			"interior-focus", &interior_focus,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"expander-size", &expander_size,
			"expander-spacing", &expander_spacing,
			NULL);

  ltr = gtk_widget_get_direction (widget) != GTK_TEXT_DIR_RTL;
  
  width = height = 0;

  if (priv->label_widget)
    {
      if (GTK_WIDGET_VISIBLE (priv->label_widget))
	{
	  GtkAllocation label_allocation = priv->label_widget->allocation;

	  width  = label_allocation.width;
	  height = label_allocation.height;
	}

      width  += 2 * focus_pad + 2 * focus_width;
      height += 2 * focus_pad + 2 * focus_width;

      x = widget->allocation.x + border_width;
      y = widget->allocation.y + border_width;

      if (ltr)
	{
	  if (interior_focus)
	    x += expander_spacing * 2 + expander_size;
	}
      else
	{
	  x += widget->allocation.width - 2 * border_width
	    - expander_spacing * 2 - expander_size - width;
	}

      if (!interior_focus)
	{
	  width += expander_size + 2 * expander_spacing;
	  height = MAX (height, expander_size + 2 * expander_spacing);
	}
    }
  else
    {
      get_expander_bounds (expander, &rect);

      x = rect.x - focus_pad;
      y = rect.y - focus_pad;
      width = rect.width + 2 * focus_pad;
      height = rect.height + 2 * focus_pad;
    }
      
  gtk_paint_focus (widget->style, widget->window, GTK_WIDGET_STATE (widget),
		   area, widget, "expander",
		   x, y, width, height);
}

static gboolean
my_expander_expose (GtkWidget      *widget,
										GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GtkExpander *expander = GTK_EXPANDER (widget);

      gtk_expander_paint (expander);

      if (GTK_WIDGET_HAS_FOCUS (expander))
				gtk_expander_paint_focus (expander, &event->area);

      my_expander_expose_ext (widget, event);
    }

  return FALSE;
}

GtkWidget *
my_expander_new (const gchar *label)
{
  return g_object_new (MY_TYPE_EXPANDER, "label", label, NULL);
}

MyExpanderAnimationStyle
my_expander_get_animation_style (MyExpander *expander)
{
  MyExpanderPriv *priv = MY_EXPANDER_GET_PRIV (expander);

	return priv->animation_style;
}

void
my_expander_set_animation_style (MyExpander *expander,
																 MyExpanderAnimationStyle style)
{
  MyExpanderPriv *priv = MY_EXPANDER_GET_PRIV (expander);

	priv->animation_style = style;
}

/* ANIMATION */
static void
my_expander_handle_animation (MyExpander *expander)
{
  GtkBin *bin;
  MyExpanderPriv *priv;

  bin = GTK_BIN (expander);

  if (gtk_bin_get_child (bin) == NULL)
    return;
      
  priv = MY_EXPANDER_GET_PRIV (expander);

  if (priv->expanded == gtk_expander_get_expanded (GTK_EXPANDER (expander)))
    {
      my_expander_animation_frame_cb (NULL, 1, expander);
			my_expander_animation_finished_cb (NULL, expander);
      return;
    }

  if (!priv->timeline)
    {
      priv->timeline = af_timeline_new (650);

      g_signal_connect (priv->timeline, "frame",
		        G_CALLBACK (my_expander_animation_frame_cb), expander);
      
      g_signal_connect (priv->timeline, "finished",
		        G_CALLBACK (my_expander_animation_finished_cb), 
			expander);

      af_timeline_start (priv->timeline);
    }
  else
    {
      af_timeline_start (priv->timeline);
    }
}

static void
my_expander_animation_frame_cb (AfTimeline *timeline,
																gdouble     progress,
																gpointer    user_data)
{
  MyExpander *expander = (MyExpander *) user_data;
  MyExpanderPriv *priv = MY_EXPANDER_GET_PRIV (expander);
  MyAllocatedWidget *alloc = &priv->alloc;
  GtkAllocation allocation;

  allocation.width = alloc->allocation.width;
  allocation.x = alloc->allocation.x;

	gtk_widget_set_child_visible (GTK_BIN (expander)->child, TRUE);

	if (priv->animation_style == MY_EXPANDER_ANIMATION_STYLE_MOVE)
		{
			GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (alloc->widget));
			GdkEventExpose expose;
			GdkRegion *region;
			GdkRectangle rect;

			rect.x = alloc->allocation.x;
			rect.width = alloc->allocation.width;

  		if (gtk_expander_get_expanded (GTK_EXPANDER (expander)))
				{
					rect.y = allocation.y = alloc->allocation.y - (1 - progress)
												 					* alloc->allocation.height;
					rect.height = allocation.height = alloc->allocation.height;
				}
			else
				{
					rect.y = allocation.y = alloc->allocation_mod.y - progress
												 * alloc->allocation_mod.height;
					rect.height = allocation.height = alloc->allocation_mod.height;
				}

			region = gdk_region_rectangle (&rect);
			
			rect.y = alloc->allocation.y;
			rect.height = alloc->allocation.height;

			gtk_widget_size_allocate (alloc->widget, &allocation);

			expose.type = GDK_EXPOSE;
			expose.window = window;
			expose.send_event = TRUE;
			expose.area = rect;
			expose.region = region;
			expose.count = 0;

			gtk_container_propagate_expose (GTK_CONTAINER (expander),
																			alloc->widget,
																			&expose);
		}
	else if (priv->animation_style == MY_EXPANDER_ANIMATION_STYLE_RESIZE)
		{
			allocation.y = alloc->allocation.y;

  		if (gtk_expander_get_expanded (GTK_EXPANDER (expander)))
				allocation.height = alloc->allocation.height * progress;
  		else
				allocation.height = alloc->allocation.height * (1 - progress);
	
			gtk_widget_size_allocate (alloc->widget, &allocation);
		}
}

static void
my_expander_animation_finished_cb (AfTimeline *timeline,
																	 gpointer    user_data)
{
  MyExpander *expander = (MyExpander *) user_data;
  MyExpanderPriv *priv = MY_EXPANDER_GET_PRIV (expander);

	if (timeline)
		{
  		af_timeline_pause (priv->timeline);
  		g_object_unref (priv->timeline);

  		priv->timeline = NULL;
		}

  priv->expanded = gtk_expander_get_expanded (GTK_EXPANDER (expander));

	if (priv->expanded)
		{
			gdk_window_invalidate_rect (gtk_widget_get_window (GTK_BIN (expander)->child), NULL, TRUE);
		}
	gtk_widget_set_child_visible (GTK_BIN (expander)->child, priv->expanded);
}

