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

#include "libgimpcolor/gimpcolor.h"

#include "pdb-types.h"
#include "procedural_db.h"

#include "core/gimpchannel-combine.h"
#include "core/gimpchannel.h"
#include "core/gimpimage.h"

static ProcRecord channel_new_proc;
static ProcRecord channel_new_from_component_proc;
static ProcRecord channel_copy_proc;
static ProcRecord channel_combine_masks_proc;
static ProcRecord channel_get_show_masked_proc;
static ProcRecord channel_set_show_masked_proc;
static ProcRecord channel_get_opacity_proc;
static ProcRecord channel_set_opacity_proc;
static ProcRecord channel_get_color_proc;
static ProcRecord channel_set_color_proc;

void
register_channel_procs (Gimp *gimp)
{
  procedural_db_register (gimp, &channel_new_proc);
  procedural_db_register (gimp, &channel_new_from_component_proc);
  procedural_db_register (gimp, &channel_copy_proc);
  procedural_db_register (gimp, &channel_combine_masks_proc);
  procedural_db_register (gimp, &channel_get_show_masked_proc);
  procedural_db_register (gimp, &channel_set_show_masked_proc);
  procedural_db_register (gimp, &channel_get_opacity_proc);
  procedural_db_register (gimp, &channel_set_opacity_proc);
  procedural_db_register (gimp, &channel_get_color_proc);
  procedural_db_register (gimp, &channel_set_color_proc);
}

static Argument *
channel_new_invoker (Gimp         *gimp,
                     GimpContext  *context,
                     GimpProgress *progress,
                     Argument     *args)
{
  gboolean success = TRUE;
  Argument *return_args;
  GimpImage *gimage;
  gint32 width;
  gint32 height;
  gchar *name;
  gdouble opacity;
  GimpRGB color;
  GimpChannel *channel = NULL;

  gimage = gimp_image_get_by_ID (gimp, args[0].value.pdb_int);
  if (! GIMP_IS_IMAGE (gimage))
    success = FALSE;

  width = args[1].value.pdb_int;
  if (width <= 0)
    success = FALSE;

  height = args[2].value.pdb_int;
  if (height <= 0)
    success = FALSE;

  name = (gchar *) args[3].value.pdb_pointer;
  if (name == NULL || !g_utf8_validate (name, -1, NULL))
    success = FALSE;

  opacity = args[4].value.pdb_float;
  if (opacity < 0.0 || opacity > 100.0)
    success = FALSE;

  color = args[5].value.pdb_color;

  if (success)
    {
      GimpRGB rgb_color = color;

      rgb_color.a = opacity / 100.0;
      channel = gimp_channel_new (gimage, width, height, name, &rgb_color);
      success = channel != NULL;
    }

  return_args = procedural_db_return_args (&channel_new_proc, success);

  if (success)
    return_args[1].value.pdb_int = gimp_item_get_ID (GIMP_ITEM (channel));

  return return_args;
}

static ProcArg channel_new_inargs[] =
{
  {
    GIMP_PDB_IMAGE,
    "image",
    "The image to which to add the channel"
  },
  {
    GIMP_PDB_INT32,
    "width",
    "The channel width: (0 < width)"
  },
  {
    GIMP_PDB_INT32,
    "height",
    "The channel height: (0 < height)"
  },
  {
    GIMP_PDB_STRING,
    "name",
    "The channel name"
  },
  {
    GIMP_PDB_FLOAT,
    "opacity",
    "The channel opacity: (0 <= opacity <= 100)"
  },
  {
    GIMP_PDB_COLOR,
    "color",
    "The channel compositing color"
  }
};

static ProcArg channel_new_outargs[] =
{
  {
    GIMP_PDB_CHANNEL,
    "channel",
    "The newly created channel"
  }
};

static ProcRecord channel_new_proc =
{
  "gimp-channel-new",
  "gimp-channel-new",
  "Create a new channel.",
  "This procedure creates a new channel with the specified width and height. Name, opacity, and color are also supplied parameters. The new channel still needs to be added to the image, as this is not automatic. Add the new channel with the 'gimp_image_add_channel' command. Other attributes such as channel show masked, should be set with explicit procedure calls. The channel's contents are undefined initially.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  NULL,
  GIMP_INTERNAL,
  6,
  channel_new_inargs,
  1,
  channel_new_outargs,
  { { channel_new_invoker } }
};

static Argument *
channel_new_from_component_invoker (Gimp         *gimp,
                                    GimpContext  *context,
                                    GimpProgress *progress,
                                    Argument     *args)
{
  gboolean success = TRUE;
  Argument *return_args;
  GimpImage *gimage;
  gint32 component;
  gchar *name;
  GimpChannel *channel = NULL;

  gimage = gimp_image_get_by_ID (gimp, args[0].value.pdb_int);
  if (! GIMP_IS_IMAGE (gimage))
    success = FALSE;

  component = args[1].value.pdb_int;
  if (component < GIMP_RED_CHANNEL || component > GIMP_ALPHA_CHANNEL)
    success = FALSE;

  name = (gchar *) args[2].value.pdb_pointer;
  if (name == NULL || !g_utf8_validate (name, -1, NULL))
    success = FALSE;

  if (success)
    {
      if (gimp_image_get_component_index (gimage, component) != -1)
        channel = gimp_channel_new_from_component (gimage,
                                                   component, name, NULL);

      if (channel)
        gimp_item_set_visible (GIMP_ITEM (channel), FALSE, FALSE);
      else
        success = FALSE;
    }

  return_args = procedural_db_return_args (&channel_new_from_component_proc, success);

  if (success)
    return_args[1].value.pdb_int = gimp_item_get_ID (GIMP_ITEM (channel));

  return return_args;
}

static ProcArg channel_new_from_component_inargs[] =
{
  {
    GIMP_PDB_IMAGE,
    "image",
    "The image to which to add the channel"
  },
  {
    GIMP_PDB_INT32,
    "component",
    "The image component: { GIMP_RED_CHANNEL (0), GIMP_GREEN_CHANNEL (1), GIMP_BLUE_CHANNEL (2), GIMP_GRAY_CHANNEL (3), GIMP_INDEXED_CHANNEL (4), GIMP_ALPHA_CHANNEL (5) }"
  },
  {
    GIMP_PDB_STRING,
    "name",
    "The channel name"
  }
};

static ProcArg channel_new_from_component_outargs[] =
{
  {
    GIMP_PDB_CHANNEL,
    "channel",
    "The newly created channel"
  }
};

static ProcRecord channel_new_from_component_proc =
{
  "gimp-channel-new-from-component",
  "gimp-channel-new-from-component",
  "Create a new channel from a color component",
  "This procedure creates a new channel from a color component.",
  "Shlomi Fish <shlomif@iglu.org.il>",
  "Shlomi Fish",
  "2005",
  NULL,
  GIMP_INTERNAL,
  3,
  channel_new_from_component_inargs,
  1,
  channel_new_from_component_outargs,
  { { channel_new_from_component_invoker } }
};

static Argument *
channel_copy_invoker (Gimp         *gimp,
                      GimpContext  *context,
                      GimpProgress *progress,
                      Argument     *args)
{
  gboolean success = TRUE;
  Argument *return_args;
  GimpChannel *channel;
  GimpChannel *channel_copy = NULL;

  channel = (GimpChannel *) gimp_item_get_by_ID (gimp, args[0].value.pdb_int);
  if (! (GIMP_IS_CHANNEL (channel) && ! gimp_item_is_removed (GIMP_ITEM (channel))))
    success = FALSE;

  if (success)
    {
      channel_copy = GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (channel),
                                   G_TYPE_FROM_INSTANCE (channel), FALSE));
      success = (channel_copy != NULL);
    }

  return_args = procedural_db_return_args (&channel_copy_proc, success);

  if (success)
    return_args[1].value.pdb_int = gimp_item_get_ID (GIMP_ITEM (channel_copy));

  return return_args;
}

static ProcArg channel_copy_inargs[] =
{
  {
    GIMP_PDB_CHANNEL,
    "channel",
    "The channel to copy"
  }
};

static ProcArg channel_copy_outargs[] =
{
  {
    GIMP_PDB_CHANNEL,
    "channel-copy",
    "The newly copied channel"
  }
};

static ProcRecord channel_copy_proc =
{
  "gimp-channel-copy",
  "gimp-channel-copy",
  "Copy a channel.",
  "This procedure copies the specified channel and returns the copy.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  NULL,
  GIMP_INTERNAL,
  1,
  channel_copy_inargs,
  1,
  channel_copy_outargs,
  { { channel_copy_invoker } }
};

static Argument *
channel_combine_masks_invoker (Gimp         *gimp,
                               GimpContext  *context,
                               GimpProgress *progress,
                               Argument     *args)
{
  gboolean success = TRUE;
  GimpChannel *channel1;
  GimpChannel *channel2;
  gint32 operation;
  gint32 offx;
  gint32 offy;

  channel1 = (GimpChannel *) gimp_item_get_by_ID (gimp, args[0].value.pdb_int);
  if (! (GIMP_IS_CHANNEL (channel1) && ! gimp_item_is_removed (GIMP_ITEM (channel1))))
    success = FALSE;

  channel2 = (GimpChannel *) gimp_item_get_by_ID (gimp, args[1].value.pdb_int);
  if (! (GIMP_IS_CHANNEL (channel2) && ! gimp_item_is_removed (GIMP_ITEM (channel2))))
    success = FALSE;

  operation = args[2].value.pdb_int;
  if (operation < GIMP_CHANNEL_OP_ADD || operation > GIMP_CHANNEL_OP_INTERSECT)
    success = FALSE;

  offx = args[3].value.pdb_int;

  offy = args[4].value.pdb_int;

  if (success)
    {
      gimp_channel_combine_mask (channel1, channel2, operation, offx, offy);
    }

  return procedural_db_return_args (&channel_combine_masks_proc, success);
}

static ProcArg channel_combine_masks_inargs[] =
{
  {
    GIMP_PDB_CHANNEL,
    "channel1",
    "The channel1"
  },
  {
    GIMP_PDB_CHANNEL,
    "channel2",
    "The channel2"
  },
  {
    GIMP_PDB_INT32,
    "operation",
    "The selection operation: { GIMP_CHANNEL_OP_ADD (0), GIMP_CHANNEL_OP_SUBTRACT (1), GIMP_CHANNEL_OP_REPLACE (2), GIMP_CHANNEL_OP_INTERSECT (3) }"
  },
  {
    GIMP_PDB_INT32,
    "offx",
    "x offset between upper left corner of channels: (second - first)"
  },
  {
    GIMP_PDB_INT32,
    "offy",
    "y offset between upper left corner of channels: (second - first)"
  }
};

static ProcRecord channel_combine_masks_proc =
{
  "gimp-channel-combine-masks",
  "gimp-channel-combine-masks",
  "Combine two channel masks.",
  "This procedure combines two channel masks. The result is stored in the first channel.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  NULL,
  GIMP_INTERNAL,
  5,
  channel_combine_masks_inargs,
  0,
  NULL,
  { { channel_combine_masks_invoker } }
};

static Argument *
channel_get_show_masked_invoker (Gimp         *gimp,
                                 GimpContext  *context,
                                 GimpProgress *progress,
                                 Argument     *args)
{
  gboolean success = TRUE;
  Argument *return_args;
  GimpChannel *channel;
  gboolean show_masked = FALSE;

  channel = (GimpChannel *) gimp_item_get_by_ID (gimp, args[0].value.pdb_int);
  if (! (GIMP_IS_CHANNEL (channel) && ! gimp_item_is_removed (GIMP_ITEM (channel))))
    success = FALSE;

  if (success)
    {
      show_masked = gimp_channel_get_show_masked (channel);
    }

  return_args = procedural_db_return_args (&channel_get_show_masked_proc, success);

  if (success)
    return_args[1].value.pdb_int = show_masked;

  return return_args;
}

static ProcArg channel_get_show_masked_inargs[] =
{
  {
    GIMP_PDB_CHANNEL,
    "channel",
    "The channel"
  }
};

static ProcArg channel_get_show_masked_outargs[] =
{
  {
    GIMP_PDB_INT32,
    "show-masked",
    "The channel composite method"
  }
};

static ProcRecord channel_get_show_masked_proc =
{
  "gimp-channel-get-show-masked",
  "gimp-channel-get-show-masked",
  "Get the composite method of the specified channel.",
  "This procedure returns the specified channel's composite method. If it is non-zero, then the channel is composited with the image so that masked regions are shown. Otherwise, selected regions are shown.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  NULL,
  GIMP_INTERNAL,
  1,
  channel_get_show_masked_inargs,
  1,
  channel_get_show_masked_outargs,
  { { channel_get_show_masked_invoker } }
};

static Argument *
channel_set_show_masked_invoker (Gimp         *gimp,
                                 GimpContext  *context,
                                 GimpProgress *progress,
                                 Argument     *args)
{
  gboolean success = TRUE;
  GimpChannel *channel;
  gboolean show_masked;

  channel = (GimpChannel *) gimp_item_get_by_ID (gimp, args[0].value.pdb_int);
  if (! (GIMP_IS_CHANNEL (channel) && ! gimp_item_is_removed (GIMP_ITEM (channel))))
    success = FALSE;

  show_masked = args[1].value.pdb_int ? TRUE : FALSE;

  if (success)
    {
      gimp_channel_set_show_masked (channel, show_masked);
    }

  return procedural_db_return_args (&channel_set_show_masked_proc, success);
}

static ProcArg channel_set_show_masked_inargs[] =
{
  {
    GIMP_PDB_CHANNEL,
    "channel",
    "The channel"
  },
  {
    GIMP_PDB_INT32,
    "show-masked",
    "The new channel composite method"
  }
};

static ProcRecord channel_set_show_masked_proc =
{
  "gimp-channel-set-show-masked",
  "gimp-channel-set-show-masked",
  "Set the composite method of the specified channel.",
  "This procedure sets the specified channel's composite method. If it is non-zero, then the channel is composited with the image so that masked regions are shown. Otherwise, selected regions are shown.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  NULL,
  GIMP_INTERNAL,
  2,
  channel_set_show_masked_inargs,
  0,
  NULL,
  { { channel_set_show_masked_invoker } }
};

static Argument *
channel_get_opacity_invoker (Gimp         *gimp,
                             GimpContext  *context,
                             GimpProgress *progress,
                             Argument     *args)
{
  gboolean success = TRUE;
  Argument *return_args;
  GimpChannel *channel;
  gdouble opacity = 0.0;

  channel = (GimpChannel *) gimp_item_get_by_ID (gimp, args[0].value.pdb_int);
  if (! (GIMP_IS_CHANNEL (channel) && ! gimp_item_is_removed (GIMP_ITEM (channel))))
    success = FALSE;

  if (success)
    {
      opacity = gimp_channel_get_opacity (channel) * 100;
    }

  return_args = procedural_db_return_args (&channel_get_opacity_proc, success);

  if (success)
    return_args[1].value.pdb_float = opacity;

  return return_args;
}

static ProcArg channel_get_opacity_inargs[] =
{
  {
    GIMP_PDB_CHANNEL,
    "channel",
    "The channel"
  }
};

static ProcArg channel_get_opacity_outargs[] =
{
  {
    GIMP_PDB_FLOAT,
    "opacity",
    "The channel opacity"
  }
};

static ProcRecord channel_get_opacity_proc =
{
  "gimp-channel-get-opacity",
  "gimp-channel-get-opacity",
  "Get the opacity of the specified channel.",
  "This procedure returns the specified channel's opacity.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  NULL,
  GIMP_INTERNAL,
  1,
  channel_get_opacity_inargs,
  1,
  channel_get_opacity_outargs,
  { { channel_get_opacity_invoker } }
};

static Argument *
channel_set_opacity_invoker (Gimp         *gimp,
                             GimpContext  *context,
                             GimpProgress *progress,
                             Argument     *args)
{
  gboolean success = TRUE;
  GimpChannel *channel;
  gdouble opacity;

  channel = (GimpChannel *) gimp_item_get_by_ID (gimp, args[0].value.pdb_int);
  if (! (GIMP_IS_CHANNEL (channel) && ! gimp_item_is_removed (GIMP_ITEM (channel))))
    success = FALSE;

  opacity = args[1].value.pdb_float;
  if (opacity < 0.0 || opacity > 100.0)
    success = FALSE;

  if (success)
    {
      gimp_channel_set_opacity (channel, opacity / 100.0, TRUE);
    }

  return procedural_db_return_args (&channel_set_opacity_proc, success);
}

static ProcArg channel_set_opacity_inargs[] =
{
  {
    GIMP_PDB_CHANNEL,
    "channel",
    "The channel"
  },
  {
    GIMP_PDB_FLOAT,
    "opacity",
    "The new channel opacity (0 <= opacity <= 100)"
  }
};

static ProcRecord channel_set_opacity_proc =
{
  "gimp-channel-set-opacity",
  "gimp-channel-set-opacity",
  "Set the opacity of the specified channel.",
  "This procedure sets the specified channel's opacity.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  NULL,
  GIMP_INTERNAL,
  2,
  channel_set_opacity_inargs,
  0,
  NULL,
  { { channel_set_opacity_invoker } }
};

static Argument *
channel_get_color_invoker (Gimp         *gimp,
                           GimpContext  *context,
                           GimpProgress *progress,
                           Argument     *args)
{
  gboolean success = TRUE;
  Argument *return_args;
  GimpChannel *channel;
  GimpRGB color = { 0.0, 0.0, 0.0, 1.0 };

  channel = (GimpChannel *) gimp_item_get_by_ID (gimp, args[0].value.pdb_int);
  if (! (GIMP_IS_CHANNEL (channel) && ! gimp_item_is_removed (GIMP_ITEM (channel))))
    success = FALSE;

  if (success)
    {
      gimp_channel_get_color (channel, &color);
    }

  return_args = procedural_db_return_args (&channel_get_color_proc, success);

  if (success)
    return_args[1].value.pdb_color = color;

  return return_args;
}

static ProcArg channel_get_color_inargs[] =
{
  {
    GIMP_PDB_CHANNEL,
    "channel",
    "The channel"
  }
};

static ProcArg channel_get_color_outargs[] =
{
  {
    GIMP_PDB_COLOR,
    "color",
    "The channel compositing color"
  }
};

static ProcRecord channel_get_color_proc =
{
  "gimp-channel-get-color",
  "gimp-channel-get-color",
  "Get the compositing color of the specified channel.",
  "This procedure returns the specified channel's compositing color.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  NULL,
  GIMP_INTERNAL,
  1,
  channel_get_color_inargs,
  1,
  channel_get_color_outargs,
  { { channel_get_color_invoker } }
};

static Argument *
channel_set_color_invoker (Gimp         *gimp,
                           GimpContext  *context,
                           GimpProgress *progress,
                           Argument     *args)
{
  gboolean success = TRUE;
  GimpChannel *channel;
  GimpRGB color;

  channel = (GimpChannel *) gimp_item_get_by_ID (gimp, args[0].value.pdb_int);
  if (! (GIMP_IS_CHANNEL (channel) && ! gimp_item_is_removed (GIMP_ITEM (channel))))
    success = FALSE;

  color = args[1].value.pdb_color;

  if (success)
    {
      GimpRGB rgb_color = color;

      rgb_color.a = channel->color.a;
      gimp_channel_set_color (channel, &rgb_color, TRUE);
    }

  return procedural_db_return_args (&channel_set_color_proc, success);
}

static ProcArg channel_set_color_inargs[] =
{
  {
    GIMP_PDB_CHANNEL,
    "channel",
    "The channel"
  },
  {
    GIMP_PDB_COLOR,
    "color",
    "The new channel compositing color"
  }
};

static ProcRecord channel_set_color_proc =
{
  "gimp-channel-set-color",
  "gimp-channel-set-color",
  "Set the compositing color of the specified channel.",
  "This procedure sets the specified channel's compositing color.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  NULL,
  GIMP_INTERNAL,
  2,
  channel_set_color_inargs,
  0,
  NULL,
  { { channel_set_color_invoker } }
};
