#include <gtk/gtk.h>
#include <af/af-timeline.h>
#include <math.h>
#include "myvbox_a.h"

typedef struct MyVBoxPriv MyVBoxPriv;
typedef struct MyAllocatedWidget MyAllocatedWidget;

enum
{
  MY_VBOX_ANIMATION_ADD = 1,
  MY_VBOX_ANIMATION_REMOVE
};

struct MyAllocatedWidget
{
  GtkWidget *widget;

  GtkAllocation allocation, allocation_mod;
  gboolean expand;
};

struct MyVBoxPriv
{
  AfTimeline *timeline;
  gdouble progress;

  GHashTable *animation_widgets;
  MyVBoxAnimationStyle animation_style;

  GList *children_pack_start;
  GList *children_pack_end;
};

static void my_vbox_size_allocate           (GtkWidget      *widget,
                                             GtkAllocation  *allocation);

static void my_vbox_add                     (GtkContainer   *container,
	                                     GtkWidget      *widget);	
static void my_vbox_remove                  (GtkContainer   *container,
                                             GtkWidget      *widget);

/* ANIMATION*/
static void my_vbox_handle_animation        (MyVBox         *vbox);
static void my_vbox_animation_frame_cb      (AfTimeline     *timeline,
		                             gdouble         progress,
		                             gpointer        user_data);
static void my_vbox_remove_animated_widgets (MyVBox         *vbox);
static void my_vbox_animation_finished_cb   (AfTimeline     *timeline,
                                             gpointer        user_data);

#define MY_VBOX_GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MY_TYPE_VBOX, MyVBoxPriv))

G_DEFINE_TYPE (MyVBox, my_vbox, GTK_TYPE_VBOX)

static void
my_vbox_class_init (MyVBoxClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);

  widget_class->size_allocate = my_vbox_size_allocate;

  container_class->add = my_vbox_add;
  container_class->remove = my_vbox_remove;

  g_type_class_add_private (class, sizeof (MyVBoxPriv));
}

static void
my_vbox_init (MyVBox *vbox)
{
  MyVBoxPriv *priv;

  priv = MY_VBOX_GET_PRIV (vbox);

  priv->animation_widgets = g_hash_table_new (g_direct_hash,
		                              g_direct_equal);

  priv->children_pack_start = priv->children_pack_end = NULL;

  priv->animation_style = MY_VBOX_ANIMATION_STYLE_MOVE;
}

static void
my_vbox_free_allocated_widgets (gpointer data,
		                gpointer user_data)
{
  g_slice_free (MyAllocatedWidget, data);
}

static void
my_vbox_free_allocated_lists (MyVBox *vbox)
{
  MyVBoxPriv *priv = MY_VBOX_GET_PRIV (vbox);

  g_list_foreach (priv->children_pack_start,
		  my_vbox_free_allocated_widgets,
		  NULL);
  g_list_free (priv->children_pack_start);

  g_list_foreach (priv->children_pack_end,
		  my_vbox_free_allocated_widgets,
		  NULL);
  g_list_free (priv->children_pack_end);

  priv->children_pack_start = priv->children_pack_end = NULL;
}

static void
my_vbox_size_allocate (GtkWidget     *widget,
                       GtkAllocation *allocation)
{
  GtkBox *box = GTK_BOX (widget);
  MyVBox *vbox = MY_VBOX (widget);
  MyVBoxPriv *vbox_priv = MY_VBOX_GET_PRIV (vbox);
  GtkBoxChild *child;
  GList *children;
  MyAllocatedWidget *alloc;
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

  if (vbox_priv->timeline)
    af_timeline_pause (vbox_priv->timeline);

  for (children = box->children; children; children = children->next)
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
      my_vbox_free_allocated_lists (vbox);

      if (box->homogeneous)
	{
          height = (allocation->height -
                    GTK_CONTAINER (box)->border_width * 2 -
                    (nvis_children - 1) * box->spacing);
          extra = height / nvis_children;
	}
      else if (nexpand_children > 0)
	{
          height = (gint) allocation->height - (gint) widget->requisition.height;
          extra = height / nexpand_children;
	}

      y = allocation->y + GTK_CONTAINER (box)->border_width;
      child_allocation.x = allocation->x + GTK_CONTAINER (box)->border_width;
      child_allocation.width = MAX (1, (gint) allocation->width - (gint) GTK_CONTAINER (box)->border_width * 2);

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_START) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      if (box->homogeneous)
		{
		  expand = TRUE;

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
		      expand = TRUE;

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
                  child_allocation.height = MAX (1, child_height - (gint)child->padding * 2);
                  child_allocation.y = y + child->padding;

		  if (!expand)
		    expand = FALSE;
		}
	      else
		{
		  GtkRequisition child_requisition;

		  gtk_widget_get_child_requisition (child->widget, &child_requisition);
                      
		  child_allocation.height = child_requisition.height;
                  child_allocation.y = y + (child_height - child_allocation.height) / 2;

		  expand = FALSE;
		}

	      alloc = g_slice_new (MyAllocatedWidget);

	      g_object_ref (child->widget);
	      alloc->widget = child->widget;
	      alloc->allocation.width = child_allocation.width;
	      alloc->allocation.height = child_allocation.height;
	      alloc->allocation.x = child_allocation.x;
	      alloc->allocation.y = child_allocation.y;
	      alloc->expand = expand;

	      expand = FALSE;

	      vbox_priv->children_pack_start = g_list_append (vbox_priv->children_pack_start,
			                                      alloc);

	      x += child_width + box->spacing;
	      y += child_height + box->spacing;
	    }
	}

      x = allocation->x + allocation->width - GTK_CONTAINER (box)->border_width;
      y = allocation->y + allocation->height - GTK_CONTAINER (box)->border_width;

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_END) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      GtkRequisition child_requisition;

	      gtk_widget_get_child_requisition (child->widget, &child_requisition);

              if (box->homogeneous)
                {
		  expand = TRUE;

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
		      expand = TRUE;

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
                  child_allocation.height = MAX (1, child_height - (gint)child->padding * 2);
                  child_allocation.y = y + child->padding - child_height;

		  if (!expand)
		    expand = FALSE;
                }
              else
                {
                  child_allocation.height = child_requisition.height;
                  child_allocation.y = y + (child_height - child_allocation.height) / 2 - child_height;

		  expand = FALSE;
                }

	      alloc = g_slice_new (MyAllocatedWidget);

	      g_object_ref (child->widget);
	      alloc->widget = child->widget;
	      alloc->allocation.width = child_allocation.width;
	      alloc->allocation.height = child_allocation.height;
	      alloc->allocation.x = child_allocation.x;
	      alloc->allocation.y = child_allocation.y;
	      alloc->expand = expand;

	      expand = FALSE;

	      vbox_priv->children_pack_end = g_list_append (vbox_priv->children_pack_end, 
			                                    alloc);

              x -= (child_width + box->spacing);
              y -= (child_height + box->spacing);
	    }
	}

      my_vbox_handle_animation (vbox);
    }
}

static void
my_vbox_pack_n (GtkBox      *box,
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
my_vbox_add (GtkContainer *container,
	     GtkWidget    *widget)
{
  MyVBox *vbox;
  MyVBoxPriv *priv;

  vbox = MY_VBOX (container);
  priv = MY_VBOX_GET_PRIV (vbox);

  if (!g_hash_table_lookup (priv->animation_widgets, widget))
    gtk_box_pack_start (GTK_BOX (container), widget,
                        TRUE,
                        TRUE,
                        0);

  g_hash_table_insert (priv->animation_widgets, 
		       widget,
		       GINT_TO_POINTER (MY_VBOX_ANIMATION_ADD));

  my_vbox_handle_animation (vbox);
}

static void
my_vbox_remove (GtkContainer *container,
		GtkWidget    *widget)
{
  MyVBox *vbox;
  MyVBoxPriv *priv;

  vbox = MY_VBOX (container);
  priv = MY_VBOX_GET_PRIV (vbox);

  g_hash_table_insert (priv->animation_widgets, 
		       widget,
		       GINT_TO_POINTER (MY_VBOX_ANIMATION_REMOVE));

  my_vbox_handle_animation (vbox);
}

GtkWidget *
my_vbox_new (gboolean homogeneous,
	     gint     spacing)
{
  return g_object_new (MY_TYPE_VBOX, NULL, NULL);
}

void
my_vbox_pack_start_n (GtkBox    *box,
		      GtkWidget *child,
		      gboolean   expand,
		      gboolean   fill,
		      guint      padding,
		      gint       position)
{
  MyVBox *vbox;
  MyVBoxPriv *priv;

  vbox = MY_VBOX (box);
  priv = MY_VBOX_GET_PRIV (vbox);

  if (!g_hash_table_lookup (priv->animation_widgets, child))
    my_vbox_pack_n (box, child, expand, fill, padding, GTK_PACK_START, position);

  g_hash_table_insert (priv->animation_widgets, 
		       child,
		       GINT_TO_POINTER (MY_VBOX_ANIMATION_ADD));
  
  my_vbox_handle_animation (vbox);
}

MyVBoxAnimationStyle
my_vbox_get_animation_style (MyVBox *vbox)
{
  MyVBoxPriv *priv;

  priv = MY_VBOX_GET_PRIV (vbox);

  return priv->animation_style;
}

void
my_vbox_set_animation_style (MyVBox *vbox,
		             MyVBoxAnimationStyle style)
{
  MyVBoxPriv *priv;

  priv = MY_VBOX_GET_PRIV (vbox);

  priv->animation_style = style;
}

/* ANIMATION */
static void
my_vbox_handle_animation (MyVBox *vbox)
{
  GtkBox *box;
  MyVBoxPriv *priv;

  box = GTK_BOX (vbox);

  if (g_list_length (box->children) == 0)
    return;
      
  priv = MY_VBOX_GET_PRIV (vbox);

  if (g_hash_table_size (priv->animation_widgets) == 0)
    {
      my_vbox_animation_frame_cb (NULL, 0, vbox);
      return;
    }

  if (!priv->timeline)
    {
      priv->timeline = af_timeline_new (650);

      g_signal_connect (priv->timeline, "frame",
		        G_CALLBACK (my_vbox_animation_frame_cb), vbox);
      
      g_signal_connect (priv->timeline, "finished",
		        G_CALLBACK (my_vbox_animation_finished_cb), 
			vbox);

      af_timeline_start (priv->timeline);
    }
  else
    {
      // af_timeline_rewind (priv->timeline);

      af_timeline_start (priv->timeline);
    }
}

static void
my_vbox_animation_frame_cb (AfTimeline *timeline,
		            gdouble     progress,
		            gpointer    user_data)
{
  MyVBox *vbox = MY_VBOX (user_data);
  MyVBoxPriv *priv = MY_VBOX_GET_PRIV (vbox);
  GList *children;
  MyAllocatedWidget *alloc;
  GtkAllocation allocation;
  gpointer type = NULL;
  gint not_animated;
  gint offset = 0;
  gint y = 0;

  /* this should be expand and fill */
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
          allocation.x = alloc->allocation.x;
          allocation.width = alloc->allocation.width;

	  if (priv->animation_style == MY_VBOX_ANIMATION_STYLE_MOVE)
	    {
	      y = 0;

	      if (GPOINTER_TO_INT (type) == MY_VBOX_ANIMATION_REMOVE)
	        {
	          if (children->next != NULL)
                    y = alloc->allocation.y - progress * alloc->allocation.height;
	          else
                    y = alloc->allocation.y + progress * alloc->allocation.height;
		}
	      else if (GPOINTER_TO_INT (type) == MY_VBOX_ANIMATION_ADD)
	        {
	          if (children->next != NULL)
                    y = alloc->allocation.y - (1 - progress) * alloc->allocation.height;
	          else
                    y = alloc->allocation.y + (1 - progress) * alloc->allocation.height;
		}

	      allocation.y = y;
	      allocation.height = alloc->allocation.height;
	      
	      offset += ABS (y - alloc->allocation.y);
	    }
	  else if (priv->animation_style == MY_VBOX_ANIMATION_STYLE_RESIZE)
	    {
	      if (GPOINTER_TO_INT (type) == MY_VBOX_ANIMATION_REMOVE)
	        {
                  allocation.height = (1 - progress) * alloc->allocation.height;
	          allocation.y = alloc->allocation.y + alloc->allocation.height / 2 - allocation.height / 2;
		}
	      else if (GPOINTER_TO_INT (type) == MY_VBOX_ANIMATION_ADD)
	        {
                  allocation.height =  progress * alloc->allocation.height;
	          allocation.y = alloc->allocation.y + alloc->allocation.height / 2 - allocation.height / 2;
		}

	      offset += allocation.height / 2;
	    }
	  
          gtk_widget_size_allocate (alloc->widget, &allocation);
	}

      children = children->next;
    }

  y = 0;

  if (not_animated > 0)
    offset /= not_animated;

  children = priv->children_pack_start;
  while (children)
    {
      alloc = (MyAllocatedWidget *)children->data;
      type = g_hash_table_lookup (priv->animation_widgets, alloc->widget);

      if (type)
        {
	  if (priv->animation_style == MY_VBOX_ANIMATION_STYLE_MOVE)
	    {
	      if (GPOINTER_TO_INT (type) == MY_VBOX_ANIMATION_REMOVE)
                y += alloc->allocation.height - progress * alloc->allocation.height;
	      else if (GPOINTER_TO_INT (type) == MY_VBOX_ANIMATION_ADD)
                y += alloc->allocation.height - (1 - progress) * alloc->allocation.height;
	    }
	  else if (priv->animation_style == MY_VBOX_ANIMATION_STYLE_RESIZE)
	    {
	      y += alloc->allocation.height;
	    }
	}
      else
        {
          allocation.width = alloc->allocation.width;
	  allocation.x = alloc->allocation.x;
          allocation.y = y;

	  if (priv->animation_style == MY_VBOX_ANIMATION_STYLE_MOVE)
	    {
	      if (children->next == NULL)
	        {
	          allocation.height = alloc->allocation.height + offset;
	        }
	      else
	        allocation.height = alloc->allocation.height + offset;
	    }
	  else if (priv->animation_style == MY_VBOX_ANIMATION_STYLE_RESIZE)
	    allocation.height = alloc->allocation.height;

	  gtk_widget_size_allocate (alloc->widget, &allocation);

	  y = allocation.y + allocation.height;
	}

      children = children->next;
    }
}

static void
my_vbox_remove_animated_widgets (MyVBox *vbox)
{
  MyVBoxPriv *priv;
  GtkBox *box;
  GtkBoxChild *child;
  GtkWidget *widget;
  GList *children;
  gpointer *type;

  box = GTK_BOX (vbox);
  priv = MY_VBOX_GET_PRIV (vbox);

  children = box->children;
  while (children)
    {
      child = children->data;
      widget = child->widget;

      type = g_hash_table_lookup (priv->animation_widgets, widget);

      if (type)
	{
	  if (GPOINTER_TO_INT (type) == MY_VBOX_ANIMATION_REMOVE)
	    {
	      gtk_widget_unparent (widget);

	      box->children = g_list_remove_link (box->children, children);

	      g_free (child);
	    }
	  
	  g_hash_table_remove (priv->animation_widgets, widget);
	}

      children = children->next;
    }
  
  gtk_widget_queue_resize (GTK_WIDGET (vbox));
}

static void
my_vbox_animation_finished_cb (AfTimeline *timeline,
                               gpointer    user_data)
{
  MyVBox *vbox;
  MyVBoxPriv *priv;

  vbox = MY_VBOX (user_data);
  priv = MY_VBOX_GET_PRIV (vbox);

  af_timeline_pause (priv->timeline);
  g_object_unref (priv->timeline);

  priv->timeline = NULL;

  my_vbox_remove_animated_widgets (vbox);

  my_vbox_free_allocated_lists (vbox);
}

