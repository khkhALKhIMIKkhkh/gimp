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

#include <glib-object.h>

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimpundo.h"


enum
{
  POP,
  FREE,
  LAST_SIGNAL
};


static void      gimp_undo_class_init  (GimpUndoClass       *klass);
static void      gimp_undo_init        (GimpUndo            *undo);

static void      gimp_undo_finalize    (GObject             *object);

static gsize     gimp_undo_get_memsize (GimpObject          *object);

static TempBuf * gimp_undo_get_preview (GimpViewable        *viewable,
                                        gint                 width,
                                        gint                 height);

static void      gimp_undo_real_pop    (GimpUndo            *undo,
                                        GimpImage           *gimage,
                                        GimpUndoMode         undo_mode,
                                        GimpUndoAccumulator *accum);
static void      gimp_undo_real_free   (GimpUndo            *undo,
                                        GimpImage           *gimage,
                                        GimpUndoMode         undo_mode);


static guint undo_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;


GType
gimp_undo_get_type (void)
{
  static GType undo_type = 0;

  if (! undo_type)
    {
      static const GTypeInfo undo_info =
      {
        sizeof (GimpUndoClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_undo_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpUndo),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_undo_init,
      };

      undo_type = g_type_register_static (GIMP_TYPE_VIEWABLE,
					  "GimpUndo",
					  &undo_info, 0);
  }

  return undo_type;
}

static void
gimp_undo_class_init (GimpUndoClass *klass)
{
  GObjectClass      *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  undo_signals[POP] = 
    g_signal_new ("pop",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpUndoClass, pop),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT_ENUM_POINTER,
		  G_TYPE_NONE, 3,
		  GIMP_TYPE_IMAGE,
                  GIMP_TYPE_UNDO_MODE,
                  G_TYPE_POINTER);

  undo_signals[FREE] = 
    g_signal_new ("free",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpUndoClass, free),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT_ENUM,
		  G_TYPE_NONE, 2,
		  GIMP_TYPE_IMAGE,
                  GIMP_TYPE_UNDO_MODE);

  object_class->finalize         = gimp_undo_finalize;

  gimp_object_class->get_memsize = gimp_undo_get_memsize;

  viewable_class->get_preview    = gimp_undo_get_preview;

  klass->pop                     = gimp_undo_real_pop;
  klass->free                    = gimp_undo_real_free;
}

static void
gimp_undo_init (GimpUndo *undo)
{
  undo->data          = NULL;
  undo->dirties_image = FALSE;
  undo->pop_func      = NULL;
  undo->free_func     = NULL;
  undo->preview       = NULL;
}

static void
gimp_undo_finalize (GObject *object)
{
  GimpUndo *undo;

  undo = GIMP_UNDO (object);

  if (undo->preview)
    {
      temp_buf_free (undo->preview);
      undo->preview = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gsize
gimp_undo_get_memsize (GimpObject *object)
{
  GimpUndo *undo;
  gsize     memsize = 0;

  undo = GIMP_UNDO (object);

  memsize += undo->size;

  if (undo->preview)
    memsize += temp_buf_get_memsize (undo->preview);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
}

static TempBuf *
gimp_undo_get_preview (GimpViewable *viewable,
                       gint          width,
                       gint          height)
{
  return (GIMP_UNDO (viewable)->preview);
}

GimpUndo *
gimp_undo_new (GimpUndoType      undo_type,
               const gchar      *name,
               gpointer          data,
               gsize             size,
               gboolean          dirties_image,
               GimpUndoPopFunc   pop_func,
               GimpUndoFreeFunc  free_func)
{
  GimpUndo *undo;

  g_return_val_if_fail (name != NULL, NULL);
  //g_return_val_if_fail (size == 0 || data != NULL, NULL);

  undo = g_object_new (GIMP_TYPE_UNDO,
                       "name", name,
                       NULL);
  
  undo->undo_type     = undo_type;
  undo->data          = data;
  undo->size          = size;
  undo->dirties_image = dirties_image ? TRUE : FALSE;
  undo->pop_func      = pop_func;
  undo->free_func     = free_func;

  return undo;
}

void
gimp_undo_pop (GimpUndo            *undo,
               GimpImage           *gimage,
               GimpUndoMode         undo_mode,
               GimpUndoAccumulator *accum)
{
  g_return_if_fail (GIMP_IS_UNDO (undo));
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (accum != NULL);

  g_signal_emit (undo, undo_signals[POP], 0, gimage, undo_mode, accum);

  if (undo->dirties_image)
    {
      switch (undo_mode)
        {
        case GIMP_UNDO_MODE_UNDO:
          gimp_image_clean (gimage);
          break;

        case GIMP_UNDO_MODE_REDO:
          gimp_image_dirty (gimage);
          break;
        }
    }
}

void
gimp_undo_free (GimpUndo     *undo,
                GimpImage    *gimage,
                GimpUndoMode  undo_mode)
{
  g_return_if_fail (GIMP_IS_UNDO (undo));
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (undo, undo_signals[FREE], 0, gimage, undo_mode);
}

static void
gimp_undo_real_pop (GimpUndo            *undo,
                    GimpImage           *gimage,
                    GimpUndoMode         undo_mode,
                    GimpUndoAccumulator *accum)
{
  if (undo->pop_func)
    undo->pop_func (undo, gimage, undo_mode, accum);
}

static void
gimp_undo_real_free (GimpUndo     *undo,
                     GimpImage    *gimage,
                     GimpUndoMode  undo_mode)
{
  if (undo->free_func)
    undo->free_func (undo, gimage, undo_mode);
}
