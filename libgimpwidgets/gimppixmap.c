/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball 
 *
 * gimppixmap.c
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
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimppixmap.h"


static void   gimp_pixmap_class_init        (GimpPixmapClass *klass);
static void   gimp_pixmap_init              (GimpPixmap      *pixmap);

static void   gimp_pixmap_realize           (GtkWidget       *widget);
static void   gimp_pixmap_create_from_xpm_d (GimpPixmap      *pixmap);


static GtkImageClass *parent_class = NULL;


GType
gimp_pixmap_get_type (void)
{
  static GType pixmap_type = 0;

  if (! pixmap_type)
    {
      static const GTypeInfo pixmap_info =
      {
        sizeof (GimpPixmapClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_pixmap_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpPixmap),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_pixmap_init,
      };

      pixmap_type = g_type_register_static (GTK_TYPE_IMAGE,
					    "GimpPixmap",
					    &pixmap_info, 0);
    }

  return pixmap_type;
}

static void
gimp_pixmap_class_init (GimpPixmapClass *klass)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->realize = gimp_pixmap_realize;
}

static void
gimp_pixmap_init (GimpPixmap *pixmap)
{
  pixmap->xpm_data = NULL;
}

/**
 * gimp_pixmap_new:
 * @xpm_data: A pointer to a XPM data structure as found in XPM files.
 *
 * Creates a new #GimpPixmap widget.
 *
 * Returns: A pointer to the new #GimpPixmap widget.
 **/
GtkWidget *
gimp_pixmap_new (gchar **xpm_data)
{
  GimpPixmap *pixmap;

  pixmap = g_object_new (GIMP_TYPE_PIXMAP, NULL);

  gimp_pixmap_set (pixmap, xpm_data);

  return GTK_WIDGET (pixmap);
}

/**
 * gimp_pixmap_set:
 * @pixmap: The pixmap widget you want to set the new xpm_data for.
 * @xpm_data: A pointer to a XPM data structure as found in XPM files.
 *
 * Sets a new image for an existing #GimpPixmap widget.
 **/
void
gimp_pixmap_set (GimpPixmap  *pixmap,
		 gchar      **xpm_data)
{
  g_return_if_fail (GIMP_IS_PIXMAP (pixmap));

  pixmap->xpm_data = xpm_data;

  GTK_WIDGET (pixmap)->requisition.width  = 0;
  GTK_WIDGET (pixmap)->requisition.height = 0;

  if (! GTK_WIDGET_REALIZED (GTK_WIDGET (pixmap)))
    {
      if (xpm_data)
	{
	  gint width, height;

	  if (sscanf (xpm_data[0], "%d %d", &width, &height) != 2)
	    {
	      g_warning ("%s: passed pointer is no XPM data", G_STRLOC);
	    }
	  else
	    {
	      GTK_WIDGET (pixmap)->requisition.width =
		width + GTK_MISC (pixmap)->xpad * 2;
	      GTK_WIDGET (pixmap)->requisition.height =
		height + GTK_MISC (pixmap)->ypad * 2;
	    }
	}
    }
  else
    {
      gimp_pixmap_create_from_xpm_d (pixmap);
    }
}

static void
gimp_pixmap_realize (GtkWidget *widget)
{
  if (GTK_WIDGET_CLASS (parent_class)->realize)
    GTK_WIDGET_CLASS (parent_class)->realize (widget);

  gimp_pixmap_create_from_xpm_d (GIMP_PIXMAP (widget));
}

static void
gimp_pixmap_create_from_xpm_d (GimpPixmap *pixmap)
{
  GtkStyle   *style;
  GdkPixmap  *gdk_pixmap = NULL;
  GdkBitmap  *mask       = NULL;

  if (pixmap->xpm_data)
    {
      GtkWidget  *widget;

      widget = GTK_WIDGET (pixmap);

      style = gtk_widget_get_style (widget);

      gdk_pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
						 &mask,
						 &style->bg[GTK_STATE_NORMAL],
						 pixmap->xpm_data);
    }

  gtk_image_set_from_pixmap (GTK_IMAGE (pixmap), gdk_pixmap, mask);

  if (gdk_pixmap)
    g_object_unref (gdk_pixmap);

  if (mask)
    g_object_unref (mask);
}
