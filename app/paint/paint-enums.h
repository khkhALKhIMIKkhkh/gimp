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

#ifndef __PAINT_ENUMS_H__
#define __PAINT_ENUMS_H__

#if 0
   This file is parsed by two scripts, enumgen.pl in tools/pdbgen
   and gimp-mkenums. All enums that are not marked with /*< pdb-skip >*/
   are exported to libgimp and the PDB. Enums that are not marked with
   /*< skip >*/ are registered with the GType system. If you want the
   enum to be skipped by both scripts, you have to use /*< pdb-skip >*/
   _before_ /*< skip >*/. 

   All enum values that are marked with /*< skip >*/ are skipped for
   both targets.
#endif


/* 
 * these enums that are registered with the type system
 */

#define GIMP_TYPE_CLONE_TYPE (gimp_clone_type_get_type ())

GType gimp_clone_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_IMAGE_CLONE,   /*< desc="Image Source"   >*/
  GIMP_PATTERN_CLONE  /*< desc="Pattern Source" >*/
} GimpCloneType;


#define GIMP_TYPE_CLONE_ALIGN_MODE (gimp_clone_align_mode_get_type ())

GType gimp_clone_align_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_CLONE_ALIGN_NO,         /*< desc="Non Aligned" >*/
  GIMP_CLONE_ALIGN_YES,        /*< desc="Aligned"     >*/
  GIMP_CLONE_ALIGN_REGISTERED  /*< desc="Registered"  >*/
} GimpCloneAlignMode;


#define GIMP_TYPE_DODGE_BURN_TYPE (gimp_dodge_burn_type_get_type ())

GType gimp_dodge_burn_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_DODGE,  /*< desc="Dodge" >*/
  GIMP_BURN    /*< desc="Burn"  >*/
} GimpDodgeBurnType;


#define GIMP_TYPE_GRADIENT_PAINT_MODE (gimp_gradient_paint_mode_get_type ())

GType gimp_gradient_paint_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_GRADIENT_ONCE_FORWARD,   /*< desc="Once Forward"  >*/
  GIMP_GRADIENT_ONCE_BACKWARD,  /*< desc="Once Backward" >*/
  GIMP_GRADIENT_LOOP_SAWTOOTH,  /*< desc="Loop Sawtooth" >*/
  GIMP_GRADIENT_LOOP_TRIANGLE   /*< desc="Loop Triangle" >*/
} GimpGradientPaintMode;


#define GIMP_TYPE_CONVOLVE_TYPE (gimp_convolve_type_get_type ())

GType gimp_convolve_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BLUR_CONVOLVE,     /*< desc="Blur"    >*/
  GIMP_SHARPEN_CONVOLVE,  /*< desc="Sharpen" >*/
  GIMP_CUSTOM_CONVOLVE    /*< skip >*/
} GimpConvolveType;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< skip >*/
{
  GIMP_BRUSH_HARD,
  GIMP_BRUSH_SOFT,
  GIMP_BRUSH_PRESSURE  /*< skip >*/
} GimpBrushApplicationMode;

typedef enum  /*< skip >*/
{
  GIMP_PAINT_CONSTANT,
  GIMP_PAINT_INCREMENTAL
} GimpPaintApplicationMode;


#endif /* __PAINT_ENUMS_H__ */
