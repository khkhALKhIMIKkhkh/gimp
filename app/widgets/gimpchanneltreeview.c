/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpchanneltreeview.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"

#include "gimpchanneltreeview.h"
#include "gimpcomponenteditor.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_channel_tree_view_class_init (GimpChannelTreeViewClass *klass);
static void   gimp_channel_tree_view_init       (GimpChannelTreeView      *view);

static void   gimp_channel_tree_view_view_iface_init   (GimpContainerViewInterface *view_iface);
static void   gimp_channel_tree_view_docked_iface_init (GimpDockedInterface *docked_iface);

static GObject * gimp_channel_tree_view_constructor   (GType              type,
                                                       guint              n_params,
                                                       GObjectConstructParam *params);
static void   gimp_channel_tree_view_drop_viewable    (GimpContainerTreeView *view,
                                                       GimpViewable      *src_viewable,
                                                       GimpViewable      *dest_viewable,
                                                       GtkTreeViewDropPosition  drop_pos);
static void   gimp_channel_tree_view_drop_component   (GimpContainerTreeView *tree_view,
                                                       GimpImage         *gimage,
                                                       GimpChannelType    component,
                                                       GimpViewable      *dest_viewable,
                                                       GtkTreeViewDropPosition drop_pos);
static void   gimp_channel_tree_view_set_image        (GimpItemTreeView  *item_view,
                                                       GimpImage         *gimage);
static GimpItem * gimp_channel_tree_view_item_new     (GimpImage         *image);

static void   gimp_channel_tree_view_set_preview_size (GimpContainerView *view);

static void   gimp_channel_tree_view_set_context      (GimpDocked        *docked,
                                                       GimpContext       *context);


static GimpDrawableTreeViewClass  *parent_class        = NULL;
static GimpContainerViewInterface *parent_view_iface   = NULL;
static GimpDockedInterface        *parent_docked_iface = NULL;


GType
gimp_channel_tree_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpChannelTreeViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_channel_tree_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpChannelTreeView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_channel_tree_view_init,
      };

      static const GInterfaceInfo view_iface_info =
      {
        (GInterfaceInitFunc) gimp_channel_tree_view_view_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };
      static const GInterfaceInfo docked_iface_info =
      {
        (GInterfaceInitFunc) gimp_channel_tree_view_docked_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      view_type = g_type_register_static (GIMP_TYPE_DRAWABLE_TREE_VIEW,
                                          "GimpChannelTreeView",
                                          &view_info, 0);

      g_type_add_interface_static (view_type, GIMP_TYPE_CONTAINER_VIEW,
                                   &view_iface_info);
      g_type_add_interface_static (view_type, GIMP_TYPE_DOCKED,
                                   &docked_iface_info);
    }

  return view_type;
}

static void
gimp_channel_tree_view_class_init (GimpChannelTreeViewClass *klass)
{
  GObjectClass               *object_class;
  GimpContainerTreeViewClass *view_class;
  GimpItemTreeViewClass      *item_view_class;

  object_class    = G_OBJECT_CLASS (klass);
  view_class      = GIMP_CONTAINER_TREE_VIEW_CLASS (klass);
  item_view_class = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor        = gimp_channel_tree_view_constructor;

  view_class->drop_viewable        = gimp_channel_tree_view_drop_viewable;
  view_class->drop_component       = gimp_channel_tree_view_drop_component;

  item_view_class->set_image       = gimp_channel_tree_view_set_image;

  item_view_class->item_type       = GIMP_TYPE_CHANNEL;
  item_view_class->signal_name     = "active-channel-changed";

  item_view_class->get_container   = gimp_image_get_channels;
  item_view_class->get_active_item = (GimpGetItemFunc) gimp_image_get_active_channel;
  item_view_class->set_active_item = (GimpSetItemFunc) gimp_image_set_active_channel;
  item_view_class->reorder_item    = (GimpReorderItemFunc) gimp_image_position_channel;
  item_view_class->add_item        = (GimpAddItemFunc) gimp_image_add_channel;
  item_view_class->remove_item     = (GimpRemoveItemFunc) gimp_image_remove_channel;
  item_view_class->new_item        = gimp_channel_tree_view_item_new;

  item_view_class->action_group        = "channels";
  item_view_class->activate_action     = "channels-edit-attributes";
  item_view_class->edit_action         = "channels-edit-attributes";
  item_view_class->new_action          = "channels-new";
  item_view_class->new_default_action  = "channels-new-last-values";
  item_view_class->raise_action        = "channels-raise";
  item_view_class->raise_top_action    = "channels-raise-to-top";
  item_view_class->lower_action        = "channels-lower";
  item_view_class->lower_bottom_action = "channels-lower-to-bottom";
  item_view_class->duplicate_action    = "channels-duplicate";
  item_view_class->delete_action       = "channels-delete";
  item_view_class->reorder_desc        = _("Reorder Channel");
}

static void
gimp_channel_tree_view_init (GimpChannelTreeView *view)
{
}

static void
gimp_channel_tree_view_view_iface_init (GimpContainerViewInterface *view_iface)
{
  parent_view_iface = g_type_interface_peek_parent (view_iface);

  view_iface->set_preview_size = gimp_channel_tree_view_set_preview_size;
}

static void
gimp_channel_tree_view_docked_iface_init (GimpDockedInterface *docked_iface)
{
  parent_docked_iface = g_type_interface_peek_parent (docked_iface);

  docked_iface->set_context = gimp_channel_tree_view_set_context;
}

static GObject *
gimp_channel_tree_view_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject               *object;
  GimpEditor            *editor;
  GimpChannelTreeView   *view;
  GimpContainerTreeView *tree_view;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor    = GIMP_EDITOR (object);
  view      = GIMP_CHANNEL_TREE_VIEW (object);
  tree_view = GIMP_CONTAINER_TREE_VIEW (object);

  gimp_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), GIMP_TYPE_LAYER,
                               NULL, tree_view);
  gimp_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), GIMP_TYPE_LAYER_MASK,
                               NULL, tree_view);
  gimp_dnd_component_dest_add (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);

  view->toselection_button =
    gimp_editor_add_action_button (GIMP_EDITOR (view), "channels",
                                   "channels-selection-replace",
                                   "channels-selection-add",
                                   GDK_SHIFT_MASK,
                                   "channels-selection-subtract",
                                   GDK_CONTROL_MASK,
                                   "channels-selection-intersect",
                                   GDK_SHIFT_MASK | GDK_CONTROL_MASK,
                                   NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (view),
				  GTK_BUTTON (view->toselection_button),
				  GIMP_TYPE_CHANNEL);
  gtk_box_reorder_child (GTK_BOX (GIMP_EDITOR (view)->button_box),
			 view->toselection_button, 5);

  return object;
}


/*  GimpContainerTreeView methods  */

static void
gimp_channel_tree_view_drop_viewable (GimpContainerTreeView   *tree_view,
                                      GimpViewable            *src_viewable,
                                      GimpViewable            *dest_viewable,
                                      GtkTreeViewDropPosition  drop_pos)
{
  GimpContainerView     *container_view = GIMP_CONTAINER_VIEW (tree_view);
  GimpItemTreeView      *item_view      = GIMP_ITEM_TREE_VIEW (tree_view);
  GimpItemTreeViewClass *item_view_class;
  GimpContainer         *container;

  container = gimp_container_view_get_container (container_view);

  item_view_class = GIMP_ITEM_TREE_VIEW_GET_CLASS (item_view);

  if (item_view->gimage != gimp_item_get_image (GIMP_ITEM (src_viewable)) ||
      G_TYPE_FROM_INSTANCE (src_viewable) != item_view_class->item_type)
    {
      GimpItem *new_item;
      gint      dest_index;

      dest_index = gimp_container_get_child_index (container,
                                                   GIMP_OBJECT (dest_viewable));

     if (drop_pos == GTK_TREE_VIEW_DROP_AFTER)
        dest_index++;

      new_item = gimp_item_convert (GIMP_ITEM (src_viewable),
                                    item_view->gimage,
                                    item_view_class->item_type, FALSE);

      item_view_class->add_item (item_view->gimage, new_item, dest_index);
      gimp_image_flush (item_view->gimage);
    }
  else
    {
      GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_viewable (tree_view,
                                                                    src_viewable,
                                                                    dest_viewable,
                                                                    drop_pos);
    }
}

static void
gimp_channel_tree_view_drop_component (GimpContainerTreeView   *tree_view,
                                       GimpImage               *src_image,
                                       GimpChannelType          component,
                                       GimpViewable            *dest_viewable,
                                       GtkTreeViewDropPosition  drop_pos)
{
  GimpItemTreeView *view       = GIMP_ITEM_TREE_VIEW (tree_view);
  GimpImage        *dest_image = view->gimage;
  GimpChannel      *new_channel;
  gint              index;
  const gchar      *desc;
  gchar            *name;

  index = gimp_image_get_channel_index (dest_image,
                                        GIMP_CHANNEL (dest_viewable));

  if (drop_pos == GTK_TREE_VIEW_DROP_AFTER)
    index++;

  gimp_enum_get_value (GIMP_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);
  name = g_strdup_printf (_("%s Channel Copy"), desc);

  new_channel = gimp_channel_new_from_component (src_image, component,
                                                 name, NULL);

  /*  copied components are invisible by default so subsequent copies
   *  of components don't affect each other
   */
  gimp_item_set_visible (GIMP_ITEM (new_channel), FALSE, FALSE);

  g_free (name);

  if (src_image != dest_image)
    GIMP_ITEM_GET_CLASS (new_channel)->convert (GIMP_ITEM (new_channel),
                                                dest_image);

  gimp_image_add_channel (dest_image, new_channel, index);
  gimp_image_flush (dest_image);
}


/*  GimpItemTreeView methods  */

static void
gimp_channel_tree_view_set_image (GimpItemTreeView *item_view,
				  GimpImage        *gimage)
{
  GimpChannelTreeView *channel_view = GIMP_CHANNEL_TREE_VIEW (item_view);

  if (! channel_view->component_editor)
    {
      GimpContainerView *view = GIMP_CONTAINER_VIEW (item_view);
      gint               preview_size;

      preview_size = gimp_container_view_get_preview_size (view, NULL);

      channel_view->component_editor =
        gimp_component_editor_new (preview_size,
                                   GIMP_EDITOR (item_view)->menu_factory);
      gimp_docked_set_context (GIMP_DOCKED (channel_view->component_editor),
                               item_view->context);
      gtk_box_pack_start (GTK_BOX (item_view), channel_view->component_editor,
                          FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (item_view),
                             channel_view->component_editor, 0);
    }

  if (! gimage)
    gtk_widget_hide (channel_view->component_editor);

  gimp_image_editor_set_image (GIMP_IMAGE_EDITOR (channel_view->component_editor),
                               gimage);

  GIMP_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (item_view, gimage);

  if (item_view->gimage)
    gtk_widget_show (channel_view->component_editor);
}

static GimpItem *
gimp_channel_tree_view_item_new (GimpImage *image)
{
  GimpChannel *new_channel;
  GimpRGB      color;

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 0.5);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                               _("New Channel"));

  new_channel = gimp_channel_new (image,
                                  gimp_image_get_width (image),
                                  gimp_image_get_height (image),
                                  _("Empty Channel"), &color);

  gimp_image_add_channel (image, new_channel, -1);

  gimp_image_undo_group_end (image);

  return GIMP_ITEM (new_channel);
}


/*  GimpContainerView methods  */

static void
gimp_channel_tree_view_set_preview_size (GimpContainerView *view)
{
  GimpChannelTreeView *channel_view = GIMP_CHANNEL_TREE_VIEW (view);
  gint                 preview_size;

  parent_view_iface->set_preview_size (view);

  preview_size = gimp_container_view_get_preview_size (view, NULL);

  if (channel_view->component_editor)
    gimp_component_editor_set_preview_size (GIMP_COMPONENT_EDITOR (channel_view->component_editor),
                                            preview_size);
}


/*  GimpDocked methods  */

static void
gimp_channel_tree_view_set_context (GimpDocked  *docked,
                                    GimpContext *context)
{
  GimpChannelTreeView *channel_view = GIMP_CHANNEL_TREE_VIEW (docked);

  parent_docked_iface->set_context (docked, context);

  if (channel_view->component_editor)
    gimp_docked_set_context (GIMP_DOCKED (channel_view->component_editor),
                             context);
}
