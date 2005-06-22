/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Object properties serialization routines
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_CONFIG_SERIALIZE_H__
#define __GIMP_CONFIG_SERIALIZE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


gboolean  gimp_config_serialize_properties         (GimpConfig       *config,
						    GimpConfigWriter *writer);
gboolean  gimp_config_serialize_changed_properties (GimpConfig       *config,
						    GimpConfigWriter *writer);

gboolean  gimp_config_serialize_property           (GimpConfig       *config,
						    GParamSpec       *param_spec,
						    GimpConfigWriter *writer);
gboolean  gimp_config_serialize_value              (const GValue     *value,
                                                    GString          *str,
                                                    gboolean          escaped);


G_END_DECLS

#endif /* __GIMP_CONFIG_SERIALIZE_H__ */
