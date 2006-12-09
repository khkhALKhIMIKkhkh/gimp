/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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

/* NOTE: This file is autogenerated by pdbgen.pl. */

#include "config.h"


#include <glib-object.h>

#include "pdb-types.h"
#include "gimppdb.h"
#include "gimpprocedure.h"
#include "core/gimpparamspecs.h"

#include "core/gimp.h"
#include "core/gimpdatafactory.h"

#include "internal_procs.h"


static GValueArray *
patterns_popup_invoker (GimpProcedure     *procedure,
                        Gimp              *gimp,
                        GimpContext       *context,
                        GimpProgress      *progress,
                        const GValueArray *args)
{
  gboolean success = TRUE;
  const gchar *pattern_callback;
  const gchar *popup_title;
  const gchar *initial_pattern;

  pattern_callback = g_value_get_string (&args->values[0]);
  popup_title = g_value_get_string (&args->values[1]);
  initial_pattern = g_value_get_string (&args->values[2]);

  if (success)
    {
      if (gimp->no_interface ||
          ! gimp_pdb_lookup_procedure (gimp->pdb, pattern_callback) ||
          ! gimp_pdb_dialog_new (gimp, context, progress,
                                 gimp->pattern_factory->container,
                                 popup_title, pattern_callback, initial_pattern,
                                 NULL))
        success = FALSE;
    }

  return gimp_procedure_get_return_values (procedure, success);
}

static GValueArray *
patterns_close_popup_invoker (GimpProcedure     *procedure,
                              Gimp              *gimp,
                              GimpContext       *context,
                              GimpProgress      *progress,
                              const GValueArray *args)
{
  gboolean success = TRUE;
  const gchar *pattern_callback;

  pattern_callback = g_value_get_string (&args->values[0]);

  if (success)
    {
      if (gimp->no_interface ||
          ! gimp_pdb_lookup_procedure (gimp->pdb, pattern_callback) ||
          ! gimp_pdb_dialog_close (gimp, gimp->pattern_factory->container,
                                   pattern_callback))
        success = FALSE;
    }

  return gimp_procedure_get_return_values (procedure, success);
}

static GValueArray *
patterns_set_popup_invoker (GimpProcedure     *procedure,
                            Gimp              *gimp,
                            GimpContext       *context,
                            GimpProgress      *progress,
                            const GValueArray *args)
{
  gboolean success = TRUE;
  const gchar *pattern_callback;
  const gchar *pattern_name;

  pattern_callback = g_value_get_string (&args->values[0]);
  pattern_name = g_value_get_string (&args->values[1]);

  if (success)
    {
      if (gimp->no_interface ||
          ! gimp_pdb_lookup_procedure (gimp->pdb, pattern_callback) ||
          ! gimp_pdb_dialog_set (gimp, gimp->pattern_factory->container,
                                 pattern_callback, pattern_name,
                                 NULL))
        success = FALSE;
    }

  return gimp_procedure_get_return_values (procedure, success);
}

void
register_pattern_select_procs (GimpPDB *pdb)
{
  GimpProcedure *procedure;

  /*
   * gimp-patterns-popup
   */
  procedure = gimp_procedure_new (patterns_popup_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-patterns-popup");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-patterns-popup",
                                     "Invokes the Gimp pattern selection.",
                                     "This procedure popups the pattern selection dialog.",
                                     "Andy Thomas",
                                     "Andy Thomas",
                                     "1998",
                                     NULL);
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("pattern-callback",
                                                       "pattern callback",
                                                       "The callback PDB proc to call when pattern selection is made",
                                                       FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("popup-title",
                                                       "popup title",
                                                       "Title to give the pattern popup window",
                                                       FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("initial-pattern",
                                                       "initial pattern",
                                                       "The name of the pattern to set as the first selected",
                                                       FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-patterns-close-popup
   */
  procedure = gimp_procedure_new (patterns_close_popup_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-patterns-close-popup");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-patterns-close-popup",
                                     "Popdown the Gimp pattern selection.",
                                     "This procedure closes an opened pattern selection dialog.",
                                     "Andy Thomas",
                                     "Andy Thomas",
                                     "1998",
                                     NULL);
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("pattern-callback",
                                                       "pattern callback",
                                                       "The name of the callback registered for this popup",
                                                       FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-patterns-set-popup
   */
  procedure = gimp_procedure_new (patterns_set_popup_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-patterns-set-popup");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-patterns-set-popup",
                                     "Sets the current pattern selection in a popup.",
                                     "Sets the current pattern selection in a popup.",
                                     "Andy Thomas",
                                     "Andy Thomas",
                                     "1998",
                                     NULL);
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("pattern-callback",
                                                       "pattern callback",
                                                       "The name of the callback registered for this popup",
                                                       FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("pattern-name",
                                                       "pattern name",
                                                       "The name of the pattern to set as selected",
                                                       FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);
}
