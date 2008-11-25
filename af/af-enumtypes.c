
/* Generated data (by glib-mkenums) */

#include <glib.h>
#include <glib-object.h>
/* enumerations from "af-enums.h" */
#include "af-enums.h"
GType
af_timeline_direction_get_type(void) {
	static GType etype = 0;
	if(!etype) {
		static const GEnumValue values[] = {
			{AF_TIMELINE_DIRECTION_FORWARD, "AF_TIMELINE_DIRECTION_FORWARD", "forward"},
			{AF_TIMELINE_DIRECTION_BACKWARD, "AF_TIMELINE_DIRECTION_BACKWARD", "backward"},
			{0, NULL, NULL}
		};

		etype = g_enum_register_static("AfTimelineDirection", values);
	}
	
	return etype;
}
GType
af_timeline_progress_type_get_type(void) {
	static GType etype = 0;
	if(!etype) {
		static const GEnumValue values[] = {
			{AF_TIMELINE_PROGRESS_LINEAR, "AF_TIMELINE_PROGRESS_LINEAR", "linear"},
			{AF_TIMELINE_PROGRESS_SINUSOIDAL, "AF_TIMELINE_PROGRESS_SINUSOIDAL", "sinusoidal"},
			{AF_TIMELINE_PROGRESS_EXPONENTIAL, "AF_TIMELINE_PROGRESS_EXPONENTIAL", "exponential"},
			{AF_TIMELINE_PROGRESS_EASE_IN_EASE_OUT, "AF_TIMELINE_PROGRESS_EASE_IN_EASE_OUT", "ease-in-ease-out"},
			{0, NULL, NULL}
		};

		etype = g_enum_register_static("AfTimelineProgressType", values);
	}
	
	return etype;
}

/* Generated data ends here */

