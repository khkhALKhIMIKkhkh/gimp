/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-params.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpgradient.h"
#include "core/gimppaintinfo.h"

#include "gimppaintoptions.h"


#define DEFAULT_APPLICATION_MODE  GIMP_PAINT_CONSTANT
#define DEFAULT_HARD              FALSE

#define DEFAULT_PRESSURE_OPACITY  TRUE
#define DEFAULT_PRESSURE_HARDNESS FALSE
#define DEFAULT_PRESSURE_RATE     FALSE
#define DEFAULT_PRESSURE_SIZE     FALSE
#define DEFAULT_PRESSURE_COLOR    FALSE
#define DEFAULT_PRESSURE_INVSIZE  FALSE

#define DEFAULT_USE_FADE          FALSE
#define DEFAULT_FADE_LENGTH       100.0
#define DEFAULT_FADE_UNIT         GIMP_UNIT_PIXEL

#define DEFAULT_USE_GRADIENT      FALSE
#define DEFAULT_GRADIENT_REVERSE  FALSE
#define DEFAULT_GRADIENT_REPEAT   GIMP_REPEAT_TRIANGULAR
#define DEFAULT_GRADIENT_LENGTH   100.0
#define DEFAULT_GRADIENT_UNIT     GIMP_UNIT_PIXEL


enum
{
  PROP_0,
  PROP_PAINT_INFO,
  PROP_APPLICATION_MODE,
  PROP_HARD,
  PROP_PRESSURE_OPACITY,
  PROP_PRESSURE_HARDNESS,
  PROP_PRESSURE_RATE,
  PROP_PRESSURE_SIZE,
  PROP_PRESSURE_COLOR,
  PROP_PRESSURE_INVSIZE,
  PROP_USE_FADE,
  PROP_FADE_LENGTH,
  PROP_FADE_UNIT,
  PROP_USE_GRADIENT,
  PROP_GRADIENT_REVERSE,
  PROP_GRADIENT_REPEAT,
  PROP_GRADIENT_LENGTH,
  PROP_GRADIENT_UNIT
};


static void   gimp_paint_options_init         (GimpPaintOptions      *options);
static void   gimp_paint_options_class_init   (GimpPaintOptionsClass *klass);

static void   gimp_paint_options_finalize     (GObject         *object);
static void   gimp_paint_options_set_property (GObject         *object,
                                               guint            property_id,
                                               const GValue    *value,
                                               GParamSpec      *pspec);
static void   gimp_paint_options_get_property (GObject         *object,
                                               guint            property_id,
                                               GValue          *value,
                                               GParamSpec      *pspec);
static void   gimp_paint_options_notify       (GObject         *object,
                                               GParamSpec      *pspec);


static GimpToolOptionsClass *parent_class = NULL;


GType
gimp_paint_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpPaintOptionsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_paint_options_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpPaintOptions),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_paint_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_TOOL_OPTIONS,
                                     "GimpPaintOptions",
                                     &info, 0);
    }

  return type;
}

static void
gimp_paint_options_class_init (GimpPaintOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_paint_options_finalize;
  object_class->set_property = gimp_paint_options_set_property;
  object_class->get_property = gimp_paint_options_get_property;
  object_class->notify       = gimp_paint_options_notify;

  g_object_class_install_property (object_class, PROP_PAINT_INFO,
                                   g_param_spec_object ("paint-info",
                                                        NULL, NULL,
                                                        GIMP_TYPE_PAINT_INFO,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_APPLICATION_MODE,
                                 "application-mode", NULL,
                                 GIMP_TYPE_PAINT_APPLICATION_MODE,
                                 DEFAULT_APPLICATION_MODE,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_HARD,
                                    "hard", NULL,
                                    DEFAULT_HARD,
                                    0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_OPACITY,
                                    "pressure-opacity", NULL,
                                    DEFAULT_PRESSURE_OPACITY,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_HARDNESS,
                                    "pressure-hardness", NULL,
                                    DEFAULT_PRESSURE_HARDNESS,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_RATE,
                                    "pressure-rate", NULL,
                                    DEFAULT_PRESSURE_RATE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_SIZE,
                                    "pressure-size", NULL,
                                    DEFAULT_PRESSURE_SIZE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_COLOR,
                                    "pressure-color", NULL,
                                    DEFAULT_PRESSURE_COLOR,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_INVSIZE,
                                    "pressure-invsize", NULL,
                                    DEFAULT_PRESSURE_INVSIZE,
                                    0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_FADE,
                                    "use-fade", NULL,
                                    DEFAULT_USE_FADE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_FADE_LENGTH,
                                   "fade-length", NULL,
                                   0.0, 32767.0, DEFAULT_FADE_LENGTH,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_FADE_UNIT,
                                 "fade-unit", NULL,
                                 TRUE, TRUE, DEFAULT_FADE_UNIT,
                                 0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_GRADIENT,
                                    "use-gradient", NULL,
                                    DEFAULT_USE_GRADIENT, 0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_GRADIENT_REVERSE,
                                    "gradient-reverse", NULL,
                                    DEFAULT_GRADIENT_REVERSE, 0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_GRADIENT_REPEAT,
                                 "gradient-repeat", NULL,
                                 GIMP_TYPE_REPEAT_MODE,
                                 DEFAULT_GRADIENT_REPEAT,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_GRADIENT_LENGTH,
                                   "gradient-length", NULL,
                                   0.0, 32767.0, DEFAULT_GRADIENT_LENGTH,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_GRADIENT_UNIT,
                                 "gradient-unit", NULL,
                                 TRUE, TRUE, DEFAULT_GRADIENT_UNIT,
                                 0);
}

static void
gimp_paint_options_init (GimpPaintOptions *options)
{
  options->application_mode_save = DEFAULT_APPLICATION_MODE;

  options->pressure_options = g_new0 (GimpPressureOptions, 1);
  options->fade_options     = g_new0 (GimpFadeOptions,     1);
  options->gradient_options = g_new0 (GimpGradientOptions, 1);
}

static void
gimp_paint_options_finalize (GObject *object)
{
  GimpPaintOptions *options = GIMP_PAINT_OPTIONS (object);

  if (options->paint_info)
    g_object_unref (options->paint_info);

  g_free (options->pressure_options);
  g_free (options->fade_options);
  g_free (options->gradient_options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_paint_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpPaintOptions    *options = GIMP_PAINT_OPTIONS (object);
  GimpPressureOptions *pressure_options;
  GimpFadeOptions     *fade_options;
  GimpGradientOptions *gradient_options;

  pressure_options = options->pressure_options;
  fade_options     = options->fade_options;
  gradient_options = options->gradient_options;

  switch (property_id)
    {
    case PROP_PAINT_INFO:
      options->paint_info = (GimpPaintInfo *) g_value_dup_object (value);
      break;

    case PROP_APPLICATION_MODE:
      options->application_mode = g_value_get_enum (value);
      break;
    case PROP_HARD:
      options->hard = g_value_get_boolean (value);
      break;

    case PROP_PRESSURE_OPACITY:
      pressure_options->opacity = g_value_get_boolean (value);
      break;
    case PROP_PRESSURE_HARDNESS:
      pressure_options->hardness = g_value_get_boolean (value);
      break;
    case PROP_PRESSURE_RATE:
      pressure_options->rate = g_value_get_boolean (value);
      break;
    case PROP_PRESSURE_SIZE:
      pressure_options->size = g_value_get_boolean (value);
      break;
    case PROP_PRESSURE_COLOR:
      pressure_options->color = g_value_get_boolean (value);
      break;
    case PROP_PRESSURE_INVSIZE:
      pressure_options->inv_size = g_value_get_boolean (value);
      break;

    case PROP_USE_FADE:
      fade_options->use_fade = g_value_get_boolean (value);
      break;
    case PROP_FADE_LENGTH:
      fade_options->fade_length = g_value_get_double (value);
      break;
    case PROP_FADE_UNIT:
      fade_options->fade_unit = g_value_get_int (value);
      break;

    case PROP_USE_GRADIENT:
      gradient_options->use_gradient = g_value_get_boolean (value);
      break;
    case PROP_GRADIENT_REVERSE:
      gradient_options->gradient_reverse = g_value_get_boolean (value);
      break;
    case PROP_GRADIENT_REPEAT:
      gradient_options->gradient_repeat = g_value_get_enum (value);
      break;
    case PROP_GRADIENT_LENGTH:
      gradient_options->gradient_length = g_value_get_double (value);
      break;
    case PROP_GRADIENT_UNIT:
      gradient_options->gradient_unit = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_paint_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpPaintOptions    *options = GIMP_PAINT_OPTIONS (object);
  GimpPressureOptions *pressure_options;
  GimpFadeOptions     *fade_options;
  GimpGradientOptions *gradient_options;

  pressure_options = options->pressure_options;
  fade_options     = options->fade_options;
  gradient_options = options->gradient_options;

  switch (property_id)
    {
    case PROP_PAINT_INFO:
      g_value_set_object (value, options->paint_info);
      break;

    case PROP_APPLICATION_MODE:
      g_value_set_enum (value, options->application_mode);
      break;
    case PROP_HARD:
      g_value_set_boolean (value, options->hard);
      break;

    case PROP_PRESSURE_OPACITY:
      g_value_set_boolean (value, pressure_options->opacity);
      break;
    case PROP_PRESSURE_HARDNESS:
      g_value_set_boolean (value, pressure_options->hardness);
      break;
    case PROP_PRESSURE_RATE:
      g_value_set_boolean (value, pressure_options->rate);
      break;
    case PROP_PRESSURE_SIZE:
      g_value_set_boolean (value, pressure_options->size);
      break;
    case PROP_PRESSURE_COLOR:
      g_value_set_boolean (value, pressure_options->color);
      break;
    case PROP_PRESSURE_INVSIZE:
      g_value_set_boolean (value, pressure_options->inv_size);
      break;

    case PROP_USE_FADE:
      g_value_set_boolean (value, fade_options->use_fade);
      break;
    case PROP_FADE_LENGTH:
      g_value_set_double (value, fade_options->fade_length);
      break;
    case PROP_FADE_UNIT:
      g_value_set_int (value, fade_options->fade_unit);
      break;

    case PROP_USE_GRADIENT:
      g_value_set_boolean (value, gradient_options->use_gradient);
      break;
    case PROP_GRADIENT_REVERSE:
      g_value_set_boolean (value, gradient_options->gradient_reverse);
      break;
    case PROP_GRADIENT_REPEAT:
      g_value_set_enum (value, gradient_options->gradient_repeat);
      break;
    case PROP_GRADIENT_LENGTH:
      g_value_set_double (value, gradient_options->gradient_length);
      break;
    case PROP_GRADIENT_UNIT:
      g_value_set_int (value, gradient_options->gradient_unit);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_paint_options_notify (GObject    *object,
                           GParamSpec *pspec)
{
  GimpPaintOptions *options = GIMP_PAINT_OPTIONS (object);

  if (pspec->param_id == PROP_USE_GRADIENT)
    {
      if (options->gradient_options->use_gradient)
        {
          options->application_mode_save = options->application_mode;
          options->application_mode      = GIMP_PAINT_INCREMENTAL;
        }
      else
        {
          options->application_mode = options->application_mode_save;
        }

      g_object_notify (object, "application-mode");
    }

  if (G_OBJECT_CLASS (parent_class)->notify)
    G_OBJECT_CLASS (parent_class)->notify (object, pspec);
}

GimpPaintOptions *
gimp_paint_options_new (GimpPaintInfo *paint_info)
{
  GimpPaintOptions *options;

  g_return_val_if_fail (GIMP_IS_PAINT_INFO (paint_info), NULL);

  options = g_object_new (paint_info->paint_options_type,
                          "gimp",       paint_info->gimp,
                          "paint-info", paint_info,
                          NULL);

  return options;
}

gdouble
gimp_paint_options_get_fade (GimpPaintOptions *paint_options,
                             GimpImage        *gimage,
                             gdouble           pixel_dist)
{
  GimpFadeOptions *fade_options;

  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options),
                        GIMP_OPACITY_OPAQUE);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), GIMP_OPACITY_OPAQUE);

  fade_options = paint_options->fade_options;

  if (fade_options->use_fade)
    {
      gdouble fade_out = 0.0;
      gdouble unit_factor;

      switch (fade_options->fade_unit)
        {
        case GIMP_UNIT_PIXEL:
          fade_out = fade_options->fade_length;
          break;
        case GIMP_UNIT_PERCENT:
          fade_out = (MAX (gimage->width, gimage->height) *
                      fade_options->fade_length / 100);
          break;
        default:
          unit_factor = gimp_unit_get_factor (fade_options->fade_unit);
          fade_out    = (fade_options->fade_length *
                         MAX (gimage->xresolution,
                              gimage->yresolution) / unit_factor);
          break;
        }

      /*  factor in the fade out value  */
      if (fade_out > 0.0)
        {
          gdouble x;

          /*  Model the amount of paint left as a gaussian curve  */
          x = pixel_dist / fade_out;

          return exp (- x * x * 5.541);    /*  ln (1/255)  */
        }

      return GIMP_OPACITY_TRANSPARENT;
    }

  return GIMP_OPACITY_OPAQUE;
}

gboolean
gimp_paint_options_get_gradient_color (GimpPaintOptions *paint_options,
                                       GimpImage        *gimage,
                                       gdouble           pressure,
                                       gdouble           pixel_dist,
                                       GimpRGB          *color)
{
  GimpPressureOptions *pressure_options;
  GimpGradientOptions *gradient_options;
  GimpGradient        *gradient;

  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  pressure_options = paint_options->pressure_options;
  gradient_options = paint_options->gradient_options;

  gradient = gimp_context_get_gradient (GIMP_CONTEXT (paint_options));

  if (pressure_options->color)
    {
      gimp_gradient_get_color_at (gradient, pressure,
                                  gradient_options->gradient_reverse,
                                  color);

      return TRUE;
    }
  else if (gradient_options->use_gradient)
    {
      gdouble gradient_length = 0.0;
      gdouble unit_factor;
      gdouble pos;

      switch (gradient_options->gradient_unit)
        {
        case GIMP_UNIT_PIXEL:
          gradient_length = gradient_options->gradient_length;
          break;
        case GIMP_UNIT_PERCENT:
          gradient_length = (MAX (gimage->width, gimage->height) *
                             gradient_options->gradient_length / 100);
          break;
        default:
          unit_factor = gimp_unit_get_factor (gradient_options->gradient_unit);
          gradient_length = (gradient_options->gradient_length *
                             MAX (gimage->xresolution,
                                  gimage->yresolution) / unit_factor);
          break;
        }

      if (gradient_length > 0.0)
        pos = pixel_dist / gradient_length;
      else
        pos = 1.0;

      /*  for no repeat, set pos close to 1.0 after the first chunk  */
      if (gradient_options->gradient_repeat == GIMP_REPEAT_NONE && pos >= 1.0)
        pos = 0.9999999;

      if (((gint) pos & 1) &&
          gradient_options->gradient_repeat != GIMP_REPEAT_SAWTOOTH)
        pos = 1.0 - (pos - (gint) pos);
      else
        pos = pos - (gint) pos;

      gimp_gradient_get_color_at (gradient, pos,
                                  gradient_options->gradient_reverse,
                                  color);

      return TRUE;
    }

  return FALSE;
}

GimpBrushApplicationMode
gimp_paint_options_get_brush_mode (GimpPaintOptions *paint_options)
{
  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), GIMP_BRUSH_SOFT);

  if (paint_options->hard)
    return GIMP_BRUSH_HARD;

  if (paint_options->pressure_options->hardness)
    return GIMP_BRUSH_PRESSURE;

  return GIMP_BRUSH_SOFT;
}
