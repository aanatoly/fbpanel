#ifndef __FB_ENUMS_H__
#define __FB_ENUMS_H__

#include <glib.h>
#include <glib-object.h>

GType fb_align_type_get_type (void) G_GNUC_CONST;
#define FB_TYPE_ALIGN_TYPE (fb_align_type_get_type ())

typedef enum {
  FB_ALIGN_LEFT,
  FB_ALIGN_CENTER,
  FB_ALIGN_RIGHT
} FbAlignType;

GType fb_edge_type_get_type (void) G_GNUC_CONST;
#define FB_TYPE_EDGE_TYPE (fb_edge_type_get_type ())

typedef enum {
  FB_EDGE_BOTTOM,
  FB_EDGE_TOP,
  FB_EDGE_LEFT,
  FB_EDGE_RIGHT
} FbEdgeType;

GType fb_width_type_get_type (void) G_GNUC_CONST;
#define FB_TYPE_WIDTH_TYPE (fb_width_type_get_type ())

typedef enum {
  FB_WIDTH_PERCENT,
  FB_WIDTH_PIXEL,
  FB_WIDTH_DYNAMIC
} FbWidthType;


#endif // __FB_ENUMS_H__
