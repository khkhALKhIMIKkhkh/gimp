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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpcontroller.h"

#include "widgets-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-error.h"
#include "config/gimpconfig-params.h"
#include "config/gimpconfig-utils.h"
#include "config/gimpconfigwriter.h"
#include "config/gimpscanner.h"

#include "core/gimp.h"
#include "core/gimplist.h"

#include "gimpcontrollerinfo.h"
#include "gimpcontrollers.h"
#include "gimpcontrollerwheel.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


#define GIMP_CONTROLLER_MANAGER_DATA_KEY "gimp-controller-manager"


typedef struct _GimpControllerManager GimpControllerManager;

struct _GimpControllerManager
{
  GimpContainer  *controllers;
  GQuark          event_mapped_id;
  GimpController *wheel;
  GimpUIManager  *ui_manager;
};


/*  local function prototypes  */

static GimpControllerManager * gimp_controller_manager_get  (Gimp *gimp);
static void   gimp_controller_manager_free (GimpControllerManager *manager);

static gboolean gimp_controllers_event_mapped (GimpControllerInfo        *info,
                                               GimpController            *controller,
                                               const GimpControllerEvent *event,
                                               const gchar               *action_name,
                                               GimpControllerManager     *manager);


/*  public functions  */

void
gimp_controllers_init (Gimp *gimp)
{
  GimpControllerManager *manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp_controller_manager_get (gimp) == NULL);

  manager = g_new0 (GimpControllerManager, 1);

  g_object_set_data_full (G_OBJECT (gimp),
                          GIMP_CONTROLLER_MANAGER_DATA_KEY, manager,
                          (GDestroyNotify) gimp_controller_manager_free);

  manager->controllers = gimp_list_new (GIMP_TYPE_CONTROLLER_INFO, TRUE);

  manager->event_mapped_id =
    gimp_container_add_handler (manager->controllers, "event-mapped",
                                G_CALLBACK (gimp_controllers_event_mapped),
                                manager);

  /*  EEEEEEK  */
  {
    static const GInterfaceInfo config_iface_info =
    {
      NULL,  /* iface_init     */
      NULL,  /* iface_finalize */
      NULL   /* iface_data     */
    };

    g_type_add_interface_static (GIMP_TYPE_CONTROLLER, GIMP_TYPE_CONFIG,
                                 &config_iface_info);
  }

  g_type_class_ref (GIMP_TYPE_CONTROLLER_WHEEL);
}

void
gimp_controllers_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp_controller_manager_get (gimp) != NULL);

  g_object_set_data (G_OBJECT (gimp), GIMP_CONTROLLER_MANAGER_DATA_KEY, NULL);

  g_type_class_unref (g_type_class_peek (GIMP_TYPE_CONTROLLER_WHEEL));
}

void
gimp_controllers_restore (Gimp          *gimp,
                          GimpUIManager *ui_manager)
{
  GimpControllerManager *manager;
  gchar                 *filename;
  GError                *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_UI_MANAGER (ui_manager));

  manager = gimp_controller_manager_get (gimp);

  g_return_if_fail (manager != NULL);
  g_return_if_fail (manager->ui_manager == NULL);

  manager->ui_manager = g_object_ref (ui_manager);

  filename = gimp_personal_rc_file ("controllerrc");

  if (! gimp_config_deserialize_file (GIMP_CONFIG (manager->controllers),
                                      filename, NULL, &error))
    {
      if (error->code == GIMP_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_clear_error (&error);
          g_free (filename);

          filename = g_build_filename (gimp_sysconf_directory (),
                                       "controllerrc", NULL);

          if (! gimp_config_deserialize_file (GIMP_CONFIG (manager->controllers),
                                              filename, NULL, &error))
            {
              g_message (error->message);
            }
        }
      else
        {
          g_message (error->message);
        }

      g_clear_error (&error);
    }

  gimp_list_reverse (GIMP_LIST (manager->controllers));

  g_free (filename);
}

void
gimp_controllers_save (Gimp *gimp)
{
  const gchar *header =
    "GIMP controllerrc\n"
    "\n"
    "This file will be entirely rewritten every time you quit the gimp.";
  const gchar *footer =
    "end of controllerrc";

  GimpControllerManager *manager;
  gchar                 *filename;
  GError                *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_controller_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  filename = gimp_personal_rc_file ("controllerrc");

  if (! gimp_config_serialize_to_file (GIMP_CONFIG (manager->controllers),
                                       filename,
                                       header, footer, NULL,
                                       &error))
    {
      g_message (error->message);
      g_error_free (error);
    }

  g_free (filename);
}

GimpContainer *
gimp_controllers_get_list (Gimp *gimp)
{
  GimpControllerManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  manager = gimp_controller_manager_get (gimp);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->controllers;
}

GimpController *
gimp_controllers_get_wheel (Gimp *gimp)
{
  GimpControllerManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  manager = gimp_controller_manager_get (gimp);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->wheel;
}

/*  private functions  */

static GimpControllerManager *
gimp_controller_manager_get (Gimp *gimp)
{
  return g_object_get_data (G_OBJECT (gimp), GIMP_CONTROLLER_MANAGER_DATA_KEY);
}

static void
gimp_controller_manager_free (GimpControllerManager *manager)
{
  gimp_container_remove_handler (manager->controllers,
                                 manager->event_mapped_id);

  g_object_unref (manager->controllers);
  g_object_unref (manager->ui_manager);

  g_free (manager);
}

static gboolean
gimp_controllers_event_mapped (GimpControllerInfo        *info,
                               GimpController            *controller,
                               const GimpControllerEvent *event,
                               const gchar               *action_name,
                               GimpControllerManager     *manager)
{
  GList *list;

  for (list = gtk_ui_manager_get_action_groups (GTK_UI_MANAGER (manager->ui_manager));
       list;
       list = g_list_next (list))
    {
      GtkActionGroup *group = list->data;
      GtkAction      *action;

      action = gtk_action_group_get_action (group, action_name);

      if (action)
        {
          gtk_action_activate (action);
          return TRUE;
        }
    }

  return FALSE;
}
