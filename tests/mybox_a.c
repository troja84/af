/* mybox_a.c
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
#include "mybox_a.h"

typedef struct MyBoxPriv MyBoxPriv;
typedef struct MyAllocatedWidget MyAllocatedWidget;
typedef enum   MyBoxAnimationType MyBoxAnimationType;

enum MyBoxAnimationType
{
  MY_BOX_ANIMATION_ADD = 1,
  MY_BOX_ANIMATION_REMOVE,
  MY_BOX_ANIMATION_ORIENTATION,
  MY_BOX_ANIMATION_ALLOCATE
};

struct MyAllocatedWidget
{
  GtkWidget *widget;

  GtkAllocation allocation, allocation_old;
  gboolean expand;
};

struct MyBoxPriv
{
  AfTimeline *timeline;
  gdouble progress;

  GHashTable *animation_widgets;
  MyBoxAnimationStyle animation_style;

  GList *children_pack_start;
  GList *children_pack_end;

  GtkOrientation orientation;
};

static void my_box_size_allocate           (GtkWidget      *widget,
                                            GtkAllocation  *allocation);
static void my_box_size_request            (GtkWidget      *widget,
                                            GtkRequisition *requisition);

static void my_box_add                     (GtkContainer   *container,
	                                    GtkWidget      *widget);	
static void my_box_remove                  (GtkContainer   *container,
                                            GtkWidget      *widget);

/* ANIMATION*/
static void my_box_handle_animation        (MyBox         *vbox);
static void my_box_animation_frame_cb      (AfTimeline    *timeline,
		                            gdouble        progress,
		                            gpointer       user_data);
static void my_box_remove_animated_widgets (MyBox         *vbox);
static void my_box_animation_finished_cb   (AfTimeline    *timeline,
                                            gpointer       user_data);

#define MY_BOX_GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MY_TYPE_BOX, MyBoxPriv))

G_DEFINE_TYPE (MyBox, my_box, GTK_TYPE_BOX)

static void
my_box_class_init (MyBoxClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);

  widget_class->size_request = my_box_size_request;
  widget_class->size_allocate = my_box_size_allocate;

  container_class->add = my_box_add;
  container_class->remove = my_box_remove;

  g_type_class_add_private (class, sizeof (MyBoxPriv));
}

static void
my_box_init (MyBox *box)
{
  MyBoxPriv *priv;

  priv = MY_BOX_GET_PRIV (box);

  priv->animation_widgets = g_hash_table_new (g_direct_hash,
		                              g_direct_equal);

  priv->children_pack_start = priv->children_pack_end = NULL;

  priv->animation_style = MY_BOX_ANIMATION_STYLE_MOVE;

  priv->orientation = GTK_ORIENTATION_VERTICAL;
}

static void
my_box_free_allocated_widgets (gpointer data,
		               gpointer user_data)
{
  g_slice_free (MyAllocatedWidget, data);
}

static void
my_box_free_allocated_lists (MyBox *box)
{
  MyBoxPriv *priv = MY_BOX_GET_PRIV (box);

  g_list_foreach (priv->children_pack_start,
		  my_box_free_allocated_widgets,
		  NULL);
  g_list_free (priv->children_pack_start);

  g_list_foreach (priv->children_pack_end,
		  my_box_free_allocated_widgets,
		  NULL);
  g_list_free (priv->children_pack_end);

  priv->children_pack_start = priv->children_pack_end = NULL;
}

static void
my_box_size_request (GtkWidget      *widget,
                     GtkRequisition *requisition)
{
  GtkBox *box = GTK_BOX (widget);
  MyBox *my_box = MY_BOX (widget);
  MyBoxPriv *priv = MY_BOX_GET_PRIV (my_box);
  GtkBoxChild *child;
  GList *children;
  gint nvis_children;
  gint width;
  gint height;

  requisition->width = 0;
  requisition->height = 0;
  nvis_children = 0;

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  GtkRequisition child_requisition;

	  gtk_widget_size_request (child->widget, &child_requisition);

	  if (box->homogeneous)
	    {
	      width = child_requisition.width + child->padding * 2;
	      height = child_requisition.height + child->padding * 2;

              if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
                requisition->width = MAX (requisition->width, width);
              else
                requisition->height = MAX (requisition->height, height);
	    }
	  else
	    {
              if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
                requisition->width += child_requisition.width + child->padding * 2;
              else
                requisition->height += child_requisition.height + child->padding * 2;
	    }

          if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            requisition->height = MAX (requisition->height, child_requisition.height);
          else
            requisition->width = MAX (requisition->width, child_requisition.width);

	  nvis_children += 1;
	}
    }

  if (nvis_children > 0)
    {
      if (box->homogeneous)
        {
          if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            requisition->width *= nvis_children;
          else
            requisition->height *= nvis_children;
        }

      if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        requisition->width += (nvis_children - 1) * box->spacing;
      else
        requisition->height += (nvis_children - 1) * box->spacing;
    }

  requisition->width += GTK_CONTAINER (box)->border_width * 2;
  requisition->height += GTK_CONTAINER (box)->border_width * 2;
}

static void
my_box_size_allocate (GtkWidget     *widget,
                      GtkAllocation *allocation)
{
  GtkBox *base = GTK_BOX (widget);
  MyBox *box = MY_BOX (widget);
  MyBoxPriv *private = MY_BOX_GET_PRIV (box);
  MyAllocatedWidget *alloc;
  GtkBoxChild *child;
  GList *children;
  GtkAllocation child_allocation;
  gint nvis_children = 0;
  gint nexpand_children = 0;
  gint child_width = 0;
  gint child_height = 0;
  gint width = 0;
  gint height = 0;
  gint extra = 0;
  gint x = 0;
  gint y = 0;
  GtkTextDirection direction;
  gboolean expand = FALSE;

  widget->allocation = *allocation;

  direction = gtk_widget_get_direction (widget);

  for (children = base->children; children; children = children->next)
    {
      child = children->data;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  nvis_children += 1;
	  if (child->expand)
	    nexpand_children += 1;
	}
    }

  if (nvis_children > 0)
    {
      if (base->homogeneous)
	{
          if (private->orientation == GTK_ORIENTATION_HORIZONTAL)
            {
              width = (allocation->width -
                       GTK_CONTAINER (base)->border_width * 2 -
                       (nvis_children - 1) * base->spacing);
              extra = width / nvis_children;
            }
          else
            {
              height = (allocation->height -
                        GTK_CONTAINER (base)->border_width * 2 -
                        (nvis_children - 1) * base->spacing);
              extra = height / nvis_children;
            }
	}
      else if (nexpand_children > 0)
	{
          if (private->orientation == GTK_ORIENTATION_HORIZONTAL)
            {
              width = (gint) allocation->width - (gint) widget->requisition.width;
              extra = width / nexpand_children;
            }
          else
            {
              height = (gint) allocation->height - (gint) widget->requisition.height;
              extra = height / nexpand_children;
            }
	}

      /* gcc */
      child_allocation.width = 0;
      child_allocation.height = 0;
      child_allocation.x = 0;
      child_allocation.y = 0;

      if (private->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          x = allocation->x + GTK_CONTAINER (base)->border_width;
          child_allocation.y = allocation->y + GTK_CONTAINER (base)->border_width;
          child_allocation.height = MAX (1, (gint) allocation->height - (gint) GTK_CONTAINER (base)->border_width * 2);
        }
      else
        {
          y = allocation->y + GTK_CONTAINER (base)->border_width;
          child_allocation.x = allocation->x + GTK_CONTAINER (base)->border_width;
          child_allocation.width = MAX (1, (gint) allocation->width - (gint) GTK_CONTAINER (base)->border_width * 2);
        }

      children = base->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_START) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      if (base->homogeneous)
		{
		  if (nvis_children == 1)
                    {
                      child_width = width;
                      child_height = height;
                    }
		  else
                    {
                      child_width = extra;
                      child_height = extra;
                    }

		  nvis_children -= 1;
		  width -= extra;
                  height -= extra;
		}
	      else
		{
		  GtkRequisition child_requisition;

		  gtk_widget_get_child_requisition (child->widget, &child_requisition);

		  child_width = child_requisition.width + child->padding * 2;
		  child_height = child_requisition.height + child->padding * 2;

		  if (child->expand)
		    {
		      if (nexpand_children == 1)
                        {
                          child_width += width;
                          child_height += height;
                        }
		      else
                        {
                          child_width += extra;
                          child_height += extra;
                        }

		      nexpand_children -= 1;
		      width -= extra;
                      height -= extra;
		    }
		}

	      if (child->fill)
		{
                  if (private->orientation == GTK_ORIENTATION_HORIZONTAL)
                    {
                      child_allocation.width = MAX (1, (gint) child_width - (gint) child->padding * 2);
                      child_allocation.x = x + child->padding;
                    }
                  else
                    {
                      child_allocation.height = MAX (1, child_height - (gint)child->padding * 2);
                      child_allocation.y = y + child->padding;
                    }
		}
	      else
		{
		  GtkRequisition child_requisition;

		  gtk_widget_get_child_requisition (child->widget, &child_requisition);
                  if (private->orientation == GTK_ORIENTATION_HORIZONTAL)
                    {
                      child_allocation.width = child_requisition.width;
                      child_allocation.x = x + (child_width - child_allocation.width) / 2;
                    }
                  else
                    {
                      child_allocation.height = child_requisition.height;
                      child_allocation.y = y + (child_height - child_allocation.height) / 2;
                    }
		}

	      if (direction == GTK_TEXT_DIR_RTL &&
                  private->orientation == GTK_ORIENTATION_HORIZONTAL)
                {
                  child_allocation.x = allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;
                }

	      alloc = g_slice_new (MyAllocatedWidget);

	      g_object_ref (child->widget);
	      alloc->widget = child->widget;

	      /* save the old state */
	      alloc->allocation_old.width = child->widget->allocation.width;
	      alloc->allocation_old.height = child->widget->allocation.height;
	      alloc->allocation_old.x = child->widget->allocation.x;
	      alloc->allocation_old.y = child->widget->allocation.y;

	      /* save the new state */
	      alloc->allocation.width = child_allocation.width;
	      alloc->allocation.height = child_allocation.height;
	      alloc->allocation.x = child_allocation.x;
	      alloc->allocation.y = child_allocation.y;

	      alloc->expand = expand;

	      expand = FALSE;

	      private->children_pack_start = g_list_append (private->children_pack_start,
			      				    alloc);

	      x += child_width + base->spacing;
	      y += child_height + base->spacing;
	    }
	}

      x = allocation->x + allocation->width - GTK_CONTAINER (base)->border_width;
      y = allocation->y + allocation->height - GTK_CONTAINER (base)->border_width;

      children = base->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_END) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      GtkRequisition child_requisition;

	      gtk_widget_get_child_requisition (child->widget, &child_requisition);

              if (base->homogeneous)
                {
                  if (nvis_children == 1)
                    {
                      child_width = width;
                      child_height = height;
                    }
                  else
                    {
                      child_width = extra;
                      child_height = extra;
                   }

                  nvis_children -= 1;
                  width -= extra;
                  height -= extra;
                }
              else
                {
		  child_width = child_requisition.width + child->padding * 2;
		  child_height = child_requisition.height + child->padding * 2;

                  if (child->expand)
                    {
                      if (nexpand_children == 1)
                        {
                          child_width += width;
                          child_height += height;
                         }
                      else
                        {
                          child_width += extra;
                          child_height += extra;
                        }

                      nexpand_children -= 1;
                      width -= extra;
                      height -= extra;
                    }
                }

              if (child->fill)
                {
                  if (private->orientation == GTK_ORIENTATION_HORIZONTAL)
                    {
                      child_allocation.width = MAX (1, (gint)child_width - (gint)child->padding * 2);
                      child_allocation.x = x + child->padding - child_width;
                    }
                  else
                    {
                      child_allocation.height = MAX (1, child_height - (gint)child->padding * 2);
                      child_allocation.y = y + child->padding - child_height;
                     }
                }
              else
                {
                  if (private->orientation == GTK_ORIENTATION_HORIZONTAL)
                    {
                      child_allocation.width = child_requisition.width;
                      child_allocation.x = x + (child_width - child_allocation.width) / 2 - child_width;
                    }
                  else
                    {
                      child_allocation.height = child_requisition.height;
                      child_allocation.y = y + (child_height - child_allocation.height) / 2 - child_height;
                    }
                }

	      if (direction == GTK_TEXT_DIR_RTL &&
                  private->orientation == GTK_ORIENTATION_HORIZONTAL)
                {
                  child_allocation.x = allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;
                }

	      alloc = g_slice_new (MyAllocatedWidget);

	      g_object_ref (child->widget);
	      alloc->widget = child->widget;

	      /* save the old state */
	      alloc->allocation_old.width = child->widget->allocation.width;
	      alloc->allocation_old.height = child->widget->allocation.height;
	      alloc->allocation_old.x = child->widget->allocation.x;
	      alloc->allocation_old.y = child->widget->allocation.y;

	      /* save the new state */
	      alloc->allocation.width = child_allocation.width;
	      alloc->allocation.height = child_allocation.height;
	      alloc->allocation.x = child_allocation.x;
	      alloc->allocation.y = child_allocation.y;

	      alloc->expand = expand;

	      expand = FALSE;

	      private->children_pack_end = g_list_append (private->children_pack_end, 
			                                  alloc);
              x -= (child_width + base->spacing);
              y -= (child_height + base->spacing);
	    }
	}

      my_box_handle_animation (box);
    }
}

static void
my_box_pack_n (GtkBox      *box,
	       GtkWidget   *child,
	       gboolean     expand,
	       gboolean     fill,
	       guint        padding,
	       GtkPackType  pack_type,
	       gint         position)
{
  GtkBoxChild *child_info;

  g_return_if_fail (GTK_IS_BOX (box));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == NULL);

  child_info = g_new (GtkBoxChild, 1);
  child_info->widget = child;
  child_info->padding = padding;
  child_info->expand = expand ? TRUE : FALSE;
  child_info->fill = fill ? TRUE : FALSE;
  child_info->pack = pack_type;
  child_info->is_secondary = FALSE;

  box->children = g_list_insert (box->children, child_info, position);

  gtk_widget_freeze_child_notify (child);

  gtk_widget_set_parent (child, GTK_WIDGET (box));
  
  gtk_widget_child_notify (child, "expand");
  gtk_widget_child_notify (child, "fill");
  gtk_widget_child_notify (child, "padding");
  gtk_widget_child_notify (child, "pack-type");
  gtk_widget_child_notify (child, "position");
  gtk_widget_thaw_child_notify (child);
}

static void
my_box_add (GtkContainer *container,
	    GtkWidget    *widget)
{
  MyBox *box;
  MyBoxPriv *priv;

  box = MY_BOX (container);
  priv = MY_BOX_GET_PRIV (box);

  if (!g_hash_table_lookup (priv->animation_widgets, widget))
    gtk_box_pack_start (GTK_BOX (container), widget,
                        TRUE,
                        TRUE,
                        0);

  g_hash_table_insert (priv->animation_widgets, 
		       widget,
		       GINT_TO_POINTER (MY_BOX_ANIMATION_ADD));

  my_box_handle_animation (box);
}

static void
my_box_remove (GtkContainer *container,
	       GtkWidget    *widget)
{
  MyBox *box;
  MyBoxPriv *priv;

  box = MY_BOX (container);
  priv = MY_BOX_GET_PRIV (box);

  g_hash_table_insert (priv->animation_widgets, 
		       widget,
		       GINT_TO_POINTER (MY_BOX_ANIMATION_REMOVE));

  gtk_widget_queue_resize (GTK_WIDGET (box));
}

GtkWidget *
my_box_new (gboolean homogeneous,
	    gint     spacing)
{
  return g_object_new (MY_TYPE_BOX, NULL, NULL);
}

void
my_box_pack_start_n (GtkBox    *box,
		     GtkWidget *child,
		     gboolean   expand,
		     gboolean   fill,
		     guint      padding,
		     gint       position)
{
  MyBox *mybox;
  MyBoxPriv *priv;

  mybox = MY_BOX (box);
  priv = MY_BOX_GET_PRIV (mybox);

  if (!g_hash_table_lookup (priv->animation_widgets, child))
    my_box_pack_n (box, child, expand, fill, padding, GTK_PACK_START, position);

  g_hash_table_insert (priv->animation_widgets, 
		       child,
		       GINT_TO_POINTER (MY_BOX_ANIMATION_ADD));
  
  my_box_handle_animation (mybox);
}

GtkOrientation
my_box_get_orientation (MyBox *box)
{
  MyBoxPriv *priv;

  priv = MY_BOX_GET_PRIV (box);

  return priv->orientation;
}

static void
my_box_add_to_hash_table (gpointer data,
			  gpointer user_data)
{
  GHashTable *table = (GHashTable *) user_data;
  GtkBoxChild *child = (GtkBoxChild *) data;

  g_hash_table_insert (table, 
		       child->widget,
		       GINT_TO_POINTER (MY_BOX_ANIMATION_ORIENTATION));
}

void
my_box_set_orientation (MyBox *box,
			GtkOrientation orientation)
{
  MyBoxPriv *priv;

  priv = MY_BOX_GET_PRIV (box);

  priv->orientation = orientation;

  g_list_foreach (GTK_BOX (box)->children, 
		  my_box_add_to_hash_table,
		 priv->animation_widgets); 

  gtk_widget_queue_resize (GTK_WIDGET (box));
}

MyBoxAnimationStyle
my_box_get_animation_style (MyBox *box)
{
  MyBoxPriv *priv;

  priv = MY_BOX_GET_PRIV (box);

  return priv->animation_style;
}

void
my_box_set_animation_style (MyBox *box,
		            MyBoxAnimationStyle style)
{
  MyBoxPriv *priv;

  priv = MY_BOX_GET_PRIV (box);

  priv->animation_style = style;
}

/* ANIMATION */
static void
my_box_handle_animation (MyBox *box)
{
  GtkBox *base;
  MyBoxPriv *priv;

  base = GTK_BOX (box);

  if (g_list_length (base->children) == 0)
    return;
      
  priv = MY_BOX_GET_PRIV (box);

  if (g_hash_table_size (priv->animation_widgets) == 0)
    {
      my_box_animation_frame_cb (NULL, 0, box);
      my_box_free_allocated_lists (box);

      return;
    }

  if (!priv->timeline)
    {
      priv->timeline = af_timeline_new (650);

      g_signal_connect (priv->timeline, "frame",
		        G_CALLBACK (my_box_animation_frame_cb), box);
      
      g_signal_connect (priv->timeline, "finished",
		        G_CALLBACK (my_box_animation_finished_cb), 
			box);

      af_timeline_start (priv->timeline);
    }
  else
    {
      af_timeline_start (priv->timeline);
    }
}

static gint
my_box_animation_frame_move_offset (MyBoxAnimationType type,
				    GtkOrientation orientation,
		                    gdouble progress,
			     	    MyAllocatedWidget  *alloc)
{
  gint size;

  if (orientation == GTK_ORIENTATION_VERTICAL)
    size = alloc->allocation.height;
  else
    size = alloc->allocation.width;

  if (type == MY_BOX_ANIMATION_REMOVE)
    return size - progress * size;
  else if (type == MY_BOX_ANIMATION_ADD)
    return size - (1 - progress) * size;

  return 0;
}

static gint
my_box_animation_frame_move (GtkOrientation orientation,
			     gdouble progress,
			     GList *children,
			     MyAllocatedWidget *alloc,
			     MyBoxAnimationType type,
			     GtkAllocation *allocation)
{
  gint z = 0;
  gint offset = 0;
  gint z_pos, z_size;

  allocation->width = alloc->allocation.width;
  allocation->height = alloc->allocation.height;

  if (orientation == GTK_ORIENTATION_VERTICAL)
    {
      allocation->x = alloc->allocation.x;

      z_pos = alloc->allocation.y;
      z_size = alloc->allocation.height;
    }
  else
    {
      allocation->y = alloc->allocation.y;

      z_pos = alloc->allocation.x;
      z_size = alloc->allocation.width;
    }

  if (type == MY_BOX_ANIMATION_REMOVE)
    {
      if (children->next != NULL)
        z = z_pos - progress * z_size;
      else
        z = z_pos + progress * z_size;
    }
  else if (type == MY_BOX_ANIMATION_ADD)
    {
      if (children->next != NULL)
        z = z_pos - (1 - progress) * z_size;
      else
        z = z_pos + (1 - progress) * z_size;
    }

  if (orientation == GTK_ORIENTATION_VERTICAL)
    {
      allocation->y = z;
      offset = ABS (z - alloc->allocation.y);
    }
  else
    {
      allocation->x = z;
      offset = ABS (z - alloc->allocation.x);
    }
	      
  return offset;
}

static gint
my_box_animation_frame_move_resize (MyBoxAnimationType type,
				    GtkOrientation orientation,
		                    gdouble progress,
			       	    MyAllocatedWidget  *alloc)
{
  gint size, pos;

  if (orientation == GTK_ORIENTATION_VERTICAL)
    {
      size = alloc->allocation.height;
      pos = alloc->allocation.y;
    }
  else
    {
      size = alloc->allocation.width;
      pos = alloc->allocation.x;
    }

  return pos + size;
}

static gint
my_box_animation_frame_resize (GtkOrientation orientation,
			       gdouble progress,
			       MyAllocatedWidget *alloc,
			       MyBoxAnimationType type,
			       GtkAllocation *allocation)
{
  gint z_old, z_new, z_size, z_pos_new, z_pos_old;

  allocation->height = alloc->allocation.height;

  if (orientation == GTK_ORIENTATION_VERTICAL)
    {
      allocation->width = alloc->allocation.width;
      allocation->x = alloc->allocation.x;

      z_old = alloc->allocation_old.height;
      z_new = alloc->allocation.height;
      z_pos_old = alloc->allocation.y;
    }
  else
    {
      allocation->height = alloc->allocation.height;
      allocation->y = alloc->allocation.y;

      z_old = alloc->allocation_old.width;
      z_new = alloc->allocation.width;
      z_pos_old = alloc->allocation.x;
    }

  if (type == MY_BOX_ANIMATION_REMOVE)
    {
      z_size = (1 - progress) * z_new;
    }
  else if (type == MY_BOX_ANIMATION_ADD)
    {
      z_size = z_old + (z_new - z_old) * progress;	
    }
  
  z_pos_new = z_pos_old + z_new / 2 - z_size / 2;

  if (orientation == GTK_ORIENTATION_VERTICAL)
    {
      allocation->height = z_size;
      allocation->y = z_pos_new;
    }
  else
    {
      allocation->width = z_size;
      allocation->x = z_pos_new;
    }

  return z_new / 2;
}

static void
my_box_animation_frame_cb (AfTimeline *timeline,
		           gdouble     progress,
		           gpointer    user_data)
{
  MyBox *box = MY_BOX (user_data);
  MyBoxPriv *priv = MY_BOX_GET_PRIV (box);
  GList *children;
  MyAllocatedWidget *alloc;
  GtkAllocation allocation;
  gpointer type = NULL;
  gint not_animated;
  gint offset = 0;
  gint z = 0;

  /* this should be expanded and filled */
  not_animated = g_list_length (priv->children_pack_start)
	         + g_list_length (priv->children_pack_end)
		 - g_hash_table_size (priv->animation_widgets);

  children = priv->children_pack_start;
  while (children)
    {
      alloc = (MyAllocatedWidget *)children->data;
      type = g_hash_table_lookup (priv->animation_widgets, alloc->widget);

      if (type)
        {
	  if (priv->animation_style == MY_BOX_ANIMATION_STYLE_MOVE)
	    {
	      if (GPOINTER_TO_INT (type) != MY_BOX_ANIMATION_ORIENTATION)
	        offset += my_box_animation_frame_move (priv->orientation,
			      			       progress,
						       children,
						       alloc,
						       GPOINTER_TO_INT (type),
						       &allocation);
	      else
	        {
                  allocation.x = alloc->allocation_old.x + 
			         (alloc->allocation.x - alloc->allocation_old.x)
				 * progress;
                  allocation.y = alloc->allocation_old.y + 
			         (alloc->allocation.y - alloc->allocation_old.y)
				 * progress;
                  allocation.width = alloc->allocation_old.width + 
			             (alloc->allocation.width - alloc->allocation_old.width)
				     * progress;
                  allocation.height = alloc->allocation_old.height + 
			              (alloc->allocation.height - alloc->allocation_old.height)
				      * progress;
		}
	    }
	  else if (priv->animation_style == MY_BOX_ANIMATION_STYLE_RESIZE)
	    {
	      if (GPOINTER_TO_INT (type) != MY_BOX_ANIMATION_ORIENTATION)
	        offset += my_box_animation_frame_resize (priv->orientation,
			      			         progress,
						         alloc,
						         GPOINTER_TO_INT (type),
						         &allocation);
	      else
	        {
                  allocation.width = alloc->allocation.width * progress;
                  allocation.height = alloc->allocation.height * progress;
                  allocation.x = alloc->allocation.x + alloc->allocation.width / 2 - allocation.width / 2;
                  allocation.y = alloc->allocation.y + alloc->allocation.height / 2- allocation.height / 2;
		  /*
		  printf ("WIDTH: %d HEIGHT: %d X: %d Y: %d\n",
			  allocation.width, allocation.height, allocation.x, allocation.y);
		  */
		}

	    }
	  
          gtk_widget_size_allocate (alloc->widget, &allocation);
	}

      children = children->next;
    }

  if (not_animated > 0)
    offset /= not_animated;

  children = priv->children_pack_start;
  while (children)
    {
      alloc = (MyAllocatedWidget *)children->data;
      type = g_hash_table_lookup (priv->animation_widgets, alloc->widget);

      if (type)
        {
	  if (priv->animation_style == MY_BOX_ANIMATION_STYLE_MOVE)
	    z += my_box_animation_frame_move_offset (GPOINTER_TO_INT (type),
			    			     priv->orientation,
						     progress,
						     alloc);
	  else if (priv->animation_style == MY_BOX_ANIMATION_STYLE_RESIZE)
	    z += my_box_animation_frame_move_resize (GPOINTER_TO_INT (type),
			    			     priv->orientation,
						     progress,
						     alloc);
	}
      else
        {
	  gint z_size, z_size_new;
	 
	  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
	    {
              allocation.width = alloc->allocation.width;
	      allocation.x = alloc->allocation.x;
              allocation.y = z;

	      z_size = alloc->allocation.height;
	    }
	  else
	    {
              allocation.height = alloc->allocation.height;
	      allocation.y = alloc->allocation.y;
              allocation.x = z;

	      z_size = alloc->allocation.width;
	    }

	  if (priv->animation_style == MY_BOX_ANIMATION_STYLE_MOVE)
	    z_size_new = z_size + offset;
	  else if (priv->animation_style == MY_BOX_ANIMATION_STYLE_RESIZE)
	    z_size_new = z_size;
	  else
	    z_size_new = z_size;

	  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
	    allocation.height = z_size_new;
	  else
	    allocation.width = z_size_new;

	  gtk_widget_size_allocate (alloc->widget, &allocation);

	  z += z_size_new;
	}

      children = children->next;
    }
}

static void
my_box_remove_animated_widgets (MyBox *box)
{
  MyBoxPriv *priv;
  GtkBox *base;
  GtkBoxChild *child;
  GtkWidget *widget;
  GList *children;
  gpointer *type;

  base = GTK_BOX (box);
  priv = MY_BOX_GET_PRIV (box);

  children = base->children;
  while (children)
    {
      child = children->data;
      widget = child->widget;

      type = g_hash_table_lookup (priv->animation_widgets, widget);

      if (type)
	{
	  if (GPOINTER_TO_INT (type) == MY_BOX_ANIMATION_REMOVE)
	    {
	      gtk_widget_unparent (widget);

	      base->children = g_list_remove_link (base->children, children);

	      g_free (child);
	    }
	  
	  g_hash_table_remove (priv->animation_widgets, widget);
	}

      children = children->next;
    }
  
  gtk_widget_queue_resize (GTK_WIDGET (box));
}

static void
my_box_animation_finished_cb (AfTimeline *timeline,
                              gpointer    user_data)
{
  MyBox *box;
  MyBoxPriv *priv;

  box = MY_BOX (user_data);
  priv = MY_BOX_GET_PRIV (box);

  if (priv->timeline)
    {
      af_timeline_pause (priv->timeline);
      g_object_unref (priv->timeline);

      priv->timeline = NULL;
    }

  my_box_remove_animated_widgets (box);

  my_box_free_allocated_lists (box);
}

