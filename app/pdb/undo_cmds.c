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


#include <glib-object.h>

#include "libgimpbase/gimpbasetypes.h"

#include "pdb-types.h"
#include "procedural_db.h"

#include "core/gimp.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage.h"
#include "plug-in/plug-in.h"

static ProcRecord image_undo_group_start_proc;
static ProcRecord image_undo_group_end_proc;
static ProcRecord image_undo_is_enabled_proc;
static ProcRecord image_undo_disable_proc;
static ProcRecord image_undo_enable_proc;
static ProcRecord image_undo_freeze_proc;
static ProcRecord image_undo_thaw_proc;

void
register_undo_procs (Gimp *gimp)
{
  procedural_db_register (gimp, &image_undo_group_start_proc);
  procedural_db_register (gimp, &image_undo_group_end_proc);
  procedural_db_register (gimp, &image_undo_is_enabled_proc);
  procedural_db_register (gimp, &image_undo_disable_proc);
  procedural_db_register (gimp, &image_undo_enable_proc);
  procedural_db_register (gimp, &image_undo_freeze_proc);
  procedural_db_register (gimp, &image_undo_thaw_proc);
}

static Argument *
image_undo_group_start_invoker (Gimp        *gimp,
                                GimpContext *context,
                                Argument    *args)
{
  gboolean success = TRUE;
  GimpImage *gimage;
  gchar *undo_desc = NULL;

  gimage = gimp_image_get_by_ID (gimp, args[0].value.pdb_int);
  if (! GIMP_IS_IMAGE (gimage))
    success = FALSE;

  if (success)
    {
      if (gimp->current_plug_in)
        undo_desc = plug_in_get_undo_desc (gimp->current_plug_in);

      gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_MISC, undo_desc);

      if (undo_desc)
        g_free (undo_desc);
    }

  return procedural_db_return_args (&image_undo_group_start_proc, success);
}

static ProcArg image_undo_group_start_inargs[] =
{
  {
    GIMP_PDB_IMAGE,
    "image",
    "The ID of the image in which to open an undo group"
  }
};

static ProcRecord image_undo_group_start_proc =
{
  "gimp_image_undo_group_start",
  "Starts a group undo.",
  "This function is used to start a group undo--necessary for logically combining two or more undo operations into a single operation. This call must be used in conjunction with a 'gimp-image-undo-group-end' call.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  GIMP_INTERNAL,
  1,
  image_undo_group_start_inargs,
  0,
  NULL,
  { { image_undo_group_start_invoker } }
};

static Argument *
image_undo_group_end_invoker (Gimp        *gimp,
                              GimpContext *context,
                              Argument    *args)
{
  gboolean success = TRUE;
  GimpImage *gimage;

  gimage = gimp_image_get_by_ID (gimp, args[0].value.pdb_int);
  if (! GIMP_IS_IMAGE (gimage))
    success = FALSE;

  if (success)
    gimp_image_undo_group_end (gimage);

  return procedural_db_return_args (&image_undo_group_end_proc, success);
}

static ProcArg image_undo_group_end_inargs[] =
{
  {
    GIMP_PDB_IMAGE,
    "image",
    "The ID of the image in which to close an undo group"
  }
};

static ProcRecord image_undo_group_end_proc =
{
  "gimp_image_undo_group_end",
  "Finish a group undo.",
  "This function must be called once for each 'gimp-image-undo-group-start' call that is made.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  GIMP_INTERNAL,
  1,
  image_undo_group_end_inargs,
  0,
  NULL,
  { { image_undo_group_end_invoker } }
};

static Argument *
image_undo_is_enabled_invoker (Gimp        *gimp,
                               GimpContext *context,
                               Argument    *args)
{
  gboolean success = TRUE;
  Argument *return_args;
  GimpImage *gimage;
  gboolean enabled = FALSE;

  gimage = gimp_image_get_by_ID (gimp, args[0].value.pdb_int);
  if (! GIMP_IS_IMAGE (gimage))
    success = FALSE;

  if (success)
    enabled = gimp_image_undo_is_enabled (gimage);

  return_args = procedural_db_return_args (&image_undo_is_enabled_proc, success);

  if (success)
    return_args[1].value.pdb_int = enabled;

  return return_args;
}

static ProcArg image_undo_is_enabled_inargs[] =
{
  {
    GIMP_PDB_IMAGE,
    "image",
    "The image"
  }
};

static ProcArg image_undo_is_enabled_outargs[] =
{
  {
    GIMP_PDB_INT32,
    "enabled",
    "True if undo is enabled for this image"
  }
};

static ProcRecord image_undo_is_enabled_proc =
{
  "gimp_image_undo_is_enabled",
  "Check if the image's undo stack is enabled.",
  "This procedure checks if the image's undo stack is currently enabled or disabled. This is useful when several plugins or scripts call each other and want to check if their caller has already used 'gimp_image_undo_disable' or 'gimp_image_undo_freeze'.",
  "Raphael Quinet",
  "Raphael Quinet",
  "1999",
  GIMP_INTERNAL,
  1,
  image_undo_is_enabled_inargs,
  1,
  image_undo_is_enabled_outargs,
  { { image_undo_is_enabled_invoker } }
};

static Argument *
image_undo_disable_invoker (Gimp        *gimp,
                            GimpContext *context,
                            Argument    *args)
{
  gboolean success = TRUE;
  Argument *return_args;
  GimpImage *gimage;

  gimage = gimp_image_get_by_ID (gimp, args[0].value.pdb_int);
  if (! GIMP_IS_IMAGE (gimage))
    success = FALSE;

  if (success)
    success = gimp_image_undo_disable (gimage);

  return_args = procedural_db_return_args (&image_undo_disable_proc, success);

  if (success)
    return_args[1].value.pdb_int = success ? TRUE : FALSE;

  return return_args;
}

static ProcArg image_undo_disable_inargs[] =
{
  {
    GIMP_PDB_IMAGE,
    "image",
    "The image"
  }
};

static ProcArg image_undo_disable_outargs[] =
{
  {
    GIMP_PDB_INT32,
    "disabled",
    "True if the image undo has been disabled"
  }
};

static ProcRecord image_undo_disable_proc =
{
  "gimp_image_undo_disable",
  "Disable the image's undo stack.",
  "This procedure disables the image's undo stack, allowing subsequent operations to ignore their undo steps. This is generally called in conjunction with 'gimp_image_undo_enable' to temporarily disable an image undo stack. This is advantageous because saving undo steps can be time and memory intensive.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  GIMP_INTERNAL,
  1,
  image_undo_disable_inargs,
  1,
  image_undo_disable_outargs,
  { { image_undo_disable_invoker } }
};

static Argument *
image_undo_enable_invoker (Gimp        *gimp,
                           GimpContext *context,
                           Argument    *args)
{
  gboolean success = TRUE;
  Argument *return_args;
  GimpImage *gimage;

  gimage = gimp_image_get_by_ID (gimp, args[0].value.pdb_int);
  if (! GIMP_IS_IMAGE (gimage))
    success = FALSE;

  if (success)
    success = gimp_image_undo_enable (gimage);

  return_args = procedural_db_return_args (&image_undo_enable_proc, success);

  if (success)
    return_args[1].value.pdb_int = success ? TRUE : FALSE;

  return return_args;
}

static ProcArg image_undo_enable_inargs[] =
{
  {
    GIMP_PDB_IMAGE,
    "image",
    "The image"
  }
};

static ProcArg image_undo_enable_outargs[] =
{
  {
    GIMP_PDB_INT32,
    "enabled",
    "True if the image undo has been enabled"
  }
};

static ProcRecord image_undo_enable_proc =
{
  "gimp_image_undo_enable",
  "Enable the image's undo stack.",
  "This procedure enables the image's undo stack, allowing subsequent operations to store their undo steps. This is generally called in conjunction with 'gimp_image_undo_disable' to temporarily disable an image undo stack.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  GIMP_INTERNAL,
  1,
  image_undo_enable_inargs,
  1,
  image_undo_enable_outargs,
  { { image_undo_enable_invoker } }
};

static Argument *
image_undo_freeze_invoker (Gimp        *gimp,
                           GimpContext *context,
                           Argument    *args)
{
  gboolean success = TRUE;
  Argument *return_args;
  GimpImage *gimage;

  gimage = gimp_image_get_by_ID (gimp, args[0].value.pdb_int);
  if (! GIMP_IS_IMAGE (gimage))
    success = FALSE;

  if (success)
    success = gimp_image_undo_freeze (gimage);

  return_args = procedural_db_return_args (&image_undo_freeze_proc, success);

  if (success)
    return_args[1].value.pdb_int = success ? TRUE : FALSE;

  return return_args;
}

static ProcArg image_undo_freeze_inargs[] =
{
  {
    GIMP_PDB_IMAGE,
    "image",
    "The image"
  }
};

static ProcArg image_undo_freeze_outargs[] =
{
  {
    GIMP_PDB_INT32,
    "frozen",
    "True if the image undo has been frozen"
  }
};

static ProcRecord image_undo_freeze_proc =
{
  "gimp_image_undo_freeze",
  "Freeze the image's undo stack.",
  "This procedure freezes the image's undo stack, allowing subsequent operations to ignore their undo steps. This is generally called in conjunction with 'gimp_image_undo_thaw' to temporarily disable an image undo stack. This is advantageous because saving undo steps can be time and memory intensive. 'gimp_image_undo_{freeze,thaw}' and 'gimp_image_undo_{disable,enable}' differ in that the former does not free up all undo steps when undo is thawed, so is more suited to interactive in-situ previews. It is important in this case that the image is back to the same state it was frozen in before thawing, else 'undo' behaviour is undefined.",
  "Adam D. Moss",
  "Adam D. Moss",
  "1999",
  GIMP_INTERNAL,
  1,
  image_undo_freeze_inargs,
  1,
  image_undo_freeze_outargs,
  { { image_undo_freeze_invoker } }
};

static Argument *
image_undo_thaw_invoker (Gimp        *gimp,
                         GimpContext *context,
                         Argument    *args)
{
  gboolean success = TRUE;
  Argument *return_args;
  GimpImage *gimage;

  gimage = gimp_image_get_by_ID (gimp, args[0].value.pdb_int);
  if (! GIMP_IS_IMAGE (gimage))
    success = FALSE;

  if (success)
    success = gimp_image_undo_thaw (gimage);

  return_args = procedural_db_return_args (&image_undo_thaw_proc, success);

  if (success)
    return_args[1].value.pdb_int = success ? TRUE : FALSE;

  return return_args;
}

static ProcArg image_undo_thaw_inargs[] =
{
  {
    GIMP_PDB_IMAGE,
    "image",
    "The image"
  }
};

static ProcArg image_undo_thaw_outargs[] =
{
  {
    GIMP_PDB_INT32,
    "thawed",
    "True if the image undo has been thawed"
  }
};

static ProcRecord image_undo_thaw_proc =
{
  "gimp_image_undo_thaw",
  "Thaw the image's undo stack.",
  "This procedure thaws the image's undo stack, allowing subsequent operations to store their undo steps. This is generally called in conjunction with 'gimp_image_undo_freeze' to temporarily freeze an image undo stack. 'gimp_image_undo_thaw' does NOT free the undo stack as 'gimp_image_undo_enable' does, so is suited for situations where one wishes to leave the undo stack in the same state in which one found it despite non-destructively playing with the image in the meantime. An example would be in-situ plugin previews. Balancing freezes and thaws and ensuring image consistancy is the responsibility of the caller.",
  "Adam D. Moss",
  "Adam D. Moss",
  "1999",
  GIMP_INTERNAL,
  1,
  image_undo_thaw_inargs,
  1,
  image_undo_thaw_outargs,
  { { image_undo_thaw_invoker } }
};
