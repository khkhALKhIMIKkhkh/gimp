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

/* NOTE: This file is autogenerated by pdbgen.pl. */

#include "procedural_db.h"

#include "undo.h"

static ProcRecord undo_push_group_start_proc;
static ProcRecord undo_push_group_end_proc;

void
register_undo_procs (void)
{
  procedural_db_register (&undo_push_group_start_proc);
  procedural_db_register (&undo_push_group_end_proc);
}

static Argument *
undo_push_group_start_invoker (Argument *args)
{
  gboolean success = TRUE;
  GimpImage *gimage;

  gimage = pdb_id_to_image (args[0].value.pdb_int);
  if (gimage == NULL)
    success = FALSE;

  if (success)
    undo_push_group_start (gimage, MISC_UNDO);

  return procedural_db_return_args (&undo_push_group_start_proc, success);
}

static ProcArg undo_push_group_start_inargs[] =
{
  {
    PDB_IMAGE,
    "image",
    "The ID of the image in which to pop an undo group"
  }
};

static ProcRecord undo_push_group_start_proc =
{
  "gimp_undo_push_group_start",
  "Starts a group undo.",
  "This function is used to start a group undo--necessary for logically combining two or more undo operations into a single operation. This call must be used in conjunction with a 'gimp-undo-push-group-end' call.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,
  1,
  undo_push_group_start_inargs,
  0,
  NULL,
  { { undo_push_group_start_invoker } }
};

static Argument *
undo_push_group_end_invoker (Argument *args)
{
  gboolean success = TRUE;
  GimpImage *gimage;

  gimage = pdb_id_to_image (args[0].value.pdb_int);
  if (gimage == NULL)
    success = FALSE;

  if (success)
    undo_push_group_end (gimage);

  return procedural_db_return_args (&undo_push_group_end_proc, success);
}

static ProcArg undo_push_group_end_inargs[] =
{
  {
    PDB_IMAGE,
    "image",
    "The ID of the image in which to pop an undo group"
  }
};

static ProcRecord undo_push_group_end_proc =
{
  "gimp_undo_push_group_end",
  "Finish a group undo.",
  "This function must be called once for each gimp-undo-push-group call that is made.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,
  1,
  undo_push_group_end_inargs,
  0,
  NULL,
  { { undo_push_group_end_invoker } }
};
