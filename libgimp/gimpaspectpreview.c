/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpaspectpreview.c
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimpuitypes.h"

#include "gimp.h"

#include "libgimp-intl.h"

#include "gimpaspectpreview.h"

static void  gimp_aspect_preview_class_init (GimpAspectPreviewClass *klass);
static void  gimp_aspect_preview_init       (GimpAspectPreview      *preview);

static void  gimp_aspect_preview_style_set  (GtkWidget              *widget,
                                             GtkStyle               *prev_style);
static void  gimp_aspect_preview_draw       (GimpPreview            *preview);


static GimpPreviewClass *parent_class = NULL;

GType
gimp_aspect_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpAspectPreviewClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_aspect_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpAspectPreview),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_aspect_preview_init,
      };

      preview_type = g_type_register_static (GIMP_TYPE_PREVIEW,
                                             "GimpAspectPreview",
                                             &preview_info, 0);
    }

  return preview_type;
}

static void
gimp_aspect_preview_class_init (GimpAspectPreviewClass *klass)
{
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  GimpPreviewClass *preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->style_set = gimp_aspect_preview_style_set;
  preview_class->draw     = gimp_aspect_preview_draw;
}

static void
gimp_aspect_preview_init (GimpAspectPreview *preview)
{
  g_object_set (GIMP_PREVIEW (preview)->area,
                "check-size", gimp_check_size (),
                "check-type", gimp_check_type (),
                NULL);
}

static void
gimp_aspect_preview_style_set (GtkWidget *widget,
                               GtkStyle  *prev_style)
{
  GimpAspectPreview *preview = GIMP_ASPECT_PREVIEW (widget);
  gint               size;
  gint               width, height;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "size", &size,
                        NULL);
  width  = gimp_drawable_width  (preview->drawable->drawable_id);
  height = gimp_drawable_height (preview->drawable->drawable_id);

  if (width > height)
    {
      GIMP_PREVIEW (preview)->width = MIN (width, size);
      GIMP_PREVIEW (preview)->height = (height * GIMP_PREVIEW (preview)->width) / width;
    }
  else
    {
      GIMP_PREVIEW (preview)->height = MIN (height, size);
      GIMP_PREVIEW (preview)->width = (width * GIMP_PREVIEW (preview)->height) / height;
    }
  gtk_widget_set_size_request (GIMP_PREVIEW (preview)->area,
                               GIMP_PREVIEW (preview)->width,
                               GIMP_PREVIEW (preview)->height);
}

GtkWidget *
gimp_aspect_preview_new (GimpDrawable *drawable,
                         gboolean     *toggle)
{
  GimpAspectPreview *preview;
  gint               width, height;

  preview = g_object_new (GIMP_TYPE_ASPECT_PREVIEW, NULL);

  preview->drawable = drawable;
  width  = gimp_drawable_width  (drawable->drawable_id);
  height = gimp_drawable_height (drawable->drawable_id);

  gimp_preview_set_bounds (GIMP_PREVIEW (preview), 0, 0, width, height);

  g_object_set (GIMP_PREVIEW (preview)->frame,
                "ratio", (gdouble)width / (gdouble)height,
                NULL);

  return GTK_WIDGET (preview);
}

static void
gimp_aspect_preview_draw (GimpPreview *preview)
{
  g_return_if_fail (GIMP_IS_ASPECT_PREVIEW (preview));

  gimp_preview_area_fill (GIMP_PREVIEW_AREA (preview->area),
                          0, 0,
                          preview->width,
                          preview->height,
                          0, 0, 0);
}

/**
 * gimp_aspect_preview_draw_buffer:
 * @preview:   a #GimpAspectPreview widget
 * @buffer:
 * @rowstride:
 *
 * Since: GIMP 2.2
 **/
void
gimp_aspect_preview_draw_buffer (GimpAspectPreview *preview,
                                 const guchar      *buffer,
                                 gint               rowstride)
{
  g_return_if_fail (GIMP_IS_ASPECT_PREVIEW (preview));
  g_return_if_fail (buffer != NULL);

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (GIMP_PREVIEW (preview)->area),
                          0, 0,
                          GIMP_PREVIEW (preview)->width,
                          GIMP_PREVIEW (preview)->height,
                          gimp_drawable_type (preview->drawable->drawable_id),
                          buffer,
                          rowstride);
}

