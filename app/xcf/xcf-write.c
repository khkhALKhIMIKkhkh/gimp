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

#include <stdio.h>

#include <glib.h>

#include "xcf-write.h"


guint
xcf_write_int32 (FILE     *fp,
		 guint32  *data,
		 gint      count)
{
  guint32 tmp;
  gint i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
        {
          tmp = g_htonl (data[i]);
          xcf_write_int8 (fp, (guint8*) &tmp, 4);
        }
    }

  return count * 4;
}

guint
xcf_write_float (FILE     *fp,
		 gfloat   *data,
		 gint      count)
{
  return (xcf_write_int32(fp, (guint32 *)((void *)data), count));
}

guint
xcf_write_int8 (FILE     *fp,
		guint8   *data,
		gint      count)
{
  guint total;
  gint bytes;

  total = count;
  while (count > 0)
    {
      bytes = fwrite ((gchar*) data, sizeof (gchar), count, fp);
      count -= bytes;
      data += bytes;
    }

  return total;
}

guint
xcf_write_string (FILE     *fp,
		  gchar   **data,
		  gint      count)
{
  guint32 tmp;
  guint total;
  gint i;

  total = 0;
  for (i = 0; i < count; i++)
    {
      if (data[i])
        tmp = strlen (data[i]) + 1;
      else
        tmp = 0;

      xcf_write_int32 (fp, &tmp, 1);
      if (tmp > 0)
        xcf_write_int8 (fp, (guint8*) data[i], tmp);

      total += 4 + tmp;
    }

  return total;
}
