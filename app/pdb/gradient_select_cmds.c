/* The GIMP -- an image manipulation program
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


#include <gtk/gtk.h>

#include "libgimpbase/gimpbasetypes.h"

#include "pdb-types.h"
#include "gui/gui-types.h"
#include "procedural_db.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"
#include "gui/gradient-select.h"

static ProcRecord gradients_popup_proc;
static ProcRecord gradients_close_popup_proc;
static ProcRecord gradients_set_popup_proc;

void
register_gradient_select_procs (Gimp *gimp)
{
  procedural_db_register (gimp, &gradients_popup_proc);
  procedural_db_register (gimp, &gradients_close_popup_proc);
  procedural_db_register (gimp, &gradients_set_popup_proc);
}

static Argument *
gradients_popup_invoker (Gimp        *gimp,
                         GimpContext *context,
                         Argument    *args)
{
  gboolean success = TRUE;
  gchar *gradient_callback;
  gchar *popup_title;
  gchar *initial_gradient;
  gint32 sample_size;
  ProcRecord *proc;

  gradient_callback = (gchar *) args[0].value.pdb_pointer;
  if (gradient_callback == NULL || !g_utf8_validate (gradient_callback, -1, NULL))
    success = FALSE;

  popup_title = (gchar *) args[1].value.pdb_pointer;
  if (popup_title == NULL || !g_utf8_validate (popup_title, -1, NULL))
    success = FALSE;

  initial_gradient = (gchar *) args[2].value.pdb_pointer;
  if (initial_gradient && !g_utf8_validate (initial_gradient, -1, NULL))
    success = FALSE;

  sample_size = args[3].value.pdb_int;
  if (sample_size <= 0 || sample_size > 10000)
    sample_size = GIMP_GRADIENT_DEFAULT_SAMPLE_SIZE;

  if (success)
    {
      if (! gimp->no_interface &&
          (proc = procedural_db_lookup (gimp, gradient_callback)))
        {
          gradient_select_new (gimp, context, popup_title,
                               initial_gradient, gradient_callback,
                               sample_size);
        }
      else
        success = FALSE;
    }

  return procedural_db_return_args (&gradients_popup_proc, success);
}

static ProcArg gradients_popup_inargs[] =
{
  {
    GIMP_PDB_STRING,
    "gradient_callback",
    "The callback PDB proc to call when gradient selection is made"
  },
  {
    GIMP_PDB_STRING,
    "popup_title",
    "Title to give the gradient popup window"
  },
  {
    GIMP_PDB_STRING,
    "initial_gradient",
    "The name of the pattern to set as the first selected"
  },
  {
    GIMP_PDB_INT32,
    "sample_size",
    "Size of the sample to return when the gradient is changed (0 < sample_size <= 10000)"
  }
};

static ProcRecord gradients_popup_proc =
{
  "gimp_gradients_popup",
  "Invokes the Gimp gradients selection.",
  "This procedure popups the gradients selection dialog.",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  GIMP_INTERNAL,
  4,
  gradients_popup_inargs,
  0,
  NULL,
  { { gradients_popup_invoker } }
};

static Argument *
gradients_close_popup_invoker (Gimp        *gimp,
                               GimpContext *context,
                               Argument    *args)
{
  gboolean success = TRUE;
  gchar *gradient_callback;
  ProcRecord *prec;
  GradientSelect *gradient_select;

  gradient_callback = (gchar *) args[0].value.pdb_pointer;
  if (gradient_callback == NULL || !g_utf8_validate (gradient_callback, -1, NULL))
    success = FALSE;

  if (success)
    {
      if (! gimp->no_interface &&
          (prec = procedural_db_lookup (gimp, gradient_callback)) &&
          (gradient_select = gradient_select_get_by_callback (gradient_callback)))
        {
          gradient_select_free (gradient_select);
        }
      else
        success = FALSE;
    }

  return procedural_db_return_args (&gradients_close_popup_proc, success);
}

static ProcArg gradients_close_popup_inargs[] =
{
  {
    GIMP_PDB_STRING,
    "gradient_callback",
    "The name of the callback registered for this popup"
  }
};

static ProcRecord gradients_close_popup_proc =
{
  "gimp_gradients_close_popup",
  "Popdown the Gimp gradient selection.",
  "This procedure closes an opened gradient selection dialog.",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  GIMP_INTERNAL,
  1,
  gradients_close_popup_inargs,
  0,
  NULL,
  { { gradients_close_popup_invoker } }
};

static Argument *
gradients_set_popup_invoker (Gimp        *gimp,
                             GimpContext *context,
                             Argument    *args)
{
  gboolean success = TRUE;
  gchar *gradient_callback;
  gchar *gradient_name;
  ProcRecord *prec;
  GradientSelect *gradient_select;

  gradient_callback = (gchar *) args[0].value.pdb_pointer;
  if (gradient_callback == NULL || !g_utf8_validate (gradient_callback, -1, NULL))
    success = FALSE;

  gradient_name = (gchar *) args[1].value.pdb_pointer;
  if (gradient_name == NULL || !g_utf8_validate (gradient_name, -1, NULL))
    success = FALSE;

  if (success)
    {
      if (! gimp->no_interface &&
          (prec = procedural_db_lookup (gimp, gradient_callback)) &&
          (gradient_select = gradient_select_get_by_callback (gradient_callback)))
        {
          GimpGradient *active = (GimpGradient *)
            gimp_container_get_child_by_name (gimp->gradient_factory->container,
                                              gradient_name);

          if (active)
            {
              gimp_context_set_gradient (gradient_select->context, active);

              gtk_window_present (GTK_WINDOW (gradient_select->shell));
            }
          else
            success = FALSE;
        }
      else
        success = FALSE;
    }

  return procedural_db_return_args (&gradients_set_popup_proc, success);
}

static ProcArg gradients_set_popup_inargs[] =
{
  {
    GIMP_PDB_STRING,
    "gradient_callback",
    "The name of the callback registered for this popup"
  },
  {
    GIMP_PDB_STRING,
    "gradient_name",
    "The name of the gradient to set as selected"
  }
};

static ProcRecord gradients_set_popup_proc =
{
  "gimp_gradients_set_popup",
  "Sets the current gradient selection in a popup.",
  "Sets the current gradient selection in a popup.",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  GIMP_INTERNAL,
  2,
  gradients_set_popup_inargs,
  0,
  NULL,
  { { gradients_set_popup_invoker } }
};
