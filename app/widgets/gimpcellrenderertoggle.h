/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcellrenderertoggle.h
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_CELL_RENDERER_TOGGLE_H__
#define __GIMP_CELL_RENDERER_TOGGLE_H__


#include <gtk/gtkcellrenderertoggle.h>


#define GIMP_TYPE_CELL_RENDERER_TOGGLE            (gimp_cell_renderer_toggle_get_type ())
#define GIMP_CELL_RENDERER_TOGGLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CELL_RENDERER_TOGGLE, GimpCellRendererToggle))
#define GIMP_CELL_RENDERER_TOGGLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CELL_RENDERER_TOGGLE, GimpCellRendererToggleClass))
#define GIMP_IS_CELL_RENDERER_TOGGLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CELL_RENDERER_TOGGLE))
#define GIMP_IS_CELL_RENDERER_TOGGLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CELL_RENDERER_TOGGLE))
#define GIMP_CELL_RENDERER_TOGGLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CELL_RENDERER_TOGGLE, GimpCellRendererToggleClass))


typedef struct _GimpCellRendererToggleClass GimpCellRendererToggleClass;

struct _GimpCellRendererToggle
{
  GtkCellRendererToggle       parent_instance;
  
  gchar                      *stock_id;
  GtkIconSize                 stock_size;
  GdkPixbuf                  *pixbuf;
};

struct _GimpCellRendererToggleClass
{
  GtkCellRendererToggleClass  parent_class;

  void (* clicked) (GimpCellRendererViewable *cell,
                    const gchar              *path,
                    GdkModifierType           state);
};


GType             gimp_cell_renderer_toggle_get_type (void) G_GNUC_CONST;

GtkCellRenderer * gimp_cell_renderer_toggle_new      (const gchar *stock_id);


#endif /* __GIMP_CELL_RENDERER_TOGGLE_H__ */
