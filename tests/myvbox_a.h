#ifndef __MY_VBOX_H__
#define __MY_VBOX_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MY_TYPE_VBOX		 (my_vbox_get_type ())
#define MY_VBOX(obj)		 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_VBOX, MyVBox))
#define MY_VBOX_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), MY_TYPE_VBOX, MyVBoxClass))
#define MY_IS_VBOX(obj)	         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_VBOX))
#define MY_IS_VBOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MY_TYPE_VBOX))
#define MY_VBOX_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_VBOX, MyVBoxClass))


typedef struct _MyVBox	      MyVBox;
typedef struct _MyVBoxClass  MyVBoxClass;

struct _MyVBox
{
  GtkBox box;
};

struct _MyVBoxClass
{
  GtkBoxClass parent_class;
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

G_END_DECLS

#endif /* __MY_VBOX_H__ */
