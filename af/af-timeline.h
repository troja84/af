/* aftimeline.h
 *
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

#ifndef __AF_TIMELINE_H__
#define __AF_TIMELINE_H__

#include <glib-object.h>
#include "af-enums.h"
#include "af-enumtypes.h"

G_BEGIN_DECLS

#define AF_TYPE_TIMELINE                 (af_timeline_get_type ())
#define AF_TIMELINE(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), AF_TYPE_TIMELINE, AfTimeline))
#define AF_TIMELINE_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), AF_TYPE_TIMELINE, AfTimelineClass))
#define AF_IS_TIMELINE(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AF_TYPE_TIMELINE))
#define AF_IS_TIMELINE_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), AF_TYPE_TIMELINE))
#define AF_TIMELINE_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), AF_TYPE_TIMELINE, AfTimelineClass))

typedef struct AfTimeline      AfTimeline;
typedef struct AfTimelineClass AfTimelineClass;

struct AfTimeline
{
  GObject parent_instance;
};

struct AfTimelineClass
{
  GObjectClass parent_class;

  void (* started)           (AfTimeline *timeline);
  void (* finished)          (AfTimeline *timeline);
  void (* paused)            (AfTimeline *timeline);

  void (* frame)             (AfTimeline *timeline,
			      gdouble     progress);
  void (* marker)            (AfTimeline *timeline,
			      gdouble     progress,
			      gchar      *marker_name);

  void (* __gtk_reserved1) (void);
  void (* __gtk_reserved2) (void);
  void (* __gtk_reserved3) (void);
  void (* __gtk_reserved4) (void);
};


GType                 af_timeline_get_type           (void) G_GNUC_CONST;

AfTimeline           *af_timeline_new                (guint                    duration);
AfTimeline           *af_timeline_new_for_screen     (guint                    duration,
                                                      GdkScreen               *screen);

void                  af_timeline_start              (AfTimeline              *timeline);
void                  af_timeline_pause              (AfTimeline              *timeline);
void                  af_timeline_stop               (AfTimeline              *timeline);
void                  af_timeline_rewind             (AfTimeline              *timeline);
void                  af_timeline_advance            (AfTimeline             *timeline,
		                                      gdouble                 new_progress);

gdouble               af_timeline_get_progress       (AfTimeline              *timeline);

gboolean              af_timeline_is_running         (AfTimeline              *timeline);

void                  af_timeline_add_marker         (AfTimeline             *timeline,
		                                      const gchar            *marker_name,
				                      gdouble                 progress);
gboolean              af_timeline_has_marker          (AfTimeline             *timeline,
		                                       const gchar            *marker_name);
gchar**               af_timeline_list_markers        (AfTimeline             *timeline,
		                                       gdouble                 start_progress,
		                                       gdouble                 end_progress);
void                  af_timeline_remove_marker       (AfTimeline             *timeline,
		                                       const gchar            *marker_name);
gboolean              af_timeline_advance_to_marker   (AfTimeline             *timeline,
		                                       const gchar            *marker_name);

guint                 af_timeline_get_fps            (AfTimeline              *timeline);
void                  af_timeline_set_fps            (AfTimeline              *timeline,
                                                      guint                    fps);

gboolean              af_timeline_get_loop           (AfTimeline              *timeline);
void                  af_timeline_set_loop           (AfTimeline              *timeline,
                                                      gboolean                 loop);

guint                 af_timeline_get_duration       (AfTimeline              *timeline);
void                  af_timeline_set_duration       (AfTimeline              *timeline,
                                                      guint                    duration);

GdkScreen            *af_timeline_get_screen         (AfTimeline              *timeline);
void                  af_timeline_set_screen         (AfTimeline              *timeline,
                                                      GdkScreen               *screen);

AfTimelineDirection   af_timeline_get_direction      (AfTimeline              *timeline);
void                  af_timeline_set_direction      (AfTimeline              *timeline,
                                                      AfTimelineDirection      direction);

gdouble               af_timeline_calculate_progress (gdouble                  linear_progress,
                                                      AfTimelineProgressType   progress_type);

G_END_DECLS

#endif /* __AF_TIMELINE_H__ */
