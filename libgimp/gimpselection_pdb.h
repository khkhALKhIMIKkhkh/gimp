/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpselection_pdb.h
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

#ifndef __GIMP_SELECTION_PDB_H__
#define __GIMP_SELECTION_PDB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


gint32   gimp_selection_bounds   (gint32  image_ID,
				  gint32 *non_empty,
				  gint32 *x1,
				  gint32 *y1,
				  gint32 *x2,
				  gint32 *y2);
gint32   gimp_selection_float    (gint32  image_ID, 
				  gint32  drawable_ID,
				  gint32  x_offset,
				  gint32  y_offset);
gint32   gimp_selection_is_empty (gint32  image_ID);
void     gimp_selection_none     (gint32  image_ID);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_SELECTION_PDB_H__ */
