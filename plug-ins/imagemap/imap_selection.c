/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2004 Maurits Rijk  m.rijk@chello.nl
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
 *
 */

#include "config.h"

#include <stdio.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "imap_commands.h"
#include "imap_edit_area_info.h"
#include "imap_main.h"
#include "imap_misc.h"
#include "imap_selection.h"

#include "libgimp/stdplugins-intl.h"

static void
set_buttons(Selection_t *data)
{
  if (gtk_tree_selection_count_selected_rows (data->selection)) {
#ifdef _OLD_
    gtk_widget_set_sensitive(data->arrow_up,
			     (data->selected_row) ? TRUE : FALSE);
    if (data->selected_row < GTK_CLIST(data->list)->rows - 1)
      gtk_widget_set_sensitive(data->arrow_down, TRUE);
    else
      gtk_widget_set_sensitive(data->arrow_down, FALSE);
#endif
    gtk_widget_set_sensitive(data->remove, TRUE);
    gtk_widget_set_sensitive(data->edit, TRUE);
  } else {
    gtk_widget_set_sensitive(data->arrow_up, FALSE);
    gtk_widget_set_sensitive(data->arrow_down, FALSE);
    gtk_widget_set_sensitive(data->remove, FALSE);
    gtk_widget_set_sensitive(data->edit, FALSE);
  }
}

static void
changed_cb(GtkTreeSelection *selection, gpointer param)
{
  Selection_t *data = (Selection_t*) param;

  if (data->select_lock) {
    data->select_lock = FALSE;
  } else {
    Command_t *command, *sub_command;
    GtkTreeModel *model;
    GList *list = gtk_tree_selection_get_selected_rows (selection, &model);

    command = subcommand_start (NULL);
    sub_command = unselect_all_command_new (data->object_list, NULL);
    command_add_subcommand (command, sub_command);

    for (; list; list = list->next)
      {
	Object_t *obj;
	GtkTreeIter iter;
	GtkTreePath *path = (GtkTreePath*) list->data;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &obj, -1);

	sub_command = select_command_new (obj);
	command_add_subcommand (command, sub_command);
      }

    command_set_name (command, sub_command->name);
    subcommand_end ();

    command_execute (command);

    g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (list);

    set_buttons (data);
  }
}

static gboolean
button_press_cb(GtkWidget *widget, GdkEventButton *event, Selection_t *data)
{
  if (event->button == 1) {
    if (data->doubleclick) {
      GtkTreePath *path;

      data->doubleclick = FALSE;

      if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
					 (gint) event->x, (gint) event->y,
					 &path, NULL, NULL, NULL)) {
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (data->store), &iter,
				     path)) {
	  Object_t *obj;
	  gtk_tree_model_get (GTK_TREE_MODEL(data->store), &iter, 0, &obj, -1);
	  object_edit (obj, TRUE);
	}
	gtk_tree_path_free (path);
      }
    } else {
      data->doubleclick = TRUE;
    }
  }
  return FALSE;
}

static gboolean
button_release_cb(GtkWidget *widget, GdkEventButton *event, Selection_t *data)
{
  if (event->button == 1)
    data->doubleclick = FALSE;
  return FALSE;
}

static void
selection_command(GtkWidget *widget, gpointer data)
{
  CommandFactory_t *factory = (CommandFactory_t*) data;
  Command_t *command = (*factory)();
  command_execute(command);
}

static GtkWidget*
make_selection_toolbar(Selection_t *data)
{
  GtkWidget *toolbar;

  toolbar = gtk_toolbar_new();
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_VERTICAL);
  gtk_container_set_border_width(GTK_CONTAINER(toolbar), 0);

  data->arrow_up = make_toolbar_stock_icon(toolbar, GTK_STOCK_GO_UP,
					   "MoveUp", _("Move Up"),
					   selection_command,
					   &data->cmd_move_up);
  data->arrow_down = make_toolbar_stock_icon(toolbar, GTK_STOCK_GO_DOWN,
					     "MoveDown", _("Move Down"),
					     selection_command,
					     &data->cmd_move_down);
  toolbar_add_space(toolbar);
  data->edit = make_toolbar_stock_icon(toolbar, GTK_STOCK_PROPERTIES,
				       "Edit", _("Edit"), selection_command,
				       &data->cmd_edit);
  toolbar_add_space(toolbar);
  data->remove = make_toolbar_stock_icon(toolbar, GTK_STOCK_DELETE, "Delete",
					 _("Delete"), selection_command,
					 &data->cmd_delete);

  gtk_widget_show(toolbar);

  return toolbar;
}

static void
selection_set_selected(Selection_t *selection, gint row)
{
  GtkTreeIter iter;

  if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (selection->store), &iter,
				     NULL, row)) {
    Object_t *obj;

    gtk_tree_model_get (GTK_TREE_MODEL(selection->store), &iter, 0, &obj, -1);

    selection->select_lock = TRUE;

    if (obj->selected) {
      gtk_tree_selection_select_iter (selection->selection, &iter);
    } else {
      gtk_tree_selection_unselect_iter (selection->selection, &iter);
    }
  }
}

static void
object_added_cb(Object_t *obj, gpointer data)
{
  Selection_t *selection = (Selection_t*) data;
  GtkTreeIter iter;
  gint position = object_get_position_in_list (obj);

  selection->nr_rows++;
  if (position < selection->nr_rows - 1) {
    gtk_list_store_insert (selection->store, &iter, position);
  } else {
    gtk_list_store_append (selection->store, &iter);
  }
  gtk_list_store_set (selection->store, &iter, 0, obj, -1);
}

static gboolean
selection_find_object(Selection_t *selection, Object_t *lookup,
		      GtkTreeIter *iter)
{
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (selection->store),
				     iter)) {
    do {
      Object_t *obj;

      gtk_tree_model_get (GTK_TREE_MODEL(selection->store), iter, 0,
			  &obj, -1);
      if (obj == lookup)
	return TRUE;

    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (selection->store),
				       iter));
  }
  return FALSE;
}

static void
object_updated_cb(Object_t *obj, gpointer data)
{
  Selection_t *selection = (Selection_t*) data;
  GtkTreeIter iter;

  if (selection_find_object (selection, obj, &iter)) {
    GtkTreePath *path;

    path = gtk_tree_model_get_path (GTK_TREE_MODEL (selection->store), &iter);
    gtk_tree_model_row_changed (GTK_TREE_MODEL (selection->store), path,
				&iter);
  }
}

static void
object_removed_cb(Object_t *obj, gpointer data)
{
  Selection_t *selection = (Selection_t*) data;
  GtkTreeIter iter;

  if (selection_find_object (selection, obj, &iter)) {
    gtk_list_store_remove (GTK_LIST_STORE (selection->store), &iter);
    set_buttons(selection);
  }
}

static void
object_selected_cb(Object_t *obj, gpointer data)
{
  Selection_t *selection = (Selection_t*) data;
  gint position = object_get_position_in_list (obj);
  selection_set_selected (selection, position);
  set_buttons(selection);
}

static void
object_moved_cb(Object_t *obj, gpointer data)
{
  Selection_t *selection = (Selection_t*) data;
  selection->select_lock = TRUE;
#ifdef _OLD_
  {
    gint row = object_get_position_in_list(obj);
    gtk_clist_set_row_data(GTK_CLIST(selection->list), row, (gpointer) obj);
    selection_set_selected(selection, row);
  }
#endif
}

static GtkTargetEntry target_table[] = {
  {"STRING", 0, 1 },
  {"text/plain", 0, 2 }
};

static Object_t*
selection_get_object (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  Object_t *obj;
  gtk_tree_model_get (tree_model, iter, 0, &obj, -1);
  return obj;
}

static void
handle_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
	    GtkSelectionData *data, guint info, guint time)
{
  gboolean success = FALSE;
  if (data->length >= 0 && data->format == 8) {
    GtkTreePath *path;
    if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), x, y,
				       &path, NULL, NULL, NULL)) {
      GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
      GtkTreeIter iter;

      if (gtk_tree_model_get_iter (model, &iter, path)) {
	Object_t *obj = selection_get_object (model, &iter);
	if (!obj->locked) {
	  command_list_add(edit_object_command_new (obj));
	  object_set_url (obj, data->data);
	  object_emit_update_signal (obj);
	  success = TRUE;
	}
      }
      gtk_tree_path_free (path);
    }
  }
  gtk_drag_finish(context, success, FALSE, time);
}

static void
render_image (GtkTreeViewColumn *column, GtkCellRenderer *cell,
	      GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
  Object_t *obj = selection_get_object (tree_model, iter);
  g_object_set(cell, "stock-id", obj->class->get_stock_icon_name(), NULL);
}

static void
render_nr (GtkTreeViewColumn *column, GtkCellRenderer *cell,
	   GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
  Object_t *obj = selection_get_object (tree_model, iter);
  gchar *scratch;

  scratch = g_strdup_printf ("%d", object_get_position_in_list (obj) + 1);
  g_object_set (cell, "text", scratch, NULL);
  g_free (scratch);
}

static void
render_url (GtkTreeViewColumn *column, GtkCellRenderer *cell,
	    GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
  Object_t *obj = selection_get_object (tree_model, iter);
  g_object_set (cell, "text", obj->url, NULL);
}

static void
render_target (GtkTreeViewColumn *column, GtkCellRenderer *cell,
	       GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
  Object_t *obj = selection_get_object (tree_model, iter);
  g_object_set (cell, "text", obj->target, NULL);
}

static void
render_comment (GtkTreeViewColumn *column, GtkCellRenderer *cell,
		GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
  Object_t *obj = selection_get_object (tree_model, iter);
  g_object_set (cell, "text", obj->comment, NULL);
}

Selection_t*
make_selection(ObjectList_t *object_list)
{
  Selection_t *data = g_new(Selection_t, 1);
  GtkWidget *swin, *frame, *hbox;
  GtkWidget *toolbar;
  GtkWidget *list;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  data->object_list = object_list;
  data->selected_child = NULL;
  data->is_visible = TRUE;
  data->nr_rows = 0;
  data->select_lock = FALSE;
  data->doubleclick = FALSE;

  data->container = frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_widget_show(frame);

  hbox = gtk_hbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(frame), hbox);
  gtk_widget_show(hbox);

  toolbar = make_selection_toolbar(data);
  gtk_container_add(GTK_CONTAINER(hbox), toolbar);

  /* Create selection */
  frame = gimp_frame_new(_("Selection"));
  gtk_container_add(GTK_CONTAINER(hbox), frame);
  gtk_widget_show(frame);

  data->store = gtk_list_store_new (1, G_TYPE_POINTER);
  data->list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (data->store));
  list = data->list;
  g_object_unref (data->store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (N_("#"),
						     renderer,
						     NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
					   render_nr, data, NULL);
  gtk_tree_view_column_set_min_width (column, 16);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("URL"));

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start(column, renderer, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
					   render_image, data, NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, render_url, data,
					   NULL);
  gtk_tree_view_column_set_min_width (column, 80);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_alignment (column, 0.5);

  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("ALT Text"), renderer,
						     NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer, render_comment,
					   data, NULL);
  gtk_tree_view_column_set_min_width (column, 64);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Target"), renderer,
						     NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
					   render_target, data, NULL);
  gtk_tree_view_column_set_min_width (column, 64);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);


  /* Create scrollable window */
  swin = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (swin, 16 + 80 + 2 * 64 + 16, -1);
  gtk_container_add (GTK_CONTAINER(frame), swin);
  gtk_widget_show (swin);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(swin), list);
  gtk_widget_show (list);

  /* Drop support */
  gtk_drag_dest_set (list, GTK_DEST_DEFAULT_ALL, target_table, 2,
		     GDK_ACTION_COPY);
  g_signal_connect (list, "drag_data_received", G_CALLBACK(handle_drop), NULL);

  /* For handling doubleclick */

  g_signal_connect (list, "button_press_event",
		    G_CALLBACK(button_press_cb), data);
  g_signal_connect (list, "button_release_event",
		    G_CALLBACK(button_release_cb), data);

  /* Callbacks we are interested in */
  data->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
  gtk_tree_selection_set_mode (data->selection, GTK_SELECTION_MULTIPLE);
  g_signal_connect (data->selection, "changed", G_CALLBACK(changed_cb), data);

  set_buttons (data);

  /* Set object list callbacks we're interested in */
  object_list_add_add_cb (object_list, object_added_cb, data);
  object_list_add_update_cb (object_list, object_updated_cb, data);
  object_list_add_remove_cb (object_list, object_removed_cb, data);
  object_list_add_select_cb (object_list, object_selected_cb, data);
  object_list_add_move_cb (object_list, object_moved_cb, data);

  return data;
}

void
selection_toggle_visibility(Selection_t *selection)
{
  if (selection->is_visible) {
    gtk_widget_hide (selection->container);
    selection->is_visible = FALSE;
  } else {
    gtk_widget_show (selection->container);
    selection->is_visible = TRUE;
  }
}

void
selection_freeze(Selection_t *selection)
{
}

void
selection_thaw(Selection_t *selection)
{
}
