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

#include "convert.h"
#include "gimage.h"
#include "palette.h"

static ProcRecord convert_rgb_proc;
static ProcRecord convert_grayscale_proc;
static ProcRecord convert_indexed_proc;
static ProcRecord convert_indexed_palette_proc;

void
register_convert_procs (void)
{
  procedural_db_register (&convert_rgb_proc);
  procedural_db_register (&convert_grayscale_proc);
  procedural_db_register (&convert_indexed_proc);
  procedural_db_register (&convert_indexed_palette_proc);
}

static Argument *
convert_rgb_invoker (Argument *args)
{
  gboolean success = TRUE;
  GimpImage *gimage;

  gimage = pdb_id_to_image (args[0].value.pdb_int);
  if (gimage == NULL)
    success = FALSE;

  if (success)
    if ((success = (gimage_base_type (gimage) != RGB)))
      convert_image ((void *) gimage, RGB, 0, 0, 0);

  return procedural_db_return_args (&convert_rgb_proc, success);
}

static ProcArg convert_rgb_inargs[] =
{
  {
    PDB_IMAGE,
    "image",
    "The image"
  }
};

static ProcRecord convert_rgb_proc =
{
  "gimp_convert_rgb",
  "Convert specified image to RGB color",
  "This procedure converts the specified image to RGB color. This process requires an image of type GRAY or INDEXED. No image content is lost in this process aside from the colormap for an indexed image.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,
  1,
  convert_rgb_inargs,
  0,
  NULL,
  { { convert_rgb_invoker } }
};

static Argument *
convert_grayscale_invoker (Argument *args)
{
  gboolean success = TRUE;
  GimpImage *gimage;

  gimage = pdb_id_to_image (args[0].value.pdb_int);
  if (gimage == NULL)
    success = FALSE;

  if (success)
    if ((success = (gimage_base_type (gimage) != GRAY)))
      convert_image ((void *) gimage, GRAY, 0, 0, 0);

  return procedural_db_return_args (&convert_grayscale_proc, success);
}

static ProcArg convert_grayscale_inargs[] =
{
  {
    PDB_IMAGE,
    "image",
    "The image"
  }
};

static ProcRecord convert_grayscale_proc =
{
  "gimp_convert_grayscale",
  "Convert specified image to grayscale (256 intensity levels)",
  "This procedure converts the specified image to grayscale with 8 bits per pixel (256 intensity levels). This process requires an image of type RGB or INDEXED.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,
  1,
  convert_grayscale_inargs,
  0,
  NULL,
  { { convert_grayscale_invoker } }
};

static Argument *
convert_indexed_invoker (Argument *args)
{
  gboolean success = TRUE;
  GimpImage *gimage;
  gboolean dither;
  gint32 num_cols;

  gimage = pdb_id_to_image (args[0].value.pdb_int);
  if (gimage == NULL)
    success = FALSE;

  dither = args[1].value.pdb_int ? TRUE : FALSE;

  num_cols = args[2].value.pdb_int;

  if (success)
    {
      success = gimage_base_type (gimage) != INDEXED;
    
      if (num_cols < 1 || num_cols > MAXNUMCOLORS)
	success = FALSE;
    
      if (success)
	convert_image ((void *) gimage, INDEXED, num_cols, dither, 0);
    }

  return procedural_db_return_args (&convert_indexed_proc, success);
}

static ProcArg convert_indexed_inargs[] =
{
  {
    PDB_IMAGE,
    "image",
    "The image"
  },
  {
    PDB_INT32,
    "dither",
    "Floyd-Steinberg dithering"
  },
  {
    PDB_INT32,
    "num_cols",
    "the number of colors to quantize to"
  }
};

static ProcRecord convert_indexed_proc =
{
  "gimp_convert_indexed",
  "Convert specified image to indexed color",
  "This procedure converts the specified image to indexed color. This process requires an image of type GRAY or RGB. The 'num_cols' arguments specifies how many colors the resulting image should be quantized to (1-256).",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,
  3,
  convert_indexed_inargs,
  0,
  NULL,
  { { convert_indexed_invoker } }
};

static Argument *
convert_indexed_palette_invoker (Argument *args)
{
  gboolean success = TRUE;
  GimpImage *gimage;
  gboolean dither;
  gint32 palette_type;
  gint32 num_cols;
  gchar *palette_name;

  gimage = pdb_id_to_image (args[0].value.pdb_int);
  if (gimage == NULL)
    success = FALSE;

  dither = args[1].value.pdb_int ? TRUE : FALSE;

  palette_type = args[2].value.pdb_int;

  num_cols = args[3].value.pdb_int;

  palette_name = (gchar *) args[4].value.pdb_pointer;
  if (palette_name == NULL)
    success = FALSE;

  if (success)
    {
      if ((success = (gimage_base_type (gimage) != INDEXED)))
	{
	  PaletteEntriesP entries, the_palette = NULL;
	  GSList *list;
    
	  switch (palette_type)
	    {
	    case MAKE_PALETTE:
	      if (num_cols < 1 || num_cols > MAXNUMCOLORS)
		success = FALSE;
	      break;
    
	    case REUSE_PALETTE:
	    case WEB_PALETTE:
	    case MONO_PALETTE:
	      break;
    
	    case CUSTOM_PALETTE:
	      if (!palette_entries_list)
		palette_init_palettes (FALSE);
    
	      for (list = palette_entries_list; list; list = list->next)
		{
		  entries = (PaletteEntriesP) list->data;
		  if (!strcmp (palette_name, entries->name))
		    {
		      the_palette = entries;
		      break;
		    }
		}
    
	      if (the_palette == NULL)
		success = FALSE;
	      else
		theCustomPalette = the_palette;
    
	      break;
    
	    default:
	      success = FALSE;
	    }
	}
    
      if (success)
	convert_image ((void *) gimage, INDEXED, num_cols, dither, palette_type);
    }

  return procedural_db_return_args (&convert_indexed_palette_proc, success);
}

static ProcArg convert_indexed_palette_inargs[] =
{
  {
    PDB_IMAGE,
    "image",
    "The image"
  },
  {
    PDB_INT32,
    "dither",
    "Floyd-Steinberg dithering"
  },
  {
    PDB_INT32,
    "palette_type",
    "The type of palette to use: { MAKE_PALETTE (0), REUSE_PALETTE (1), WEB_PALETTE (2), MONO_PALETTE (3), CUSTOM_PALETTE (4) }"
  },
  {
    PDB_INT32,
    "num_cols",
    "the number of colors to quantize to, ignored unless (palette_type == MAKE_PALETTE)"
  },
  {
    PDB_STRING,
    "palette",
    "The name of the custom palette to use, ignored unless (palette_type == CUSTOM_PALETTE)"
  }
};

static ProcRecord convert_indexed_palette_proc =
{
  "gimp_convert_indexed_palette",
  "Convert specified image to indexed color",
  "This procedure converts the specified image to indexed color. This process requires an image of type GRAY or RGB. The 'palette_type' specifies what kind of palette to use, A type of '0' means to use an optimal palette of 'num_cols' generated from the colors in the image. A type of '1' means to re-use the previous palette. A type of '2' means to use the WWW-optimized palette. Type '3' means to use only black and white colors. A type of '4' means to use a palette from the gimp palettes directories.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,
  5,
  convert_indexed_palette_inargs,
  0,
  NULL,
  { { convert_indexed_palette_invoker } }
};
