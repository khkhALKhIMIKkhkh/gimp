/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpgradient_pdb.h
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

#ifndef __GIMP_GRADIENT_PDB_H__
#define __GIMP_GRADIENT_PDB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


gchar   ** gimp_gradients_get_list       (gint    *num_gradients);
gchar    * gimp_gradients_get_active     (void);
void       gimp_gradients_set_active     (gchar   *name);
gdouble  * gimp_gradients_sample_uniform (gint     num_samples);
gdouble  * gimp_gradients_sample_custom  (gint     num_samples,
					  gdouble *positions);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_GRADIENT_PDB_H__ */
