/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * IfsCompose is a interface for creating IFS fractals by
 * direct manipulation.
 * Copyright (C) 1997 Owen Taylor
 *
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

/* TODO
 * ----
 *
 * 1. Run in non-interactive mode (need to figure out useful
 *    way for a script to give the 19N paramters for an image).
 *    Perhaps just support saving parameters to a file, script
 *    passes file name.  (The save-to-file part is already done [Yeti])
 * 2. Save settings on a per-layer basis (long term, needs GIMP
 *    support to do properly). Load/save from affine parameters?
 * 3. Figure out if we need multiple phases for supersampled
 *    brushes.
 * 4. (minor) Make undo work correctly when focus is in entry widget.
 *    (This seems fixed now (by mere change to spinbuttons) [Yeti])
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "ifscompose.h"

#include "libgimp/stdplugins-intl.h"


#define RESPONSE_RESET           1
#define RESPONSE_OPEN            2
#define RESPONSE_SAVE            3

#define SCALE_WIDTH            150
#define ENTRY_WIDTH             60
#define DESIGN_AREA_MAX_SIZE   300

#define PREVIEW_RENDER_CHUNK 10000

#define UNDO_LEVELS             24

#define IFSCOMPOSE_PARASITE "ifscompose-parasite"
#define IFSCOMPOSE_DATA     "plug_in_ifscompose"
#define HELP_ID             "plug-in-ifs-compose"

typedef enum
{
  OP_TRANSLATE,
  OP_ROTATE,        /* or scale */
  OP_STRETCH
} DesignOp;

typedef enum
{
  VALUE_PAIR_INT,
  VALUE_PAIR_DOUBLE
} ValuePairType;

typedef struct
{
  GtkObject *adjustment;
  GtkWidget *scale;
  GtkWidget *spin;

  ValuePairType type;

  union
  {
    gdouble *d;
    gint    *i;
  } data;
} ValuePair;

typedef struct
{
  IfsComposeVals   ifsvals;
  AffElement     **elements;
  gboolean        *element_selected;
  gint             current_element;
} UndoItem;

typedef struct
{
  GimpRGB   *color;
  GtkWidget *hbox;
  GtkWidget *orig_preview;
  GtkWidget *button;
  gboolean   fixed_point;
} ColorMap;

typedef struct
{
  GtkWidget *dialog;

  ValuePair *iterations_pair;
  ValuePair *subdivide_pair;
  ValuePair *radius_pair;
  ValuePair *memory_pair;
} IfsOptionsDialog;

typedef struct
{
  GtkWidget    *area;
  GtkUIManager *ui_manager;
  GdkPixmap    *pixmap;

  DesignOp      op;
  gdouble       op_x;
  gdouble       op_y;
  gdouble       op_xcenter;
  gdouble       op_ycenter;
  gdouble       op_center_x;
  gdouble       op_center_y;
  guint         button_state;
  gint          num_selected;

  GdkGC        *selected_gc;
} IfsDesignArea;

typedef struct
{
  ValuePair *prob_pair;
  ValuePair *x_pair;
  ValuePair *y_pair;
  ValuePair *scale_pair;
  ValuePair *angle_pair;
  ValuePair *asym_pair;
  ValuePair *shear_pair;
  GtkWidget *flip_check_button;

  ColorMap  *red_cmap;
  ColorMap  *green_cmap;
  ColorMap  *blue_cmap;
  ColorMap  *black_cmap;
  ColorMap  *target_cmap;
  ValuePair *hue_scale_pair;
  ValuePair *value_scale_pair;
  GtkWidget *simple_button;
  GtkWidget *full_button;
  GtkWidget *current_frame;

  GtkWidget *preview;
  guchar    *preview_data;
  gint       preview_iterations;

  gint       drawable_width;
  gint       drawable_height;
  gint       preview_width;
  gint       preview_height;

  AffElement     *selected_orig;
  gint            current_element;
  AffElementVals  current_vals;

  gboolean   in_update;         /* true if we're currently in
                                   update_values() - don't do anything
                                   on updates */
} IfsDialog;

typedef struct
{
  gboolean   run;
} IfsComposeInterface;

/* Declare local functions.
 */
static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

/*  user interface functions  */
static gint           ifs_compose_dialog          (GimpDrawable *drawable);
static void           ifs_options_dialog          (GtkWidget    *parent);
static GtkWidget    * ifs_compose_trans_page      (void);
static GtkWidget    * ifs_compose_color_page      (void);
static GtkUIManager * design_op_menu_create       (GtkWidget   *window);
static void           design_op_actions_update    (void);
static void           design_area_create          (GtkWidget   *window,
                                                   gint         design_width,
                                                   gint         design_height);

/* functions for drawing design window */
static void update_values                   (void);
static void set_current_element             (gint               index);
static void design_area_realize             (GtkWidget         *widget);
static gint design_area_expose              (GtkWidget         *widget,
                                             GdkEventExpose    *event);
static gint design_area_button_press        (GtkWidget         *widget,
                                             GdkEventButton    *event);
static gint design_area_button_release      (GtkWidget         *widget,
                                             GdkEventButton    *event);
static void design_area_select_all_callback (GtkWidget         *widget,
                                             gpointer           data);
static gint design_area_configure           (GtkWidget         *widget,
                                             GdkEventConfigure *event);
static gint design_area_motion              (GtkWidget         *widget,
                                             GdkEventMotion    *event);
static void design_area_redraw              (void);

/* Undo ring functions */
static void undo_begin    (void);
static void undo_update   (gint element);
static void undo_exchange (gint el);
static void undo          (void);
static void redo          (void);

static void recompute_center              (gboolean   save_undo);
static void recompute_center_cb           (GtkWidget *widget,
                                           gpointer   data);

static void ifs_compose                   (GimpDrawable *drawable);

static ColorMap *color_map_create         (gchar     *name,
                                           GimpRGB   *orig_color,
                                           GimpRGB   *data,
                                           gboolean   fixed_point);
static void color_map_color_changed_cb    (GtkWidget *widget,
                                           ColorMap  *color_map);
static void color_map_update              (ColorMap  *color_map);

/* interface functions */
static void simple_color_toggled          (GtkWidget *widget, gpointer data);
static void simple_color_set_sensitive    (void);
static void val_changed_update            (void);
static ValuePair *value_pair_create       (gpointer   data,
                                           gdouble    lower,
                                           gdouble    upper,
                                           gboolean   create_scale,
                                           ValuePairType type);
static void value_pair_update             (ValuePair *value_pair);
static void value_pair_destroy_callback   (GtkWidget *widget,
                                           ValuePair *value_pair);
static void value_pair_scale_callback     (GtkAdjustment *adjustment,
                                           ValuePair *value_pair);

static void design_op_update_callback     (GtkRadioAction *action,
                                           GtkRadioAction *current,
                                           gpointer        data);
static void flip_check_button_callback    (GtkWidget *widget, gpointer data);
static gint preview_idle_render           (gpointer   data);

static void ifs_compose_preview           (void);
static void ifs_compose_set_defaults      (void);
static void ifs_compose_new_callback      (GtkAction *action,
                                           gpointer   data);
static void ifs_compose_delete_callback   (GtkAction *action,
                                           gpointer   data);
static void ifs_compose_options_callback  (GtkAction *action,
                                           gpointer   data);
static void ifs_compose_load              (GtkWidget *parent);
static void ifs_compose_save              (GtkWidget *parent);
static void ifs_compose_response          (GtkWidget *widget,
                                           gint       repsonse_id,
                                           gpointer   data);

/*
 *  Some static variables
 */

static IfsDialog        *ifsD      = NULL;
static IfsOptionsDialog *ifsOptD   = NULL;
static IfsDesignArea    *ifsDesign = NULL;

static AffElement **elements = NULL;
static gint        *element_selected = NULL;
/* labels are generated by printing this int */
static gint         count_for_naming = 0;

static UndoItem     undo_ring[UNDO_LEVELS];
static gint         undo_cur = -1;
static gint         undo_num = 0;
static gint         undo_start = 0;


/* num_elements = 0, signals not inited */
static IfsComposeVals ifsvals =
{
  0,           /* num_elements */
  50000,   /* iterations   */
  4096,           /* max_memory   */
  4,           /* subdivide    */
  0.75,           /* radius       */
  1.0,           /* aspect ratio */
  0.5,           /* center_x     */
  0.5,           /* center_y     */
};

static IfsComposeInterface ifscint =
{
  FALSE,        /* run */
};

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
  };

  static GimpParamDef *return_vals = NULL;
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_ifs_compose",
                          "Create an Iterated Function System (IFS) Fractal",
                          "Interactively create an Iterated Function System "
                          "fractal. Use the window on the upper left to adjust "
                          "the component transformations of the fractal. The "
                          "operation that is performed is selected by the "
                          "buttons underneath the window, or from a menu "
                          "popped up by the right mouse button. The fractal "
                          "will be rendered with a transparent background if "
                          "the current image has an alpha channel.",
                          "Owen Taylor",
                          "Owen Taylor",
                          "1997",
                          N_("_IFS Fractal..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), nreturn_vals,
                          args, return_vals);

  gimp_plugin_menu_register ("plug_in_ifs_compose",
                             "<Image>/Filters/Render/Nature");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *active_drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpParasite      *parasite = NULL;
  guint32            image_id;
  gboolean           found_parasite;

  run_mode = param[0].data.d_int32;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals = values;

  INIT_I18N ();

  image_id        = param[1].data.d_image;
  active_drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data; first look for a parasite -
       *  if not found, fall back to global values
       */
      parasite = gimp_drawable_parasite_find (active_drawable->drawable_id,
                                              IFSCOMPOSE_PARASITE);
      found_parasite = FALSE;
      if (parasite)
        {
          found_parasite = ifsvals_parse_string (gimp_parasite_data (parasite),
                                                 &ifsvals, &elements);
          gimp_parasite_free (parasite);
        }

      if (!found_parasite)
        {
          gint length;

          length = gimp_get_data_size (IFSCOMPOSE_DATA);
          if (length)
            {
              gchar *data = g_new (gchar, length);

              gimp_get_data (IFSCOMPOSE_DATA, data);
              ifsvals_parse_string (data, &ifsvals, &elements);
              g_free (data);
            }
        }

      /* after ifsvals_parse_string, need to set up naming */
      count_for_naming = ifsvals.num_elements;

      /*  First acquire information with a dialog  */
      if (! ifs_compose_dialog (active_drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
        {
          gint length;

          length = gimp_get_data_size (IFSCOMPOSE_DATA);
          if (length)
            {
              gchar *data = g_new (gchar, length);

              gimp_get_data (IFSCOMPOSE_DATA, data);
              ifsvals_parse_string (data, &ifsvals, &elements);
              g_free (data);
            }
          else
            {
              ifs_compose_set_defaults ();
            }
        }
      break;

    default:
      break;
    }

  /*  Render the fractal  */
  if ((status == GIMP_PDB_SUCCESS) &&
      (gimp_drawable_is_rgb (active_drawable->drawable_id) ||
       gimp_drawable_is_gray (active_drawable->drawable_id)))
    {
      /*  set the tile cache size so that the operation works well  */
      gimp_tile_cache_ntiles (2 * (MAX (active_drawable->width,
                                        active_drawable->height) /
                                   gimp_tile_width () + 1));

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          gchar        *str;
          GimpParasite *parasite;

          gimp_image_undo_group_start (image_id);

          /*  run the effect  */
          ifs_compose (active_drawable);

          /*  Store data for next invocation - both globally and
           *  as a parasite on this layer
           */
          str = ifsvals_stringify (&ifsvals, elements);

          gimp_set_data (IFSCOMPOSE_DATA, str, strlen (str) + 1);

          parasite = gimp_parasite_new (IFSCOMPOSE_PARASITE,
                                        GIMP_PARASITE_PERSISTENT |
                                        GIMP_PARASITE_UNDOABLE,
                                        strlen (str) + 1, str);
          gimp_drawable_parasite_attach (active_drawable->drawable_id,
                                         parasite);
          gimp_parasite_free (parasite);

          g_free (str);

          gimp_image_undo_group_end (image_id);
        }
      else
        {
          /*  run the effect  */
          ifs_compose (active_drawable);
        }

      /*  If the run mode is interactive, flush the displays  */
      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
  else if (status == GIMP_PDB_SUCCESS)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (active_drawable);
}

static GtkWidget *
ifs_compose_trans_page (void)
{
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

  table = gtk_table_new (3, 6, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_table_set_col_spacing (GTK_TABLE (table), 2, 6);
  gtk_table_set_col_spacing (GTK_TABLE (table), 4, 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 12);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* X */

  label = gtk_label_new (_("X:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->x_pair = value_pair_create (&ifsD->current_vals.x, 0.0, 1.0, FALSE,
                                    VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->x_pair->spin, 1, 2, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->x_pair->spin);

  /* Y */

  label = gtk_label_new (_("Y:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->y_pair = value_pair_create (&ifsD->current_vals.y, 0.0, 1.0, FALSE,
                                    VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->y_pair->spin, 1, 2, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->y_pair->spin);

  /* Scale */

  label = gtk_label_new (_("Scale:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->scale_pair = value_pair_create (&ifsD->current_vals.scale, 0.0, 1.0,
                                        FALSE, VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->scale_pair->spin, 3, 4, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->scale_pair->spin);

  /* Angle */

  label = gtk_label_new (_("Angle:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->angle_pair = value_pair_create (&ifsD->current_vals.theta, -180, 180,
                                        FALSE, VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->angle_pair->spin, 3, 4, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->angle_pair->spin);

  /* Asym */

  label = gtk_label_new (_("Asymmetry:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 4, 5, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->asym_pair = value_pair_create (&ifsD->current_vals.asym, 0.10, 10.0,
                                       FALSE, VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->asym_pair->spin, 5, 6, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->asym_pair->spin);

  /* Shear */

  label = gtk_label_new (_("Shear:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 4, 5, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->shear_pair = value_pair_create (&ifsD->current_vals.shear, -10.0, 10.0,
                                        FALSE, VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->shear_pair->spin, 5, 6, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->shear_pair->spin);

  /* Flip */

  ifsD->flip_check_button = gtk_check_button_new_with_label (_("Flip"));
  gtk_table_attach (GTK_TABLE (table), ifsD->flip_check_button, 0, 6, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect (ifsD->flip_check_button, "toggled",
                    G_CALLBACK (flip_check_button_callback),
                    NULL);
  gtk_widget_show (ifsD->flip_check_button);

  return vbox;
}

static GtkWidget *
ifs_compose_color_page (void)
{
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  GSList    *group = NULL;
  GimpRGB    color;

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

  table = gtk_table_new (3, 5, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Simple color control section */

  ifsD->simple_button = gtk_radio_button_new_with_label (group, _("Simple"));
  gtk_table_attach (GTK_TABLE (table), ifsD->simple_button, 0, 1, 0, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (ifsD->simple_button));
  g_signal_connect (ifsD->simple_button, "toggled",
                    G_CALLBACK (simple_color_toggled),
                    NULL);
  gtk_widget_show (ifsD->simple_button);

  ifsD->target_cmap = color_map_create (_("IFS Fractal: Target"), NULL,
                                        &ifsD->current_vals.target_color, TRUE);
  gtk_table_attach (GTK_TABLE (table), ifsD->target_cmap->hbox, 1, 2, 0, 2,
                    GTK_FILL, 0, 0, 0);
  gtk_widget_show (ifsD->target_cmap->hbox);

  label = gtk_label_new (_("Scale Hue by:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->hue_scale_pair = value_pair_create (&ifsD->current_vals.hue_scale,
                                            0.0, 1.0, TRUE, VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->hue_scale_pair->scale, 3, 4, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->hue_scale_pair->scale);
  gtk_table_attach (GTK_TABLE (table), ifsD->hue_scale_pair->spin, 4, 5, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->hue_scale_pair->spin);

  label = gtk_label_new (_("Scale Value by:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  ifsD->value_scale_pair = value_pair_create (&ifsD->current_vals.value_scale,
                                              0.0, 1.0, TRUE, VALUE_PAIR_DOUBLE);
  gtk_table_attach (GTK_TABLE (table), ifsD->value_scale_pair->scale,
                    3, 4, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->value_scale_pair->scale);
  gtk_table_attach (GTK_TABLE (table), ifsD->value_scale_pair->spin,
                    4, 5, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->value_scale_pair->spin);

  /* Full color control section */

  ifsD->full_button = gtk_radio_button_new_with_label (group, _("Full"));
  gtk_table_attach (GTK_TABLE (table), ifsD->full_button, 0, 1, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (ifsD->full_button));
  gtk_widget_show (ifsD->full_button);

  gimp_rgb_set (&color, 1.0, 0.0, 0.0);
  ifsD->red_cmap = color_map_create (_("IFS Fractal: Red"), &color,
                                     &ifsD->current_vals.red_color, FALSE);
  gtk_table_attach (GTK_TABLE (table), ifsD->red_cmap->hbox, 1, 2, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->red_cmap->hbox);

  gimp_rgb_set (&color, 0.0, 1.0, 0.0);
  ifsD->green_cmap = color_map_create (_("IFS Fractal: Green"), &color,
                                       &ifsD->current_vals.green_color, FALSE);
  gtk_table_attach (GTK_TABLE (table), ifsD->green_cmap->hbox, 2, 3, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->green_cmap->hbox);

  gimp_rgb_set (&color, 0.0, 0.0, 1.0);
  ifsD->blue_cmap = color_map_create (_("IFS Fractal: Blue"), &color,
                                      &ifsD->current_vals.blue_color, FALSE);
  gtk_table_attach (GTK_TABLE (table), ifsD->blue_cmap->hbox, 3, 4, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->blue_cmap->hbox);

  gimp_rgb_set (&color, 0.0, 0.0, 0.0);
  ifsD->black_cmap = color_map_create (_("IFS Fractal: Black"), &color,
                                       &ifsD->current_vals.black_color, FALSE);
  gtk_table_attach (GTK_TABLE (table), ifsD->black_cmap->hbox, 4, 5, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ifsD->black_cmap->hbox);

  return vbox;
}

static gint
ifs_compose_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *main_vbox;
  GtkWidget *toolbar;
  GtkWidget *aspect_frame;
  GtkWidget *notebook;
  GtkWidget *page;
  gint       design_width  = drawable->width;
  gint       design_height = drawable->height;

  if (design_width > design_height)
    {
      if (design_width > DESIGN_AREA_MAX_SIZE)
        {
          design_height = design_height * DESIGN_AREA_MAX_SIZE / design_width;
          design_width = DESIGN_AREA_MAX_SIZE;
        }
    }
  else
    {
      if (design_height > DESIGN_AREA_MAX_SIZE)
        {
          design_width = design_width * DESIGN_AREA_MAX_SIZE / design_height;
          design_height = DESIGN_AREA_MAX_SIZE;
        }
    }

  ifsD = g_new0 (IfsDialog, 1);

  ifsD->drawable_width  = drawable->width;
  ifsD->drawable_height = drawable->height;
  ifsD->preview_width   = design_width;
  ifsD->preview_height  = design_height;

  gimp_ui_init ("ifscompose", TRUE);

  dialog = gimp_dialog_new (_("IFS Fractal"), "ifscompose",
                            NULL, 0,
                            gimp_standard_help_func, HELP_ID,

                            GTK_STOCK_OPEN,   RESPONSE_OPEN,
                            GTK_STOCK_SAVE,   RESPONSE_SAVE,
                            GIMP_STOCK_RESET, RESPONSE_RESET,
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
					      RESPONSE_OPEN,
					      RESPONSE_SAVE,
					      RESPONSE_RESET,
                                              GTK_RESPONSE_OK,
                                              GTK_RESPONSE_CANCEL,
                                              -1);

  g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer) &dialog);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (ifs_compose_response),
                    NULL);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  design_area_create (dialog, design_width, design_height);

  toolbar = gtk_ui_manager_get_widget (ifsDesign->ui_manager,
                                       "/ifs-compose-toolbar");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), toolbar,
                      FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  /*  The main vbox */
  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), main_vbox,
                      TRUE, TRUE, 0);

  /*  The design area */

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  aspect_frame = gtk_aspect_frame_new (NULL,
                                       0.5, 0.5,
                                       (gdouble) design_width / design_height,
                                       0);
  gtk_frame_set_shadow_type (GTK_FRAME (aspect_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), aspect_frame, TRUE, TRUE, 0);
  gtk_widget_show (aspect_frame);

  gtk_container_add (GTK_CONTAINER (aspect_frame), ifsDesign->area);
  gtk_widget_show (ifsDesign->area);

  /*  The Preview  */

  aspect_frame = gtk_aspect_frame_new (NULL,
                                       0.5, 0.5,
                                       (gdouble) design_width / design_height,
                                       0);
  gtk_frame_set_shadow_type (GTK_FRAME (aspect_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), aspect_frame, TRUE, TRUE, 0);

  ifsD->preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (ifsD->preview,
                               ifsD->preview_width,
                               ifsD->preview_height);
  gtk_container_add (GTK_CONTAINER (aspect_frame), ifsD->preview);
  gtk_widget_show (ifsD->preview);

  gtk_widget_show (aspect_frame);

  gtk_widget_show (hbox);

  /* The current transformation frame */

  ifsD->current_frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), ifsD->current_frame,
                      FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (ifsD->current_frame), vbox);

  /* The notebook */

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  page = ifs_compose_trans_page ();
  label = gtk_label_new (_("Spatial Transformation"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  page = ifs_compose_color_page ();
  label = gtk_label_new (_("Color Transformation"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  /* The probability entry */

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Relative probability:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  ifsD->prob_pair = value_pair_create (&ifsD->current_vals.prob, 0.0, 5.0, TRUE,
                                       VALUE_PAIR_DOUBLE);
  gtk_box_pack_start (GTK_BOX (hbox), ifsD->prob_pair->scale, TRUE, TRUE, 0);
  gtk_widget_show (ifsD->prob_pair->scale);
  gtk_box_pack_start (GTK_BOX (hbox), ifsD->prob_pair->spin, FALSE, TRUE, 0);
  gtk_widget_show (ifsD->prob_pair->spin);

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (ifsD->current_frame);

  gtk_widget_show (main_vbox);

  if (ifsvals.num_elements == 0)
    {
      ifs_compose_set_defaults ();
    }
  else
    {
      gint i;
      gdouble ratio = (gdouble) ifsD->drawable_height / ifsD->drawable_width;

      element_selected = g_new (gint, ifsvals.num_elements);
      element_selected[0] = TRUE;
      for (i = 1; i < ifsvals.num_elements; i++)
        element_selected[i] = FALSE;

      if (ratio != ifsvals.aspect_ratio)
        {
          /* Adjust things so that what fit onto the old image, fits
             onto the new image */
          Aff2 t1, t2, t3;
          gdouble x_offset, y_offset;
          gdouble center_x, center_y;
          gdouble scale;

          if (ratio < ifsvals.aspect_ratio)
            {
              scale = ratio/ifsvals.aspect_ratio;
              x_offset = (1-scale)/2;
              y_offset = 0;
            }
          else
            {
              scale = 1;
              x_offset = 0;
              y_offset = (ratio - ifsvals.aspect_ratio)/2;
            }
          aff2_scale (&t1, scale, 0);
          aff2_translate (&t2, x_offset, y_offset);
          aff2_compose (&t3, &t2, &t1);
          aff2_invert (&t1, &t3);

          aff2_apply (&t3, ifsvals.center_x, ifsvals.center_y, &center_x,
                      &center_y);

          for (i = 0; i < ifsvals.num_elements; i++)
            {
              aff_element_compute_trans (elements[i],1, ifsvals.aspect_ratio,
                                         ifsvals.center_x, ifsvals.center_y);
              aff2_compose (&t2, &elements[i]->trans, &t1);
              aff2_compose (&elements[i]->trans, &t3, &t2);
              aff_element_decompose_trans (elements[i],&elements[i]->trans,
                                           1, ifsvals.aspect_ratio,
                                           center_x, center_y);
            }
          ifsvals.center_x = center_x;
          ifsvals.center_y = center_y;

          ifsvals.aspect_ratio = ratio;
        }

      for (i = 0; i < ifsvals.num_elements; i++)
        aff_element_compute_color_trans (elements[i]);
      /* boundary and spatial transformations will be computed
         when the design_area gets a ConfigureNotify event */

      set_current_element (0);

      ifsD->selected_orig = g_new (AffElement, ifsvals.num_elements);
    }

  gtk_widget_show (dialog);

  ifs_compose_preview ();

  gtk_main ();

  g_object_unref (ifsDesign->ui_manager);

  if (dialog)
    gtk_widget_destroy (dialog);

  if (ifsOptD)
    gtk_widget_destroy (ifsOptD->dialog);

  gdk_flush ();

  g_object_unref (ifsDesign->selected_gc);

  g_free (ifsD);

  return ifscint.run;
}

static void
design_area_create (GtkWidget *window,
                    gint       design_width,
                    gint       design_height)
{
  ifsDesign = g_new0 (IfsDesignArea, 1);

  ifsDesign->op = OP_TRANSLATE;

  ifsDesign->area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (ifsDesign->area, design_width, design_height);

  g_signal_connect (ifsDesign->area, "realize",
                    G_CALLBACK (design_area_realize),
                    NULL);
  g_signal_connect (ifsDesign->area, "expose_event",
                    G_CALLBACK (design_area_expose),
                    NULL);
  g_signal_connect (ifsDesign->area, "button_press_event",
                    G_CALLBACK (design_area_button_press),
                    NULL);
  g_signal_connect (ifsDesign->area, "button_release_event",
                    G_CALLBACK (design_area_button_release),
                    NULL);
  g_signal_connect (ifsDesign->area, "motion_notify_event",
                    G_CALLBACK (design_area_motion),
                    NULL);
  g_signal_connect (ifsDesign->area, "configure_event",
                    G_CALLBACK (design_area_configure),
                    NULL);
  gtk_widget_set_events (ifsDesign->area,
                         GDK_EXPOSURE_MASK       |
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_POINTER_MOTION_HINT_MASK);

  ifsDesign->ui_manager = design_op_menu_create (window);
  design_op_actions_update ();
}

static GtkUIManager *
design_op_menu_create (GtkWidget *window)
{
  static GtkActionEntry actions[] =
  {
    { "ifs-compose-menu", NULL, "IFS Fractal Menu" },

    { "new", GTK_STOCK_NEW,
      NULL, "<control>N", NULL,
      G_CALLBACK (ifs_compose_new_callback) },

    { "delete", GTK_STOCK_DELETE,
      NULL, "<control>D", NULL,
      G_CALLBACK (ifs_compose_delete_callback) },

    { "undo", GTK_STOCK_UNDO,
      NULL, "<control>Z", NULL,
      G_CALLBACK (undo) },

    { "redo", GTK_STOCK_REDO,
      NULL, "<control>Y", NULL,
      G_CALLBACK (redo) },

    { "select-all", GIMP_STOCK_SELECTION_ALL,
      N_("Select _All"), "<control>A", NULL,
      G_CALLBACK (design_area_select_all_callback) },

    { "center", GIMP_STOCK_CENTER,
      N_("Re_center"), "<control>C", N_("Recompute Center"),
      G_CALLBACK (recompute_center_cb) },

    { "options", GTK_STOCK_PREFERENCES,
      N_("Render options"), NULL, NULL,
      G_CALLBACK (ifs_compose_options_callback) }
  };
  static GtkRadioActionEntry radio_actions[] =
  {
    { "move", GIMP_STOCK_TOOL_MOVE,
      N_("Move"), "M", NULL, OP_TRANSLATE },

    { "rotate", GIMP_STOCK_TOOL_ROTATE,
      N_("Rotate"), "R", N_("Rotate / Scale"), OP_ROTATE },

    { "stretch", GIMP_STOCK_TOOL_PERSPECTIVE,
      N_("Stretch"), "S", NULL, OP_STRETCH }
  };

  GtkUIManager   *ui_manager = gtk_ui_manager_new ();
  GtkActionGroup *group      = gtk_action_group_new ("Actions");

  gtk_action_group_set_translation_domain (group, NULL);

  gtk_action_group_add_actions (group,
                                actions,
                                G_N_ELEMENTS (actions),
                                window);
  gtk_action_group_add_radio_actions (group,
                                      radio_actions,
                                      G_N_ELEMENTS (radio_actions),
                                      ifsDesign->op,
                                      G_CALLBACK (design_op_update_callback),
                                      window);

  gtk_window_add_accel_group (GTK_WINDOW (window),
                              gtk_ui_manager_get_accel_group (ui_manager));
  gtk_accel_group_lock (gtk_ui_manager_get_accel_group (ui_manager));

  gtk_ui_manager_insert_action_group (ui_manager, group, -1);
  g_object_unref (group);

  gtk_ui_manager_add_ui_from_string (ui_manager,
                                     "<ui>"
                                     "  <menubar name=\"dummy-menubar\">"
                                     "    <menu action=\"ifs-compose-menu\">"
                                     "      <menuitem action=\"move\" />"
                                     "      <menuitem action=\"rotate\" />"
                                     "      <menuitem action=\"stretch\" />"
                                     "      <separator />"
                                     "      <menuitem action=\"new\" />"
                                     "      <menuitem action=\"delete\" />"
                                     "      <menuitem action=\"undo\" />"
                                     "      <menuitem action=\"redo\" />"
                                     "      <menuitem action=\"select-all\" />"
                                     "      <menuitem action=\"center\" />"
                                     "      <separator />"
                                     "      <menuitem action=\"options\" />"
                                     "    </menu>"
                                     "  </menubar>"
                                     "</ui>",
                                     -1, NULL);

  gtk_ui_manager_add_ui_from_string (ui_manager,
                                     "<ui>"
                                     "  <toolbar name=\"ifs-compose-toolbar\">"
                                     "    <toolitem action=\"move\" />"
                                     "    <toolitem action=\"rotate\" />"
                                     "    <toolitem action=\"stretch\" />"
                                     "    <separator />"
                                     "    <toolitem action=\"new\" />"
                                     "    <toolitem action=\"delete\" />"
                                     "    <toolitem action=\"undo\" />"
                                     "    <toolitem action=\"redo\" />"
                                     "    <toolitem action=\"select-all\" />"
                                     "    <toolitem action=\"center\" />"
                                     "    <separator />"
                                     "    <toolitem action=\"options\" />"
                                     "  </toolbar>"
                                     "</ui>",
                                     -1, NULL);

  return ui_manager;
}

static void
design_op_actions_update (void)
{
  g_object_set (gtk_ui_manager_get_action (ifsDesign->ui_manager,
                                           "/ui/dummy-menubar/ifs-compose-menu/undo"),
                "sensitive", undo_cur >= 0,
                NULL);
  g_object_set (gtk_ui_manager_get_action (ifsDesign->ui_manager,
                                           "/ui/dummy-menubar/ifs-compose-menu/redo"),
                "sensitive", undo_cur != undo_num - 1,
                NULL);
  g_object_set (gtk_ui_manager_get_action (ifsDesign->ui_manager,
                                           "/ui/dummy-menubar/ifs-compose-menu/delete"),
                "sensitive", ifsvals.num_elements > 2,
                NULL);
}

static void
ifs_options_dialog (GtkWidget *parent)
{
  if (!ifsOptD)
    {
      GtkWidget *table;
      GtkWidget *label;

      ifsOptD = g_new0 (IfsOptionsDialog, 1);

      ifsOptD->dialog =
        gimp_dialog_new (_("IFS Fractal Render Options"), "ifscompose",
                         parent, 0, NULL, NULL,

                         GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                         NULL);

      g_signal_connect (ifsOptD->dialog, "response",
                        G_CALLBACK (gtk_widget_hide),
                        NULL);

      /* Table of options */

      table = gtk_table_new (4, 3, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (table), 12);
      gtk_table_set_row_spacings (GTK_TABLE (table), 6);
      gtk_table_set_col_spacings (GTK_TABLE (table), 6);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ifsOptD->dialog)->vbox), table,
                          FALSE, FALSE, 0);
      gtk_widget_show (table);

      label = gtk_label_new (_("Max. Memory:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      ifsOptD->memory_pair = value_pair_create (&ifsvals.max_memory,
                                                1, 1000000, FALSE,
                                                VALUE_PAIR_INT);
      gtk_table_attach (GTK_TABLE (table), ifsOptD->memory_pair->spin,
                        1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->memory_pair->spin);

      label = gtk_label_new (_("Iterations:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      ifsOptD->iterations_pair = value_pair_create (&ifsvals.iterations,
                                                    1, 10000000, FALSE,
                                                    VALUE_PAIR_INT);
      gtk_table_attach (GTK_TABLE (table), ifsOptD->iterations_pair->spin,
                        1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->iterations_pair->spin);
      gtk_widget_show (label);

      label = gtk_label_new (_("Subdivide:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      ifsOptD->subdivide_pair = value_pair_create (&ifsvals.subdivide,
                                                   1, 10, FALSE,
                                                   VALUE_PAIR_INT);
      gtk_table_attach (GTK_TABLE (table), ifsOptD->subdivide_pair->spin,
                        1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->subdivide_pair->spin);

      label = gtk_label_new (_("Spot Radius:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      ifsOptD->radius_pair = value_pair_create (&ifsvals.radius,
                                                0, 5, TRUE,
                                                VALUE_PAIR_DOUBLE);
      gtk_table_attach (GTK_TABLE (table), ifsOptD->radius_pair->scale,
                        1, 2, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->radius_pair->scale);
      gtk_table_attach (GTK_TABLE (table), ifsOptD->radius_pair->spin,
                        2, 3, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (ifsOptD->radius_pair->spin);

      value_pair_update (ifsOptD->iterations_pair);
      value_pair_update (ifsOptD->subdivide_pair);
      value_pair_update (ifsOptD->memory_pair);
      value_pair_update (ifsOptD->radius_pair);

      gtk_widget_show (ifsOptD->dialog);
    }
  else
    {
      gtk_window_present (GTK_WINDOW (ifsOptD->dialog));
    }
}

static void
ifs_compose (GimpDrawable *drawable)
{
  GimpImageType  type   = gimp_drawable_type (drawable->drawable_id);
  gint           width  = drawable->width;
  gint           height = drawable->height;
  gint           num_bands, band_height, band_y, band_no;
  gint           i, j;
  gchar         *buffer;
  guchar        *data;
  guchar        *mask = NULL;
  guchar        *nhits;
  guchar         rc, gc, bc;
  GimpRGB        color;

  num_bands = ceil((gdouble)(width*height*SQR(ifsvals.subdivide)*5)
                   / (1024 * ifsvals.max_memory));
  band_height = (height + num_bands - 1) / num_bands;

  if (band_height > height)
    band_height = height;

  mask  = g_new (guchar, width * band_height * SQR (ifsvals.subdivide));
  data  = g_new (guchar, width * band_height * SQR (ifsvals.subdivide) * 3);
  nhits = g_new (guchar, width * band_height * SQR (ifsvals.subdivide));

  gimp_context_get_background (&color);
  gimp_rgb_get_uchar (&color, &rc, &gc, &bc);

  band_y = 0;
  for (band_no = 0; band_no < num_bands; band_no++)
    {
      guchar *ptr;
      guchar *maskptr;
      guchar *dest;
      guchar *destrow;
      guchar maskval;
      GimpPixelRgn dest_rgn;
      gint progress;
      gint max_progress;

      gpointer pr;

      buffer = g_strdup_printf (_("Rendering IFS (%d/%d)..."),
                                band_no+1, num_bands);
      gimp_progress_init (buffer);
      g_free (buffer);

      /* render the band to a buffer */
      if (band_y + band_height > height)
        band_height = height - band_y;

      /* we don't need to clear data since we store nhits */
      memset(mask, 0, width*band_height*SQR(ifsvals.subdivide));
      memset(nhits, 0, width*band_height*SQR(ifsvals.subdivide));

      ifs_render (elements,
                  ifsvals.num_elements, width, height, ifsvals.iterations,
                  &ifsvals, band_y, band_height, data, mask, nhits, FALSE);

      /* transfer the image to the drawable */


      buffer = g_strdup_printf (_("Copying IFS to image (%d/%d)..."),
                                band_no+1, num_bands);
      gimp_progress_init (buffer);
      g_free (buffer);

      progress = 0;
      max_progress = band_height * width;

      gimp_pixel_rgn_init (&dest_rgn, drawable, 0, band_y,
                           width, band_height, TRUE, TRUE);

      for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
           pr != NULL;
           pr = gimp_pixel_rgns_process (pr))
        {
          destrow = dest_rgn.data;

          for (j = dest_rgn.y; j < (dest_rgn.y + dest_rgn.h); j++)
            {
              dest = destrow;

              for (i = dest_rgn.x; i < (dest_rgn.x + dest_rgn.w); i++)
                {
                  /* Accumulate a reduced pixel */

                  gint ii, jj;
                  gint rtot=0;
                  gint btot=0;
                  gint gtot=0;
                  gint mtot=0;
                  for (jj = 0; jj < ifsvals.subdivide; jj++)
                    {
                      ptr = data + 3 *
                        (((j-band_y)*ifsvals.subdivide+jj) *
                         ifsvals.subdivide*width +
                         i*ifsvals.subdivide);

                      maskptr = mask +
                        ((j-band_y)*ifsvals.subdivide+jj) *
                        ifsvals.subdivide*width +
                        i*ifsvals.subdivide;
                      for (ii = 0; ii < ifsvals.subdivide; ii++)
                        {
                          maskval = *maskptr++;
                          mtot += maskval;
                          rtot += maskval* *ptr++;
                          gtot += maskval* *ptr++;
                          btot += maskval* *ptr++;
                        }
                    }
                  if (mtot)
                    {
                      rtot /= mtot;
                      gtot /= mtot;
                      btot /= mtot;
                      mtot /= SQR(ifsvals.subdivide);
                    }
                  /* and store it */
                  switch (type)
                    {
                     case GIMP_GRAY_IMAGE:
                      *dest++ = (mtot*(rtot+btot+gtot)+
                                 (255-mtot)*(rc+gc+bc))/(3*255);
                      break;
                    case GIMP_GRAYA_IMAGE:
                      *dest++ = (rtot+btot+gtot)/3;
                      *dest++ = mtot;
                      break;
                    case GIMP_RGB_IMAGE:
                      *dest++ = (mtot*rtot + (255-mtot)*rc)/255;
                      *dest++ = (mtot*gtot + (255-mtot)*gc)/255;
                      *dest++ = (mtot*btot + (255-mtot)*bc)/255;
                      break;
                    case GIMP_RGBA_IMAGE:
                      *dest++ = rtot;
                      *dest++ = gtot;
                      *dest++ = btot;
                      *dest++ = mtot;
                      break;
                    case GIMP_INDEXED_IMAGE:
                    case GIMP_INDEXEDA_IMAGE:
                      g_error ("Indexed images not supported by IFS Fractal");
                      break;
                    }
                }
              destrow += dest_rgn.rowstride;
            }
          progress += dest_rgn.w * dest_rgn.h;
          gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
        }
      band_y += band_height;
    }

  g_free (mask);
  g_free (data);
  g_free (nhits);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, 0, 0, width, height);
}

static void
update_values (void)
{
  ifsD->in_update = TRUE;

  ifsD->current_vals = elements[ifsD->current_element]->v;
  ifsD->current_vals.theta *= 180/G_PI;

  value_pair_update (ifsD->prob_pair);
  value_pair_update (ifsD->x_pair);
  value_pair_update (ifsD->y_pair);
  value_pair_update (ifsD->scale_pair);
  value_pair_update (ifsD->angle_pair);
  value_pair_update (ifsD->asym_pair);
  value_pair_update (ifsD->shear_pair);
  color_map_update (ifsD->red_cmap);
  color_map_update (ifsD->green_cmap);
  color_map_update (ifsD->blue_cmap);
  color_map_update (ifsD->black_cmap);
  color_map_update (ifsD->target_cmap);
  value_pair_update (ifsD->hue_scale_pair);
  value_pair_update (ifsD->value_scale_pair);

  if (elements[ifsD->current_element]->v.simple_color)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ifsD->simple_button),
                                  TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ifsD->full_button),
                                 TRUE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ifsD->flip_check_button),
                                elements[ifsD->current_element]->v.flip);

  ifsD->in_update = FALSE;

  simple_color_set_sensitive ();
}

static void
set_current_element (gint index)
{
  gchar *frame_name = g_strdup_printf (_("Transformation %s"),
                                      elements[index]->name);

  ifsD->current_element = index;

  gtk_frame_set_label (GTK_FRAME (ifsD->current_frame),frame_name);
  g_free (frame_name);

  update_values ();
}

static void
design_area_realize (GtkWidget *widget)
{
  GdkDisplay *display = gtk_widget_get_display (widget);
  GdkCursor  *cursor  = gdk_cursor_new_for_display (display, GDK_CROSSHAIR);

  gdk_window_set_cursor (widget->window, cursor);
  gdk_cursor_unref (cursor);
}

static gboolean
design_area_expose (GtkWidget      *widget,
                    GdkEventExpose *event)
{
  PangoLayout *layout;
  gint         i;
  gint         cx, cy;

  if (!ifsDesign->selected_gc)
    {
      ifsDesign->selected_gc = gdk_gc_new (ifsDesign->area->window);
      gdk_gc_set_line_attributes (ifsDesign->selected_gc, 2,
                                 GDK_LINE_SOLID, GDK_CAP_ROUND,
                                 GDK_JOIN_ROUND);
    }

  gdk_draw_rectangle (ifsDesign->pixmap,
                     widget->style->bg_gc[widget->state],
                     TRUE,
                     event->area.x,
                     event->area.y,
                     event->area.width, event->area.height);

  /* draw an indicator for the center */

  cx = ifsvals.center_x * widget->allocation.width;
  cy = ifsvals.center_y * widget->allocation.width;
  gdk_draw_line (ifsDesign->pixmap,
                 widget->style->fg_gc[widget->state],
                 cx - 10, cy, cx + 10, cy);
  gdk_draw_line (ifsDesign->pixmap,
                 widget->style->fg_gc[widget->state],
                 cx, cy - 10, cx, cy + 10);

  layout = gtk_widget_create_pango_layout (widget, NULL);

  for (i = 0; i < ifsvals.num_elements; i++)
    {
      aff_element_draw (elements[i], element_selected[i],
                        widget->allocation.width,
                        widget->allocation.height,
                        ifsDesign->pixmap,
                        widget->style->fg_gc[widget->state],
                        ifsDesign->selected_gc,
                        layout);
    }

  g_object_unref (layout);

  gdk_draw_drawable (widget->window,
                     widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                     ifsDesign->pixmap,
                     event->area.x, event->area.y,
                     event->area.x, event->area.y,
                     event->area.width, event->area.height);

  return FALSE;
}

static gboolean
design_area_configure (GtkWidget         *widget,
                       GdkEventConfigure *event)
{
  gint i;
  gdouble width = widget->allocation.width;
  gdouble height = widget->allocation.height;

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_compute_trans (elements[i],width, height,
                              ifsvals.center_x, ifsvals.center_y);
  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_compute_boundary (elements[i],width, height,
                                 elements, ifsvals.num_elements);

  if (ifsDesign->pixmap)
    {
      g_object_unref (ifsDesign->pixmap);
    }
  ifsDesign->pixmap = gdk_pixmap_new (widget->window,
                                     widget->allocation.width,
                                     widget->allocation.height,
                                     -1); /* Is this correct? */

  return FALSE;
}

static gint
design_area_button_press (GtkWidget      *widget,
                          GdkEventButton *event)
{
  gdouble width = ifsDesign->area->allocation.width;
  gint    i;
  gint    old_current;

  gtk_widget_grab_focus (widget);

  if (event->button != 1 || (ifsDesign->button_state & GDK_BUTTON1_MASK))
    {
      if (event->button == 3)
        {
          GtkWidget *menu =
            gtk_ui_manager_get_widget (ifsDesign->ui_manager,
                                       "/dummy-menubar/ifs-compose-menu");

          if (GTK_IS_MENU_ITEM (menu))
            menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (menu));

          gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));

          gtk_menu_popup (GTK_MENU (menu),
                          NULL, NULL, NULL, NULL,
                          event->button, event->time);

          return FALSE;
        }
    }

  old_current = ifsD->current_element;
  ifsD->current_element = -1;

  /* Find out where the button press was */
  for (i = 0; i < ifsvals.num_elements; i++)
    {
      if (ipolygon_contains (elements[i]->click_boundary, event->x, event->y))
        {
          set_current_element (i);
          break;
        }
    }

  /* if the user started manipulating an object, set up a new
     position on the undo ring */
  if (ifsD->current_element >= 0)
    undo_begin ();

  if (!(event->state & GDK_SHIFT_MASK)
      && ( (ifsD->current_element<0)
           || !element_selected[ifsD->current_element] ))
    {
      for (i = 0; i < ifsvals.num_elements; i++)
        element_selected[i] = FALSE;
    }

  if (ifsD->current_element >= 0)
    {
      ifsDesign->button_state |= GDK_BUTTON1_MASK;

      element_selected[ifsD->current_element] = TRUE;

      ifsDesign->num_selected = 0;
      ifsDesign->op_xcenter = 0.0;
      ifsDesign->op_ycenter = 0.0;
      for (i = 0; i < ifsvals.num_elements; i++)
        {
          if (element_selected[i])
            {
              ifsD->selected_orig[i] = *elements[i];
              ifsDesign->op_xcenter += elements[i]->v.x;
              ifsDesign->op_ycenter += elements[i]->v.y;
              ifsDesign->num_selected++;
              undo_update (i);
            }
        }
      ifsDesign->op_xcenter /= ifsDesign->num_selected;
      ifsDesign->op_ycenter /= ifsDesign->num_selected;
      ifsDesign->op_x = (gdouble)event->x / width;
      ifsDesign->op_y = (gdouble)event->y / width;
      ifsDesign->op_center_x = ifsvals.center_x;
      ifsDesign->op_center_y = ifsvals.center_y;
    }
  else
    {
      ifsD->current_element = old_current;
      element_selected[old_current] = TRUE;
    }

  design_area_redraw ();

  return FALSE;
}

static gint
design_area_button_release (GtkWidget      *widget,
                            GdkEventButton *event)
{
  if (event->button == 1 &&
      (ifsDesign->button_state & GDK_BUTTON1_MASK))
    {
      ifsDesign->button_state &= ~GDK_BUTTON1_MASK;
      ifs_compose_preview ();
    }
  return FALSE;
}

static gint
design_area_motion (GtkWidget      *widget,
                    GdkEventMotion *event)
{
  gint i;
  gdouble xo;
  gdouble yo;
  gdouble xn;
  gdouble yn;
  gint px, py;
  gdouble width = ifsDesign->area->allocation.width;
  gdouble height = ifsDesign->area->allocation.height;

  Aff2 trans, t1, t2, t3;

  if (!(ifsDesign->button_state & GDK_BUTTON1_MASK)) return FALSE;

  if (event->is_hint)
    {
      gtk_widget_get_pointer (ifsDesign->area, &px, &py);
      event->x = px;
      event->y = py;
    }

  xo = (ifsDesign->op_x - ifsDesign->op_xcenter);
  yo = (ifsDesign->op_y - ifsDesign->op_ycenter);
  xn = (gdouble) event->x / width - ifsDesign->op_xcenter;
  yn = (gdouble) event->y / width - ifsDesign->op_ycenter;

  switch (ifsDesign->op)
    {
    case OP_ROTATE:
      {
        aff2_translate (&t1,-ifsDesign->op_xcenter*width,
                       -ifsDesign->op_ycenter*width);
        aff2_scale (&t2,
                   sqrt((SQR(xn)+SQR(yn))/(SQR(xo)+SQR(yo))),
                   0);
        aff2_compose (&t3, &t2, &t1);
        aff2_rotate (&t1, - atan2(yn, xn) + atan2(yo, xo));
        aff2_compose (&t2, &t1, &t3);
        aff2_translate (&t3, ifsDesign->op_xcenter*width,
                       ifsDesign->op_ycenter*width);
        aff2_compose (&trans, &t3, &t2);
        break;
      }
    case OP_STRETCH:
      {
        aff2_translate (&t1,-ifsDesign->op_xcenter*width,
                       -ifsDesign->op_ycenter*width);
        aff2_compute_stretch (&t2, xo, yo, xn, yn);
        aff2_compose (&t3, &t2, &t1);
        aff2_translate (&t1, ifsDesign->op_xcenter*width,
                       ifsDesign->op_ycenter*width);
        aff2_compose (&trans, &t1, &t3);
        break;
      }
    case OP_TRANSLATE:
      {
        aff2_translate (&trans,(xn-xo)*width,(yn-yo)*width);
        break;
      }
    }

  for (i = 0; i < ifsvals.num_elements; i++)
    if (element_selected[i])
      {
        if (ifsDesign->num_selected == ifsvals.num_elements)
          {
            gdouble cx, cy;
            aff2_invert (&t1, &trans);
            aff2_compose (&t2, &trans, &ifsD->selected_orig[i].trans);
            aff2_compose (&elements[i]->trans, &t2, &t1);

            cx = ifsDesign->op_center_x * width;
            cy = ifsDesign->op_center_y * width;
            aff2_apply (&trans, cx, cy, &cx, &cy);
            ifsvals.center_x = cx / width;
            ifsvals.center_y = cy / width;
          }
        else
          {
            aff2_compose (&elements[i]->trans, &trans,
                         &ifsD->selected_orig[i].trans);
          }
        aff_element_decompose_trans (elements[i],&elements[i]->trans,
                                    width, height, ifsvals.center_x,
                                    ifsvals.center_y);
        aff_element_compute_trans (elements[i],width, height,
                              ifsvals.center_x, ifsvals.center_y);
      }

  update_values ();
  design_area_redraw ();

  return FALSE;
}

static void
design_area_redraw (void)
{
  gdouble width  = ifsDesign->area->allocation.width;
  gdouble height = ifsDesign->area->allocation.height;
  gint    i;

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_compute_boundary (elements[i],width, height,
                                  elements, ifsvals.num_elements);

  gtk_widget_queue_draw (ifsDesign->area);
}

/* Undo ring functions */
static void
undo_begin (void)
{
  gint i, j;
  gint to_delete;
  gint new_index;

  if (undo_cur == UNDO_LEVELS-1)
    {
      to_delete = 1;
      undo_start = (undo_start + 1) % UNDO_LEVELS;
    }
  else
    {
      undo_cur++;
      to_delete = undo_num - undo_cur;
    }

  undo_num = undo_num - to_delete + 1;
  new_index = (undo_start + undo_cur) % UNDO_LEVELS;

  /* remove any redo elements or the oldest element if necessary */
  for (j = new_index; to_delete > 0; j = (j+1) % UNDO_LEVELS, to_delete--)
    {
      for (i = 0; i < undo_ring[j].ifsvals.num_elements; i++)
        if (undo_ring[j].elements[i])
          aff_element_free (undo_ring[j].elements[i]);
      g_free (undo_ring[j].elements);
      g_free (undo_ring[j].element_selected);
    }

  undo_ring[new_index].ifsvals = ifsvals;
  undo_ring[new_index].elements = g_new (AffElement *,ifsvals.num_elements);
  undo_ring[new_index].element_selected = g_new (gboolean,
                                                 ifsvals.num_elements);
  undo_ring[new_index].current_element = ifsD->current_element;

  for (i = 0; i < ifsvals.num_elements; i++)
    {
      undo_ring[new_index].elements[i] = NULL;
      undo_ring[new_index].element_selected[i] = element_selected[i];
    }

  design_op_actions_update ();
}

static void
undo_update (gint el)
{
  AffElement *elem;
  /* initialize */

  elem = NULL;

  if (!undo_ring[(undo_start + undo_cur) % UNDO_LEVELS].elements[el])
    undo_ring[(undo_start + undo_cur) % UNDO_LEVELS].elements[el]
      = elem = g_new (AffElement, 1);

  *elem = *elements[el];
  elem->draw_boundary = NULL;
  elem->click_boundary = NULL;
}

static void
undo_exchange (gint el)
{
  gint i;
  AffElement **telements;
  gboolean *tselected;
  IfsComposeVals tifsvals;
  gint tcurrent;
  gdouble width = ifsDesign->area->allocation.width;
  gdouble height = ifsDesign->area->allocation.height;

  /* swap the arrays and values*/
  telements = elements;
  elements = undo_ring[el].elements;
  undo_ring[el].elements = telements;

  tifsvals = ifsvals;
  ifsvals = undo_ring[el].ifsvals;
  undo_ring[el].ifsvals = tifsvals;

  tselected = element_selected;
  element_selected = undo_ring[el].element_selected;
  undo_ring[el].element_selected = tselected;

  tcurrent = ifsD->current_element;
  ifsD->current_element = undo_ring[el].current_element;
  undo_ring[el].current_element = tcurrent;

  /* now swap back any unchanged elements */
  for (i = 0; i < ifsvals.num_elements; i++)
    if (!elements[i])
      {
        elements[i] = undo_ring[el].elements[i];
        undo_ring[el].elements[i] = NULL;
      }
    else
      aff_element_compute_trans (elements[i],width, height,
                                ifsvals.center_x, ifsvals.center_y);

  set_current_element (ifsD->current_element);

  design_area_redraw ();

  ifs_compose_preview ();
}

static void
undo (void)
{
  if (undo_cur >= 0)
    {
      undo_exchange ((undo_start + undo_cur) % UNDO_LEVELS);
      undo_cur--;
    }

  design_op_actions_update ();
}

static void
redo (void)
{
  if (undo_cur != undo_num - 1)
    {
      undo_cur++;
      undo_exchange ((undo_start + undo_cur) % UNDO_LEVELS);
    }

  design_op_actions_update ();
}

static void
design_area_select_all_callback (GtkWidget *widget,
                                 gpointer   data)
{
  gint i;

  for (i = 0; i < ifsvals.num_elements; i++)
    element_selected[i] = TRUE;

  design_area_redraw ();
}

/*  Interface functions  */

static void
val_changed_update (void)
{
  AffElement *cur;
  gdouble     width;
  gdouble     height;

  if (ifsD->in_update)
    return;

  width = ifsDesign->area->allocation.width;
  height = ifsDesign->area->allocation.height;
  cur = elements[ifsD->current_element];

  undo_begin ();
  undo_update (ifsD->current_element);

  cur->v = ifsD->current_vals;
  cur->v.theta *= G_PI/180.0;
  aff_element_compute_trans (cur, width, height,
                              ifsvals.center_x, ifsvals.center_y);
  aff_element_compute_color_trans (cur);

  design_area_redraw ();

  ifs_compose_preview ();
}

/* Pseudo-widget representing a color mapping */

#define COLOR_SAMPLE_SIZE 30

static ColorMap *
color_map_create (gchar    *name,
                  GimpRGB  *orig_color,
                  GimpRGB  *data,
                  gboolean  fixed_point)
{
  GtkWidget *frame;
  GtkWidget *arrow;
  ColorMap  *color_map = g_new (ColorMap, 1);

  gimp_rgb_set_alpha (data, 1.0);
  color_map->color       = data;
  color_map->fixed_point = fixed_point;
  color_map->hbox        = gtk_hbox_new (FALSE, 2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (color_map->hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  color_map->orig_preview =
    gimp_color_area_new (fixed_point ? data : orig_color,
                         GIMP_COLOR_AREA_FLAT, 0);
  gtk_drag_dest_unset (color_map->orig_preview);
  gtk_widget_set_size_request (color_map->orig_preview,
                               COLOR_SAMPLE_SIZE, COLOR_SAMPLE_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), color_map->orig_preview);
  gtk_widget_show (color_map->orig_preview);

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (color_map->hbox), arrow, FALSE, FALSE, 0);
  gtk_widget_show (arrow);

  color_map->button = gimp_color_button_new (name,
                                             COLOR_SAMPLE_SIZE,
                                             COLOR_SAMPLE_SIZE,
                                             data,
                                             GIMP_COLOR_AREA_FLAT);
  gtk_box_pack_start (GTK_BOX (color_map->hbox), color_map->button,
                      FALSE, FALSE, 0);
  gtk_widget_show (color_map->button);

  g_signal_connect (color_map->button, "color_changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    data);

  g_signal_connect (color_map->button, "color_changed",
                    G_CALLBACK (color_map_color_changed_cb),
                    color_map);

  return color_map;
}

static void
color_map_color_changed_cb (GtkWidget *widget,
                            ColorMap  *color_map)
{
  if (ifsD->in_update)
    return;

  undo_begin ();
  undo_update (ifsD->current_element);

  elements[ifsD->current_element]->v = ifsD->current_vals;
  elements[ifsD->current_element]->v.theta *= G_PI/180.0;
  aff_element_compute_color_trans (elements[ifsD->current_element]);

  update_values ();

  ifs_compose_preview ();
}

static void
color_map_update (ColorMap *color_map)
{
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (color_map->button),
                               color_map->color);

  if (color_map->fixed_point)
    gimp_color_area_set_color (GIMP_COLOR_AREA (color_map->orig_preview),
                               color_map->color);
}

static void
simple_color_set_sensitive (void)
{
  gint sc = elements[ifsD->current_element]->v.simple_color;

  gtk_widget_set_sensitive (ifsD->target_cmap->hbox,       sc);
  gtk_widget_set_sensitive (ifsD->hue_scale_pair->scale,   sc);
  gtk_widget_set_sensitive (ifsD->hue_scale_pair->spin,   sc);
  gtk_widget_set_sensitive (ifsD->value_scale_pair->scale, sc);
  gtk_widget_set_sensitive (ifsD->value_scale_pair->spin, sc);

  gtk_widget_set_sensitive (ifsD->red_cmap->hbox,   !sc);
  gtk_widget_set_sensitive (ifsD->green_cmap->hbox, !sc);
  gtk_widget_set_sensitive (ifsD->blue_cmap->hbox,  !sc);
  gtk_widget_set_sensitive (ifsD->black_cmap->hbox, !sc);
}

static void
simple_color_toggled (GtkWidget *widget,
                      gpointer   data)
{
  AffElement *cur = elements[ifsD->current_element];

  cur->v.simple_color = GTK_TOGGLE_BUTTON (widget)->active;
  ifsD->current_vals.simple_color = cur->v.simple_color;
  if (cur->v.simple_color)
    aff_element_compute_color_trans (cur);

  val_changed_update ();
  simple_color_set_sensitive ();
}

/* Generic mechanism for scale/entry combination (possibly without
   scale) */

static ValuePair *
value_pair_create (gpointer      data,
                   gdouble       lower,
                   gdouble       upper,
                   gboolean      create_scale,
                   ValuePairType type)
{

  ValuePair *value_pair = g_new (ValuePair, 1);

  value_pair->data.d = data;
  value_pair->type   = type;

  value_pair->adjustment =
    gtk_adjustment_new (1.0, lower, upper,
                        (upper-lower) / 100, (upper-lower) / 10,
                        0.0);
  /* We need to sink the adjustment, since we may not create a scale for
   * it, so nobody will assume the initial refcount
   */
  g_object_ref (value_pair->adjustment);
  gtk_object_sink (value_pair->adjustment);

  if (create_scale)
    {
      value_pair->scale =
        gtk_hscale_new (GTK_ADJUSTMENT (value_pair->adjustment));
      gtk_widget_ref (value_pair->scale);

      if (type == VALUE_PAIR_INT)
          gtk_scale_set_digits (GTK_SCALE (value_pair->scale), 0);
      else
          gtk_scale_set_digits (GTK_SCALE (value_pair->scale), 3);

      gtk_scale_set_draw_value (GTK_SCALE (value_pair->scale), FALSE);
      gtk_range_set_update_policy (GTK_RANGE (value_pair->scale),
                                   GTK_UPDATE_DELAYED);
    }
  else
    value_pair->scale = NULL;

  /* We destroy the value pair when the spinbutton is destroyed, so
   * we don't need to hold a refcount on the entry
   */

  value_pair->spin
    = gtk_spin_button_new (GTK_ADJUSTMENT (value_pair->adjustment),
                           1.0, 3);
  gtk_widget_set_size_request (value_pair->spin, 72, -1);
  g_signal_connect (value_pair->spin, "destroy",
                    G_CALLBACK (value_pair_destroy_callback),
                    value_pair);
  g_signal_connect (value_pair->adjustment, "value_changed",
                    G_CALLBACK (value_pair_scale_callback),
                    value_pair);

  return value_pair;
}

static void
value_pair_update (ValuePair *value_pair)
{
  if (value_pair->type == VALUE_PAIR_INT)
      gtk_adjustment_set_value (GTK_ADJUSTMENT (value_pair->adjustment),
                                *value_pair->data.i);
  else
      gtk_adjustment_set_value (GTK_ADJUSTMENT (value_pair->adjustment),
                                *value_pair->data.d);

}

static void
value_pair_scale_callback (GtkAdjustment *adjustment,
                           ValuePair     *value_pair)
{
  gint changed = FALSE;

  if (value_pair->type == VALUE_PAIR_DOUBLE)
    {
      if ((gdouble) *value_pair->data.d != adjustment->value)
        {
          changed = TRUE;
          *value_pair->data.d = adjustment->value;
        }
    }
  else
    {
      if (*value_pair->data.i != (gint) adjustment->value)
        {
          changed = TRUE;
          *value_pair->data.i = adjustment->value;
        }
    }

  if (changed)
      val_changed_update ();
}

static void
value_pair_destroy_callback (GtkWidget *widget,
                             ValuePair *value_pair)
{
  if (value_pair->scale)
    g_object_unref (value_pair->scale);
  g_object_unref (value_pair->adjustment);
}

static void
design_op_update_callback (GtkRadioAction *action,
                           GtkRadioAction *current,
                           gpointer        data)
{
  ifsDesign->op = gtk_radio_action_get_current_value (action);
}

static void
recompute_center_cb (GtkWidget *widget,
                     gpointer   data)
{
  recompute_center (TRUE);
}

static void
recompute_center (gboolean save_undo)
{
  gint    i;
  gdouble x, y;
  gdouble center_x = 0.0;
  gdouble center_y = 0.0;
  gdouble width    = ifsDesign->area->allocation.width;
  gdouble height   = ifsDesign->area->allocation.height;

  if (save_undo)
    undo_begin ();

  for (i = 0; i < ifsvals.num_elements; i++)
    {
      if (save_undo)
        undo_update (i);

      aff_element_compute_trans (elements[i],1, ifsvals.aspect_ratio,
                                ifsvals.center_x, ifsvals.center_y);
      aff2_fixed_point (&elements[i]->trans, &x, &y);
      center_x += x;
      center_y += y;
    }

  ifsvals.center_x = center_x/ifsvals.num_elements;
  ifsvals.center_y = center_y/ifsvals.num_elements;

  for (i = 0; i < ifsvals.num_elements; i++)
    {
        aff_element_decompose_trans (elements[i],&elements[i]->trans,
                                    1, ifsvals.aspect_ratio,
                                    ifsvals.center_x, ifsvals.center_y);
    }

  if (width > 1 && height > 1)
    {
      for (i = 0; i < ifsvals.num_elements; i++)
        aff_element_compute_trans (elements[i],width, height,
                                  ifsvals.center_x, ifsvals.center_y);
      design_area_redraw ();
      update_values ();
    }
}

static void
flip_check_button_callback (GtkWidget *widget,
                            gpointer   data)
{
  guint    i;
  gdouble  width  = ifsDesign->area->allocation.width;
  gdouble  height = ifsDesign->area->allocation.height;
  gboolean active;

  if (ifsD->in_update)
    return;

  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  undo_begin ();
  for (i = 0; i < ifsvals.num_elements; i++)
    {
      if (element_selected[i])
        {
          undo_update (i);
          elements[i]->v.flip = active;
          aff_element_compute_trans (elements[i], width, height,
                                     ifsvals.center_x, ifsvals.center_y);
        }
    }

  update_values ();
  design_area_redraw ();

  ifs_compose_preview ();
}

static void
ifs_compose_set_defaults (void)
{
  gint     i;
  GimpRGB  color;

  gimp_context_get_foreground (&color);

  ifsvals.aspect_ratio =
    (gdouble)ifsD->drawable_height / ifsD->drawable_width;

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_free (elements[i]);
  count_for_naming = 0;

  ifsvals.num_elements = 3;
  elements = g_realloc (elements, ifsvals.num_elements * sizeof(AffElement *));
  element_selected = g_realloc (element_selected,
                                ifsvals.num_elements * sizeof(gboolean));

  elements[0] = aff_element_new (0.3, 0.37 * ifsvals.aspect_ratio, &color,
                                 ++count_for_naming);
  element_selected[0] = FALSE;
  elements[1] = aff_element_new (0.7, 0.37 * ifsvals.aspect_ratio, &color,
                                 ++count_for_naming);
  element_selected[1] = FALSE;
  elements[2] = aff_element_new (0.5, 0.7 * ifsvals.aspect_ratio, &color,
                                 ++count_for_naming);
  element_selected[2] = FALSE;

  ifsvals.center_x   = 0.5;
  ifsvals.center_y   = 0.5 * ifsvals.aspect_ratio;
  ifsvals.iterations = ifsD->drawable_height * ifsD->drawable_width;
  ifsvals.subdivide  = 3;
  ifsvals.max_memory = 4096;

  if (ifsOptD)
    {
      value_pair_update (ifsOptD->iterations_pair);
      value_pair_update (ifsOptD->subdivide_pair);
      value_pair_update (ifsOptD->radius_pair);
      value_pair_update (ifsOptD->memory_pair);
    }

  ifsvals.radius = 0.7;

  set_current_element (0);
  element_selected[0] = TRUE;
  recompute_center (FALSE);

  if (ifsD->selected_orig)
    g_free (ifsD->selected_orig);

  ifsD->selected_orig = g_new (AffElement, ifsvals.num_elements);
}

/* show a transient message dialog */
static void
ifscompose_message_dialog (GtkMessageType  type,
                           GtkWindow      *parent,
                           const gchar    *title,
                           const gchar    *message)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (parent, 0, type, GTK_BUTTONS_OK, message);

  if (title)
    gtk_window_set_title (GTK_WINDOW (dialog), title);

  gtk_window_set_role (GTK_WINDOW (dialog), "ifscompose-message");
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/* save an ifs file */
static void
ifsfile_save_response (GtkWidget *dialog,
                       gint       response_id,
                       gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *filename;
      gchar *str;
      FILE  *fh;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      str = ifsvals_stringify (&ifsvals, elements);

      fh = fopen (filename, "w");
      if (! fh)
        {
          gchar *message =
            g_strdup_printf (_("Could not open '%s' for writing: %s"),
                             gimp_filename_to_utf8 (filename),
                             g_strerror (errno));

          ifscompose_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dialog),
                                     _("Save failed"), message);

          g_free (message);
          g_free (filename);

          return;
        }

      fputs (str, fh);
      fclose (fh);
    }

  gtk_widget_destroy (dialog);
}

/* replace ifsvals and elements with specified new values
 * recompute and update everything necessary */
static void
ifsfile_replace_ifsvals (IfsComposeVals  *new_ifsvals,
                         AffElement     **new_elements)
{
  gdouble width  = ifsDesign->area->allocation.width;
  gdouble height = ifsDesign->area->allocation.height;
  guint   i;

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_free (elements[i]);
  g_free (elements);

  ifsvals = *new_ifsvals;
  elements = new_elements;
  for (i = 0; i < ifsvals.num_elements; i++)
    {
      aff_element_compute_trans (elements[i], width, height,
                                 ifsvals.center_x, ifsvals.center_y);
      aff_element_compute_color_trans (elements[i]);
    }

  element_selected = g_realloc (element_selected,
                                ifsvals.num_elements * sizeof(gboolean));
  for (i = 0; i < ifsvals.num_elements; i++)
    element_selected[i] = FALSE;

  if (ifsOptD)
    {
      value_pair_update (ifsOptD->iterations_pair);
      value_pair_update (ifsOptD->subdivide_pair);
      value_pair_update (ifsOptD->radius_pair);
      value_pair_update (ifsOptD->memory_pair);
    }

  set_current_element (0);
  element_selected[0] = TRUE;
  recompute_center (FALSE);

  if (ifsD->selected_orig)
    g_free (ifsD->selected_orig);

  ifsD->selected_orig = g_new (AffElement, ifsvals.num_elements);
}

/* load an ifs file */
static void
ifsfile_load_response (GtkWidget *dialog,
                       gint       response_id,
                       gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar           *filename;
      gchar           *buffer;
      AffElement     **new_elements;
      IfsComposeVals   new_ifsvals;
      GError          *error = NULL;
      guint            i;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (! g_file_get_contents (filename, &buffer, NULL, &error))
        {
          ifscompose_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dialog),
                                     _("Open failed"), error->message);
          g_error_free (error);
          g_free (filename);
          return;
        }

      if (! ifsvals_parse_string (buffer, &new_ifsvals, &new_elements))
        {
          gchar *message = g_strdup_printf (_("File '%s' doesn't seem to be "
                                              "an IFS Fractal file."),
                                            gimp_filename_to_utf8 (filename));

          ifscompose_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dialog),
                                     _("Open failed"), message);
          g_free (filename);
          g_free (message);
          g_free (buffer);

          return;
        }

      g_free (buffer);
      g_free (filename);

      undo_begin ();
      for (i = 0; i < ifsvals.num_elements; i++)
        undo_update (i);

      ifsfile_replace_ifsvals (&new_ifsvals, new_elements);

      design_op_actions_update ();

      ifs_compose_preview ();

      design_area_redraw ();
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
ifs_compose_save (GtkWidget *parent)
{
  static GtkWidget *dialog = NULL;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Save as IFS Fraktal file"),
                                     GTK_WINDOW (parent),
                                     GTK_FILE_CHOOSER_ACTION_SAVE,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dialog);
      g_signal_connect (dialog, "response",
                        G_CALLBACK (ifsfile_save_response),
                        NULL);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
ifs_compose_load (GtkWidget *parent)
{
  static GtkWidget *dialog = NULL;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Open IFS Fraktal file"),
                                     GTK_WINDOW (parent),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OPEN,   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dialog);
      g_signal_connect (dialog, "response",
                        G_CALLBACK (ifsfile_load_response),
                        NULL);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
ifs_compose_new_callback (GtkAction *action,
                          gpointer   data)
{
  GimpRGB     color;
  gdouble     width  = ifsDesign->area->allocation.width;
  gdouble     height = ifsDesign->area->allocation.height;
  gint        i;
  AffElement *elem;

  undo_begin ();

  gimp_context_get_foreground (&color);

  elem = aff_element_new (0.5, 0.5 * height / width, &color,
                          ++count_for_naming);

  ifsvals.num_elements++;
  elements = g_realloc (elements, ifsvals.num_elements * sizeof (AffElement *));
  element_selected = g_realloc (element_selected,
                                ifsvals.num_elements * sizeof (gboolean));

  for (i = 0; i < ifsvals.num_elements-1; i++)
    element_selected[i] = FALSE;
  element_selected[ifsvals.num_elements-1] = TRUE;

  elements[ifsvals.num_elements-1] = elem;
  set_current_element (ifsvals.num_elements-1);

  ifsD->selected_orig = g_realloc (ifsD->selected_orig,
                                  ifsvals.num_elements * sizeof(AffElement));
  aff_element_compute_trans (elem, width, height,
                              ifsvals.center_x, ifsvals.center_y);

  design_area_redraw ();

  ifs_compose_preview ();

  design_op_actions_update ();
}

static void
ifs_compose_delete_callback (GtkAction *action,
                             gpointer   data)
{
  gint i;
  gint new_current;

  undo_begin ();
  undo_update (ifsD->current_element);

  aff_element_free (elements[ifsD->current_element]);

  if (ifsD->current_element < ifsvals.num_elements-1)
    {
      undo_update (ifsvals.num_elements-1);
      elements[ifsD->current_element] = elements[ifsvals.num_elements-1];
      new_current = ifsD->current_element;
    }
  else
    new_current = ifsvals.num_elements-2;

  ifsvals.num_elements--;

  for (i = 0; i < ifsvals.num_elements; i++)
    if (element_selected[i])
      {
        new_current = i;
        break;
      }

  element_selected[new_current] = TRUE;
  set_current_element (new_current);

  design_area_redraw ();

  ifs_compose_preview ();

  design_op_actions_update ();
}

static void
ifs_compose_options_callback (GtkAction *action,
                              gpointer   data)
{
  ifs_options_dialog (GTK_WIDGET (data));
}

static gint
preview_idle_render (gpointer data)
{
  gint width  = ifsD->preview_width;
  gint height = ifsD->preview_height;
  gint iterations = PREVIEW_RENDER_CHUNK;
  gint i;

  if (iterations > ifsD->preview_iterations)
    iterations = ifsD->preview_iterations;

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_compute_trans (elements[i], width, height,
                              ifsvals.center_x, ifsvals.center_y);

  ifs_render (elements, ifsvals.num_elements, width, height,
             iterations,&ifsvals, 0, height,
             ifsD->preview_data, NULL, NULL, TRUE);

  for (i = 0; i < ifsvals.num_elements; i++)
    aff_element_compute_trans (elements[i],
                              ifsDesign->area->allocation.width,
                              ifsDesign->area->allocation.height,
                              ifsvals.center_x, ifsvals.center_y);

  ifsD->preview_iterations -= iterations;

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (ifsD->preview),
                          0, 0, width, height,
                          GIMP_RGB_IMAGE,
                          ifsD->preview_data,
                          width * 3);

  return (ifsD->preview_iterations != 0);
}

static void
ifs_compose_preview (void)
{
  /* Expansion isn't really supported for previews */
  gint     i;
  gint     width  = ifsD->preview_width;
  gint     height = ifsD->preview_height;
  guchar   rc, gc, bc;
  guchar  *ptr;
  GimpRGB  color;

  if (!ifsD->preview_data)
    ifsD->preview_data = g_new (guchar, 3 * width * height);

  gimp_context_get_background (&color);
  gimp_rgb_get_uchar (&color, &rc, &gc, &bc);

  ptr = ifsD->preview_data;
  for (i = 0; i < width * height; i++)
    {
      *ptr++ = rc;
      *ptr++ = gc;
      *ptr++ = bc;
    }

  if (ifsD->preview_iterations == 0)
    g_idle_add (preview_idle_render, NULL);

  ifsD->preview_iterations =
    ifsvals.iterations * ((gdouble) width * height /
                          (ifsD->drawable_width * ifsD->drawable_height));
}

static void
ifs_compose_response (GtkWidget *widget,
                      gint       response_id,
                      gpointer   data)
{
  switch (response_id)
    {
    case RESPONSE_OPEN:
      ifs_compose_load (widget);
      break;

    case RESPONSE_SAVE:
      ifs_compose_save (widget);
      break;

    case RESPONSE_RESET:
      {
        gint    i;
        gdouble width  = ifsDesign->area->allocation.width;
        gdouble height = ifsDesign->area->allocation.height;

        undo_begin ();
        for (i = 0; i < ifsvals.num_elements; i++)
          undo_update (i);

        ifs_compose_set_defaults ();

        ifs_compose_preview ();

        for (i = 0; i < ifsvals.num_elements; i++)
          aff_element_compute_trans (elements[i], width, height,
                                     ifsvals.center_x, ifsvals.center_y);

        design_area_redraw ();
        design_op_actions_update ();
      }
      break;

    case GTK_RESPONSE_OK:
      ifscint.run = TRUE;

    default:
      gtk_widget_destroy (widget);
      break;
    }
}
