#include "enums.h"


GType
fb_align_type_get_type (void)
{
    static GType etype = 0;
    if (G_UNLIKELY(etype == 0)) {
        static const GEnumValue values[] = {
            { FB_ALIGN_LEFT, "FB_ALIGN_LEFT", "left" },
            { FB_ALIGN_CENTER, "FB_ALIGN_CENTER", "center" },
            { FB_ALIGN_RIGHT, "FB_ALIGN_RIGHT", "right" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static(
            g_intern_static_string ("FbAlignType"), values);
    }
    return etype;
}


GType
fb_edge_type_get_type (void)
{
    static GType etype = 0;
    if (G_UNLIKELY(etype == 0)) {
        static const GEnumValue values[] = {
            { FB_EDGE_BOTTOM, "FB_EDGE_BOTTOM", "bottom" },
            { FB_EDGE_TOP, "FB_EDGE_TOP", "top" },
            { FB_EDGE_LEFT, "FB_EDGE_LEFT", "left" },
            { FB_EDGE_RIGHT, "FB_EDGE_RIGHT", "right" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static(
            g_intern_static_string ("FbEdgeType"), values);
    }
    return etype;
}


GType
fb_width_type_get_type (void)
{
    static GType etype = 0;
    if (G_UNLIKELY(etype == 0)) {
        static const GEnumValue values[] = {
            { FB_WIDTH_PERCENT, "FB_WIDTH_PERCENT", "percent" },
            { FB_WIDTH_PIXEL, "FB_WIDTH_PIXEL", "pixel" },
            { FB_WIDTH_DYNAMIC, "FB_WIDTH_DYNAMIC", "dynamic" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static(
            g_intern_static_string ("FbWidthType"), values);
    }
    return etype;
}
