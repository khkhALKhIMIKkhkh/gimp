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

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpcoreconfig.h"
#include "gimpimage.h"
#include "gimpimage-colorhash.h"
#include "gimpimage-mask.h"
#include "gimpimage-projection.h"
#include "gimpimage-undo.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "gimplist.h"
#include "gimpmarshal.h"
#include "gimpparasite.h"
#include "gimpparasitelist.h"
#include "gimpundostack.h"

#include "floating_sel.h"
#include "path.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


#ifdef DEBUG
#define TRC(x) printf x
#else
#define TRC(x)
#endif


enum
{
  MODE_CHANGED,
  ALPHA_CHANGED,
  FLOATING_SELECTION_CHANGED,
  ACTIVE_LAYER_CHANGED,
  ACTIVE_CHANNEL_CHANGED,
  COMPONENT_VISIBILITY_CHANGED,
  COMPONENT_ACTIVE_CHANGED,
  MASK_CHANGED,
  RESOLUTION_CHANGED,
  UNIT_CHANGED,
  QMASK_CHANGED,
  SELECTION_CONTROL,

  CLEAN,
  DIRTY,
  UPDATE,
  UPDATE_GUIDE,
  COLORMAP_CHANGED,
  UNDO_EVENT,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void     gimp_image_class_init            (GimpImageClass *klass);
static void     gimp_image_init                  (GimpImage      *gimage);

static void     gimp_image_dispose               (GObject        *object);
static void     gimp_image_finalize              (GObject        *object);

static void     gimp_image_name_changed          (GimpObject     *object);

static void     gimp_image_invalidate_preview    (GimpViewable   *viewable);
static void     gimp_image_size_changed          (GimpViewable   *viewable);
static TempBuf *gimp_image_get_preview           (GimpViewable   *gimage,
						  gint            width,
						  gint            height);
static TempBuf *gimp_image_get_new_preview       (GimpViewable   *viewable,
						  gint            width, 
						  gint            height);

static void     gimp_image_real_colormap_changed (GimpImage      *gimage,
						  gint            ncol);


static gint valid_combinations[][MAX_CHANNELS + 1] =
{
  /* RGB GIMAGE */
  { -1, -1, -1, COMBINE_INTEN_INTEN, COMBINE_INTEN_INTEN_A },
  /* RGBA GIMAGE */
  { -1, -1, -1, COMBINE_INTEN_A_INTEN, COMBINE_INTEN_A_INTEN_A },
  /* GRAY GIMAGE */
  { -1, COMBINE_INTEN_INTEN, COMBINE_INTEN_INTEN_A, -1, -1 },
  /* GRAYA GIMAGE */
  { -1, COMBINE_INTEN_A_INTEN, COMBINE_INTEN_A_INTEN_A, -1, -1 },
  /* INDEXED GIMAGE */
  { -1, COMBINE_INDEXED_INDEXED, COMBINE_INDEXED_INDEXED_A, -1, -1 },
  /* INDEXEDA GIMAGE */
  { -1, -1, COMBINE_INDEXED_A_INDEXED_A, -1, -1 },
};

static guint gimp_image_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;


GType 
gimp_image_get_type (void) 
{
  static GType image_type = 0;

  if (! image_type)
    {
      static const GTypeInfo image_info =
      {
        sizeof (GimpImageClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_image_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpImage),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_image_init,
      };

      image_type = g_type_register_static (GIMP_TYPE_VIEWABLE,
					   "GimpImage",
					   &image_info, 0);
    }

  return image_type;
}


/*  private functions  */

static void
gimp_image_class_init (GimpImageClass *klass)
{
  GObjectClass      *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_image_signals[MODE_CHANGED] =
    g_signal_new ("mode_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, mode_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  gimp_image_signals[ALPHA_CHANGED] =
    g_signal_new ("alpha_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, alpha_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  gimp_image_signals[FLOATING_SELECTION_CHANGED] =
    g_signal_new ("floating_selection_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, floating_selection_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  gimp_image_signals[ACTIVE_LAYER_CHANGED] =
    g_signal_new ("active_layer_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, active_layer_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  gimp_image_signals[ACTIVE_CHANNEL_CHANGED] =
    g_signal_new ("active_channel_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, active_channel_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  gimp_image_signals[COMPONENT_VISIBILITY_CHANGED] =
    g_signal_new ("component_visibility_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, component_visibility_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__INT,
		  G_TYPE_NONE, 1,
		  G_TYPE_INT);

  gimp_image_signals[COMPONENT_ACTIVE_CHANGED] =
    g_signal_new ("component_active_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, component_active_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__INT,
		  G_TYPE_NONE, 1,
		  G_TYPE_INT);

  gimp_image_signals[MASK_CHANGED] =
    g_signal_new ("mask_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, mask_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  gimp_image_signals[RESOLUTION_CHANGED] =
    g_signal_new ("resolution_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, resolution_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  gimp_image_signals[UNIT_CHANGED] =
    g_signal_new ("unit_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, unit_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  gimp_image_signals[QMASK_CHANGED] =
    g_signal_new ("qmask_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, qmask_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  gimp_image_signals[SELECTION_CONTROL] =
    g_signal_new ("selection_control",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, selection_control),
		  NULL, NULL,
		  gimp_marshal_VOID__INT,
		  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  gimp_image_signals[CLEAN] =
    g_signal_new ("clean",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, clean),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  gimp_image_signals[DIRTY] =
    g_signal_new ("dirty",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, dirty),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  gimp_image_signals[UPDATE] =
    g_signal_new ("update",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, update),
		  NULL, NULL,
		  gimp_marshal_VOID__INT_INT_INT_INT,
		  G_TYPE_NONE, 4,
		  G_TYPE_INT,
		  G_TYPE_INT,
		  G_TYPE_INT,
		  G_TYPE_INT);

  gimp_image_signals[UPDATE_GUIDE] =
    g_signal_new ("update_guide",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, update_guide),
		  NULL, NULL,
		  gimp_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_image_signals[COLORMAP_CHANGED] =
    g_signal_new ("colormap_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, colormap_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__INT,
		  G_TYPE_NONE, 1,
		  G_TYPE_INT);

  gimp_image_signals[UNDO_EVENT] = 
    g_signal_new ("undo_event",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImageClass, undo_event),
		  NULL, NULL,
		  gimp_marshal_VOID__INT,
		  G_TYPE_NONE, 1,
		  G_TYPE_INT);

  object_class->dispose               = gimp_image_dispose;
  object_class->finalize              = gimp_image_finalize;

  gimp_object_class->name_changed     = gimp_image_name_changed;

  viewable_class->invalidate_preview  = gimp_image_invalidate_preview;
  viewable_class->size_changed        = gimp_image_size_changed;
  viewable_class->get_preview         = gimp_image_get_preview;
  viewable_class->get_new_preview     = gimp_image_get_new_preview;

  klass->mode_changed                 = NULL;
  klass->alpha_changed                = NULL;
  klass->floating_selection_changed   = NULL;
  klass->active_layer_changed         = NULL;
  klass->active_channel_changed       = NULL;
  klass->component_visibility_changed = NULL;
  klass->component_active_changed     = NULL;
  klass->mask_changed                 = NULL;

  klass->clean                        = NULL;
  klass->dirty                        = NULL;
  klass->update                       = NULL;
  klass->update_guide                 = NULL;
  klass->colormap_changed             = gimp_image_real_colormap_changed;
  klass->undo_event                   = NULL;
  klass->undo                         = gimp_image_undo;
  klass->redo                         = gimp_image_redo;

  gimp_image_color_hash_init ();
}

static void 
gimp_image_init (GimpImage *gimage)
{
  gimage->ID                    = 0;

  gimage->save_proc             = NULL;

  gimage->width                 = 0;
  gimage->height                = 0;
  gimage->xresolution           = 1.0;
  gimage->yresolution           = 1.0;
  gimage->unit                  = GIMP_UNIT_INCH;
  gimage->base_type             = RGB;

  gimage->cmap                  = NULL;
  gimage->num_cols              = 0;

  gimage->dirty                 = 1;
  gimage->undo_on               = TRUE;

  gimage->instance_count        = 0;
  gimage->disp_count            = 0;

  gimage->tattoo_state          = 0;

  gimage->shadow                = NULL;

  gimage->construct_flag        = FALSE;
  gimage->proj_type             = RGBA_GIMAGE;
  gimage->projection            = NULL;

  gimage->guides                = NULL;

  gimage->layers                = gimp_list_new (GIMP_TYPE_LAYER, 
						 GIMP_CONTAINER_POLICY_STRONG);
  gimage->channels              = gimp_list_new (GIMP_TYPE_CHANNEL, 
						 GIMP_CONTAINER_POLICY_STRONG);
  gimage->layer_stack           = NULL;

  gimage->active_layer          = NULL;
  gimage->active_channel        = NULL;
  gimage->floating_sel          = NULL;
  gimage->selection_mask        = NULL;

  gimage->parasites             = gimp_parasite_list_new ();

  gimage->paths                 = NULL;

  gimage->qmask_state           = FALSE;
  gimage->qmask_color.r         = 1.0;
  gimage->qmask_color.g         = 0.0;
  gimage->qmask_color.b         = 0.0;
  gimage->qmask_color.a         = 0.5;

  gimage->undo_stack            = NULL;
  gimage->redo_stack            = NULL;
  gimage->undo_bytes            = 0;
  gimage->undo_levels           = 0;
  gimage->group_count           = 0;
  gimage->pushing_undo_group    = UNDO_NULL;

  gimage->new_undo_stack        = gimp_undo_stack_new (gimage);
  gimage->new_redo_stack        = gimp_undo_stack_new (gimage);

  gimage->comp_preview          = NULL;
  gimage->comp_preview_valid    = FALSE;
}

static void
gimp_image_dispose (GObject *object)
{
  GimpImage *gimage;

  gimage = GIMP_IMAGE (object);

  undo_free (gimage);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_image_finalize (GObject *object)
{
  GimpImage *gimage;

  gimage = GIMP_IMAGE (object);

  if (gimage->gimp && gimage->gimp->image_table)
    {
      g_hash_table_remove (gimage->gimp->image_table,
			   GINT_TO_POINTER (gimage->ID));
      gimage->gimp = NULL;
    }

  if (gimage->projection)
    gimp_image_projection_free (gimage);

  if (gimage->shadow)
    gimp_image_free_shadow (gimage);

  if (gimage->cmap)
    {
      g_free (gimage->cmap);
      gimage->cmap = NULL;
    }

  if (gimage->layers)
    {
      g_object_unref (G_OBJECT (gimage->layers));
      gimage->layers = NULL;
    }
  if (gimage->channels)
    {
      g_object_unref (G_OBJECT (gimage->channels));
      gimage->channels = NULL;
    }
  if (gimage->layer_stack)
    {
      g_slist_free (gimage->layer_stack);
      gimage->layer_stack = NULL;
    }

  if (gimage->selection_mask)
    {
      g_object_unref (G_OBJECT (gimage->selection_mask));
      gimage->selection_mask = NULL;
    }

  if (gimage->comp_preview)
    {
      temp_buf_free (gimage->comp_preview);
      gimage->comp_preview = NULL;
    }

  if (gimage->parasites)
    {
      g_object_unref (G_OBJECT (gimage->parasites));
      gimage->parasites = NULL;
    }

  if (gimage->guides)
    {
      g_list_foreach (gimage->guides, (GFunc) g_free, NULL);
      g_list_free (gimage->guides);
      gimage->guides = NULL;
    }

  if (gimage->new_undo_stack)
    {
      g_object_unref (G_OBJECT (gimage->new_undo_stack));
      gimage->new_undo_stack = NULL;
    }
  if (gimage->new_redo_stack)
    {
      g_object_unref (G_OBJECT (gimage->new_redo_stack));
      gimage->new_redo_stack = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_image_name_changed (GimpObject *object)
{
  GimpImage   *gimage;
  const gchar *name;

  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);

  gimage = GIMP_IMAGE (object);
  name   = gimp_object_get_name (object);

  if (! (name && strlen (name)))
    {
      g_free (object->name);
      object->name = NULL;
    }
}

static void
gimp_image_invalidate_preview (GimpViewable *viewable)
{
  GimpImage *gimage;

  if (GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview)
    GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview (viewable);

  gimage = GIMP_IMAGE (viewable);

  gimage->comp_preview_valid = FALSE;
}

static void
gimp_image_size_changed (GimpViewable *viewable)
{
  GimpImage *gimage;

  if (GIMP_VIEWABLE_CLASS (parent_class)->size_changed)
    GIMP_VIEWABLE_CLASS (parent_class)->size_changed (viewable);

  gimage = GIMP_IMAGE (viewable);

  gimp_image_invalidate_layer_previews (gimage);
  gimp_image_invalidate_channel_previews (gimage);
}

static TempBuf *
gimp_image_get_preview (GimpViewable *viewable,
			gint          width, 
			gint          height)
{
  GimpImage *gimage;

  gimage = GIMP_IMAGE (viewable);

  if (gimage->comp_preview_valid            &&
      gimage->comp_preview->width  == width &&
      gimage->comp_preview->height == height)
    {
      /*  The easy way  */
      return gimage->comp_preview;
    }
  else
    {
      /*  The hard way  */
      if (gimage->comp_preview)
	temp_buf_free (gimage->comp_preview);

      /*  Actually construct the composite preview from the layer previews!
       *  This might seem ridiculous, but it's actually the best way, given
       *  a number of unsavory alternatives.
       */
      gimage->comp_preview = gimp_image_get_new_preview (viewable,
							 width, height);

      gimage->comp_preview_valid = TRUE;

      return gimage->comp_preview;
    }
}

static TempBuf *
gimp_image_get_new_preview (GimpViewable *viewable,
			    gint          width, 
			    gint          height)
{
  GimpImage   *gimage;
  GimpLayer   *layer;
  GimpLayer   *floating_sel;
  PixelRegion  src1PR, src2PR, maskPR;
  PixelRegion *mask;
  TempBuf     *comp;
  TempBuf     *layer_buf;
  TempBuf     *mask_buf;
  GList       *list;
  GSList      *reverse_list = NULL;
  gdouble      ratio;
  gint         x, y, w, h;
  gint         x1, y1, x2, y2;
  gint         bytes;
  gboolean     construct_flag;
  gint         visible[MAX_CHANNELS] = { 1, 1, 1, 1 };
  gint         off_x, off_y;

  gimage = GIMP_IMAGE (viewable);

  ratio = (gdouble) width / (gdouble) gimage->width;

  switch (gimp_image_base_type (gimage))
    {
    case RGB:
    case INDEXED:
      bytes = 4;
      break;
    case GRAY:
      bytes = 2;
      break;
    default:
      bytes = 0;
      break;
    }

  /*  The construction buffer  */
  comp = temp_buf_new (width, height, bytes, 0, 0, NULL);
  temp_buf_data_clear (comp);

  floating_sel = NULL;

  for (list = GIMP_LIST (gimage->layers)->list; 
       list; 
       list = g_list_next (list))
    {
      layer = (GimpLayer *) list->data;

      /*  only add layers that are visible to the list  */
      if (gimp_drawable_get_visible (GIMP_DRAWABLE (layer)))
	{
	  /*  floating selections are added right above the layer 
	   *  they are attached to
	   */
	  if (gimp_layer_is_floating_sel (layer))
	    {
	      floating_sel = layer;
	    }
	  else
	    {
	      if (floating_sel &&
		  floating_sel->fs.drawable == GIMP_DRAWABLE (layer))
		{
		  reverse_list = g_slist_prepend (reverse_list, floating_sel);
		}

	      reverse_list = g_slist_prepend (reverse_list, layer);
	    }
	}
    }

  construct_flag = FALSE;

  for (; reverse_list; reverse_list = g_slist_next (reverse_list))
    {
      layer = (GimpLayer *) reverse_list->data;

      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

      x = (gint) RINT (ratio * off_x);
      y = (gint) RINT (ratio * off_y);
      w = (gint) RINT (ratio * gimp_drawable_width (GIMP_DRAWABLE (layer)));
      h = (gint) RINT (ratio * gimp_drawable_height (GIMP_DRAWABLE (layer)));

      if (w < 1 || h < 1)
	continue;

      x1 = CLAMP (x, 0, width);
      y1 = CLAMP (y, 0, height);
      x2 = CLAMP (x + w, 0, width);
      y2 = CLAMP (y + h, 0, height);

      src1PR.bytes     = comp->bytes;
      src1PR.x         = x1;
      src1PR.y         = y1;
      src1PR.w         = (x2 - x1);
      src1PR.h         = (y2 - y1);
      src1PR.rowstride = comp->width * src1PR.bytes;
      src1PR.data      = (temp_buf_data (comp) +
                          y1 * src1PR.rowstride + x1 * src1PR.bytes);

      layer_buf = gimp_viewable_get_preview (GIMP_VIEWABLE (layer), w, h);
      src2PR.bytes     = layer_buf->bytes;
      src2PR.w         = src1PR.w;  
      src2PR.h         = src1PR.h;
      src2PR.x         = src1PR.x; 
      src2PR.y         = src1PR.y;
      src2PR.rowstride = layer_buf->width * src2PR.bytes;
      src2PR.data      = (temp_buf_data (layer_buf) + 
                          (y1 - y) * src2PR.rowstride +
                          (x1 - x) * src2PR.bytes);

      if (layer->mask && layer->mask->apply_mask)
	{
	  mask_buf = gimp_viewable_get_preview (GIMP_VIEWABLE (layer->mask),
						w, h);
	  maskPR.bytes     = mask_buf->bytes;
	  maskPR.rowstride = mask_buf->width;
	  maskPR.data      = (mask_buf_data (mask_buf) +
                              (y1 - y) * maskPR.rowstride +
                              (x1 - x) * maskPR.bytes);
	  mask = &maskPR;
	}
      else
	{
	  mask = NULL;
	}

      /*  Based on the type of the layer, project the layer onto the
       *   composite preview...
       *  Indexed images are actually already converted to RGB and RGBA,
       *   so just project them as if they were type "intensity"
       *  Send in all TRUE for visible since that info doesn't matter
       *   for previews
       */
      switch (gimp_drawable_type (GIMP_DRAWABLE (layer)))
	{
	case RGB_GIMAGE: case GRAY_GIMAGE: case INDEXED_GIMAGE:
	  if (! construct_flag)
	    initial_region (&src2PR, &src1PR, 
			    mask, NULL, layer->opacity,
			    layer->mode, visible, INITIAL_INTENSITY);
	  else
	    combine_regions (&src1PR, &src2PR, &src1PR, 
			     mask, NULL, layer->opacity,
			     layer->mode, visible, COMBINE_INTEN_A_INTEN);
	  break;

	case RGBA_GIMAGE: case GRAYA_GIMAGE: case INDEXEDA_GIMAGE:
	  if (! construct_flag)
	    initial_region (&src2PR, &src1PR, 
			    mask, NULL, layer->opacity,
			    layer->mode, visible, INITIAL_INTENSITY_ALPHA);
	  else
	    combine_regions (&src1PR, &src2PR, &src1PR, 
			     mask, NULL, layer->opacity,
			     layer->mode, visible, COMBINE_INTEN_A_INTEN_A);
	  break;

	default:
	  break;
	}

      construct_flag = TRUE;
    }

  g_slist_free (reverse_list);

  return comp;
}

static void 
gimp_image_real_colormap_changed (GimpImage *gimage,
				  gint       ncol)
{
  if (gimp_image_base_type (gimage) == INDEXED)
    {
      /* A colormap alteration affects the whole image */
      gimp_image_update (gimage, 0, 0, gimage->width, gimage->height);

      gimp_image_color_hash_invalidate (gimage, ncol);
    }
}


/*  public functions  */

GimpImage *
gimp_image_new (Gimp              *gimp,
		gint               width,
		gint               height,
		GimpImageBaseType  base_type)
{
  GimpImage *gimage;
  gint       i;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  gimage = GIMP_IMAGE (g_object_new (GIMP_TYPE_IMAGE, NULL));

  gimage->gimp        = gimp;
  gimage->ID          = gimp->next_image_ID++;

  g_hash_table_insert (gimp->image_table,
		       GINT_TO_POINTER (gimage->ID),
		       (gpointer) gimage);

  gimage->width       = width;
  gimage->height      = height;
  gimage->base_type   = base_type;

  gimage->xresolution = gimp->config->default_xresolution;
  gimage->yresolution = gimp->config->default_yresolution;
  gimage->unit        = gimp->config->default_units;

  switch (base_type)
    {
    case RGB:
    case GRAY:
      break;
    case INDEXED:
      /* always allocate 256 colors for the colormap */
      gimage->num_cols = 0;
      gimage->cmap     = (guchar *) g_malloc0 (COLORMAP_SIZE);
      break;
    default:
      break;
    }

  /*  set all color channels visible and active  */
  for (i = 0; i < MAX_CHANNELS; i++)
    {
      gimage->visible[i] = TRUE;
      gimage->active[i]  = TRUE;
    }

  /* create the selection mask */
  gimage->selection_mask = gimp_channel_new_mask (gimage,
						  gimage->width,
						  gimage->height);

  return gimage;
}

GimpImageBaseType
gimp_image_base_type (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);

  return gimage->base_type;
}

GimpImageType
gimp_image_base_type_with_alpha (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);

  switch (gimage->base_type)
    {
    case RGB:
      return RGBA_GIMAGE;
    case GRAY:
      return GRAYA_GIMAGE;
    case INDEXED:
      return INDEXEDA_GIMAGE;
    }

  return RGB_GIMAGE;
}

CombinationMode
gimp_image_get_combination_mode (GimpImageType dest_type,
                                 gint          src_bytes)
{
  return valid_combinations[dest_type][src_bytes];
}

gint
gimp_image_get_ID (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);

  return gimage->ID;
}

GimpImage *
gimp_image_get_by_ID (Gimp *gimp,
		      gint  image_id)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp->image_table == NULL)
    return NULL;

  return (GimpImage *) g_hash_table_lookup (gimp->image_table, 
					    GINT_TO_POINTER (image_id));
}

void
gimp_image_set_filename (GimpImage   *gimage,
			 const gchar *filename)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimp_object_set_name (GIMP_OBJECT (gimage), filename);
}

const gchar *
gimp_image_get_filename (const GimpImage *gimage)
{
  const gchar *filename;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  filename = gimp_object_get_name (GIMP_OBJECT (gimage));

  return filename ? filename : _("Untitled");
}

void
gimp_image_set_save_proc (GimpImage     *gimage, 
			  PlugInProcDef *proc)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimage->save_proc = proc;
}

PlugInProcDef *
gimp_image_get_save_proc (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->save_proc;
}

void
gimp_image_set_resolution (GimpImage *gimage,
			   gdouble    xresolution,
			   gdouble    yresolution)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /* don't allow to set the resolution out of bounds */
  if (xresolution < GIMP_MIN_RESOLUTION || xresolution > GIMP_MAX_RESOLUTION ||
      yresolution < GIMP_MIN_RESOLUTION || yresolution > GIMP_MAX_RESOLUTION)
    return;

  if ((ABS (gimage->xresolution - xresolution) >= 1e-5) ||
      (ABS (gimage->yresolution - yresolution) >= 1e-5))
    {
      undo_push_resolution (gimage);

      gimage->xresolution = xresolution;
      gimage->yresolution = yresolution;

      gimp_image_resolution_changed (gimage);
      gimp_viewable_size_changed (GIMP_VIEWABLE (gimage));
    }
}

void
gimp_image_get_resolution (const GimpImage *gimage,
			   gdouble         *xresolution,
			   gdouble         *yresolution)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (xresolution && yresolution);

  *xresolution = gimage->xresolution;
  *yresolution = gimage->yresolution;
}

void
gimp_image_resolution_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[RESOLUTION_CHANGED], 0);
}

void
gimp_image_set_unit (GimpImage *gimage,
		     GimpUnit   unit)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (gimage->unit != unit)
    {
      undo_push_resolution (gimage);

      gimage->unit = unit;

      gimp_image_unit_changed (gimage);
    }
}

GimpUnit
gimp_image_get_unit (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), GIMP_UNIT_INCH);

  return gimage->unit;
}

void
gimp_image_unit_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[UNIT_CHANGED], 0);
}

void
gimp_image_set_qmask_state (GimpImage *gimage,
                            gboolean   qmask_state)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (qmask_state != gimage->qmask_state)
    {
      gimage->qmask_state = qmask_state ? TRUE : FALSE;

      gimp_image_qmask_changed (gimage);
    }
}

gboolean
gimp_image_get_qmask_state (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  return gimage->qmask_state;
}

void
gimp_image_qmask_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[QMASK_CHANGED], 0);
}

gint
gimp_image_get_width (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), 0);

  return gimage->width;
}

gint
gimp_image_get_height (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), 0);

  return gimage->height;
}

gboolean
gimp_image_is_empty (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), TRUE);

  return (gimp_container_num_children (gimage->layers) == 0);
}

GimpLayer *
gimp_image_floating_sel (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  if (gimage->floating_sel == NULL)
    return NULL;
  else
    return gimage->floating_sel;
}

void
gimp_image_floating_selection_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (gimage),
		 gimp_image_signals[FLOATING_SELECTION_CHANGED], 0);
}

guchar *
gimp_image_get_colormap (const GimpImage *gimage)
{
  return gimp_drawable_cmap (gimp_image_active_drawable (gimage));
}

void
gimp_image_colormap_changed (GimpImage *gimage, 
			     gint       col)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (col < gimage->num_cols);

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[COLORMAP_CHANGED], 0,
		 col);
}

GimpChannel *
gimp_image_get_mask (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->selection_mask;
}

void
gimp_image_mask_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[MASK_CHANGED], 0);
}

void
gimp_image_set_component_active (GimpImage   *gimage, 
				 ChannelType  type, 
				 gboolean     active)
{
  gint pixel = -1;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  switch (type)
    {
    case RED_CHANNEL:     pixel = RED_PIX;     break;
    case GREEN_CHANNEL:   pixel = GREEN_PIX;   break;
    case BLUE_CHANNEL:    pixel = BLUE_PIX;    break;
    case GRAY_CHANNEL:    pixel = GRAY_PIX;    break;
    case INDEXED_CHANNEL: pixel = INDEXED_PIX; break;
    case ALPHA_CHANNEL:
      switch (gimp_image_base_type (gimage))
	{
	case RGB:     pixel = ALPHA_PIX;   break;
	case GRAY:    pixel = ALPHA_G_PIX; break;
	case INDEXED: pixel = ALPHA_I_PIX; break;
	}
      break;

    default:
      break;
    }

  if (pixel != -1 && active != gimage->active[pixel])
    {
      gimage->active[pixel] = active ? TRUE : FALSE;

      /*  If there is an active channel and we mess with the components,
       *  the active channel gets unset...
       */
      gimp_image_unset_active_channel (gimage);

      g_signal_emit (G_OBJECT (gimage),
		     gimp_image_signals[COMPONENT_ACTIVE_CHANGED], 0,
		     type);
    }
}

gboolean
gimp_image_get_component_active (const GimpImage *gimage, 
				 ChannelType      type)
{
  /*  No sanity checking here...  */
  switch (type)
    {
    case RED_CHANNEL:     return gimage->active[RED_PIX];     break;
    case GREEN_CHANNEL:   return gimage->active[GREEN_PIX];   break;
    case BLUE_CHANNEL:    return gimage->active[BLUE_PIX];    break;
    case GRAY_CHANNEL:    return gimage->active[GRAY_PIX];    break;
    case INDEXED_CHANNEL: return gimage->active[INDEXED_PIX]; break;
    case ALPHA_CHANNEL:
      switch (gimp_image_base_type (gimage))
	{
	case RGB:     return gimage->active[ALPHA_PIX];   break;
	case GRAY:    return gimage->active[ALPHA_G_PIX]; break;
	case INDEXED: return gimage->active[ALPHA_I_PIX]; break;
	}
      break;

    default:
      break;
    }

  return FALSE;
}

void
gimp_image_get_active_components (GimpImage         *gimage,
                                  GimpDrawable      *drawable,
                                  gint              *active)
{
  GimpLayer *layer;
  gint       i;

  /*  first, blindly copy the gimage active channels  */
  for (i = 0; i < MAX_CHANNELS; i++)
    active[i] = gimage->active[i];

  /*  If the drawable is a channel (a saved selection, etc.)
   *  make sure that the alpha channel is not valid
   */
  if (GIMP_IS_CHANNEL (drawable))
    {
      active[ALPHA_G_PIX] = 0;  /*  no alpha values in channels  */
    }
  else
    {
      /*  otherwise, check whether preserve transparency is
       *  enabled in the layer and if the layer has alpha
       */
      if (GIMP_IS_LAYER (drawable))
        {
          layer = GIMP_LAYER (drawable);
          if (gimp_layer_has_alpha (layer) && layer->preserve_trans)
            active[gimp_drawable_bytes (drawable) - 1] = 0;
        }
    }
}

void
gimp_image_set_component_visible (GimpImage   *gimage, 
				  ChannelType  type, 
				  gboolean     visible)
{
  gint pixel = -1;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  switch (type)
    {
    case RED_CHANNEL:     pixel = RED_PIX;     break;
    case GREEN_CHANNEL:   pixel = GREEN_PIX;   break;
    case BLUE_CHANNEL:    pixel = BLUE_PIX;    break;
    case GRAY_CHANNEL:    pixel = GRAY_PIX;    break;
    case INDEXED_CHANNEL: pixel = INDEXED_PIX; break;
    case ALPHA_CHANNEL:
      switch (gimp_image_base_type (gimage))
	{
	case RGB:     pixel = ALPHA_PIX;   break;
	case GRAY:    pixel = ALPHA_G_PIX; break;
	case INDEXED: pixel = ALPHA_I_PIX; break;
	}
      break;

    default:
      break;
    }

  if (pixel != -1 && visible != gimage->visible[pixel])
    {
      gimage->visible[pixel] = visible ? TRUE : FALSE;

      g_signal_emit (G_OBJECT (gimage),
		     gimp_image_signals[COMPONENT_VISIBILITY_CHANGED], 0,
		     type);

      gimp_image_update (gimage, 0, 0, gimage->width, gimage->height);
    }
}

gboolean
gimp_image_get_component_visible (const GimpImage *gimage, 
				  ChannelType      type)
{
  /*  No sanity checking here...  */
  switch (type)
    {
    case RED_CHANNEL:     return gimage->visible[RED_PIX];     break;
    case GREEN_CHANNEL:   return gimage->visible[GREEN_PIX];   break;
    case BLUE_CHANNEL:    return gimage->visible[BLUE_PIX];    break;
    case GRAY_CHANNEL:    return gimage->visible[GRAY_PIX];    break;
    case INDEXED_CHANNEL: return gimage->visible[INDEXED_PIX]; break;
    case ALPHA_CHANNEL:
      switch (gimp_image_base_type (gimage))
	{
	case RGB:     return gimage->visible[ALPHA_PIX];   break;
	case GRAY:    return gimage->visible[ALPHA_G_PIX]; break;
	case INDEXED: return gimage->visible[ALPHA_I_PIX]; break;
	}
      break;

    default:
      break;
    }

  return FALSE;
}

void
gimp_image_mode_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[MODE_CHANGED], 0);
}

void
gimp_image_alpha_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[ALPHA_CHANGED], 0);
}

void
gimp_image_update (GimpImage *gimage,
		   gint       x,
		   gint       y,
		   gint       width,
		   gint       height)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[UPDATE], 0,
		 x, y, width, height);
}

void
gimp_image_update_guide (GimpImage *gimage,
                         GimpGuide *guide)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (guide != NULL);

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[UPDATE_GUIDE], 0,
                 guide);
}

void
gimp_image_selection_control (GimpImage            *gimage,
                              GimpSelectionControl  control)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[SELECTION_CONTROL], 0,
                 control);
}


/*  undo  */

gboolean
gimp_image_undo_is_enabled (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  return gimage->undo_on;
}

gboolean
gimp_image_undo_enable (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  /*  Free all undo steps as they are now invalidated  */
  undo_free (gimage);

  return gimp_image_undo_thaw (gimage);
}

gboolean
gimp_image_undo_disable (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  return gimp_image_undo_freeze (gimage);
}

gboolean
gimp_image_undo_freeze (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  gimage->undo_on = FALSE;

  return TRUE;
}

gboolean
gimp_image_undo_thaw (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  gimage->undo_on = TRUE;

  return TRUE;
}

void
gimp_image_undo_event (GimpImage *gimage, 
		       gint       event)
{
  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[UNDO_EVENT], 0,
		 event);
}


/* NOTE about the gimage->dirty counter:
 *   If 0, then the image is clean (ie, copy on disk is the same as the one 
 *      in memory).
 *   If positive, then that's the number of dirtying operations done
 *       on the image since the last save.
 *   If negative, then user has hit undo and gone back in time prior
 *       to the saved copy.  Hitting redo will eventually come back to
 *       the saved copy.
 *
 *   The image is dirty (ie, needs saving) if counter is non-zero.
 *
 *   If the counter is around 10000, this is due to undo-ing back
 *   before a saved version, then mutating the image (thus destroying
 *   the redo stack).  Once this has happened, it's impossible to get
 *   the image back to the state on disk, since the redo info has been
 *   freed.  See undo.c for the gorey details.
 */


/*
 * NEVER CALL gimp_image_dirty() directly!
 *
 * If your code has just dirtied the image, push an undo instead.
 * Failing that, push the trivial undo which tells the user the
 * command is not undoable: undo_push_cantundo() (But really, it would
 * be best to push a proper undo).  If you just dirty the image
 * without pushing an undo then the dirty count is increased, but
 * popping that many undo actions won't lead to a clean image.
 */

gint
gimp_image_dirty (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  gimage->dirty++;

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[DIRTY], 0);

  TRC (("dirty %d -> %d\n", gimage->dirty-1, gimage->dirty));

  return gimage->dirty;
}

gint
gimp_image_clean (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  gimage->dirty--;

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[CLEAN], 0);
  
  TRC (("clean %d -> %d\n", gimage->dirty+1, gimage->dirty));
  
  return gimage->dirty;
}

void
gimp_image_clean_all (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimage->dirty = 0;

  g_signal_emit (G_OBJECT (gimage), gimp_image_signals[CLEAN], 0);
}


/*  color transforms / utilities  */


/* Get rid of these! A "foreground" is an UI concept.. */

void
gimp_image_get_foreground (const GimpImage    *gimage, 
			   const GimpDrawable *drawable, 
			   guchar             *fg)
{
  GimpRGB  color;
  guchar   pfg[3];

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (! drawable || GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (fg != NULL);

  gimp_context_get_foreground (gimp_get_current_context (gimage->gimp), &color);

  gimp_rgb_get_uchar (&color, &pfg[0], &pfg[1], &pfg[2]);

  gimp_image_transform_color (gimage, drawable, pfg, fg, RGB);
}

void
gimp_image_get_background (const GimpImage    *gimage, 
			   const GimpDrawable *drawable, 
			   guchar             *bg)
{
  GimpRGB  color;
  guchar   pbg[3];

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (! drawable || GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (bg != NULL);

  gimp_context_get_background (gimp_get_current_context (gimage->gimp), &color);

  gimp_rgb_get_uchar (&color, &pbg[0], &pbg[1], &pbg[2]);

  gimp_image_transform_color (gimage, drawable, pbg, bg, RGB);
}

void
gimp_image_get_color (const GimpImage *gimage, 
		      GimpImageType    d_type,
		      guchar          *rgb, 
		      guchar          *src)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  switch (d_type)
    {
    case RGB_GIMAGE: case RGBA_GIMAGE:
      map_to_color (0, NULL, src, rgb);
      break;
    case GRAY_GIMAGE: case GRAYA_GIMAGE:
      map_to_color (1, NULL, src, rgb);
      break;
    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
      map_to_color (2, gimage->cmap, src, rgb);
      break;
    }
}

void
gimp_image_transform_color (const GimpImage    *gimage, 
			    const GimpDrawable *drawable,
			    guchar             *src, 
			    guchar             *dest, 
			    GimpImageBaseType   type)
{
  GimpImageType d_type;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  d_type = (drawable != NULL) ? gimp_drawable_type (drawable) :
    gimp_image_base_type_with_alpha (gimage);

  switch (type)
    {
    case RGB:
      switch (d_type)
	{
	case RGB_GIMAGE: case RGBA_GIMAGE:
	  /*  Straight copy  */
	  *dest++ = *src++;
	  *dest++ = *src++;
	  *dest++ = *src++;
	  break;
	case GRAY_GIMAGE: case GRAYA_GIMAGE:
	  /*  NTSC conversion  */
	  *dest = INTENSITY (src[RED_PIX],
			     src[GREEN_PIX],
			     src[BLUE_PIX]);
	  break;
	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  /*  Least squares method  */
	  *dest = gimp_image_color_hash_rgb_to_indexed (gimage,
							src[RED_PIX],
							src[GREEN_PIX],
							src[BLUE_PIX]);
	  break;
	}
      break;
    case GRAY:
      switch (d_type)
	{
	case RGB_GIMAGE: case RGBA_GIMAGE:
	  /*  Gray to RG&B */
	  *dest++ = *src;
	  *dest++ = *src;
	  *dest++ = *src;
	  break;
	case GRAY_GIMAGE: case GRAYA_GIMAGE:
	  /*  Straight copy  */
	  *dest = *src;
	  break;
	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  /*  Least squares method  */
	  *dest = gimp_image_color_hash_rgb_to_indexed (gimage,
							src[GRAY_PIX],
							src[GRAY_PIX],
							src[GRAY_PIX]);
	  break;
	}
      break;
    default:
      break;
    }
}


/*  shadow tiles  */

TileManager *
gimp_image_shadow (GimpImage *gimage, 
		   gint       width, 
		   gint       height, 
		   gint       bpp)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  if (gimage->shadow)
    {
      if ((width != tile_manager_width (gimage->shadow)) ||
          (height != tile_manager_height (gimage->shadow)) ||
          (bpp != tile_manager_bpp (gimage->shadow)))
        {
          gimp_image_free_shadow (gimage);
        }
      else
        {
          return gimage->shadow;
        }
    }

  gimage->shadow = tile_manager_new (width, height, bpp);

  return gimage->shadow;
}

void
gimp_image_free_shadow (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (gimage->shadow)
    {
      tile_manager_destroy (gimage->shadow);
      gimage->shadow = NULL;
    }
}


/*  combine functions  */

void
gimp_image_apply_image (GimpImage	 *gimage,
			GimpDrawable	 *drawable,
			PixelRegion	 *src2PR,
			gboolean          undo,
			gint              opacity,
			LayerModeEffects  mode,
			/*  alternative to using drawable tiles as src1: */
			TileManager	 *src1_tiles,
			gint              x,
			gint              y)
{
  GimpChannel *mask;
  gint         x1, y1, x2, y2;
  gint         offset_x, offset_y;
  PixelRegion  src1PR, destPR, maskPR;
  gint         operation;
  gint         active [MAX_CHANNELS];

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  get the selection mask if one exists  */
  mask = (gimage_mask_is_empty (gimage)) ? NULL : gimp_image_get_mask (gimage);

  /*  configure the active channel array  */
  gimp_image_get_active_components (gimage, drawable, active);

  /*  determine what sort of operation is being attempted and
   *  if it's actually legal...
   */
  operation = gimp_image_get_combination_mode (gimp_drawable_type (drawable),
                                               src2PR->bytes);
  if (operation == -1)
    {
      g_warning ("%s: illegal parameters.", G_GNUC_PRETTY_FUNCTION);
      return;
    }

  /*  get the layer offsets  */
  gimp_drawable_offsets (drawable, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within gimage bounds  */
  x1 = CLAMP (x, 0, gimp_drawable_width  (drawable));
  y1 = CLAMP (y, 0, gimp_drawable_height (drawable));
  x2 = CLAMP (x + src2PR->w, 0, gimp_drawable_width  (drawable));
  y2 = CLAMP (y + src2PR->h, 0, gimp_drawable_height (drawable));

  if (mask)
    {
      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      x1 = CLAMP (x1, -offset_x, gimp_drawable_width (GIMP_DRAWABLE (mask))-offset_x);
      y1 = CLAMP (y1, -offset_y, gimp_drawable_height(GIMP_DRAWABLE (mask))-offset_y);
      x2 = CLAMP (x2, -offset_x, gimp_drawable_width (GIMP_DRAWABLE (mask))-offset_x);
      y2 = CLAMP (y2, -offset_y, gimp_drawable_height(GIMP_DRAWABLE (mask))-offset_y);
    }

  /*  If the calling procedure specified an undo step...  */
  if (undo)
    undo_push_image (gimp_drawable_gimage (drawable), drawable, x1, y1, x2, y2);

  /* configure the pixel regions
   *  If an alternative to using the drawable's data as src1 was provided...
   */
  if (src1_tiles)
    pixel_region_init (&src1PR, src1_tiles, 
		       x1, y1, (x2 - x1), (y2 - y1), FALSE);
  else
    pixel_region_init (&src1PR, gimp_drawable_data (drawable), 
		       x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, gimp_drawable_data (drawable), 
		     x1, y1, (x2 - x1), (y2 - y1), TRUE);
  pixel_region_resize (src2PR, 
		       src2PR->x + (x1 - x), src2PR->y + (y1 - y), 
		       (x2 - x1), (y2 - y1));

  if (mask)
    {
      gint mx, my;

      /*  configure the mask pixel region
       *  don't use x1 and y1 because they are in layer
       *  coordinate system.  Need mask coordinate system
       */
      mx = x1 + offset_x;
      my = y1 + offset_y;

      pixel_region_init (&maskPR, 
			 gimp_drawable_data (GIMP_DRAWABLE (mask)), 
			 mx, my, 
			 (x2 - x1), (y2 - y1), 
			 FALSE);
      combine_regions (&src1PR, src2PR, &destPR, &maskPR, NULL,
		       opacity, mode, active, operation);
    }
  else
    {
      combine_regions (&src1PR, src2PR, &destPR, NULL, NULL,
		       opacity, mode, active, operation);
    }
}

/* Similar to gimp_image_apply_image but works in "replace" mode (i.e.
   transparent pixels in src2 make the result transparent rather
   than opaque.

   Takes an additional mask pixel region as well.
*/
void
gimp_image_replace_image (GimpImage    *gimage, 
			  GimpDrawable *drawable, 
			  PixelRegion  *src2PR,
			  gboolean      undo, 
			  gint          opacity,
			  PixelRegion  *maskPR,
			  gint          x, 
			  gint          y)
{
  GimpChannel *mask;
  gint         x1, y1, x2, y2;
  gint         offset_x, offset_y;
  PixelRegion  src1PR, destPR;
  PixelRegion  mask2PR, tempPR;
  guchar      *temp_data;
  gint         operation;
  gint         active [MAX_CHANNELS];

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  get the selection mask if one exists  */
  mask = (gimage_mask_is_empty (gimage)) ? NULL : gimp_image_get_mask (gimage);

  /*  configure the active channel array  */
  gimp_image_get_active_components (gimage, drawable, active);

  /*  determine what sort of operation is being attempted and
   *  if it's actually legal...
   */
  operation = gimp_image_get_combination_mode (gimp_drawable_type (drawable),
                                               src2PR->bytes);
  if (operation == -1)
    {
      g_warning ("%s: illegal parameters.", G_GNUC_PRETTY_FUNCTION);
      return;
    }

  /*  get the layer offsets  */
  gimp_drawable_offsets (drawable, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within gimage bounds  */
  x1 = CLAMP (x, 0, gimp_drawable_width (drawable));
  y1 = CLAMP (y, 0, gimp_drawable_height (drawable));
  x2 = CLAMP (x + src2PR->w, 0, gimp_drawable_width (drawable));
  y2 = CLAMP (y + src2PR->h, 0, gimp_drawable_height (drawable));

  if (mask)
    {
      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      x1 = CLAMP (x1, -offset_x, gimp_drawable_width (GIMP_DRAWABLE (mask))-offset_x);
      y1 = CLAMP (y1, -offset_y, gimp_drawable_height(GIMP_DRAWABLE (mask))-offset_y);
      x2 = CLAMP (x2, -offset_x, gimp_drawable_width (GIMP_DRAWABLE (mask))-offset_x);
      y2 = CLAMP (y2, -offset_y, gimp_drawable_height(GIMP_DRAWABLE (mask))-offset_y);
    }

  /*  If the calling procedure specified an undo step...  */
  if (undo)
    gimp_drawable_apply_image (drawable, x1, y1, x2, y2, NULL, FALSE);

  /* configure the pixel regions
   *  If an alternative to using the drawable's data as src1 was provided...
   */
  pixel_region_init (&src1PR, gimp_drawable_data (drawable),
		     x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, gimp_drawable_data (drawable),
		     x1, y1, (x2 - x1), (y2 - y1), TRUE);
  pixel_region_resize (src2PR,
		       src2PR->x + (x1 - x), src2PR->y + (y1 - y),
		       (x2 - x1), (y2 - y1));

  if (mask)
    {
      int mx, my;

      /*  configure the mask pixel region
       *  don't use x1 and y1 because they are in layer
       *  coordinate system.  Need mask coordinate system
       */
      mx = x1 + offset_x;
      my = y1 + offset_y;

      pixel_region_init (&mask2PR, 
			 gimp_drawable_data (GIMP_DRAWABLE (mask)), 
			 mx, my, 
			 (x2 - x1), (y2 - y1), 
			 FALSE);

      tempPR.bytes = 1;
      tempPR.x = 0;
      tempPR.y = 0;
      tempPR.w = x2 - x1;
      tempPR.h = y2 - y1;
      tempPR.rowstride = tempPR.w * tempPR.bytes;
      temp_data = g_malloc (tempPR.h * tempPR.rowstride);
      tempPR.data = temp_data;

      copy_region (&mask2PR, &tempPR);

      /* apparently, region operations can mutate some PR data. */
      tempPR.x = 0;
      tempPR.y = 0;
      tempPR.w = x2 - x1;
      tempPR.h = y2 - y1;
      tempPR.data = temp_data;

      apply_mask_to_region (&tempPR, maskPR, OPAQUE_OPACITY);

      tempPR.x = 0;
      tempPR.y = 0;
      tempPR.w = x2 - x1;
      tempPR.h = y2 - y1;
      tempPR.data = temp_data;

      combine_regions_replace (&src1PR, src2PR, &destPR, &tempPR, NULL,
		       opacity, active, operation);

      g_free (temp_data);
    }
  else
    {
      combine_regions_replace (&src1PR, src2PR, &destPR, maskPR, NULL,
			       opacity, active, operation);
    }
}


/*  parasites  */

GimpParasite *
gimp_image_parasite_find (const GimpImage *gimage, 
			  const gchar     *name)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimp_parasite_list_find (gimage->parasites, name);
}

static void
list_func (gchar          *key, 
	   GimpParasite   *p, 
	   gchar        ***cur)
{
  *(*cur)++ = (gchar *) g_strdup (key);
}

gchar **
gimp_image_parasite_list (const GimpImage *gimage, 
			  gint            *count)
{
  gchar **list;
  gchar **cur;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  *count = gimp_parasite_list_length (gimage->parasites);
  cur = list = g_new (gchar*, *count);

  gimp_parasite_list_foreach (gimage->parasites, (GHFunc) list_func, &cur);
  
  return list;
}

void
gimp_image_parasite_attach (GimpImage    *gimage, 
			    GimpParasite *parasite)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage) && parasite != NULL);

  /* only set the dirty bit manually if we can be saved and the new
     parasite differs from the current one and we aren't undoable */
  if (gimp_parasite_is_undoable (parasite))
    undo_push_image_parasite (gimage, parasite);

  /*  We used to push an cantundo on te stack here. This made the undo stack
      unusable (NULL on the stack) and prevented people from undoing after a 
      save (since most save plug-ins attach an undoable comment parasite).
      Now we simply attach the parasite without pushing an undo. That way it's
      undoable but does not block the undo system.   --Sven
   */

  gimp_parasite_list_add (gimage->parasites, parasite);

  if (gimp_parasite_has_flag (parasite, GIMP_PARASITE_ATTACH_PARENT))
    {
      gimp_parasite_shift_parent (parasite);
      gimp_parasite_attach (gimage->gimp, parasite);
    }
}

void
gimp_image_parasite_detach (GimpImage   *gimage, 
			    const gchar *parasite)
{
  GimpParasite *p;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (parasite != NULL);

  if (!(p = gimp_parasite_list_find (gimage->parasites, parasite)))
    return;

  if (gimp_parasite_is_undoable (p))
    undo_push_image_parasite_remove (gimage, gimp_parasite_name (p));

  gimp_parasite_list_remove (gimage->parasites, parasite);
}


/*  tattoos  */

GimpTattoo
gimp_image_get_new_tattoo (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), 0);

  gimage->tattoo_state++;

  if (gimage->tattoo_state <= 0)
    g_warning ("%s: Tattoo state corrupted "
	       "(integer overflow).", G_GNUC_PRETTY_FUNCTION);

  return gimage->tattoo_state;
}

GimpTattoo
gimp_image_get_tattoo_state (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), 0);

  return gimage->tattoo_state;
}

gboolean
gimp_image_set_tattoo_state (GimpImage  *gimage, 
			     GimpTattoo  val)
{
  GList       *list;
  gboolean     retval = TRUE;
  GimpChannel *channel;
  GimpTattoo   maxval = 0;
  Path        *pptr   = NULL;
  PathList    *plist;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  for (list = GIMP_LIST (gimage->layers)->list; 
       list; 
       list = g_list_next (list))
    {
      GimpTattoo ltattoo;
      
      ltattoo = gimp_drawable_get_tattoo (GIMP_DRAWABLE (list->data));
      if (ltattoo > maxval)
	maxval = ltattoo;
      if (gimp_image_get_channel_by_tattoo (gimage, ltattoo) != NULL)
	{
	  retval = FALSE; /* Oopps duplicated tattoo in channel */
	}

      /* Now check path an't got this tattoo */
      if (path_get_path_by_tattoo (gimage, ltattoo) != NULL)
	{
	  retval = FALSE; /* Oopps duplicated tattoo in layer */
	}
    }

  /* Now check that the paths channel tattoos don't overlap */
  for (list = GIMP_LIST (gimage->channels)->list; 
       list; 
       list = g_list_next (list))
    {
      GimpTattoo ctattoo;

      channel = (GimpChannel *) list->data;
      
      ctattoo = gimp_drawable_get_tattoo (GIMP_DRAWABLE (channel));
      if (ctattoo > maxval)
	maxval = ctattoo;
      /* Now check path an't got this tattoo */
      if (path_get_path_by_tattoo (gimage, ctattoo) != NULL)
	{
	  retval = FALSE; /* Oopps duplicated tattoo in layer */
	}
    }

  /* Find the max tatto value in the paths */
  plist = gimage->paths;
      
  if (plist && plist->bz_paths)
    {
      GimpTattoo  ptattoo;
      GSList     *pl;

      for (pl = plist->bz_paths; pl; pl = g_slist_next (pl))
	{
	  pptr = pl->data;

	  ptattoo = path_get_tattoo (pptr);
	  
	  if (ptattoo > maxval)
	    maxval = ptattoo;
	}
    }

  if (val < maxval)
    retval = FALSE;
  /* Must check the state is valid */
  if (retval == TRUE)
    gimage->tattoo_state = val;

  return retval;
}


/*  layers / channels / paths  */

GimpContainer *
gimp_image_get_layers (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->layers;
}

GimpContainer *
gimp_image_get_channels (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->channels;
}

void
gimp_image_set_paths (GimpImage *gimage,
		      PathList  *paths)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimage->paths = paths;
}

PathList *
gimp_image_get_paths (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->paths;
}

GimpDrawable *
gimp_image_active_drawable (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  /*  If there is an active channel (a saved selection, etc.),
   *  we ignore the active layer
   */
  if (gimage->active_channel)
    {
      return GIMP_DRAWABLE (gimage->active_channel);
    }
  else if (gimage->active_layer)
    {
      GimpLayer *layer;

      layer = gimage->active_layer;

      if (layer->mask && layer->mask->edit_mask)
	return GIMP_DRAWABLE (layer->mask);
      else
	return GIMP_DRAWABLE (layer);
    }

  return NULL;
}

GimpLayer *
gimp_image_get_active_layer (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->active_layer;
}

GimpChannel *
gimp_image_get_active_channel (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->active_channel;
}

GimpLayer *
gimp_image_set_active_layer (GimpImage *gimage, 
			     GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);

  /*  First, find the layer in the gimage
   *  If it isn't valid, find the first layer that is
   */
  if (! gimp_container_have (gimage->layers, GIMP_OBJECT (layer)))
    layer = (GimpLayer *) gimp_container_get_child_by_index (gimage->layers, 0);

  if (layer)
    {
      /*  Configure the layer stack to reflect this change  */
      gimage->layer_stack = g_slist_remove (gimage->layer_stack, layer);
      gimage->layer_stack = g_slist_prepend (gimage->layer_stack, layer);

      /*  invalidate the selection boundary because of a layer modification  */
      gimp_layer_invalidate_boundary (layer);
    }

  if (layer != gimage->active_layer)
    {
      gimage->active_layer = layer;

      g_signal_emit (G_OBJECT (gimage),
		     gimp_image_signals[ACTIVE_LAYER_CHANGED], 0);

      if (gimage->active_channel)
	{
	  gimage->active_channel = NULL;

	  g_signal_emit (G_OBJECT (gimage),
			 gimp_image_signals[ACTIVE_CHANNEL_CHANGED], 0);
	}
    }

  /*  return the layer  */
  return layer;
}

GimpChannel *
gimp_image_set_active_channel (GimpImage   *gimage, 
			       GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  /*  Not if there is a floating selection  */
  if (gimp_image_floating_sel (gimage))
    return NULL;

  /*  First, find the channel
   *  If it doesn't exist, find the first channel that does
   */
  if (! gimp_container_have (gimage->channels, GIMP_OBJECT (channel)))
    channel = (GimpChannel *) gimp_container_get_child_by_index (gimage->channels, 0);

  if (channel != gimage->active_channel)
    {
      gimage->active_channel = channel;

      g_signal_emit (G_OBJECT (gimage),
		     gimp_image_signals[ACTIVE_CHANNEL_CHANGED], 0);

      if (gimage->active_layer)
	{
	  gimage->active_layer = NULL;

	  g_signal_emit (G_OBJECT (gimage),
			 gimp_image_signals[ACTIVE_LAYER_CHANGED], 0);
	}
    }

  /*  return the channel  */
  return channel;
}

GimpChannel *
gimp_image_unset_active_channel (GimpImage *gimage)
{
  GimpChannel *channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  channel = gimp_image_get_active_channel (gimage);

  if (channel)
    {
      gimage->active_channel = NULL;

      g_signal_emit (G_OBJECT (gimage),
		     gimp_image_signals[ACTIVE_CHANNEL_CHANGED], 0);

      if (gimage->layer_stack)
	{
	  GimpLayer *layer;

	  layer = (GimpLayer *) gimage->layer_stack->data;

	  gimp_image_set_active_layer (gimage, layer);
	}
    }

  return channel;
}

gint            
gimp_image_get_layer_index (const GimpImage   *gimage,
			    const GimpLayer   *layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), -1);

  return gimp_container_get_child_index (gimage->layers, 
					 GIMP_OBJECT (layer));
}

gint            
gimp_image_get_channel_index (const GimpImage   *gimage,
			      const GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), -1);

  return gimp_container_get_child_index (gimage->channels, 
					 GIMP_OBJECT (channel));
}

GimpLayer *
gimp_image_get_layer_by_tattoo (const GimpImage *gimage, 
				GimpTattoo       tattoo)
{
  GimpLayer *layer;
  GList     *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  for (list = GIMP_LIST (gimage->layers)->list; 
       list; 
       list = g_list_next (list))
    {
      layer = (GimpLayer *) list->data;

      if (gimp_drawable_get_tattoo (GIMP_DRAWABLE (layer)) == tattoo)
	return layer;
    }

  return NULL;
}

GimpChannel *
gimp_image_get_channel_by_tattoo (const GimpImage *gimage, 
				  GimpTattoo       tattoo)
{
  GimpChannel *channel;
  GList       *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  for (list = GIMP_LIST (gimage->channels)->list; 
       list; 
       list = g_list_next (list))
    {
      channel = (GimpChannel *) list->data;

      if (gimp_drawable_get_tattoo (GIMP_DRAWABLE (channel)) == tattoo)
	return channel;
    }

  return NULL;
}

GimpChannel *
gimp_image_get_channel_by_name (const GimpImage *gimage,
				const gchar     *name)
{
  GimpChannel *channel;
  GList       *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  for (list = GIMP_LIST (gimage->channels)->list; 
       list; 
       list = g_list_next (list))
    {
      channel = (GimpChannel *) list->data;
      if (! strcmp (gimp_object_get_name (GIMP_OBJECT (channel)), name))
      return channel;
    }

  return NULL;
}

gboolean
gimp_image_add_layer (GimpImage *gimage, 
		      GimpLayer *layer, 
		      gint       position)
{
  LayerUndo *lu;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  if (GIMP_DRAWABLE (layer)->gimage != NULL && 
      GIMP_DRAWABLE (layer)->gimage != gimage) 
    {
      g_warning ("%s: attempting to add layer to wrong image.",
		 G_GNUC_PRETTY_FUNCTION);
      return FALSE;
    }

  if (gimp_container_have (gimage->layers, GIMP_OBJECT (layer)))
    {
      g_warning ("%s: trying to add layer to image twice.",
		 G_GNUC_PRETTY_FUNCTION);
      return FALSE;
    }

  /*  Prepare a layer undo and push it  */
  lu = g_new (LayerUndo, 1);
  lu->layer         = layer;
  lu->prev_position = 0;
  lu->prev_layer    = gimp_image_get_active_layer (gimage);
  undo_push_layer (gimage, LAYER_ADD_UNDO, lu);

  /*  If the layer is a floating selection, set the ID  */
  if (gimp_layer_is_floating_sel (layer))
    gimage->floating_sel = layer;

  /*  let the layer know about the gimage  */
  gimp_drawable_set_gimage (GIMP_DRAWABLE (layer), gimage);
  
  /*  If the layer has a mask, set the mask's gimage  */
  if (layer->mask)
    {
      gimp_drawable_set_gimage (GIMP_DRAWABLE (layer->mask), gimage);
    }

  /*  add the layer to the list at the specified position  */
  if (position == -1)
    {
      GimpLayer *active_layer;

      active_layer = gimp_image_get_active_layer (gimage);

      if (active_layer)
	{
	  position = gimp_container_get_child_index (gimage->layers, 
						     GIMP_OBJECT (active_layer));
	}
      else
	{
	  position = 0;
	}
    }

  /*  If there is a floating selection (and this isn't it!),
   *  make sure the insert position is greater than 0
   */
  if (position == 0                    &&
      gimp_image_floating_sel (gimage) &&
      (gimage->floating_sel != layer))
    {
      position = 1;
    }

  gimp_container_insert (gimage->layers, GIMP_OBJECT (layer), position);
  g_object_unref (G_OBJECT (layer));

  /*  notify the layers dialog of the currently active layer  */
  gimp_image_set_active_layer (gimage, layer);

  /*  update the new layer's area  */
  gimp_drawable_update (GIMP_DRAWABLE (layer),
			0, 0,
			gimp_drawable_width  (GIMP_DRAWABLE (layer)),
			gimp_drawable_height (GIMP_DRAWABLE (layer)));

  return TRUE;
}

void
gimp_image_remove_layer (GimpImage *gimage, 
			 GimpLayer *layer)
{
  LayerUndo *lu;
  gint       x, y, w, h;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_LAYER (layer));

  g_return_if_fail (gimp_container_have (gimage->layers, 
					 GIMP_OBJECT (layer)));

  /*  Push a layer undo  */
  lu = g_new (LayerUndo, 1);
  lu->layer         = layer;
  lu->prev_position = gimp_container_get_child_index (gimage->layers, 
						      GIMP_OBJECT (layer));
  lu->prev_layer    = layer;
  
  undo_push_layer (gimage, LAYER_REMOVE_UNDO, lu);

  g_object_ref (G_OBJECT (layer));

  gimp_container_remove (gimage->layers, GIMP_OBJECT (layer));
  gimage->layer_stack = g_slist_remove (gimage->layer_stack, layer);  

  /*  If this was the floating selection, reset the fs pointer  */
  if (gimage->floating_sel == layer)
    {
      gimage->floating_sel = NULL;
      
      floating_sel_reset (layer);
    }

  if (layer == gimp_image_get_active_layer (gimage))
    {
      if (gimage->layer_stack)
	{
	  gimp_image_set_active_layer (gimage, gimage->layer_stack->data);
	}
      else
	{
	  gimage->active_layer = NULL;

	  g_signal_emit (G_OBJECT (gimage),
			 gimp_image_signals[ACTIVE_LAYER_CHANGED], 0);
	}
    }

  /* Send out REMOVED signal from layer */
  gimp_drawable_removed (GIMP_DRAWABLE (layer));

  gimp_drawable_offsets (GIMP_DRAWABLE (layer), &x, &y);
  w = gimp_drawable_width (GIMP_DRAWABLE (layer));
  h = gimp_drawable_height (GIMP_DRAWABLE (layer));

  g_object_unref (G_OBJECT (layer));

  gimp_image_update (gimage, x, y, w, h);

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
}

gboolean
gimp_image_raise_layer (GimpImage *gimage, 
			GimpLayer *layer)
{
  gint curpos;
  
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  curpos = gimp_container_get_child_index (gimage->layers, 
					   GIMP_OBJECT (layer));

  /* is this the top layer already? */
  if (curpos == 0)
    {
      g_message (_("Layer cannot be raised higher."));
      return FALSE;
    }
  
  return gimp_image_position_layer (gimage, layer, curpos - 1, TRUE);
}

gboolean
gimp_image_lower_layer (GimpImage *gimage, 
			GimpLayer *layer)
{
  gint curpos;
  gint length;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  curpos = gimp_container_get_child_index (gimage->layers, 
					   GIMP_OBJECT (layer));
  
  /* is this the bottom layer already? */
  length = gimp_container_num_children (gimage->layers);
  if (curpos >= length - 1)
    {
      g_message (_("Layer cannot be lowered more."));
      return FALSE;
    }
  
  return gimp_image_position_layer (gimage, layer, curpos + 1, TRUE);
}

gboolean
gimp_image_raise_layer_to_top (GimpImage *gimage, 
			       GimpLayer *layer)
{
  gint curpos;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  curpos = gimp_container_get_child_index (gimage->layers, 
					   GIMP_OBJECT (layer));
  
  if (curpos == 0)
    {
      g_message (_("Layer is already on top."));
      return FALSE;
    }
  
  if (! gimp_layer_has_alpha (layer))
    {
      g_message (_("Cannot raise a layer without alpha."));
      return FALSE;
    }
  
  return gimp_image_position_layer (gimage, layer, 0, TRUE);
}

gboolean
gimp_image_lower_layer_to_bottom (GimpImage *gimage, 
				  GimpLayer *layer)
{
  gint curpos;
  gint length;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  curpos = gimp_container_get_child_index (gimage->layers, 
					   GIMP_OBJECT (layer));
 
  length = gimp_container_num_children (gimage->layers);

  if (curpos >= length - 1)
    {
      g_message (_("Layer is already on the bottom."));
      return FALSE;
    }
  
  return gimp_image_position_layer (gimage, layer, length - 1, TRUE);
}

gboolean
gimp_image_position_layer (GimpImage *gimage, 
			   GimpLayer *layer,
			   gint       new_index,
			   gboolean   push_undo)
{
  gint off_x, off_y;
  gint index;
  gint num_layers;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  index = gimp_container_get_child_index (gimage->layers, 
					  GIMP_OBJECT (layer));
  if (index < 0)
    return FALSE;

  num_layers = gimp_container_num_children (gimage->layers);

  if (new_index < 0)
    new_index = 0;

  if (new_index >= num_layers)
    new_index = num_layers - 1;

  if (new_index == index)
    return TRUE;

  /* check if we want to move it below a bottom layer without alpha */
  if (new_index == num_layers - 1)
    {
      GimpLayer *tmp;
      
      tmp = (GimpLayer *) gimp_container_get_child_by_index (gimage->layers, 
							     num_layers - 1);

      if (new_index == num_layers - 1 &&
	  ! gimp_layer_has_alpha (tmp))
	{
	  g_message (_("Layer \"%s\" has no alpha.\nLayer was placed above it."),
		     GIMP_OBJECT (tmp)->name);
	  new_index--;
	}
    }

  if (push_undo)
    undo_push_layer_reposition (gimage, layer);

  gimp_container_reorder (gimage->layers, GIMP_OBJECT (layer), new_index);

  gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

  gimp_image_update (gimage,
		     off_x, off_y,
		     gimp_drawable_width (GIMP_DRAWABLE (layer)),
		     gimp_drawable_height (GIMP_DRAWABLE (layer)));

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));

  return TRUE;
}

gboolean
gimp_image_add_channel (GimpImage   *gimage, 
			GimpChannel *channel, 
			gint         position)
{
  ChannelUndo *cu;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  if (GIMP_DRAWABLE (channel)->gimage != NULL &&
      GIMP_DRAWABLE (channel)->gimage != gimage)
    {
      g_warning ("%s: attempting to add channel to wrong image.",
		 G_GNUC_PRETTY_FUNCTION);
      return FALSE;
    }

  if (gimp_container_have (gimage->channels, GIMP_OBJECT (channel)))
    {
      g_warning ("%s: trying to add channel to image twice.",
		 G_GNUC_PRETTY_FUNCTION);
      return FALSE;
    }

  /*  Push a channel undo  */
  cu = g_new (ChannelUndo, 1);
  cu->channel       = channel;
  cu->prev_position = 0;
  cu->prev_channel  = gimp_image_get_active_channel (gimage);
  undo_push_channel (gimage, CHANNEL_ADD_UNDO, cu);

  /*  add the channel to the list  */
  gimp_container_add (gimage->channels, GIMP_OBJECT (channel));
  g_object_unref (G_OBJECT (channel));

  /*  notify this gimage of the currently active channel  */
  gimp_image_set_active_channel (gimage, channel);

  /*  if channel is visible, update the image  */
  if (gimp_drawable_get_visible (GIMP_DRAWABLE (channel)))
    gimp_drawable_update (GIMP_DRAWABLE (channel), 
			  0, 0, 
			  gimp_drawable_width (GIMP_DRAWABLE (channel)), 
			  gimp_drawable_height (GIMP_DRAWABLE (channel)));

  return TRUE;
}

void
gimp_image_remove_channel (GimpImage   *gimage, 
			   GimpChannel *channel)
{
  ChannelUndo *cu;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  g_return_if_fail (gimp_container_have (gimage->channels, 
					 GIMP_OBJECT (channel)));

  /*  Prepare a channel undo--push it below  */
  cu = g_new (ChannelUndo, 1);
  cu->channel       = channel;
  cu->prev_position = gimp_container_get_child_index (gimage->channels, 
						      GIMP_OBJECT (channel));
  cu->prev_channel  = gimp_image_get_active_channel (gimage);
  undo_push_channel (gimage, CHANNEL_REMOVE_UNDO, cu);

  g_object_ref (G_OBJECT (channel));

  gimp_container_remove (gimage->channels, GIMP_OBJECT (channel));

  /* Send out REMOVED signal from channel */
  gimp_drawable_removed (GIMP_DRAWABLE (channel));

  if (channel == gimp_image_get_active_channel (gimage))
    {
      if (gimp_container_num_children (gimage->channels) > 0)
	{
	  gimp_image_set_active_channel
	    (gimage,
	     GIMP_CHANNEL (gimp_container_get_child_by_index (gimage->channels,
							      0)));
	}
      else
	{
	  gimp_image_unset_active_channel (gimage);
	}
    }

  g_object_unref (G_OBJECT (channel));

  gimp_image_update (gimage, 0, 0, gimage->width, gimage->height);

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
}

gboolean
gimp_image_raise_channel (GimpImage   *gimage, 
			  GimpChannel *channel)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);  

  index = gimp_container_get_child_index (gimage->channels, 
					  GIMP_OBJECT (channel));
  if (index == 0)
    {
      g_message (_("Channel cannot be raised higher."));
      return FALSE;
    }

  return gimp_image_position_channel (gimage, channel, index - 1, TRUE);
}

gboolean
gimp_image_lower_channel (GimpImage   *gimage, 
			  GimpChannel *channel)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);  
  
  index = gimp_container_get_child_index (gimage->channels, 
					  GIMP_OBJECT (channel));
  if (index == gimp_container_num_children (gimage->channels) - 1)
    {
      g_message (_("Channel cannot be lowered more."));
      return FALSE;
    }

  return gimp_image_position_channel (gimage, channel, index + 1, TRUE);
}

gboolean
gimp_image_position_channel (GimpImage   *gimage, 
			     GimpChannel *channel,
			     gint         new_index,
                             gboolean     push_undo /* FIXME unused */)
{
  gint index;
  gint num_channels;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);  

  index = gimp_container_get_child_index (gimage->channels, 
					  GIMP_OBJECT (channel));
  if (index < 0)
    return FALSE;

  num_channels = gimp_container_num_children (gimage->channels);

  new_index = CLAMP (new_index, 0, num_channels - 1);

  if (new_index == index)
    return TRUE;

  gimp_container_reorder (gimage->channels, 
			  GIMP_OBJECT (channel), new_index);

  gimp_drawable_update (GIMP_DRAWABLE (channel),
			0, 0,
			gimp_drawable_width  (GIMP_DRAWABLE (channel)),
			gimp_drawable_height (GIMP_DRAWABLE (channel)));

  return TRUE;
}

gboolean
gimp_image_layer_boundary (const GimpImage  *gimage, 
			   BoundSeg        **segs, 
			   gint             *n_segs)
{
  GimpLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (segs != NULL, FALSE);
  g_return_val_if_fail (n_segs != NULL, FALSE);

  /*  The second boundary corresponds to the active layer's
   *  perimeter...
   */
  layer = gimp_image_get_active_layer (gimage);

  if (layer)
    {
      *segs = gimp_layer_boundary (layer, n_segs);
      return TRUE;
    }
  else
    {
      *segs = NULL;
      *n_segs = 0;
      return FALSE;
    }
}

GimpLayer *
gimp_image_pick_correlate_layer (const GimpImage *gimage, 
				 gint             x, 
				 gint             y)
{
  GimpLayer *layer;
  GList     *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  for (list = GIMP_LIST (gimage->layers)->list; 
       list; 
       list = g_list_next (list))
    {
      layer = (GimpLayer *) list->data;

      if (gimp_layer_pick_correlate (layer, x, y))
	return layer;
    }

  return NULL;
}

void
gimp_image_invalidate_layer_previews (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimp_container_foreach (gimage->layers, 
			  (GFunc) gimp_viewable_invalidate_preview, 
			  NULL);
}

void
gimp_image_invalidate_channel_previews (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimp_container_foreach (gimage->channels, 
			  (GFunc) gimp_viewable_invalidate_preview, 
			  NULL);
}
