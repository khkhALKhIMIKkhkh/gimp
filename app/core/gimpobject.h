/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_OBJECT_H__
#define __GIMP_OBJECT_H__


#define GIMP_TYPE_OBJECT            (gimp_object_get_type ())
#define GIMP_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OBJECT, GimpObject))
#define GIMP_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_OBJECT, GimpObjectClass))
#define GIMP_IS_OBJECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OBJECT))
#define GIMP_IS_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_OBJECT))
#define GIMP_OBJECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_OBJECT, GimpObjectClass))


typedef struct _GimpObjectClass GimpObjectClass;

struct _GimpObject
{
  GObject  parent_instance;

  gchar   *name;

  /*<  private  >*/
  gchar   *normalized;
};

struct _GimpObjectClass
{
  GObjectClass  parent_class;

  /*  signals  */
  void  (* disconnect)   (GimpObject *object);
  void  (* name_changed) (GimpObject *object);

  /*  virtual functions  */
  gsize (* get_memsize)  (GimpObject *object,
                          gsize      *gui_size);
};


GType         gimp_object_get_type      (void) G_GNUC_CONST;

void          gimp_object_set_name      (GimpObject       *object,
                                         const gchar      *name);
const gchar * gimp_object_get_name      (const GimpObject *object);
void          gimp_object_set_name_safe (GimpObject       *object,
                                         const gchar      *name);
void          gimp_object_name_changed  (GimpObject       *object);

gint          gimp_object_name_collate  (GimpObject       *object1,
                                         GimpObject       *object2);

gsize         gimp_object_get_memsize   (GimpObject       *object,
                                         gsize            *gui_size);
gsize         gimp_g_object_get_memsize (GObject          *object);


#endif  /* __GIMP_OBJECT_H__ */
