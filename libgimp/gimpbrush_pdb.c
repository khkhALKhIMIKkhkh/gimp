/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpbrush_pdb.c
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

/* NOTE: This file is autogenerated by pdbgen.pl */

#include "config.h"

#include <string.h>

#include "gimp.h"

/**
 * gimp_brush_new:
 * @name: The requested name of the new brush.
 *
 * Creates a new brush
 *
 * This procedure creates a new, uninitialized brush
 *
 * Returns: The actual new brush name.
 *
 * Since: GIMP 2.2
 */
gchar *
gimp_brush_new (const gchar *name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gchar *ret_name = NULL;

  return_vals = gimp_run_procedure ("gimp_brush_new",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    ret_name = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return ret_name;
}

/**
 * gimp_brush_duplicate:
 * @name: The brush name.
 *
 * Duplicates a brush
 *
 * This procedure creates an identical brush by a different name
 *
 * Returns: The name of the brush's copy.
 *
 * Since: GIMP 2.2
 */
gchar *
gimp_brush_duplicate (const gchar *name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gchar *ret_name = NULL;

  return_vals = gimp_run_procedure ("gimp_brush_duplicate",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    ret_name = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return ret_name;
}

/**
 * gimp_brush_is_generated:
 * @name: The brush name.
 *
 * Tests if generated
 *
 * Returns True if this brush is parametric, False for other types
 *
 * Returns: True if the brush is generated.
 *
 * Since: GIMP 2.2
 */
gboolean
gimp_brush_is_generated (const gchar *name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean generated = FALSE;

  return_vals = gimp_run_procedure ("gimp_brush_is_generated",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    generated = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return generated;
}

/**
 * gimp_brush_rename:
 * @name: The brush name.
 * @new_name: The new name of the brush.
 *
 * Rename a brush
 *
 * This procedure renames a brush
 *
 * Returns: The actual new name of the brush.
 *
 * Since: GIMP 2.2
 */
gchar *
gimp_brush_rename (const gchar *name,
		   const gchar *new_name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gchar *ret_name = NULL;

  return_vals = gimp_run_procedure ("gimp_brush_rename",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_STRING, new_name,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    ret_name = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return ret_name;
}

/**
 * gimp_brush_delete:
 * @name: The brush name.
 *
 * Deletes a brush
 *
 * This procedure deletes a brush
 *
 * Returns: TRUE on success.
 *
 * Since: GIMP 2.2
 */
gboolean
gimp_brush_delete (const gchar *name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp_brush_delete",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_brush_is_editable:
 * @name: The brush name.
 *
 * Tests if brush can be edited
 *
 * Returns True if you have permission to change the brush
 *
 * Returns: True if the brush can be edited.
 *
 * Since: GIMP 2.2
 */
gboolean
gimp_brush_is_editable (const gchar *name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean editable = FALSE;

  return_vals = gimp_run_procedure ("gimp_brush_is_editable",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    editable = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return editable;
}

/**
 * gimp_brush_get_info:
 * @name: The brush name.
 * @width: The brush width.
 * @height: The brush height.
 * @mask_bpp: The brush mask bpp.
 * @color_bpp: The brush color bpp.
 *
 * Retrieve information about the specified brush.
 *
 * This procedure retrieves information about the specified brush. This
 * includes the brush name, and the brush extents (width and height).
 *
 * Returns: TRUE on success.
 *
 * Since: GIMP 2.2
 */
gboolean
gimp_brush_get_info (const gchar *name,
		     gint        *width,
		     gint        *height,
		     gint        *mask_bpp,
		     gint        *color_bpp)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp_brush_get_info",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  *width = 0;
  *height = 0;
  *mask_bpp = 0;
  *color_bpp = 0;

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  if (success)
    {
      *width = return_vals[1].data.d_int32;
      *height = return_vals[2].data.d_int32;
      *mask_bpp = return_vals[3].data.d_int32;
      *color_bpp = return_vals[4].data.d_int32;
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_brush_get_pixels:
 * @name: The brush name.
 * @width: The brush width.
 * @height: The brush height.
 * @mask_bpp: The brush mask bpp.
 * @num_mask_bytes: Length of brush mask data.
 * @mask_bytes: The brush mask data.
 * @color_bpp: The brush color bpp.
 * @num_color_bytes: Length of brush color data.
 * @color_bytes: The brush color data.
 *
 * Retrieve information about the specified brush.
 *
 * This procedure retrieves information about the specified brush. This
 * includes the brush extents (width and height) and its pixels data.
 *
 * Returns: TRUE on success.
 *
 * Since: GIMP 2.2
 */
gboolean
gimp_brush_get_pixels (const gchar  *name,
		       gint         *width,
		       gint         *height,
		       gint         *mask_bpp,
		       gint         *num_mask_bytes,
		       guint8      **mask_bytes,
		       gint         *color_bpp,
		       gint         *num_color_bytes,
		       guint8      **color_bytes)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp_brush_get_pixels",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  *width = 0;
  *height = 0;
  *mask_bpp = 0;
  *num_mask_bytes = 0;
  *mask_bytes = NULL;
  *color_bpp = 0;
  *num_color_bytes = 0;
  *color_bytes = NULL;

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  if (success)
    {
      *width = return_vals[1].data.d_int32;
      *height = return_vals[2].data.d_int32;
      *mask_bpp = return_vals[3].data.d_int32;
      *num_mask_bytes = return_vals[4].data.d_int32;
      *mask_bytes = g_new (guint8, *num_mask_bytes);
      memcpy (*mask_bytes, return_vals[5].data.d_int8array,
	      *num_mask_bytes * sizeof (guint8));
      *color_bpp = return_vals[6].data.d_int32;
      *num_color_bytes = return_vals[7].data.d_int32;
      *color_bytes = g_new (guint8, *num_color_bytes);
      memcpy (*color_bytes, return_vals[8].data.d_int8array,
	      *num_color_bytes * sizeof (guint8));
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_brush_get_spacing:
 * @name: The brush name.
 * @spacing: The brush spacing.
 *
 * Get the brush spacing.
 *
 * This procedure returns the spacing setting for the specified brush.
 * The return value is an integer between 0 and 1000 which represents
 * percentage of the maximum of the width and height of the mask.
 *
 * Returns: TRUE on success.
 *
 * Since: GIMP 2.2
 */
gboolean
gimp_brush_get_spacing (const gchar *name,
			gint        *spacing)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp_brush_get_spacing",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  *spacing = 0;

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  if (success)
    *spacing = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_brush_set_spacing:
 * @name: The brush name.
 * @spacing: The brush spacing.
 *
 * Set the brush spacing.
 *
 * This procedure modifies the spacing setting for the specified brush.
 * The value should be a integer between 0 and 1000.
 *
 * Returns: TRUE on success.
 *
 * Since: GIMP 2.2
 */
gboolean
gimp_brush_set_spacing (const gchar *name,
			gint         spacing)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp_brush_set_spacing",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_INT32, spacing,
				    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_brush_get_shape:
 * @name: The brush name.
 *
 * Get the shape of a generated brush.
 *
 * This procedure gets the shape value for a generated brush. If called
 * for any other type of brush, it does not succeed. The current
 * possibilities are Circle (GIMP_BRUSH_GENERATED_CIRCLE), Square
 * (GIMP_BRUSH_GENERATED_SQUARE), and Diamond
 * (GIMP_BRUSH_GENERATED_DIAMOND). Other shapes are likely to be added
 * in the future.
 *
 * Returns: An enumerated value representing the brush shape.
 *
 * Since: GIMP 2.2
 */
gint
gimp_brush_get_shape (const gchar *name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gint shape = 0;

  return_vals = gimp_run_procedure ("gimp_brush_get_shape",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    shape = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return shape;
}

/**
 * gimp_brush_get_radius:
 * @name: The brush name.
 *
 * Get the radius of a generated brush.
 *
 * This procedure gets the radius value for a generated brush. If
 * called for any other type of brush, it does not succeed.
 *
 * Returns: The radius of the brush in pixels.
 *
 * Since: GIMP 2.2
 */
gdouble
gimp_brush_get_radius (const gchar *name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gdouble radius = 0;

  return_vals = gimp_run_procedure ("gimp_brush_get_radius",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    radius = return_vals[1].data.d_float;

  gimp_destroy_params (return_vals, nreturn_vals);

  return radius;
}

/**
 * gimp_brush_get_spikes:
 * @name: The brush name.
 *
 * Get the number of spikes for a generated brush.
 *
 * This procedure gets the number of spikes for a generated brush. If
 * called for any other type of brush, it does not succeed.
 *
 * Returns: The number of spikes on the brush.
 *
 * Since: GIMP 2.2
 */
gint
gimp_brush_get_spikes (const gchar *name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gint spikes = 0;

  return_vals = gimp_run_procedure ("gimp_brush_get_spikes",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    spikes = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return spikes;
}

/**
 * gimp_brush_get_hardness:
 * @name: The brush name.
 *
 * Get the hardness of a generated brush.
 *
 * This procedure gets the hardness of a generated brush. The hardness
 * of a brush is the amount its intensity fades at the outside edge. If
 * called for any other type of brush, the function does not succeed.
 *
 * Returns: The hardness of the brush.
 *
 * Since: GIMP 2.2
 */
gdouble
gimp_brush_get_hardness (const gchar *name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gdouble hardness = 0;

  return_vals = gimp_run_procedure ("gimp_brush_get_hardness",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    hardness = return_vals[1].data.d_float;

  gimp_destroy_params (return_vals, nreturn_vals);

  return hardness;
}

/**
 * gimp_brush_get_aspect_ratio:
 * @name: The brush name.
 *
 * Get the aspect ratio of a generated brush.
 *
 * This procedure gets the aspect ratio of a generated brush. If called
 * for any other type of brush, it does not succeed.
 *
 * Returns: The aspect ratio of the brush.
 *
 * Since: GIMP 2.2
 */
gdouble
gimp_brush_get_aspect_ratio (const gchar *name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gdouble aspect_ratio = 0;

  return_vals = gimp_run_procedure ("gimp_brush_get_aspect_ratio",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    aspect_ratio = return_vals[1].data.d_float;

  gimp_destroy_params (return_vals, nreturn_vals);

  return aspect_ratio;
}

/**
 * gimp_brush_get_angle:
 * @name: The brush name.
 *
 * Get the rotation angle of a generated brush.
 *
 * This procedure gets the angle of rotation for a generated brush. If
 * called for any other type of brush, it does not succeed.
 *
 * Returns: The rotation angle of the brush.
 *
 * Since: GIMP 2.2
 */
gdouble
gimp_brush_get_angle (const gchar *name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gdouble angle = 0;

  return_vals = gimp_run_procedure ("gimp_brush_get_angle",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    angle = return_vals[1].data.d_float;

  gimp_destroy_params (return_vals, nreturn_vals);

  return angle;
}

/**
 * gimp_brush_set_shape:
 * @name: The brush name.
 * @shape_in: An enumerated value representing the desired brush shape.
 *
 * Set the shape of a generated brush.
 *
 * This procedure sets the shape value for a generated brush. If called
 * for any other type of brush, it does not succeed. The current
 * possibilities are Circle (GIMP_BRUSH_GENERATED_CIRCLE), Square
 * (GIMP_BRUSH_GENERATED_SQUARE), and Diamond
 * (GIMP_BRUSH_GENERATED_DIAMOND). Other shapes are likely to be added
 * in the future.
 *
 * Returns: The brush shape actually assigned.
 *
 * Since: GIMP 2.2
 */
gint
gimp_brush_set_shape (const gchar *name,
		      gint         shape_in)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gint shape_out = 0;

  return_vals = gimp_run_procedure ("gimp_brush_set_shape",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_INT32, shape_in,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    shape_out = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return shape_out;
}

/**
 * gimp_brush_set_radius:
 * @name: The brush name.
 * @radius_in: The desired brush radius.
 *
 * Set the radius of a generated brush.
 *
 * This procedure sets the radius for a generated brush. If called for
 * any other type of brush, it does not succeed.
 *
 * Returns: The brush radius actually assigned.
 *
 * Since: GIMP 2.2
 */
gdouble
gimp_brush_set_radius (const gchar *name,
		       gdouble      radius_in)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gdouble radius_out = 0;

  return_vals = gimp_run_procedure ("gimp_brush_set_radius",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_FLOAT, radius_in,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    radius_out = return_vals[1].data.d_float;

  gimp_destroy_params (return_vals, nreturn_vals);

  return radius_out;
}

/**
 * gimp_brush_set_spikes:
 * @name: The brush name.
 * @spikes_in: The desired number of spikes.
 *
 * Set the number of spikes for a generated brush.
 *
 * This procedure sets the number of spikes for a generated brush. If
 * called for any other type of brush, it does not succeed.
 *
 * Returns: The number of spikes actually assigned.
 *
 * Since: GIMP 2.2
 */
gint
gimp_brush_set_spikes (const gchar *name,
		       gint         spikes_in)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gint spikes_out = 0;

  return_vals = gimp_run_procedure ("gimp_brush_set_spikes",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_INT32, spikes_in,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    spikes_out = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return spikes_out;
}

/**
 * gimp_brush_set_hardness:
 * @name: The brush name.
 * @hardness_in: The desired brush hardness.
 *
 * Set the hardness of a generated brush.
 *
 * This procedure sets the hardness for a generated brush. If called
 * for any other type of brush, it does not succeed.
 *
 * Returns: The brush hardness actually assigned.
 *
 * Since: GIMP 2.2
 */
gdouble
gimp_brush_set_hardness (const gchar *name,
			 gdouble      hardness_in)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gdouble hardness_out = 0;

  return_vals = gimp_run_procedure ("gimp_brush_set_hardness",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_FLOAT, hardness_in,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    hardness_out = return_vals[1].data.d_float;

  gimp_destroy_params (return_vals, nreturn_vals);

  return hardness_out;
}

/**
 * gimp_brush_set_aspect_ratio:
 * @name: The brush name.
 * @aspect_ratio_in: The desired brush aspect ratio.
 *
 * Set the aspect ratio of a generated brush.
 *
 * This procedure sets the aspect ratio for a generated brush. If
 * called for any other type of brush, it does not succeed.
 *
 * Returns: The brush aspect ratio actually assigned.
 *
 * Since: GIMP 2.2
 */
gdouble
gimp_brush_set_aspect_ratio (const gchar *name,
			     gdouble      aspect_ratio_in)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gdouble aspect_ratio_out = 0;

  return_vals = gimp_run_procedure ("gimp_brush_set_aspect_ratio",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_FLOAT, aspect_ratio_in,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    aspect_ratio_out = return_vals[1].data.d_float;

  gimp_destroy_params (return_vals, nreturn_vals);

  return aspect_ratio_out;
}

/**
 * gimp_brush_set_angle:
 * @name: The brush name.
 * @angle_in: The desired brush rotation angle.
 *
 * Set the rotation angle of a generated brush.
 *
 * This procedure sets the rotation angle for a generated brush. If
 * called for any other type of brush, it does not succeed.
 *
 * Returns: The brush rotation angle actually assigned.
 *
 * Since: GIMP 2.2
 */
gdouble
gimp_brush_set_angle (const gchar *name,
		      gdouble      angle_in)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gdouble angle_out = 0;

  return_vals = gimp_run_procedure ("gimp_brush_set_angle",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_FLOAT, angle_in,
				    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    angle_out = return_vals[1].data.d_float;

  gimp_destroy_params (return_vals, nreturn_vals);

  return angle_out;
}
