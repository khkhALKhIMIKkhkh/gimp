/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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

/* NOTE: This file is autogenerated by pdbgen.pl. */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "pdb-types.h"
#include "gimppdb.h"
#include "gimpprocedure.h"
#include "core/gimpparamspecs.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer-filter.h"
#include "core/gimpcontainer.h"
#include "gimp-intl.h"

#include "internal_procs.h"


static GValueArray *
buffers_get_list_invoker (GimpProcedure     *procedure,
                          Gimp              *gimp,
                          GimpContext       *context,
                          GimpProgress      *progress,
                          const GValueArray *args)
{
  gboolean success = TRUE;
  GValueArray *return_vals;
  const gchar *filter;
  gint32 num_buffers = 0;
  gchar **buffer_list = NULL;

  filter = g_value_get_string (&args->values[0]);

  if (success)
    {
      buffer_list = gimp_container_get_filtered_name_array (gimp->named_buffers,
                                                            filter, &num_buffers);
    }

  return_vals = gimp_procedure_get_return_values (procedure, success);

  if (success)
    {
      g_value_set_int (&return_vals->values[1], num_buffers);
      gimp_value_take_stringarray (&return_vals->values[2], buffer_list, num_buffers);
    }

  return return_vals;
}

static GValueArray *
buffer_rename_invoker (GimpProcedure     *procedure,
                       Gimp              *gimp,
                       GimpContext       *context,
                       GimpProgress      *progress,
                       const GValueArray *args)
{
  gboolean success = TRUE;
  GValueArray *return_vals;
  const gchar *buffer_name;
  const gchar *new_name;
  gchar *real_name = NULL;

  buffer_name = g_value_get_string (&args->values[0]);
  new_name = g_value_get_string (&args->values[1]);

  if (success)
    {
      GimpBuffer *buffer = (GimpBuffer *)
        gimp_container_get_child_by_name (gimp->named_buffers, buffer_name);

      if (buffer && strlen (new_name))
        {
          gimp_object_set_name (GIMP_OBJECT (buffer), new_name);
          real_name = g_strdup (gimp_object_get_name (GIMP_OBJECT (buffer)));
        }
      else
        success = FALSE;
    }

  return_vals = gimp_procedure_get_return_values (procedure, success);

  if (success)
    g_value_take_string (&return_vals->values[1], real_name);

  return return_vals;
}

static GValueArray *
buffer_delete_invoker (GimpProcedure     *procedure,
                       Gimp              *gimp,
                       GimpContext       *context,
                       GimpProgress      *progress,
                       const GValueArray *args)
{
  gboolean success = TRUE;
  const gchar *buffer_name;

  buffer_name = g_value_get_string (&args->values[0]);

  if (success)
    {
      GimpBuffer *buffer = (GimpBuffer *)
        gimp_container_get_child_by_name (gimp->named_buffers, buffer_name);

      if (buffer)
        success = gimp_container_remove (gimp->named_buffers, GIMP_OBJECT (buffer));
      else
        success = FALSE;
    }

  return gimp_procedure_get_return_values (procedure, success);
}

static GValueArray *
buffer_get_width_invoker (GimpProcedure     *procedure,
                          Gimp              *gimp,
                          GimpContext       *context,
                          GimpProgress      *progress,
                          const GValueArray *args)
{
  gboolean success = TRUE;
  GValueArray *return_vals;
  const gchar *buffer_name;
  gint32 width = 0;

  buffer_name = g_value_get_string (&args->values[0]);

  if (success)
    {
      GimpBuffer *buffer = (GimpBuffer *)
        gimp_container_get_child_by_name (gimp->named_buffers, buffer_name);

      if (buffer)
        width = gimp_buffer_get_width (buffer);
      else
        success = FALSE;
    }

  return_vals = gimp_procedure_get_return_values (procedure, success);

  if (success)
    g_value_set_int (&return_vals->values[1], width);

  return return_vals;
}

static GValueArray *
buffer_get_height_invoker (GimpProcedure     *procedure,
                           Gimp              *gimp,
                           GimpContext       *context,
                           GimpProgress      *progress,
                           const GValueArray *args)
{
  gboolean success = TRUE;
  GValueArray *return_vals;
  const gchar *buffer_name;
  gint32 height = 0;

  buffer_name = g_value_get_string (&args->values[0]);

  if (success)
    {
      GimpBuffer *buffer = (GimpBuffer *)
        gimp_container_get_child_by_name (gimp->named_buffers, buffer_name);

      if (buffer)
        height = gimp_buffer_get_height (buffer);
      else
        success = FALSE;
    }

  return_vals = gimp_procedure_get_return_values (procedure, success);

  if (success)
    g_value_set_int (&return_vals->values[1], height);

  return return_vals;
}

static GValueArray *
buffer_get_bytes_invoker (GimpProcedure     *procedure,
                          Gimp              *gimp,
                          GimpContext       *context,
                          GimpProgress      *progress,
                          const GValueArray *args)
{
  gboolean success = TRUE;
  GValueArray *return_vals;
  const gchar *buffer_name;
  gint32 bytes = 0;

  buffer_name = g_value_get_string (&args->values[0]);

  if (success)
    {
      GimpBuffer *buffer = (GimpBuffer *)
        gimp_container_get_child_by_name (gimp->named_buffers, buffer_name);

      if (buffer)
        bytes = gimp_buffer_get_bytes (buffer);
      else
        success = FALSE;
    }

  return_vals = gimp_procedure_get_return_values (procedure, success);

  if (success)
    g_value_set_int (&return_vals->values[1], bytes);

  return return_vals;
}

static GValueArray *
buffer_get_image_type_invoker (GimpProcedure     *procedure,
                               Gimp              *gimp,
                               GimpContext       *context,
                               GimpProgress      *progress,
                               const GValueArray *args)
{
  gboolean success = TRUE;
  GValueArray *return_vals;
  const gchar *buffer_name;
  gint32 image_type = 0;

  buffer_name = g_value_get_string (&args->values[0]);

  if (success)
    {
      GimpBuffer *buffer = (GimpBuffer *)
        gimp_container_get_child_by_name (gimp->named_buffers, buffer_name);

      if (buffer)
        image_type = gimp_buffer_get_image_type (buffer);
      else
        success = FALSE;
    }

  return_vals = gimp_procedure_get_return_values (procedure, success);

  if (success)
    g_value_set_enum (&return_vals->values[1], image_type);

  return return_vals;
}

void
register_buffer_procs (GimpPDB *pdb)
{
  GimpProcedure *procedure;

  /*
   * gimp-buffers-get-list
   */
  procedure = gimp_procedure_new (buffers_get_list_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-buffers-get-list");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-buffers-get-list",
                                     "Retrieve a complete listing of the available buffers.",
                                     "This procedure returns a complete listing of available named buffers.",
                                     "Michael Natterer <mitch@gimp.org>",
                                     "Michael Natterer",
                                     "2005",
                                     NULL);
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("filter",
                                                       "filter",
                                                       "An optional regular expression used to filter the list",
                                                       FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("num-buffers",
                                                          "num buffers",
                                                          "The number of buffers",
                                                          0, G_MAXINT32, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_string_array ("buffer-list",
                                                                 "buffer list",
                                                                 "The list of buffer names",
                                                                 GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-buffer-rename
   */
  procedure = gimp_procedure_new (buffer_rename_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-buffer-rename");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-buffer-rename",
                                     "Renames a named buffer.",
                                     "This procedure renames a named buffer.",
                                     "Michael Natterer <mitch@gimp.org>",
                                     "Michael Natterer",
                                     "2005",
                                     NULL);
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("buffer-name",
                                                       "buffer name",
                                                       "The buffer name",
                                                       FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("new-name",
                                                       "new name",
                                                       "The buffer's new name",
                                                       FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_string ("real-name",
                                                           "real name",
                                                           "The real name given to the buffer",
                                                           FALSE, FALSE,
                                                           NULL,
                                                           GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-buffer-delete
   */
  procedure = gimp_procedure_new (buffer_delete_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-buffer-delete");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-buffer-delete",
                                     "Deletes a named buffer.",
                                     "This procedure deletes a named buffer.",
                                     "David Gowers <neota@softhome.net>",
                                     "David Gowers <neota@softhome.net>",
                                     "2005",
                                     NULL);
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("buffer-name",
                                                       "buffer name",
                                                       "The buffer name",
                                                       FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-buffer-get-width
   */
  procedure = gimp_procedure_new (buffer_get_width_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-buffer-get-width");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-buffer-get-width",
                                     "Retrieves the specified buffer's width.",
                                     "This procedure retrieves the specified named buffer's width.",
                                     "Michael Natterer <mitch@gimp.org>",
                                     "Michael Natterer",
                                     "2005",
                                     NULL);
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("buffer-name",
                                                       "buffer name",
                                                       "The buffer name",
                                                       FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("width",
                                                          "width",
                                                          "The buffer width",
                                                          G_MININT32, G_MAXINT32, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-buffer-get-height
   */
  procedure = gimp_procedure_new (buffer_get_height_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-buffer-get-height");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-buffer-get-height",
                                     "Retrieves the specified buffer's height.",
                                     "This procedure retrieves the specified named buffer's height.",
                                     "Michael Natterer <mitch@gimp.org>",
                                     "Michael Natterer",
                                     "2005",
                                     NULL);
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("buffer-name",
                                                       "buffer name",
                                                       "The buffer name",
                                                       FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("height",
                                                          "height",
                                                          "The buffer height",
                                                          G_MININT32, G_MAXINT32, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-buffer-get-bytes
   */
  procedure = gimp_procedure_new (buffer_get_bytes_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-buffer-get-bytes");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-buffer-get-bytes",
                                     "Retrieves the specified buffer's bytes.",
                                     "This procedure retrieves the specified named buffer's bytes.",
                                     "Michael Natterer <mitch@gimp.org>",
                                     "Michael Natterer",
                                     "2005",
                                     NULL);
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("buffer-name",
                                                       "buffer name",
                                                       "The buffer name",
                                                       FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("bytes",
                                                          "bytes",
                                                          "The buffer bpp",
                                                          G_MININT32, G_MAXINT32, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-buffer-get-image-type
   */
  procedure = gimp_procedure_new (buffer_get_image_type_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-buffer-get-image-type");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-buffer-get-image-type",
                                     "Retrieves the specified buffer's image type.",
                                     "This procedure retrieves the specified named buffer's image type.",
                                     "Michael Natterer <mitch@gimp.org>",
                                     "Michael Natterer",
                                     "2005",
                                     NULL);
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("buffer-name",
                                                       "buffer name",
                                                       "The buffer name",
                                                       FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   g_param_spec_enum ("image-type",
                                                      "image type",
                                                      "The buffer image type",
                                                      GIMP_TYPE_IMAGE_BASE_TYPE,
                                                      GIMP_RGB,
                                                      GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);
}
