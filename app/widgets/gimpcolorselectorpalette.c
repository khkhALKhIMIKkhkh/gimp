/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorselectorpalette.c
 * Copyright (C) 2006 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimppalette.h"

#include "gimpcolorselectorpalette.h"
#include "gimppaletteview.h"
#include "gimpviewrendererpalette.h"


static void   gimp_color_selector_palette_realize   (GtkWidget         *widget);

static void   gimp_color_selector_palette_set_color (GimpColorSelector *selector,
                                                     const GimpRGB     *rgb,
                                                     const GimpHSV     *hsv);


G_DEFINE_TYPE (GimpColorSelectorPalette, gimp_color_selector_palette,
               GIMP_TYPE_COLOR_SELECTOR)

#define parent_class gimp_color_selector_palette_parent_class


static void
gimp_color_selector_palette_class_init (GimpColorSelectorPaletteClass *klass)
{
  GtkWidgetClass         *widget_class   = GTK_WIDGET_CLASS (klass);
  GimpColorSelectorClass *selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  widget_class->realize     = gimp_color_selector_palette_realize;

  selector_class->name      = "Palette";
  selector_class->help_id   = "gimp-colorselector-palette";
  selector_class->stock_id  = GIMP_STOCK_PALETTE;
  selector_class->set_color = gimp_color_selector_palette_set_color;
}

static void
gimp_color_selector_palette_init (GimpColorSelectorPalette *select)
{
}

static void
gimp_color_selector_palette_palette_changed (GimpContext *context,
                                             GimpPalette *palette,
                                             GimpView    *view)
{
  gimp_view_set_viewable (view, GIMP_VIEWABLE (palette));
}

static void
gimp_color_selector_palette_entry_clicked (GimpPaletteView   *view,
                                           GimpPaletteEntry  *entry,
                                           GdkModifierType    state,
                                           GimpColorSelector *selector)
{
  selector->rgb = entry->color;
  gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);

  gimp_color_selector_color_changed (selector);
}

static void
gimp_color_selector_palette_realize (GtkWidget *widget)
{
  GimpColorSelectorPalette *select = GIMP_COLOR_SELECTOR_PALETTE (widget);

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  if (! select->context)
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (select));
      GtkWidget *frame;

      select->context = g_object_get_data (G_OBJECT (toplevel),
                                           "gimp-context");

      g_return_if_fail (GIMP_IS_CONTEXT (select->context));

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_container_add (GTK_CONTAINER (select), frame);
      gtk_widget_show (frame);

      select->view = gimp_view_new_full_by_types (select->context,
                                                  GIMP_TYPE_PALETTE_VIEW,
                                                  GIMP_TYPE_PALETTE,
                                                  100, 100, 0,
                                                  FALSE, TRUE, FALSE);
      gimp_view_set_expand (GIMP_VIEW (select->view), TRUE);
      gimp_view_renderer_palette_set_cell_size
        (GIMP_VIEW_RENDERER_PALETTE (GIMP_VIEW (select->view)->renderer), -1);
      gimp_view_renderer_palette_set_draw_grid
        (GIMP_VIEW_RENDERER_PALETTE (GIMP_VIEW (select->view)->renderer), TRUE);
      gtk_container_add (GTK_CONTAINER (frame), select->view);
      gtk_widget_show (select->view);

      g_signal_connect (select->view, "entry-clicked",
                        G_CALLBACK (gimp_color_selector_palette_entry_clicked),
                        select);

      g_signal_connect_object (select->context, "palette-changed",
                               G_CALLBACK (gimp_color_selector_palette_palette_changed),
                               select->view, 0);

      gimp_color_selector_palette_palette_changed (select->context,
                                                   gimp_context_get_palette (select->context),
                                                   GIMP_VIEW (select->view));
    }
}

static void
gimp_color_selector_palette_set_color (GimpColorSelector *selector,
                                       const GimpRGB     *rgb,
                                       const GimpHSV     *hsv)
{
  GimpColorSelectorPalette *select = GIMP_COLOR_SELECTOR_PALETTE (selector);

  if (select->context)
    {
      GimpPalette *palette = gimp_context_get_palette (select->context);

      if (palette)
        {
          GList *list;

          for (list = palette->colors; list; list = g_list_next (list))
            {
              GimpPaletteEntry *entry = list->data;

#define EPSILON 1e-10

              if (gimp_rgb_distance (&entry->color, rgb) < EPSILON)
                {
                  gimp_palette_view_select_entry (GIMP_PALETTE_VIEW (select->view),
                                                  entry);
                  break;
                }
            }
        }
    }
}