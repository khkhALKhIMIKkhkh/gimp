/* GIMP - The GNU Image Manipulation Program
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print.h"
#include "print-page-layout.h"
#include "print-preview.h"

#include "libgimp/stdplugins-intl.h"


typedef struct
{
  PrintData       *data;
  gint             image_width;
  gint             image_height;
  GimpSizeEntry   *size_entry;
  GimpSizeEntry   *resolution_entry;
  GimpSizeEntry   *offset_entry;
  GimpChainButton *chain;
  GtkWidget       *area_label;
  GtkWidget       *preview;
} PrintSizeInfo;


static void        run_page_setup_dialog              (GtkWidget     *widget,
                                                       PrintData     *data);

static GtkWidget * print_size_frame                   (PrintData     *data,
                                                       GtkSizeGroup *label_group,
                                                       GtkSizeGroup *entry_group);
static GtkWidget * print_offset_frame                 (PrintData     *data,
                                                       GtkSizeGroup *label_group,
                                                       GtkSizeGroup *entry_group);

static void        print_size_info_size_changed       (GtkWidget     *widget);
static void        print_size_info_resolution_changed (GtkWidget     *widget);
static void        print_size_info_unit_changed       (GtkWidget     *widget);
static void        print_size_info_chain_toggled      (GtkWidget     *widget);
static void        print_size_info_offset_changed     (GtkWidget     *widget);
static void        print_size_info_preview_offset_changed
                                                      (GtkWidget     *widget,
                                                       gdouble        offset_x,
                                                       gdouble        offset_y);
static void        print_size_info_center_clicked     (GtkWidget     *widget,
                                                       gpointer       data);
static void        print_size_info_use_full_page_toggled
                                                      (GtkWidget     *widget);

static void        print_size_info_set_resolution     (PrintSizeInfo *info,
                                                       gdouble        xres,
                                                       gdouble        yres);


static void        print_size_info_set_page_setup     (PrintSizeInfo *info);


static PrintSizeInfo  info;


GtkWidget *
print_page_layout_gui (PrintData *data)
{
  GtkWidget    *main_hbox;
  GtkWidget    *main_vbox;
  GtkWidget    *hbox;
  GtkWidget    *vbox;
  GtkWidget    *button;
  GtkWidget    *label;
  GtkWidget    *frame;
  GtkPageSetup *setup;
  GtkSizeGroup *label_group;
  GtkSizeGroup *entry_group;

  memset (&info, 0, sizeof (PrintSizeInfo));

  info.data         = data;
  info.image_width  = gimp_drawable_width (data->drawable_id);
  info.image_height = gimp_drawable_height (data->drawable_id);

  main_hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_widget_show (main_hbox);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_Adjust Page Size "
                                           "and Orientation"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (run_page_setup_dialog),
                    data);
  gtk_widget_show (button);

  /* label for the printable area */

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Printable area:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  info.area_label = label;

  gtk_box_pack_start (GTK_BOX (hbox), info.area_label, FALSE, FALSE, 0);
  gtk_widget_show (info.area_label);

  label_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  entry_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

#if 0
  /* Commented out until the header becomes a little more configurable
   * and we can provide a user interface to include/exclude information.
   */
  button = gtk_check_button_new_with_label ("Print image header");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                data->show_info_header);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &data->show_info_header);
  gtk_widget_show (button);
#endif

  /* size entry area for the image's print size */

  frame = print_size_frame (data, label_group, entry_group);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* offset entry area for the image's offset position */

  frame = print_offset_frame (data, label_group, entry_group);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_object_unref (label_group);
  g_object_unref (entry_group);

  button = gtk_check_button_new_with_mnemonic (_("Ignore Page _Margins"));

  data->use_full_page = FALSE;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (print_size_info_use_full_page_toggled),
                    NULL);
  gtk_widget_show (button);

  setup = gtk_print_operation_get_default_page_setup (data->operation);

  info.preview = gimp_print_preview_new (setup, data->drawable_id);
  gtk_box_pack_start (GTK_BOX (main_hbox), info.preview, TRUE, TRUE, 0);
  gtk_widget_show (info.preview);

  g_signal_connect (info.preview, "offsets-changed",
                    G_CALLBACK (print_size_info_preview_offset_changed),
                    NULL);

  print_size_info_set_page_setup (&info);

  return main_hbox;
}

static void
run_page_setup_dialog (GtkWidget *widget,
                       PrintData *data)
{
  GtkPrintOperation *operation;
  GtkPrintSettings  *settings;
  GtkPageSetup      *page_setup;
  GtkWidget         *toplevel;

  operation = data->operation;

  /* find a transient parent if possible */
  toplevel = gtk_widget_get_toplevel (widget);
  if (! GTK_WIDGET_TOPLEVEL (toplevel))
    toplevel = NULL;

  settings = gtk_print_operation_get_print_settings (operation);
  if (! settings)
    settings = gtk_print_settings_new ();

  page_setup = gtk_print_operation_get_default_page_setup (operation);

  page_setup = gtk_print_run_page_setup_dialog (GTK_WINDOW (toplevel),
                                                page_setup, settings);

  gtk_print_operation_set_default_page_setup (operation, page_setup);

  gimp_print_preview_set_page_setup (GIMP_PRINT_PREVIEW (info.preview),
                                     page_setup);

  print_size_info_set_page_setup (&info);
}

#define SB_WIDTH 8


static GtkWidget *
print_size_frame (PrintData *data,
                  GtkSizeGroup *label_group,
                  GtkSizeGroup *entry_group)
{
  GtkWidget    *entry;
  GtkWidget    *height;
  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *chain;
  GtkWidget    *frame;
  GtkWidget    *label;
  GtkObject    *adj;
  gdouble       image_width;
  gdouble       image_height;

  image_width  = (info.image_width *
                  gimp_unit_get_factor (data->unit) / data->xres);
  image_height = (info.image_height *
                  gimp_unit_get_factor (data->unit) / data->yres);

  frame = gimp_frame_new (_("Image Size"));

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  the print size entry  */

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  entry = gimp_size_entry_new (1, data->unit, "%p",
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  info.size_entry = GIMP_SIZE_ENTRY (entry);

  gtk_table_set_row_spacings (GTK_TABLE (entry), 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 0, 6);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 2, 6);

  height = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), height, 1, 2, 0, 1);
  gtk_widget_show (height);

  gtk_size_group_add_widget (entry_group, height);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                _("_Width:"), 0, 0, 0.0);
  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                        _("_Height:"), 1, 0, 0.0);

  gtk_size_group_add_widget (label_group, label);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0,
                                  data->xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 1,
                                  data->yres, FALSE);

  gimp_size_entry_set_value (GIMP_SIZE_ENTRY (entry), 0, image_width);
  gimp_size_entry_set_value (GIMP_SIZE_ENTRY (entry), 1, image_height);

  /*  the resolution entry  */

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  entry = gimp_size_entry_new (1, data->image_unit,
                               _("pixels/%a"),
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  info.resolution_entry = GIMP_SIZE_ENTRY (entry);

  gtk_table_set_row_spacings (GTK_TABLE (entry), 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 0, 6);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 2, 6);

  height = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), height, 1, 2, 0, 1);
  gtk_widget_show (height);

  gtk_size_group_add_widget (entry_group, height);

  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                        _("X resolution:"), 0, 0, 0.0);
  gtk_size_group_add_widget (label_group, label);

  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                        _("Y resolution:"), 1, 0, 0.0);
  gtk_size_group_add_widget (label_group, label);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (entry), 0,
                                         GIMP_MIN_RESOLUTION,
                                         GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (entry), 1,
                                         GIMP_MIN_RESOLUTION,
                                         GIMP_MAX_RESOLUTION);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 0, data->xres);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 1, data->yres);

  chain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain), TRUE);
  gtk_table_attach (GTK_TABLE (entry), chain, 2, 3, 0, 2,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (chain);

  info.chain = GIMP_CHAIN_BUTTON (chain);


  g_signal_connect (info.size_entry, "value-changed",
                    G_CALLBACK (print_size_info_size_changed),
                    NULL);
  g_signal_connect (info.resolution_entry, "value-changed",
                    G_CALLBACK (print_size_info_resolution_changed),
                    NULL);
  g_signal_connect (info.size_entry, "unit-changed",
                    G_CALLBACK (print_size_info_unit_changed),
                    NULL);
  g_signal_connect (info.chain, "toggled",
                    G_CALLBACK (print_size_info_chain_toggled),
                    NULL);

  return frame;
}

static GtkWidget *
print_offset_frame (PrintData *data,
                    GtkSizeGroup *label_group,
                    GtkSizeGroup *entry_group)
{
  GtkWidget    *entry;
  GtkWidget    *height;
  GtkWidget    *button;
  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *bbox;
  GtkWidget    *frame;
  GtkWidget    *label;
  GtkObject    *adj;

  frame = gimp_frame_new (_("Image Offset"));

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  the offset entry  */

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  entry = gimp_size_entry_new (1, data->unit, "%p",
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  info.offset_entry = GIMP_SIZE_ENTRY (entry);

  gtk_table_set_row_spacings (GTK_TABLE (entry), 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 0, 6);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 2, 6);

  height = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), height, 1, 2, 0, 1);
  gtk_widget_show (height);

  gtk_size_group_add_widget (entry_group, height);

  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                        _("_X:"), 0, 0, 0.0);
  gtk_size_group_add_widget (label_group, label);

  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                        _("_Y:"), 1, 0, 0.0);
  gtk_size_group_add_widget (label_group, label);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0,
                                  72.0, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 1,
                                  72.0, FALSE);

  gimp_size_entry_set_value (GIMP_SIZE_ENTRY (entry), 0, 0);
  gimp_size_entry_set_value (GIMP_SIZE_ENTRY (entry), 1, 0);

  g_signal_connect (info.offset_entry, "value-changed",
                    G_CALLBACK (print_size_info_offset_changed),
                    NULL);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Center:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_size_group_add_widget (label_group, label);
  gtk_widget_show (label);

  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_START);
  gtk_box_pack_start (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);

  button = gtk_button_new_with_mnemonic (_("H_orizontally"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (print_size_info_center_clicked),
                    GINT_TO_POINTER (1));
  gtk_widget_show (button);

  button = gtk_button_new_with_mnemonic (_("_Vertically"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (print_size_info_center_clicked),
                    GINT_TO_POINTER (2));
  gtk_widget_show (button);


  return frame;
}

static void
print_size_info_preview_offset_changed (GtkWidget *widget,
                                        gdouble    offset_x,
                                        gdouble    offset_y)
{
  info.data->offset_x = offset_x;
  info.data->offset_y = offset_y;

  g_signal_handlers_block_by_func (info.offset_entry,
                                   print_size_info_offset_changed,
                                   NULL);

  gimp_size_entry_set_refval (info.offset_entry, 0, info.data->offset_x);
  gimp_size_entry_set_refval (info.offset_entry, 1, info.data->offset_y);

  g_signal_handlers_unblock_by_func (info.offset_entry,
                                     print_size_info_offset_changed,
                                     NULL);
}

static void
print_size_info_get_page_dimensions (PrintSizeInfo *info,
                                     gdouble       *page_width,
                                     gdouble       *page_height,
                                     GtkUnit        unit)
{
  GtkPageSetup *setup;

  setup = gtk_print_operation_get_default_page_setup (info->data->operation);

  if (info->data->use_full_page)
    {
      *page_width = gtk_page_setup_get_paper_width (setup, unit);
      *page_height = gtk_page_setup_get_paper_height (setup, unit);
    } else {
      *page_width = gtk_page_setup_get_page_width (setup, unit);
      *page_height = gtk_page_setup_get_page_height (setup, unit);
    }

}

static void
gimp_size_info_get_max_offsets (gdouble *offset_x_max,
                                gdouble *offset_y_max)
{
  gdouble width;
  gdouble height;

  print_size_info_get_page_dimensions (&info, &width, &height, GTK_UNIT_POINTS);

  *offset_x_max = width - 72.0 * info.image_width / info.data->xres;
  *offset_x_max = MAX (0, *offset_x_max);

  *offset_y_max = height - 72.0 * info.image_height / info.data->yres;
  *offset_y_max = MAX (0, *offset_y_max);
}

static void
print_size_info_center_clicked (GtkWidget *widget, gpointer data)
{
  gdouble offset_x_max;
  gdouble offset_y_max;

  gimp_size_info_get_max_offsets (&offset_x_max, &offset_y_max);

  switch (GPOINTER_TO_INT (data))
    {
    case 0:
      info.data->offset_x = offset_x_max / 2.0;
      info.data->offset_y = offset_y_max / 2.0;
      break;
    case 1:
      info.data->offset_x = offset_x_max / 2.0;
      break;
    case 2:
      info.data->offset_y = offset_y_max / 2.0;
      break;
    }

  g_signal_handlers_block_by_func (info.offset_entry,
                                   print_size_info_offset_changed,
                                   NULL);

  gimp_size_entry_set_refval (info.offset_entry, 0, info.data->offset_x);
  gimp_size_entry_set_refval (info.offset_entry, 1, info.data->offset_y);

  g_signal_handlers_unblock_by_func (info.offset_entry,
                                     print_size_info_offset_changed,
                                     NULL);

  gimp_print_preview_set_image_offsets (GIMP_PRINT_PREVIEW (info.preview),
                                        info.data->offset_x,
                                        info.data->offset_y);
}

static void
print_size_info_size_changed (GtkWidget *widget)
{
  gdouble width;
  gdouble height;
  gdouble xres;
  gdouble yres;
  gdouble scale;

  scale = gimp_unit_get_factor (gimp_size_entry_get_unit (info.size_entry));

  width  = gimp_size_entry_get_value (info.size_entry, 0);
  height = gimp_size_entry_get_value (info.size_entry, 1);

  xres = scale * info.image_width  / MAX (0.0001, width);
  yres = scale * info.image_height / MAX (0.0001, height);

  print_size_info_set_resolution (&info, xres, yres);
}

static void
print_size_info_resolution_changed (GtkWidget *widget)
{
  GimpSizeEntry *entry = info.resolution_entry;
  gdouble        xres  = gimp_size_entry_get_refval (entry, 0);
  gdouble        yres  = gimp_size_entry_get_refval (entry, 1);

  print_size_info_set_resolution (&info, xres, yres);
}

static void
print_size_info_unit_changed (GtkWidget *widget)
{
  PrintData     *data   = info.data;
  GimpSizeEntry *entry  = info.size_entry;
  gdouble        factor = gimp_unit_get_factor (data->unit);
  gdouble        w, h;

  data->unit = gimp_size_entry_get_unit (entry);

  factor = gimp_unit_get_factor (data->unit) / factor;

  w = gimp_size_entry_get_value (entry, 0) * factor;
  h = gimp_size_entry_get_value (entry, 1) * factor;

  print_size_info_set_page_setup (&info);
}

static void
print_size_info_use_full_page_toggled (GtkWidget *widget)
{
  gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  info.data->use_full_page = active;

  print_size_info_set_page_setup (&info);

  gimp_print_preview_set_use_full_page (GIMP_PRINT_PREVIEW(info.preview),
                                        active);
}

static void
print_size_info_chain_toggled (GtkWidget *widget)
{
  print_size_info_set_page_setup (&info);
}

static void
print_size_info_offset_changed (GtkWidget *widget)
{
  info.data->offset_x = gimp_size_entry_get_refval (info.offset_entry, 0);
  info.data->offset_y = gimp_size_entry_get_refval (info.offset_entry, 1);

  gimp_print_preview_set_image_offsets ( GIMP_PRINT_PREVIEW(info.preview),
                                         info.data->offset_x,
                                         info.data->offset_y);
}

static void
print_size_info_set_resolution (PrintSizeInfo *info,
                                gdouble        xres,
                                gdouble        yres)
{
  PrintData    *data  = info->data;
  gdouble       offset_x;
  gdouble       offset_y;
  gdouble       offset_x_max;
  gdouble       offset_y_max;

  if (info->chain && gimp_chain_button_get_active (info->chain))
    {
      if (xres != data->xres)
          yres = xres;
      else
          xres = yres;
    }

  data->xres = xres;
  data->yres = yres;

  g_signal_handlers_block_by_func (info->resolution_entry,
                                   print_size_info_resolution_changed,
                                   NULL);

  gimp_size_entry_set_refval (info->resolution_entry, 0, xres);
  gimp_size_entry_set_refval (info->resolution_entry, 1, yres);

  g_signal_handlers_unblock_by_func (info->resolution_entry,
                                     print_size_info_resolution_changed,
                                     NULL);

  g_signal_handlers_block_by_func (info->size_entry,
                                   print_size_info_size_changed,
                                   NULL);

  gimp_size_entry_set_value (info->size_entry, 0,
                             info->image_width *
                             gimp_unit_get_factor (data->unit) / xres);
  gimp_size_entry_set_value (info->size_entry, 1,
                             info->image_height *
                             gimp_unit_get_factor (data->unit) / yres);

  g_signal_handlers_unblock_by_func (info->size_entry,
                                     print_size_info_size_changed,
                                     NULL);

  gimp_size_info_get_max_offsets (&offset_x_max, &offset_y_max);

  offset_x = gimp_size_entry_get_refval (info->offset_entry, 0);
  offset_y = gimp_size_entry_get_refval (info->offset_entry, 1);

  offset_x = CLAMP (offset_x, 0, offset_x_max);
  offset_y = CLAMP (offset_y, 0, offset_y_max);

  data->offset_x = offset_x;
  data->offset_y = offset_y;

  g_signal_handlers_block_by_func (info->offset_entry,
                                   print_size_info_offset_changed,
                                   NULL);

  gimp_size_entry_set_refval (info->offset_entry, 0, offset_x);
  gimp_size_entry_set_refval (info->offset_entry, 1, offset_y);

  gimp_size_entry_set_refval_boundaries (info->offset_entry, 0, 0, offset_x_max);
  gimp_size_entry_set_refval_boundaries (info->offset_entry, 1, 0, offset_y_max);

  g_signal_handlers_unblock_by_func (info->offset_entry,
                                     print_size_info_offset_changed,
                                     NULL);

  gimp_print_preview_set_image_dpi (GIMP_PRINT_PREVIEW (info->preview),
                                    data->xres, data->yres);

  gimp_print_preview_set_image_offsets (GIMP_PRINT_PREVIEW (info->preview),
                                        offset_x, offset_y);
  gimp_print_preview_set_image_offsets_max (GIMP_PRINT_PREVIEW (info->preview),
                                            offset_x_max, offset_y_max);
}

static void
print_size_info_set_page_setup (PrintSizeInfo *info)
{
  GtkPageSetup *setup;
  PrintData    *data = info->data;
  gchar        *format;
  gchar        *text;
  gdouble       page_width;
  gdouble       page_height;
  gdouble       x;
  gdouble       y;

  setup = gtk_print_operation_get_default_page_setup (data->operation);
  if (! setup)
    {
      setup = gtk_page_setup_new ();
      gtk_print_operation_set_default_page_setup (data->operation, setup);
    }

  print_size_info_get_page_dimensions (info, &page_width, &page_height, GTK_UNIT_INCH);

  page_width  *= gimp_unit_get_factor (data->unit);
  page_height *= gimp_unit_get_factor (data->unit);

  format = g_strdup_printf ("%%.%df x %%.%df %s",
                            gimp_unit_get_digits (data->unit),
                            gimp_unit_get_digits (data->unit),
                            gimp_unit_get_plural (data->unit));
  text = g_strdup_printf (format, page_width, page_height);
  g_free (format);

  gtk_label_set_text (GTK_LABEL (info->area_label), text);
  g_free (text);

  x = page_width;
  y = page_height;

  if (info->chain && gimp_chain_button_get_active (info->chain))
    {
      gdouble ratio_x = page_width / (gdouble) info->image_width;
      gdouble ratio_y = page_height / (gdouble) info->image_height;

      if (ratio_x < ratio_y)
        y = (gdouble) info->image_height * ratio_x;
      else
        x = (gdouble) info->image_width * ratio_y;
    }

  gimp_size_entry_set_value_boundaries (info->size_entry, 0, 0.0, x);
  gimp_size_entry_set_value_boundaries (info->size_entry, 1, 0.0, y);

  print_size_info_get_page_dimensions (info, &page_width, &page_height, GTK_UNIT_POINTS);

  x = (gdouble) info->image_width / page_width * 72.0;
  y = (gdouble) info->image_height / page_height * 72.0;

  if (info->chain && gimp_chain_button_get_active (info->chain))
    {
      gdouble max = MAX (x, y);

      x = y = max;
    }

  gimp_size_entry_set_refval_boundaries (info->resolution_entry, 0,
                                         x, GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval_boundaries (info->resolution_entry, 1,
                                         y, GIMP_MAX_RESOLUTION);

 /* FIXME: is this still needed at all? */
  data->orientation = gtk_page_setup_get_orientation (setup);
}
