/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphelp_pdb.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <gtk/gtk.h>

#include "gimp.h"

void
gimp_standard_help_func (gchar *help_data)
{
  gimp_help (gimp_get_progname (), help_data);
}

void
gimp_help (gchar *prog_name,
	   gchar *help_data)
{
  GParam *return_vals;
  gint    nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_help",
                                    &nreturn_vals,
				    PARAM_STRING, prog_name,
                                    PARAM_STRING, help_data,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_plugin_help_register (gchar *help_path)
{
  GParam *return_vals;
  gint    nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_plugin_help_register",
                                    &nreturn_vals,
                                    PARAM_STRING, help_path,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}
