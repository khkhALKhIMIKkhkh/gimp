/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplayertreeview.c
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimpimage.h"
#include "core/gimpitemundo.h"
#include "core/gimpundostack.h"

#include "gimpcellrenderertoggle.h"
#include "gimpcellrendererviewable.h"
#include "gimpdnd.h"
#include "gimplayertreeview.h"
#include "gimppreviewrenderer.h"
#include "gimpwidgets-constructors.h"

#include "libgimp/gimpintl.h"


static void   gimp_layer_tree_view_class_init (GimpLayerTreeViewClass *klass);
static void   gimp_layer_tree_view_init       (GimpLayerTreeView      *view);

static GObject * gimp_layer_tree_view_constructor (GType                type,
                                                   guint                n_params,
                                                   GObjectConstructParam *params);
static void   gimp_layer_tree_view_style_set      (GtkWidget           *widget,
						   GtkStyle            *prev_style);

static void   gimp_layer_tree_view_set_container  (GimpContainerView   *view,
						   GimpContainer       *container);
static gpointer gimp_layer_tree_view_insert_item  (GimpContainerView   *view,
                                                   GimpViewable        *viewable,
                                                   gint                 index);
static void   gimp_layer_tree_view_select_item    (GimpContainerView   *view,
						   GimpViewable        *item,
						   gpointer             insert_data);
static void   gimp_layer_tree_view_set_preview_size (GimpContainerView *view);

static gboolean gimp_layer_tree_view_drop_possible  (GimpContainerTreeView *view,
                                                     GimpViewable      *src_viewable,
                                                     GimpViewable      *dest_viewable,
                                                     GtkTreeViewDropPosition  drop_pos,
                                                     GdkDragAction     *drag_action);

static void   gimp_layer_tree_view_remove_item    (GimpImage           *gimage,
                                                   GimpItem            *layer);

static void   gimp_layer_tree_view_anchor_clicked (GtkWidget           *widget,
						   GimpLayerTreeView   *view);

static void   gimp_layer_tree_view_paint_mode_menu_callback
                                                  (GtkWidget           *widget,
						   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_preserve_button_toggled
                                                  (GtkWidget           *widget,
						   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_opacity_scale_changed
                                                  (GtkAdjustment       *adj,
						   GimpLayerTreeView   *view);

static void   gimp_layer_tree_view_layer_signal_handler
                                                  (GimpLayer           *layer,
						   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_update_options (GimpLayerTreeView   *view,
						   GimpLayer           *layer);

static void   gimp_layer_tree_view_linked_changed (GimpLayer           *layer,
                                                   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_chain_clicked  (GtkCellRendererToggle *toggle,
                                                   gchar               *path,
                                                   GdkModifierType      state,
                                                   GimpLayerTreeView   *view);

static void   gimp_layer_tree_view_mask_update    (GimpLayerTreeView   *view,
                                                   GtkTreeIter         *iter,
                                                   GimpLayer           *layer);
static void   gimp_layer_tree_view_mask_changed   (GimpLayer           *layer,
                                                   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_renderer_update(GimpPreviewRenderer *renderer,
                                                   GimpLayerTreeView   *view);

static void   gimp_layer_tree_view_update_borders (GimpLayerTreeView   *view,
                                                   GtkTreeIter         *iter);
static void   gimp_layer_tree_view_mask_callback  (GimpLayerMask       *mask,
                                                   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_layer_clicked  (GimpCellRendererViewable *cell,
                                                   const gchar         *path,
                                                   GdkModifierType      state,
                                                   GimpLayerTreeView   *view);
static void   gimp_layer_tree_view_mask_clicked   (GimpCellRendererViewable *cell,
                                                   const gchar         *path,
                                                   GdkModifierType      state,
                                                   GimpLayerTreeView   *view);


static GimpDrawableTreeViewClass *parent_class = NULL;

static GimpRGB  black_color;
static GimpRGB  white_color;
static GimpRGB  green_color;
static GimpRGB  red_color;


GType
gimp_layer_tree_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpLayerTreeViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_layer_tree_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpLayerTreeView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_layer_tree_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_DRAWABLE_TREE_VIEW,
                                          "GimpLayerTreeView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_layer_tree_view_class_init (GimpLayerTreeViewClass *klass)
{
  GObjectClass               *object_class;
  GtkWidgetClass             *widget_class;
  GimpContainerViewClass     *container_view_class;
  GimpContainerTreeViewClass *tree_view_class;
  GimpItemTreeViewClass      *item_view_class;

  object_class         = G_OBJECT_CLASS (klass);
  widget_class         = GTK_WIDGET_CLASS (klass);
  container_view_class = GIMP_CONTAINER_VIEW_CLASS (klass);
  tree_view_class      = GIMP_CONTAINER_TREE_VIEW_CLASS (klass);
  item_view_class      = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gimp_layer_tree_view_constructor;

  widget_class->style_set   = gimp_layer_tree_view_style_set;

  container_view_class->set_container    = gimp_layer_tree_view_set_container;
  container_view_class->insert_item      = gimp_layer_tree_view_insert_item;
  container_view_class->select_item      = gimp_layer_tree_view_select_item;
  container_view_class->set_preview_size = gimp_layer_tree_view_set_preview_size;

  tree_view_class->drop_possible   = gimp_layer_tree_view_drop_possible;

  item_view_class->get_container   = gimp_image_get_layers;
  item_view_class->get_active_item = (GimpGetItemFunc) gimp_image_get_active_layer;
  item_view_class->set_active_item = (GimpSetItemFunc) gimp_image_set_active_layer;
  item_view_class->reorder_item    = (GimpReorderItemFunc) gimp_image_position_layer;
  item_view_class->add_item        = (GimpAddItemFunc) gimp_image_add_layer;
  item_view_class->remove_item     = gimp_layer_tree_view_remove_item;
  item_view_class->convert_item    = (GimpConvertItemFunc) gimp_layer_new_from_drawable;

  item_view_class->new_desc             = _("New Layer");
  item_view_class->duplicate_desc       = _("Duplicate Layer");
  item_view_class->edit_desc            = _("Edit Layer Attributes");
  item_view_class->delete_desc          = _("Delete Layer");
  item_view_class->raise_desc           = _("Raise Layer");
  item_view_class->raise_to_top_desc    = _("Raise Layer to Top");
  item_view_class->lower_desc           = _("Lower Layer");
  item_view_class->lower_to_bottom_desc = _("Lower Layer to Bottom");
  item_view_class->reorder_desc         = _("Reorder Layer");

  gimp_rgba_set (&black_color, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&white_color, 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&green_color, 0.0, 1.0, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&red_color,   1.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
}

static void
gimp_layer_tree_view_init (GimpLayerTreeView *view)
{
  GimpContainerTreeView *tree_view;
  GimpDrawableTreeView  *drawable_view;
  GtkWidget             *abox;
  GtkWidget             *hbox;

  tree_view     = GIMP_CONTAINER_TREE_VIEW (view);
  drawable_view = GIMP_DRAWABLE_TREE_VIEW (view);

  view->model_column_linked = tree_view->n_model_columns;
  tree_view->model_columns[tree_view->n_model_columns++] = G_TYPE_BOOLEAN;

  view->model_column_mask = tree_view->n_model_columns;
  tree_view->model_columns[tree_view->n_model_columns++] = GIMP_TYPE_PREVIEW_RENDERER;

  view->model_column_mask_visible = tree_view->n_model_columns;
  tree_view->model_columns[tree_view->n_model_columns++] = G_TYPE_BOOLEAN;

  tree_view->preview_border_width = 2;

  view->options_box = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (view->options_box), 2);
  gtk_box_pack_start (GTK_BOX (view), view->options_box, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (view), view->options_box, 0);
  gtk_widget_show (view->options_box);

  hbox = gtk_hbox_new (FALSE, 2);

  /*  Paint mode menu  */

  view->paint_mode_menu =
    gimp_paint_mode_menu_new (G_CALLBACK (gimp_layer_tree_view_paint_mode_menu_callback),
			      view,
			      FALSE,
			      GIMP_NORMAL_MODE);
  gtk_box_pack_start (GTK_BOX (hbox), view->paint_mode_menu, FALSE, FALSE, 0);
  gtk_widget_show (view->paint_mode_menu);

  gimp_help_set_help_data (view->paint_mode_menu, 
			   NULL, "#paint_mode_menu");

  /*  Preserve transparency toggle  */

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  view->preserve_trans_toggle =
    gtk_toggle_button_new_with_label (_("Keep Trans."));
  gtk_container_add (GTK_CONTAINER (abox), view->preserve_trans_toggle);
  gtk_widget_show (view->preserve_trans_toggle);

  g_signal_connect (view->preserve_trans_toggle, "toggled",
		    G_CALLBACK (gimp_layer_tree_view_preserve_button_toggled),
		    view);

  gimp_help_set_help_data (view->preserve_trans_toggle,
			   _("Keep Transparency"), "#keep_trans_button");

  gimp_table_attach_aligned (GTK_TABLE (view->options_box), 0, 0,
			     _("Mode:"), 1.0, 0.5,
			     hbox, 2, TRUE);

  /*  Opacity scale  */

  view->opacity_adjustment =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (view->options_box), 0, 1,
					  _("Opacity:"), -1, -1,
					  100.0, 0.0, 100.0, 1.0, 10.0, 1,
					  TRUE, 0.0, 0.0,
					  NULL, "#opacity_sacle"));

  g_signal_connect (view->opacity_adjustment, "value_changed",
		    G_CALLBACK (gimp_layer_tree_view_opacity_scale_changed),
		    view);

  /*  Anchor button  */

  view->anchor_button =
    gimp_editor_add_button (GIMP_EDITOR (view),
                            GIMP_STOCK_ANCHOR, _("Anchor Floating Layer"), NULL,
                            G_CALLBACK (gimp_layer_tree_view_anchor_clicked),
                            NULL,
                            view);

  gtk_box_reorder_child (GTK_BOX (GIMP_EDITOR (view)->button_box),
			 view->anchor_button, 5);

  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (view),
				  GTK_BUTTON (view->anchor_button),
				  GIMP_TYPE_LAYER);

  gtk_widget_set_sensitive (view->options_box,   FALSE);
  gtk_widget_set_sensitive (view->anchor_button, FALSE);

  view->mode_changed_handler_id           = 0;
  view->opacity_changed_handler_id        = 0;
  view->preserve_trans_changed_handler_id = 0;
  view->linked_changed_handler_id         = 0;
  view->mask_changed_handler_id           = 0;
}

static GObject *
gimp_layer_tree_view_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GimpContainerTreeView *tree_view;
  GimpLayerTreeView     *layer_view;
  GObject               *object;
  GtkTreeViewColumn     *column;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  tree_view  = GIMP_CONTAINER_TREE_VIEW (object);
  layer_view = GIMP_LAYER_TREE_VIEW (object);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_insert_column (tree_view->view, column, 1);

  layer_view->chain_cell = gimp_cell_renderer_toggle_new (GIMP_STOCK_LINKED);
  gtk_tree_view_column_pack_start (column, layer_view->chain_cell, FALSE);
  gtk_tree_view_column_set_attributes (column, layer_view->chain_cell,
                                       "active",
                                       layer_view->model_column_linked,
                                       NULL);

  layer_view->mask_cell = gimp_cell_renderer_viewable_new ();
  gtk_tree_view_column_pack_start (tree_view->main_column,
                                   layer_view->mask_cell,
                                   FALSE);
  gtk_tree_view_column_set_attributes (tree_view->main_column,
                                       layer_view->mask_cell,
                                       "renderer",
                                       layer_view->model_column_mask,
                                       "visible",
                                       layer_view->model_column_mask_visible,
                                       NULL);

  tree_view->toggle_cells = g_list_prepend (tree_view->toggle_cells,
                                            layer_view->chain_cell);
  tree_view->renderer_cells = g_list_prepend (tree_view->renderer_cells,
                                              layer_view->mask_cell);

  g_signal_connect (layer_view->chain_cell, "clicked",
                    G_CALLBACK (gimp_layer_tree_view_chain_clicked),
                    layer_view);

  g_signal_connect (tree_view->renderer_cell, "clicked",
                    G_CALLBACK (gimp_layer_tree_view_layer_clicked),
                    layer_view);
  g_signal_connect (layer_view->mask_cell, "clicked",
                    G_CALLBACK (gimp_layer_tree_view_mask_clicked),
                    layer_view);

  return object;
}

static void
gimp_layer_tree_view_style_set (GtkWidget *widget,
				GtkStyle  *prev_style)
{
  GimpLayerTreeView *layer_view;
  gint               content_spacing;
  gint               button_spacing;

  layer_view = GIMP_LAYER_TREE_VIEW (widget);

  gtk_widget_style_get (widget,
                        "content_spacing", &content_spacing,
                        "button_spacing",  &button_spacing,
			NULL);

  gtk_table_set_col_spacings (GTK_TABLE (layer_view->options_box),
			      button_spacing);
  gtk_table_set_row_spacings (GTK_TABLE (layer_view->options_box),
			      content_spacing);

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}


/*  GimpContainerView methods  */

static void
gimp_layer_tree_view_set_container (GimpContainerView *view,
				    GimpContainer     *container)
{
  GimpLayerTreeView *layer_view;

  layer_view = GIMP_LAYER_TREE_VIEW (view);

  if (view->container)
    {
      gimp_container_remove_handler (view->container,
				     layer_view->mode_changed_handler_id);
      gimp_container_remove_handler (view->container,
				     layer_view->opacity_changed_handler_id);
      gimp_container_remove_handler (view->container,
				     layer_view->preserve_trans_changed_handler_id);
      gimp_container_remove_handler (view->container,
				     layer_view->linked_changed_handler_id);
      gimp_container_remove_handler (view->container,
				     layer_view->mask_changed_handler_id);
    }

  GIMP_CONTAINER_VIEW_CLASS (parent_class)->set_container (view, container);

  if (view->container)
    {
      layer_view->mode_changed_handler_id =
	gimp_container_add_handler (view->container, "mode_changed",
				    G_CALLBACK (gimp_layer_tree_view_layer_signal_handler),
				    view);
      layer_view->opacity_changed_handler_id =
	gimp_container_add_handler (view->container, "opacity_changed",
				    G_CALLBACK (gimp_layer_tree_view_layer_signal_handler),
				    view);
      layer_view->preserve_trans_changed_handler_id =
	gimp_container_add_handler (view->container, "preserve_trans_changed",
				    G_CALLBACK (gimp_layer_tree_view_layer_signal_handler),
				    view);
      layer_view->linked_changed_handler_id =
	gimp_container_add_handler (view->container, "linked_changed",
				    G_CALLBACK (gimp_layer_tree_view_linked_changed),
				    view);
      layer_view->mask_changed_handler_id =
	gimp_container_add_handler (view->container, "mask_changed",
				    G_CALLBACK (gimp_layer_tree_view_mask_changed),
				    view);
    }
}

static gpointer
gimp_layer_tree_view_insert_item (GimpContainerView *view,
                                  GimpViewable      *viewable,
                                  gint               index)
{
  GimpContainerTreeView *tree_view;
  GimpLayerTreeView     *layer_view;
  GimpLayer             *layer;
  GtkTreeIter           *iter;

  tree_view  = GIMP_CONTAINER_TREE_VIEW (view);
  layer_view = GIMP_LAYER_TREE_VIEW (view);

  iter = GIMP_CONTAINER_VIEW_CLASS (parent_class)->insert_item (view, viewable,
                                                                index);

  layer = GIMP_LAYER (viewable);

  gtk_list_store_set (GTK_LIST_STORE (tree_view->model), iter,
                      layer_view->model_column_linked,
                      gimp_layer_get_linked (layer),
                      -1);

  gimp_layer_tree_view_mask_update (layer_view, iter, layer);

  return iter;
}

static void
gimp_layer_tree_view_select_item (GimpContainerView *view,
				  GimpViewable      *item,
				  gpointer           insert_data)
{
  GimpItemTreeView  *item_view;
  GimpLayerTreeView *layer_view;
  gboolean           options_sensitive   = FALSE;
  gboolean           anchor_sensitive    = FALSE;
  gboolean           raise_sensitive     = FALSE;

  item_view  = GIMP_ITEM_TREE_VIEW (view);
  layer_view = GIMP_LAYER_TREE_VIEW (view);

  GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item (view, item,
                                                         insert_data);

  if (item)
    {
      gimp_layer_tree_view_update_borders (layer_view,
                                           (GtkTreeIter *) insert_data);
      gimp_layer_tree_view_update_options (layer_view, GIMP_LAYER (item));

      options_sensitive = TRUE;

      if (gimp_layer_is_floating_sel (GIMP_LAYER (item)))
	{
	  anchor_sensitive = TRUE;

	  gtk_widget_set_sensitive (item_view->lower_button,     FALSE);
	  gtk_widget_set_sensitive (item_view->duplicate_button, FALSE);
	  gtk_widget_set_sensitive (item_view->edit_button,      FALSE);
	}
      else
	{
	  if (gimp_drawable_has_alpha (GIMP_DRAWABLE (item)) &&
	      gimp_container_get_child_index (view->container,
					      GIMP_OBJECT (item)))
	    {
	      raise_sensitive = TRUE;
	    }
	}
    }

  gtk_widget_set_sensitive (layer_view->options_box,   options_sensitive);
  gtk_widget_set_sensitive (item_view->raise_button,   raise_sensitive);
  gtk_widget_set_sensitive (layer_view->anchor_button, anchor_sensitive);
}

static void
gimp_layer_tree_view_set_preview_size (GimpContainerView *view)
{
  GimpContainerTreeView *tree_view;
  GimpLayerTreeView     *layer_view;
  GtkTreeIter            iter;
  gboolean               iter_valid;

  tree_view  = GIMP_CONTAINER_TREE_VIEW (view);
  layer_view = GIMP_LAYER_TREE_VIEW (view);

  for (iter_valid = gtk_tree_model_get_iter_first (tree_view->model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (tree_view->model, &iter))
    {
      GimpPreviewRenderer *renderer;

      gtk_tree_model_get (tree_view->model, &iter,
                          layer_view->model_column_mask, &renderer,
                          -1);

      if (renderer)
        {
          gimp_preview_renderer_set_size (renderer, view->preview_size,
                                          tree_view->preview_border_width);
          g_object_unref (renderer);
        }
    }

  GIMP_CONTAINER_VIEW_CLASS (parent_class)->set_preview_size (view);
}


/*  GimpContainerTreeView methods  */

static gboolean
gimp_layer_tree_view_drop_possible (GimpContainerTreeView   *tree_view,
                                    GimpViewable            *src_viewable,
                                    GimpViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos,
                                    GdkDragAction           *drag_action)
{
  GimpLayer *src_layer;
  GimpLayer *dest_layer;
  GimpImage *src_image;
  GimpImage *dest_image;

  src_layer  = GIMP_LAYER (src_viewable);
  dest_layer = GIMP_LAYER (dest_viewable);

  src_image  = gimp_item_get_image (GIMP_ITEM (src_layer));
  dest_image = gimp_item_get_image (GIMP_ITEM (dest_layer));

  if (gimp_image_floating_sel (dest_image))
    return FALSE;

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (dest_layer)) &&
      drop_pos == GTK_TREE_VIEW_DROP_AFTER)
    return FALSE;

  if (src_image == dest_image &&
      ! gimp_drawable_has_alpha (GIMP_DRAWABLE (src_layer)))
    return FALSE;

  return GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (tree_view,
                                                                       src_viewable,
                                                                       dest_viewable,
                                                                       drop_pos,
                                                                       drag_action);
}


/*  GimpItemTreeView methods  */

static void
gimp_layer_tree_view_remove_item (GimpImage *gimage,
                                  GimpItem  *item)
{
  if (gimp_layer_is_floating_sel (GIMP_LAYER (item)))
    floating_sel_remove (GIMP_LAYER (item));
  else
    gimp_image_remove_layer (gimage, GIMP_LAYER (item));
}


/*  Anchor callback  */

static void
gimp_layer_tree_view_anchor_clicked (GtkWidget         *widget,
				     GimpLayerTreeView *view)
{
  GimpImage *gimage;
  GimpLayer *layer;

  gimage = GIMP_ITEM_TREE_VIEW (view)->gimage;

  layer = (GimpLayer *)
    GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (gimage);

  if (layer && gimp_layer_is_floating_sel (layer))
    {
      floating_sel_anchor (layer);
      gimp_image_flush (gimage);
    }
}


/*  Paint Mode, Opacity and Preserve trans. callbacks  */

#define BLOCK() \
        g_signal_handlers_block_by_func (layer, \
	gimp_layer_tree_view_layer_signal_handler, view)

#define UNBLOCK() \
        g_signal_handlers_unblock_by_func (layer, \
	gimp_layer_tree_view_layer_signal_handler, view)


static void
gimp_layer_tree_view_paint_mode_menu_callback (GtkWidget         *widget,
					       GimpLayerTreeView *view)
{
  GimpImage *gimage;
  GimpLayer *layer;

  gimage = GIMP_ITEM_TREE_VIEW (view)->gimage;

  layer = (GimpLayer *)
    GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (gimage);

  if (layer)
    {
      GimpLayerModeEffects mode;

      mode = (GimpLayerModeEffects)
	GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                            "gimp-item-data"));

      if (gimp_layer_get_mode (layer) != mode)
	{
	  BLOCK();
	  gimp_layer_set_mode (layer, mode, TRUE);
	  UNBLOCK();

	  gimp_image_flush (gimage);
	}
    }
}

static void
gimp_layer_tree_view_preserve_button_toggled (GtkWidget         *widget,
					      GimpLayerTreeView *view)
{
  GimpImage *gimage;
  GimpLayer *layer;

  gimage = GIMP_ITEM_TREE_VIEW (view)->gimage;

  layer = (GimpLayer *)
    GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (gimage);

  if (layer)
    {
      gboolean preserve_trans;

      preserve_trans = GTK_TOGGLE_BUTTON (widget)->active;

      if (gimp_layer_get_preserve_trans (layer) != preserve_trans)
	{
	  BLOCK();
	  gimp_layer_set_preserve_trans (layer, preserve_trans, TRUE);
	  UNBLOCK();
	}
    }
}

static void
gimp_layer_tree_view_opacity_scale_changed (GtkAdjustment     *adjustment,
					    GimpLayerTreeView *view)
{
  GimpImage *gimage;
  GimpLayer *layer;

  gimage = GIMP_ITEM_TREE_VIEW (view)->gimage;

  layer = (GimpLayer *)
    GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (gimage);

  if (layer)
    {
      gdouble opacity;

      opacity = adjustment->value / 100.0;

      if (gimp_layer_get_opacity (layer) != opacity)
	{
          GimpUndo *undo;
          gboolean  push_undo = TRUE;

          undo = gimp_undo_stack_peek (gimage->undo_stack);

          /*  compress opacity undos  */
          if (GIMP_IS_ITEM_UNDO (undo)                   &&
              undo->undo_type == GIMP_UNDO_LAYER_OPACITY &&
              GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
            push_undo = FALSE;

	  BLOCK();
	  gimp_layer_set_opacity (layer, opacity, push_undo);
	  UNBLOCK();

	  gimp_image_flush (gimage);
	}
    }
}

#undef BLOCK
#undef UNBLOCK


static void
gimp_layer_tree_view_layer_signal_handler (GimpLayer         *layer,
					   GimpLayerTreeView *view)
{
  GimpItemTreeView *item_view;
  GimpLayer        *active_layer;

  item_view = GIMP_ITEM_TREE_VIEW (view);

  active_layer = (GimpLayer *)
    GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (item_view->gimage);

  if (active_layer == layer)
    gimp_layer_tree_view_update_options (view, layer);
}


#define BLOCK(object,function) \
        g_signal_handlers_block_by_func ((object), (function), view)

#define UNBLOCK(object,function) \
        g_signal_handlers_unblock_by_func ((object), (function), view)

static void
gimp_layer_tree_view_update_options (GimpLayerTreeView *view,
				     GimpLayer         *layer)
{
  gimp_option_menu_set_history (GTK_OPTION_MENU (view->paint_mode_menu),
				GINT_TO_POINTER (layer->mode));

  if (layer->preserve_trans !=
      GTK_TOGGLE_BUTTON (view->preserve_trans_toggle)->active)
    {
      BLOCK (view->preserve_trans_toggle,
	     gimp_layer_tree_view_preserve_button_toggled);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (view->preserve_trans_toggle),
				    layer->preserve_trans);

      UNBLOCK (view->preserve_trans_toggle,
	       gimp_layer_tree_view_preserve_button_toggled);
    }

  if (layer->opacity * 100.0 != view->opacity_adjustment->value)
    {
      BLOCK (view->opacity_adjustment,
	     gimp_layer_tree_view_opacity_scale_changed);

      gtk_adjustment_set_value (view->opacity_adjustment,
                                layer->opacity * 100.0);

      UNBLOCK (view->opacity_adjustment,
	       gimp_layer_tree_view_opacity_scale_changed);
    }
}

#undef BLOCK
#undef UNBLOCK


/*  "Linked" callbacks  */

static void
gimp_layer_tree_view_linked_changed (GimpLayer         *layer,
                                     GimpLayerTreeView *view)
{
  GimpContainerView     *container_view;
  GimpContainerTreeView *tree_view;
  GtkTreeIter           *iter;

  container_view = GIMP_CONTAINER_VIEW (view);
  tree_view      = GIMP_CONTAINER_TREE_VIEW (view);

  iter = g_hash_table_lookup (container_view->hash_table, layer);

  if (iter)
    gtk_list_store_set (GTK_LIST_STORE (tree_view->model), iter,
                        view->model_column_linked,
                        gimp_layer_get_linked (layer),
                        -1);
}

static void
gimp_layer_tree_view_chain_clicked (GtkCellRendererToggle *toggle,
                                    gchar                 *path_str,
                                    GdkModifierType        state,
                                    GimpLayerTreeView     *view)
{
  GimpContainerTreeView *tree_view;
  GtkTreePath           *path;
  GtkTreeIter            iter;

  tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpPreviewRenderer *renderer;
      GimpLayer           *layer;
      gboolean             linked;

      gtk_tree_model_get (tree_view->model, &iter,
                          tree_view->model_column_renderer, &renderer,
                          -1);
      g_object_get (toggle,
                    "active", &linked,
                    NULL);

      layer  = GIMP_LAYER (renderer->viewable);

      gimp_layer_set_linked (layer, !linked, TRUE);

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}


/*  Layer Mask callbacks  */

static void
gimp_layer_tree_view_mask_update (GimpLayerTreeView *layer_view,
                                  GtkTreeIter       *iter,
                                  GimpLayer         *layer)
{
  GimpContainerView     *container_view;
  GimpContainerTreeView *tree_view;
  GimpLayerMask         *mask;
  GimpPreviewRenderer   *renderer     = NULL;
  gboolean               mask_visible = FALSE;

  container_view = GIMP_CONTAINER_VIEW (layer_view);
  tree_view      = GIMP_CONTAINER_TREE_VIEW (layer_view);

  mask = gimp_layer_get_mask (layer);

  if (mask)
    {
      GClosure *closure;

      mask_visible = TRUE;

      renderer = gimp_preview_renderer_new (G_TYPE_FROM_INSTANCE (mask),
                                            container_view->preview_size,
                                            tree_view->preview_border_width,
                                            FALSE);
      gimp_preview_renderer_set_viewable (renderer, GIMP_VIEWABLE (mask));

      g_signal_connect (renderer, "update",
                        G_CALLBACK (gimp_layer_tree_view_renderer_update),
                        layer_view);

      closure = g_cclosure_new (G_CALLBACK (gimp_layer_tree_view_mask_callback),
                                layer_view, NULL);
      g_object_watch_closure (G_OBJECT (renderer), closure);
      g_signal_connect_closure (mask, "apply_changed", closure, FALSE);
      g_signal_connect_closure (mask, "edit_changed", closure, FALSE);
      g_signal_connect_closure (mask, "show_changed", closure, FALSE);
    }

  gtk_list_store_set (GTK_LIST_STORE (tree_view->model), iter,
                      layer_view->model_column_mask,         renderer,
                      layer_view->model_column_mask_visible, mask_visible,
                      -1);

  if (renderer)
    {
      gimp_layer_tree_view_update_borders (layer_view, iter);
      gimp_preview_renderer_remove_idle (renderer);
      g_object_unref (renderer);
    }
}

static void
gimp_layer_tree_view_mask_changed (GimpLayer         *layer,
                                   GimpLayerTreeView *layer_view)
{
  GimpContainerView *view;
  GtkTreeIter       *iter;

  view = GIMP_CONTAINER_VIEW (layer_view);

  iter = g_hash_table_lookup (view->hash_table, layer);

  if (iter)
    gimp_layer_tree_view_mask_update (layer_view, iter, layer);
}

static void
gimp_layer_tree_view_renderer_update (GimpPreviewRenderer *renderer,
                                      GimpLayerTreeView   *layer_view)
{
  GimpContainerView     *view;
  GimpContainerTreeView *tree_view;
  GimpLayerMask         *mask;
  GtkTreeIter           *iter;

  view      = GIMP_CONTAINER_VIEW (layer_view);
  tree_view = GIMP_CONTAINER_TREE_VIEW (layer_view);

  mask = GIMP_LAYER_MASK (renderer->viewable);

  iter = g_hash_table_lookup (view->hash_table,
                              gimp_layer_mask_get_layer (mask));

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (tree_view->model, iter);

      gtk_tree_model_row_changed (tree_view->model, path, iter);

      gtk_tree_path_free (path);
    }
}

static void
gimp_layer_tree_view_update_borders (GimpLayerTreeView *layer_view,
                                     GtkTreeIter       *iter)
{
  GimpContainerTreeView *tree_view;
  GimpPreviewRenderer   *layer_renderer;
  GimpPreviewRenderer   *mask_renderer;
  GimpLayerMask         *mask = NULL;
  GimpRGB               *layer_color = &black_color;
  GimpRGB               *mask_color  = &black_color;

  tree_view = GIMP_CONTAINER_TREE_VIEW (layer_view);

  gtk_tree_model_get (tree_view->model, iter,
                      tree_view->model_column_renderer,
                      &layer_renderer,
                      layer_view->model_column_mask,
                      &mask_renderer,
                      -1);

  if (mask_renderer)
    mask = GIMP_LAYER_MASK (mask_renderer->viewable);

  if (! mask || (mask && ! gimp_layer_mask_get_edit (mask)))
    layer_color = &white_color;

  if (mask)
    {
      if (gimp_layer_mask_get_show (mask))
	{
	  mask_color = &green_color;
	}
      else if (! gimp_layer_mask_get_apply (mask))
	{
	  mask_color = &red_color;
	}
      else if (gimp_layer_mask_get_edit (mask))
	{
          mask_color = &white_color;
	}
    }

  gimp_preview_renderer_set_border_color (layer_renderer, layer_color);

  if (mask)
    gimp_preview_renderer_set_border_color (mask_renderer, mask_color);

  if (layer_renderer)
    g_object_unref (layer_renderer);

  if (mask_renderer)
    g_object_unref (mask_renderer);
}

static void
gimp_layer_tree_view_mask_callback (GimpLayerMask     *mask,
                                    GimpLayerTreeView *layer_view)
{
  GtkTreeIter *iter;

  iter = g_hash_table_lookup (GIMP_CONTAINER_VIEW (layer_view)->hash_table,
                              gimp_layer_mask_get_layer (mask));

  gimp_layer_tree_view_update_borders (layer_view, iter);
}

static void
gimp_layer_tree_view_layer_clicked (GimpCellRendererViewable *cell,
                                    const gchar              *path_str,
                                    GdkModifierType           state,
                                    GimpLayerTreeView        *layer_view)
{
  GimpContainerTreeView *tree_view;
  GtkTreePath           *path;
  GtkTreeIter            iter;

  tree_view = GIMP_CONTAINER_TREE_VIEW (layer_view);

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpPreviewRenderer *renderer;

      gtk_tree_model_get (tree_view->model, &iter,
                          layer_view->model_column_mask, &renderer,
                          -1);

      if (renderer)
        {
          GimpLayerMask *mask;

          mask = GIMP_LAYER_MASK (renderer->viewable);

          if (gimp_layer_mask_get_edit (mask))
            {
              gimp_layer_mask_set_edit (mask, FALSE);
              gimp_image_flush (gimp_item_get_image (GIMP_ITEM (mask)));
            }

          g_object_unref (renderer);
        }
    }

  gtk_tree_path_free (path);
}

static void
gimp_layer_tree_view_mask_clicked (GimpCellRendererViewable *cell,
                                   const gchar              *path_str,
                                   GdkModifierType           state,
                                   GimpLayerTreeView        *layer_view)
{
  GimpContainerTreeView *tree_view;
  GtkTreePath           *path;
  GtkTreeIter            iter;

  tree_view = GIMP_CONTAINER_TREE_VIEW (layer_view);

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpPreviewRenderer *renderer;
      GimpLayerMask       *mask;
      gboolean             flush = FALSE;

      gtk_tree_model_get (tree_view->model, &iter,
                          layer_view->model_column_mask, &renderer,
                          -1);

      mask = GIMP_LAYER_MASK (renderer->viewable);

      if (state & GDK_MOD1_MASK)
        {
          gimp_layer_mask_set_show (mask, ! gimp_layer_mask_get_show (mask));
          flush = TRUE;
        }
      else if (state & GDK_CONTROL_MASK)
        {
          gimp_layer_mask_set_apply (mask, ! gimp_layer_mask_get_apply (mask));
          flush = TRUE;
        }
      else if (! gimp_layer_mask_get_edit (mask))
        {
          gimp_layer_mask_set_edit (mask, TRUE);
          flush = TRUE;
        }

      if (flush)
        gimp_image_flush (gimp_item_get_image (GIMP_ITEM (mask)));

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}
