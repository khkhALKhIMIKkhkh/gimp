/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimppaths_pdb.c
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

#include "gimp.h"

gchar **
gimp_path_list (gint32  image_ID,
		gint   *num_paths)
{
  GParam *return_vals;
  gint nreturn_vals;
  gchar **path_list = NULL;
  gint i;

  return_vals = gimp_run_procedure ("gimp_path_list",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  *num_paths = 0;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *num_paths = return_vals[1].data.d_int32;
      path_list = g_new (gchar *, *num_paths);
      for (i = 0; i < *num_paths; i++)
	path_list[i] = g_strdup (return_vals[2].data.d_stringarray[i]);
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return path_list;
}

gint
gimp_path_get_points (gint32    image_ID,
		      gchar    *pathname,
		      gint     *num_path_point_details,
		      gint     *path_closed,
		      gdouble **points_pairs)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint path_type = 0;

  return_vals = gimp_run_procedure ("gimp_path_get_points",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_STRING, pathname,
				    PARAM_END);

  *num_path_point_details = 0;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      path_type = return_vals[1].data.d_int32;
      *path_closed = return_vals[2].data.d_int32;
      *num_path_point_details = return_vals[3].data.d_int32;
      *points_pairs = g_new (gdouble, *num_path_point_details);
      memcpy (*points_pairs, return_vals[4].data.d_floatarray,
	      *num_path_point_details * sizeof (gdouble));
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return path_type;
}

gchar *
gimp_path_get_current (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gchar *current_path_name = NULL;

  return_vals = gimp_run_procedure ("gimp_path_get_current",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    current_path_name = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return current_path_name;
}

void
gimp_path_set_current (gint32  image_ID,
		       gchar  *set_current_path_name)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_path_set_current",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_STRING, set_current_path_name,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_path_set_points (gint32   image_ID,
		      gchar   *pathname,
		      gint     ptype,
		      gint     num_path_points,
		      gdouble *points_pairs)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_path_set_points",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_STRING, pathname,
				    PARAM_INT32, ptype,
				    PARAM_INT32, num_path_points,
				    PARAM_FLOATARRAY, points_pairs,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_path_stroke_current (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_path_stroke_current",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint
gimp_path_get_point_at_dist (gint32   image_ID,
			     gdouble  distance,
			     gint    *y_point,
			     gdouble *gradient)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint x_point = 0;

  return_vals = gimp_run_procedure ("gimp_path_get_point_at_dist",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_FLOAT, distance,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      x_point = return_vals[1].data.d_int32;
      *y_point = return_vals[2].data.d_int32;
      *gradient = return_vals[3].data.d_float;
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return x_point;
}

gint
gimp_path_get_tattoo (gint32  image_ID,
		      gchar  *pathname)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint tattoo = 0;

  return_vals = gimp_run_procedure ("gimp_path_get_tattoo",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_STRING, pathname,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    tattoo = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return tattoo;
}

gchar *
gimp_get_path_by_tattoo (gint32 image_ID,
			 gint   tattoo)
{
  GParam *return_vals;
  gint nreturn_vals;
  gchar *path_name = NULL;

  return_vals = gimp_run_procedure ("gimp_get_path_by_tattoo",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, tattoo,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    path_name = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return path_name;
}

void
gimp_path_delete (gint32  image_ID,
		  gchar  *path_name_to_del)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_path_delete",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_STRING, path_name_to_del,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint
gimp_path_get_locked (gint32  image_ID,
		      gchar  *pathname)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint lockstatus = 0;

  return_vals = gimp_run_procedure ("gimp_path_get_locked",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_STRING, pathname,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    lockstatus = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return lockstatus;
}

void
gimp_path_set_locked (gint32  image_ID,
		      gchar  *pathname,
		      gint    lockstatus)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_path_set_locked",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_STRING, pathname,
				    PARAM_INT32, lockstatus,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_path_set_tattoo (gint32  image_ID,
		      gchar  *pathname,
		      gint    tattovalue)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_path_set_tattoo",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_STRING, pathname,
				    PARAM_INT32, tattovalue,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}
