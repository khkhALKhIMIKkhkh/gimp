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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdata.h"
#include "core/gimpdatafactory.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdataeditor.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "data-commands.h"

#include "gimp-intl.h"


typedef struct _GimpDataDeleteData GimpDataDeleteData;

struct _GimpDataDeleteData
{
  GimpDataFactory *factory;
  GimpData        *data;
};


/*  local function prototypes  */

static void  data_delete_confirm_response (GtkWidget          *dialog,
                                           gint                response_id,
                                           GimpDataDeleteData *delete_data);


/*  public functions  */

void
data_open_as_image_cmd_callback (GtkAction *action,
                                 gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
			      view->factory->container->children_type);

  if (data && data->filename)
    {
      gchar *uri = g_filename_to_uri (data->filename, NULL, NULL);

      if (uri)
        {
          GimpImage         *image;
          GimpPDBStatusType  status;
          GError            *error = NULL;

          image = file_open_with_display (context->gimp, context, NULL, uri,
                                          &status, &error);

          if (! image && status != GIMP_PDB_CANCEL)
            {
              gchar *filename;

              filename = file_utils_uri_to_utf8_filename (uri);
              g_message (_("Opening '%s' failed:\n\n%s"),
                         filename, error->message);
              g_clear_error (&error);

              g_free (filename);
            }

          g_free (uri);
        }
    }
}

void
data_new_cmd_callback (GtkAction *action,
                       gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);

  if (view->factory->data_new_func)
    {
      GimpContext *context;
      GimpData    *data;

      context =
        gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

      data = gimp_data_factory_data_new (view->factory, _("Untitled"));

      if (data)
	{
	  gimp_context_set_by_type (context,
				    view->factory->container->children_type,
				    GIMP_OBJECT (data));

	  gtk_button_clicked (GTK_BUTTON (view->edit_button));
	}
    }
}

void
data_duplicate_cmd_callback (GtkAction *action,
                             gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
			      view->factory->container->children_type);

  if (data && gimp_container_have (view->factory->container,
				   GIMP_OBJECT (data)))
    {
      GimpData *new_data;

      new_data = gimp_data_factory_data_duplicate (view->factory, data);

      if (new_data)
	{
	  gimp_context_set_by_type (context,
				    view->factory->container->children_type,
				    GIMP_OBJECT (new_data));

	  gtk_button_clicked (GTK_BUTTON (view->edit_button));
	}
    }
}

void
data_delete_cmd_callback (GtkAction *action,
                          gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
			      view->factory->container->children_type);

  if (data && data->deletable && gimp_container_have (view->factory->container,
                                                      GIMP_OBJECT (data)))
    {
      GimpDataDeleteData *delete_data;
      GtkWidget          *dialog;

      delete_data = g_new0 (GimpDataDeleteData, 1);

      delete_data->factory = view->factory;
      delete_data->data    = data;

      dialog = gimp_message_dialog_new (_("Delete Object"), GIMP_STOCK_QUESTION,
                                        GTK_WIDGET (view), 0,
                                        gimp_standard_help_func, NULL,

                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_DELETE, GTK_RESPONSE_OK,

                                        NULL);

      g_signal_connect_object (data, "disconnect",
                               G_CALLBACK (gtk_widget_destroy),
                               dialog, G_CONNECT_SWAPPED);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (data_delete_confirm_response),
                        delete_data);

      gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                         _("Are you sure you want to delete "
                                           "'%s' from the list and from disk?"),
                                         GIMP_OBJECT (data)->name);
      gtk_widget_show (dialog);
    }
}

void
data_refresh_cmd_callback (GtkAction *action,
                           gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);

  gimp_data_factory_data_save (view->factory);
  gimp_data_factory_data_init (view->factory, FALSE);
}

void
data_edit_cmd_callback (GtkAction   *action,
                        const gchar *value,
                        gpointer     user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
			      view->factory->container->children_type);

  if (data && gimp_container_have (view->factory->container,
                                   GIMP_OBJECT (data)))
    {
      GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (view));
      GtkWidget *dockable;

      dockable = gimp_dialog_factory_dialog_raise (global_dock_factory, screen,
                                                   value, -1);

      gimp_data_editor_set_data (GIMP_DATA_EDITOR (GTK_BIN (dockable)->child),
                                 data);
    }
}


/*  private functions  */

static void
data_delete_confirm_response (GtkWidget          *dialog,
                              gint                response_id,
                              GimpDataDeleteData *delete_data)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      GError *error = NULL;

      if (! gimp_data_factory_data_delete (delete_data->factory,
                                           delete_data->data,
                                           TRUE, &error))
        {
          g_message (error->message);
          g_clear_error (&error);
        }
    }

  g_free (delete_data);
}
