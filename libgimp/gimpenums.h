/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* NOTE: This file is autogenerated by enumcode.pl */

#ifndef __GIMP_ENUMS_H__
#define __GIMP_ENUMS_H__

G_BEGIN_DECLS

typedef enum
{
  GIMP_ADD_WHITE_MASK,
  GIMP_ADD_BLACK_MASK,
  GIMP_ADD_ALPHA_MASK,
  GIMP_ADD_SELECTION_MASK,
  GIMP_ADD_COPY_MASK
} GimpAddMaskType;

typedef enum
{
  GIMP_FG_BG_RGB_MODE,
  GIMP_FG_BG_HSV_MODE,
  GIMP_FG_TRANSPARENT_MODE,
  GIMP_CUSTOM_MODE
} GimpBlendMode;

typedef enum
{
  GIMP_BRUSH_HARD,
  GIMP_BRUSH_SOFT
} GimpBrushApplicationMode;

typedef enum
{
  GIMP_FG_BUCKET_FILL,
  GIMP_BG_BUCKET_FILL,
  GIMP_PATTERN_BUCKET_FILL
} GimpBucketFillMode;

typedef enum
{
  GIMP_VALUE_LUT,
  GIMP_RED_LUT,
  GIMP_GREEN_LUT,
  GIMP_BLUE_LUT,
  GIMP_ALPHA_LUT
} GimpChannelLutType;

typedef enum
{
  GIMP_CHANNEL_OP_ADD,
  GIMP_CHANNEL_OP_SUBTRACT,
  GIMP_CHANNEL_OP_REPLACE,
  GIMP_CHANNEL_OP_INTERSECT
} GimpChannelOps;

typedef enum
{
  GIMP_RED_CHANNEL,
  GIMP_GREEN_CHANNEL,
  GIMP_BLUE_CHANNEL,
  GIMP_GRAY_CHANNEL,
  GIMP_INDEXED_CHANNEL,
  GIMP_ALPHA_CHANNEL
} GimpChannelType;

typedef enum
{
  GIMP_IMAGE_CLONE,
  GIMP_PATTERN_CLONE
} GimpCloneType;

typedef enum
{
  GIMP_NO_DITHER,
  GIMP_FS_DITHER,
  GIMP_FSLOWBLEED_DITHER,
  GIMP_FIXED_DITHER
} GimpConvertDitherType;

typedef enum
{
  GIMP_MAKE_PALETTE,
  GIMP_REUSE_PALETTE,
  GIMP_WEB_PALETTE,
  GIMP_MONO_PALETTE,
  GIMP_CUSTOM_PALETTE
} GimpConvertPaletteType;

typedef enum
{
  GIMP_NORMAL_CONVOL,
  GIMP_ABSOLUTE_CONVOL,
  GIMP_NEGATIVE_CONVOL
} GimpConvolutionType;

typedef enum
{
  GIMP_BLUR_CONVOLVE,
  GIMP_SHARPEN_CONVOLVE
} GimpConvolveType;

typedef enum
{
  GIMP_DODGE,
  GIMP_BURN
} GimpDodgeBurnType;

typedef enum
{
  GIMP_FOREGROUND_FILL,
  GIMP_BACKGROUND_FILL,
  GIMP_WHITE_FILL,
  GIMP_TRANSPARENT_FILL,
  GIMP_NO_FILL
} GimpFillType;

typedef enum
{
  GIMP_LINEAR,
  GIMP_BILINEAR,
  GIMP_RADIAL,
  GIMP_SQUARE,
  GIMP_CONICAL_SYMMETRIC,
  GIMP_CONICAL_ASYMMETRIC,
  GIMP_SHAPEBURST_ANGULAR,
  GIMP_SHAPEBURST_SPHERICAL,
  GIMP_SHAPEBURST_DIMPLED,
  GIMP_SPIRAL_CLOCKWISE,
  GIMP_SPIRAL_ANTICLOCKWISE
} GimpGradientType;

typedef enum
{
  GIMP_ALL_HUES,
  GIMP_RED_HUES,
  GIMP_YELLOW_HUES,
  GIMP_GREEN_HUES,
  GIMP_CYAN_HUES,
  GIMP_BLUE_HUES,
  GIMP_MAGENTA_HUES
} GimpHueRange;

typedef enum
{
  GIMP_RGB,
  GIMP_GRAY,
  GIMP_INDEXED
} GimpImageBaseType;

typedef enum
{
  GIMP_RGB_IMAGE,
  GIMP_RGBA_IMAGE,
  GIMP_GRAY_IMAGE,
  GIMP_GRAYA_IMAGE,
  GIMP_INDEXED_IMAGE,
  GIMP_INDEXEDA_IMAGE
} GimpImageType;

typedef enum
{
  GIMP_NORMAL_MODE,
  GIMP_DISSOLVE_MODE,
  GIMP_BEHIND_MODE,
  GIMP_MULTIPLY_MODE,
  GIMP_SCREEN_MODE,
  GIMP_OVERLAY_MODE,
  GIMP_DIFFERENCE_MODE,
  GIMP_ADDITION_MODE,
  GIMP_SUBTRACT_MODE,
  GIMP_DARKEN_ONLY_MODE,
  GIMP_LIGHTEN_ONLY_MODE,
  GIMP_HUE_MODE,
  GIMP_SATURATION_MODE,
  GIMP_COLOR_MODE,
  GIMP_VALUE_MODE,
  GIMP_DIVIDE_MODE,
  GIMP_DODGE_MODE,
  GIMP_BURN_MODE,
  GIMP_HARDLIGHT_MODE,
  GIMP_SOFTLIGHT_MODE,
  GIMP_GRAIN_EXTRACT_MODE,
  GIMP_GRAIN_MERGE_MODE,
  GIMP_COLOR_ERASE_MODE
} GimpLayerModeEffects;

typedef enum
{
  GIMP_MASK_APPLY,
  GIMP_MASK_DISCARD
} GimpMaskApplyMode;

typedef enum
{
  GIMP_EXPAND_AS_NECESSARY,
  GIMP_CLIP_TO_IMAGE,
  GIMP_CLIP_TO_BOTTOM_LAYER,
  GIMP_FLATTEN_IMAGE
} GimpMergeType;

typedef enum
{
  GIMP_OFFSET_BACKGROUND,
  GIMP_OFFSET_TRANSPARENT
} GimpOffsetType;

typedef enum
{
  GIMP_ORIENTATION_HORIZONTAL,
  GIMP_ORIENTATION_VERTICAL,
  GIMP_ORIENTATION_UNKNOWN
} GimpOrientationType;

typedef enum
{
  GIMP_PAINT_CONSTANT,
  GIMP_PAINT_INCREMENTAL
} GimpPaintApplicationMode;

typedef enum
{
  GIMP_REPEAT_NONE,
  GIMP_REPEAT_SAWTOOTH,
  GIMP_REPEAT_TRIANGULAR
} GimpRepeatMode;

typedef enum
{
  GIMP_RUN_INTERACTIVE,
  GIMP_RUN_NONINTERACTIVE,
  GIMP_RUN_WITH_LAST_VALS
} GimpRunMode;

typedef enum
{
  GIMP_SHADOWS,
  GIMP_MIDTONES,
  GIMP_HIGHLIGHTS
} GimpTransferMode;

typedef enum
{
  GIMP_PIXELS,
  GIMP_POINTS
} GimpSizeType;


G_END_DECLS

#endif /* __GIMP_ENUMS_H__ */
