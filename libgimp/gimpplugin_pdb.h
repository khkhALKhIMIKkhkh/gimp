/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpplugin_pdb.h
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

/* NOTE: This file is autogenerated by pdbgen.pl */

#ifndef __GIMP_PLUG_IN_PDB_H__
#define __GIMP_PLUG_IN_PDB_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


gboolean gimp_plugin_domain_register      (const gchar  *domain_name,
					   const gchar  *domain_path);
gboolean gimp_plugin_help_register        (const gchar  *domain_name,
					   const gchar  *domain_uri);
gboolean gimp_plugin_menu_register        (const gchar  *procedure_name,
					   const gchar  *menu_path);
gboolean gimp_plugin_menu_branch_register (const gchar  *menu_path,
					   const gchar  *menu_name);
gboolean _gimp_plugin_icon_register       (const gchar  *procedure_name,
					   GimpIconType  icon_type,
					   gint          icon_data_length,
					   const guint8 *icon_data);


G_END_DECLS

#endif /* __GIMP_PLUG_IN_PDB_H__ */
