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

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"

#include "gimppaintbrush.h"
#include "gimppaintoptions.h"

#include "gimp-intl.h"


static void   gimp_paintbrush_class_init (GimpPaintbrushClass *klass);
static void   gimp_paintbrush_init       (GimpPaintbrush      *paintbrush);

static void   gimp_paintbrush_paint      (GimpPaintCore       *paint_core,
                                          GimpDrawable        *drawable,
                                          GimpPaintOptions    *paint_options,
                                          GimpPaintCoreState   paint_state);


static GimpPaintCoreClass *parent_class = NULL;


void
gimp_paintbrush_register (Gimp                      *gimp,
                          GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_PAINTBRUSH,
                GIMP_TYPE_PAINT_OPTIONS,
                _("Paintbrush"));
}

GType
gimp_paintbrush_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpPaintbrushClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_paintbrush_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPaintbrush),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_paintbrush_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_CORE,
                                     "GimpPaintbrush",
                                     &info, 0);
    }

  return type;
}

static void
gimp_paintbrush_class_init (GimpPaintbrushClass *klass)
{
  GimpPaintCoreClass *paint_core_class;

  paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  paint_core_class->paint = gimp_paintbrush_paint;
}

static void
gimp_paintbrush_init (GimpPaintbrush *paintbrush)
{
  GimpPaintCore *paint_core;

  paint_core = GIMP_PAINT_CORE (paintbrush);

  paint_core->flags |= CORE_HANDLES_CHANGING_BRUSH;
}

static void
gimp_paintbrush_paint (GimpPaintCore      *paint_core,
                       GimpDrawable       *drawable,
                       GimpPaintOptions   *paint_options,
                       GimpPaintCoreState  paint_state)
{
  switch (paint_state)
    {
    case MOTION_PAINT:
      _gimp_paintbrush_motion (paint_core, drawable, paint_options,
                               GIMP_OPACITY_OPAQUE);
      break;

    default:
      break;
    }
}

void
_gimp_paintbrush_motion (GimpPaintCore    *paint_core,
                         GimpDrawable     *drawable,
                         GimpPaintOptions *paint_options,
                         gdouble           opacity)
{
  GimpPressureOptions      *pressure_options;
  GimpFadeOptions          *fade_options;
  GimpGradientOptions      *gradient_options;
  GimpContext              *context;
  GimpImage                *gimage;
  GimpRGB                   gradient_color;
  TempBuf                  *area;
  guchar                    col[MAX_CHANNELS];
  gdouble                   scale;
  GimpPaintApplicationMode  paint_appl_mode;

  context = GIMP_CONTEXT (paint_options);

  pressure_options = paint_options->pressure_options;
  fade_options     = paint_options->fade_options;
  gradient_options = paint_options->gradient_options;

  if (! (gimage = gimp_item_get_image (GIMP_ITEM (drawable))))
    return;

  opacity *= gimp_paint_options_get_fade (paint_options, gimage,
                                          paint_core->pixel_dist);

  if (opacity == 0.0)
    return;

  paint_appl_mode = paint_options->application_mode;

  if (pressure_options->size)
    scale = paint_core->cur_coords.pressure;
  else
    scale = 1.0;

  /*  Get a region which can be used to paint to  */
  if (! (area = gimp_paint_core_get_paint_area (paint_core, drawable, scale)))
    return;

  if (gimp_paint_options_get_gradient_color (paint_options, gimage,
                                             paint_core->cur_coords.pressure,
                                             paint_core->pixel_dist,
                                             &gradient_color))
    {
      opacity *= gradient_color.a;

      gimp_rgb_get_uchar (&gradient_color,
                          &col[RED_PIX],
                          &col[GREEN_PIX],
                          &col[BLUE_PIX]);
      col[ALPHA_PIX] = OPAQUE_OPACITY;

      color_pixels (temp_buf_data (area), col,
                    area->width * area->height,
                    area->bytes);

      paint_appl_mode = GIMP_PAINT_INCREMENTAL;
    }
  else if (paint_core->brush && paint_core->brush->pixmap)
    {
      /* if it's a pixmap, do pixmap stuff */
      gimp_paint_core_color_area_with_pixmap (paint_core, gimage, drawable,
                                              area,
                                              scale,
                                              gimp_paint_options_get_brush_mode (paint_options));

      paint_appl_mode = GIMP_PAINT_INCREMENTAL;
    }
  else
    {
      gimp_image_get_foreground (gimage, drawable, context, col);
      col[area->bytes - 1] = OPAQUE_OPACITY;
      color_pixels (temp_buf_data (area), col,
                    area->width * area->height,
                    area->bytes);
    }

  if (pressure_options->opacity)
    opacity *= 2.0 * paint_core->cur_coords.pressure;

  gimp_paint_core_paste_canvas (paint_core, drawable,
                                MIN (opacity, GIMP_OPACITY_OPAQUE),
                                gimp_context_get_opacity (context),
                                gimp_context_get_paint_mode (context),
                                gimp_paint_options_get_brush_mode (paint_options),
                                scale,
                                paint_appl_mode);
}
