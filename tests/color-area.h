#ifndef __AF_COLOR_AREA_H__
#define __AF_COLOR_AREA_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define AF_TYPE_COLOR_AREA                 (af_color_area_get_type ())
#define AF_COLOR_AREA(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), AF_TYPE_COLOR_AREA, AfColorArea))
#define AF_COLOR_AREA_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), AF_TYPE_COLOR_AREA, AfColorAreaClass))
#define AF_IS_COLOR_AREA(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AF_TYPE_COLOR_AREA))
#define AF_IS_COLOR_AREA_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), AF_TYPE_COLOR_AREA))
#define AF_COLOR_AREA_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), AF_TYPE_COLOR_AREA, AfColorAreaClass))

typedef struct AfColorArea      AfColorArea;
typedef struct AfColorAreaClass AfColorAreaClass;

struct AfColorArea
{
  GtkDrawingArea parent_instance;
  GdkColor background_color;
};

struct AfColorAreaClass
{
  GtkDrawingAreaClass parent_class;
};

GType                 af_color_area_get_type             (void) G_GNUC_CONST;

GtkWidget            *af_color_area_new                  (void);
void                  af_color_area_set_background_color (AfColorArea    *area,
                                                          const GdkColor *color);
void                  af_color_area_get_background_color (AfColorArea    *area,
                                                          GdkColor       *color);

G_END_DECLS

#endif /* __GTK_COLOR_AREA_H__ */
