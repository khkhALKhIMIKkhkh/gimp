/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "display-types.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvas.h"


/*  local function prototypes  */

static void    gimp_canvas_class_init    (GimpCanvasClass     *klass);
static void    gimp_canvas_init          (GimpCanvas          *gdisp);
static void    gimp_canvas_realize       (GtkWidget           *widget);
static void    gimp_canvas_unrealize     (GtkWidget           *widget);

static GdkGC * gimp_canvas_gc_new        (GtkWidget           *widget,
                                          GimpCanvasStyle      style);
static GdkGC * gimp_canvas_guides_gc_new (GtkWidget           *widget,
                                          GimpOrientationType  orientation,
                                          GdkColor            *fg,
                                          GdkColor            *bg);


static GtkDrawingAreaClass *parent_class = NULL;


GType
gimp_canvas_get_type (void)
{
  static GType canvas_type = 0;

  if (! canvas_type)
    {
      static const GTypeInfo canvas_info =
      {
        sizeof (GimpCanvasClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_canvas_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpCanvas),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_canvas_init,
      };

      canvas_type = g_type_register_static (GTK_TYPE_DRAWING_AREA,
                                            "GimpCanvas",
                                            &canvas_info, 0);
    }

  return canvas_type;
}

static void
gimp_canvas_class_init (GimpCanvasClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->realize   = gimp_canvas_realize;
  widget_class->unrealize = gimp_canvas_unrealize;
}

static void
gimp_canvas_init (GimpCanvas *canvas)
{
  gint i;

  for (i = 0; i < GIMP_CANVAS_NUM_STYLES; i++)
    canvas->gc[i] = NULL;
}

static void
gimp_canvas_realize (GtkWidget *widget)
{
  GimpCanvas *canvas = GIMP_CANVAS (widget);

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  canvas->gc[GIMP_CANVAS_STYLE_RENDER] =
    gimp_canvas_gc_new (widget, GIMP_CANVAS_STYLE_RENDER);
}

static void
gimp_canvas_unrealize (GtkWidget *widget)
{
  GimpCanvas *canvas = GIMP_CANVAS (widget);
  gint        i;

  for (i = 0; i < GIMP_CANVAS_NUM_STYLES; i++)
    {
      if (canvas->gc[i])
        {
          g_object_unref (canvas->gc[i]);
          canvas->gc[i] = NULL;
        }
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static GdkGC *
gimp_canvas_gc_new (GtkWidget       *widget,
                    GimpCanvasStyle  style)
{
  GdkGC      *gc = NULL;
  GdkColor    fg;
  GdkColor    bg;

  switch (style)
    {
    case GIMP_CANVAS_STYLE_XOR:
    case GIMP_CANVAS_STYLE_XOR_DASHED:
      fg.pixel = 0xFFFFFFFF;
      bg.pixel = 0x00000000;
      break;

    case GIMP_CANVAS_STYLE_HGUIDE_NORMAL:
    case GIMP_CANVAS_STYLE_VGUIDE_NORMAL:
      fg.red   = 0x0;
      fg.green = 0x0;
      fg.blue  = 0x0;

      bg.red   = 0x0;
      bg.green = 0x7f7f;
      bg.blue  = 0xffff;
      break;

    case GIMP_CANVAS_STYLE_HGUIDE_ACTIVE:
    case GIMP_CANVAS_STYLE_VGUIDE_ACTIVE:
      fg.red   = 0x0;
      fg.green = 0x0;
      fg.blue  = 0x0;

      bg.red   = 0xffff;
      bg.green = 0x0;
      bg.blue  = 0x0;
      break;

    default:
      break;
    }

  switch (style)
    {
    case GIMP_CANVAS_STYLE_RENDER:
      gc = gdk_gc_new (widget->window);
      gdk_gc_set_exposures (gc, TRUE);
      break;

    case GIMP_CANVAS_STYLE_XOR:
    case GIMP_CANVAS_STYLE_XOR_DASHED:
      gc = gdk_gc_new (widget->window);
      gdk_gc_set_function (gc, GDK_INVERT);
      gdk_gc_set_foreground (gc, &fg);
      gdk_gc_set_background (gc, &bg);
      gdk_gc_set_line_attributes (gc,
                                  0,
                                  (style == GIMP_CANVAS_STYLE_XOR ?
                                   GDK_LINE_SOLID : GDK_LINE_ON_OFF_DASH),
                                  GDK_CAP_NOT_LAST,
                                  GDK_JOIN_MITER);
      break;

    case GIMP_CANVAS_STYLE_HGUIDE_NORMAL:
    case GIMP_CANVAS_STYLE_HGUIDE_ACTIVE:
      gc = gimp_canvas_guides_gc_new (widget,
                                      GIMP_ORIENTATION_HORIZONTAL, &fg, &bg);
      break;

    case GIMP_CANVAS_STYLE_VGUIDE_NORMAL:
    case GIMP_CANVAS_STYLE_VGUIDE_ACTIVE:
      gc = gimp_canvas_guides_gc_new (widget,
                                      GIMP_ORIENTATION_VERTICAL, &fg, &bg);
      break;

    case GIMP_CANVAS_STYLE_CUSTOM:
      g_assert_not_reached ();
      break;

    default:
      break;
    }

  return gc;
}

static GdkGC *
gimp_canvas_guides_gc_new (GtkWidget           *widget,
                           GimpOrientationType  orient,
                           GdkColor            *fg,
                           GdkColor            *bg)
{
  const guchar stipple[] =
    {
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
    };

  GdkGC       *gc;
  GdkGCValues  values;

  values.fill = GDK_OPAQUE_STIPPLED;
  values.stipple =
    gdk_bitmap_create_from_data (widget->window,
                                 (const gchar *) stipple,
                                 orient == GIMP_ORIENTATION_HORIZONTAL ? 8 : 1,
                                 orient == GIMP_ORIENTATION_VERTICAL   ? 8 : 1);

  gc = gdk_gc_new_with_values (widget->window,
                               &values, GDK_GC_FILL | GDK_GC_STIPPLE);

  gdk_gc_set_rgb_fg_color (gc, fg);
  gdk_gc_set_rgb_bg_color (gc, bg);

  return gc;
}


/*  public functions  */

GtkWidget *
gimp_canvas_new (void)
{
  return g_object_new (GIMP_TYPE_CANVAS,
                       "name", "gimp-canvas",
                       NULL);
}

void
gimp_canvas_draw_cursor (GimpCanvas *canvas,
                         gint        x,
                         gint        y)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  gdk_draw_line (widget->window,
                 widget->style->white_gc, x - 7, y - 1, x + 7, y - 1);
  gdk_draw_line (widget->window,
                 widget->style->black_gc, x - 7, y,     x + 7, y    );
  gdk_draw_line (widget->window,
                 widget->style->white_gc, x - 7, y + 1, x + 7, y + 1);
  gdk_draw_line (widget->window,
                 widget->style->white_gc, x - 1, y - 7, x - 1, y + 7);
  gdk_draw_line (widget->window,
                 widget->style->black_gc, x,     y - 7, x,     y + 7);
  gdk_draw_line (widget->window,
                 widget->style->white_gc, x + 1, y - 7, x + 1, y + 7);
}

void
gimp_canvas_draw_point (GimpCanvas      *canvas,
                        GimpCanvasStyle  style,
                        gint             x,
                        gint             y)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  if (! canvas->gc[style])
    canvas->gc[style] = gimp_canvas_gc_new (widget, style);

  gdk_draw_point (widget->window, canvas->gc[style], x, y);
}

void
gimp_canvas_draw_line (GimpCanvas      *canvas,
                       GimpCanvasStyle  style,
                       gint             x1,
                       gint             y1,
                       gint             x2,
                       gint             y2)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  if (! canvas->gc[style])
    canvas->gc[style] = gimp_canvas_gc_new (widget, style);

  gdk_draw_line (widget->window, canvas->gc[style], x1, y1, x2, y2);
}

void
gimp_canvas_draw_lines (GimpCanvas      *canvas,
                        GimpCanvasStyle  style,
                        GdkPoint        *points,
                        gint             num_points)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  if (! canvas->gc[style])
    canvas->gc[style] = gimp_canvas_gc_new (widget, style);

  gdk_draw_lines (widget->window, canvas->gc[style], points, num_points);
}

void
gimp_canvas_draw_rectangle (GimpCanvas      *canvas,
                            GimpCanvasStyle  style,
                            gboolean         filled,
                            gint             x,
                            gint             y,
                            gint             width,
                            gint             height)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  if (! canvas->gc[style])
    canvas->gc[style] = gimp_canvas_gc_new (widget, style);

  gdk_draw_rectangle (widget->window, canvas->gc[style],
                      filled, x, y, width, height);
}

void
gimp_canvas_draw_arc (GimpCanvas      *canvas,
                      GimpCanvasStyle  style,
                      gboolean         filled,
                      gint             x,
                      gint             y,
                      gint             width,
                      gint             height,
                      gint             angle1,
                      gint             angle2)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  if (! canvas->gc[style])
    canvas->gc[style] = gimp_canvas_gc_new (widget, style);

  gdk_draw_arc (widget->window, canvas->gc[style],
                filled, x, y, width, height, angle1, angle2);
}

void
gimp_canvas_draw_polygon (GimpCanvas      *canvas,
                          GimpCanvasStyle  style,
                          gboolean         filled,
                          GdkPoint        *points,
                          gint             num_points)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  if (! canvas->gc[style])
    canvas->gc[style] = gimp_canvas_gc_new (widget, style);

  gdk_draw_polygon (widget->window, canvas->gc[style],
                    filled, points, num_points);
}

void
gimp_canvas_draw_segments (GimpCanvas      *canvas,
                           GimpCanvasStyle  style,
                           GdkSegment      *segments,
                           gint             num_segments)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  if (! canvas->gc[style])
    canvas->gc[style] = gimp_canvas_gc_new (widget, style);

  gdk_draw_segments (widget->window, canvas->gc[style],
                     segments, num_segments);
}

void
gimp_canvas_draw_rgb (GimpCanvas      *canvas,
                      GimpCanvasStyle  style,
                      gint             x,
                      gint             y,
                      gint             width,
                      gint             height,
                      guchar          *rgb_buf,
                      gint             rowstride,
                      gint             xdith,
                      gint             ydith)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  if (! canvas->gc[style])
    canvas->gc[style] = gimp_canvas_gc_new (widget, style);

  gdk_draw_rgb_image_dithalign (widget->window, canvas->gc[style],
                                x, y, width, height,
                                GDK_RGB_DITHER_MAX,
                                rgb_buf, rowstride, xdith, ydith);
}

void
gimp_canvas_set_bg_color (GimpCanvas *canvas,
                          GimpRGB    *color)
{
  GtkWidget   *widget = GTK_WIDGET (canvas);
  GdkColormap *colormap;
  GdkColor     gdk_color;

  if (! GTK_WIDGET_REALIZED (widget))
    return;

  gimp_rgb_get_gdk_color (color, &gdk_color);

  colormap = gdk_drawable_get_colormap (widget->window);
  g_return_if_fail (colormap != NULL);
  gdk_colormap_alloc_color (colormap, &gdk_color, FALSE, TRUE);

  gdk_window_set_background (widget->window, &gdk_color);
}

void
gimp_canvas_set_custom_gc (GimpCanvas *canvas,
                           GdkGC      *gc)
{
  if (canvas->gc[GIMP_CANVAS_STYLE_CUSTOM])
    g_object_unref (canvas->gc[GIMP_CANVAS_STYLE_CUSTOM]);

  canvas->gc[GIMP_CANVAS_STYLE_CUSTOM] = gc;

  if (gc)
    g_object_ref (gc);
}
