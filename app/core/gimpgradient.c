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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpimage.h"
#include "gimpgradient.h"

#include "gimp-intl.h"


#define EPSILON 1e-10


static void       gimp_gradient_class_init       (GimpGradientClass *klass);
static void       gimp_gradient_init             (GimpGradient      *gradient);

static void       gimp_gradient_finalize         (GObject           *object);

static gint64     gimp_gradient_get_memsize      (GimpObject        *object,
                                                  gint64            *gui_size);

static void       gimp_gradient_get_preview_size (GimpViewable      *viewable,
                                                  gint               size,
                                                  gboolean           popup,
                                                  gboolean           dot_for_dot,
                                                  gint              *width,
                                                  gint              *height);
static gboolean   gimp_gradient_get_popup_size   (GimpViewable      *viewable,
                                                  gint               width,
                                                  gint               height,
                                                  gboolean           dot_for_dot,
                                                  gint              *popup_width,
                                                  gint              *popup_height);
static TempBuf  * gimp_gradient_get_new_preview  (GimpViewable      *viewable,
                                                  gint               width,
                                                  gint               height);
static void       gimp_gradient_dirty            (GimpData          *data);
static gboolean   gimp_gradient_save             (GimpData          *data,
                                                  GError           **error);
static gchar    * gimp_gradient_get_extension    (GimpData          *data);
static GimpData * gimp_gradient_duplicate        (GimpData          *data,
                                                  gboolean           stingy_memory_use);

static gdouble    gimp_gradient_calc_linear_factor            (gdouble  middle,
							       gdouble  pos);
static gdouble    gimp_gradient_calc_curved_factor            (gdouble  middle,
							       gdouble  pos);
static gdouble    gimp_gradient_calc_sine_factor              (gdouble  middle,
							       gdouble  pos);
static gdouble    gimp_gradient_calc_sphere_increasing_factor (gdouble  middle,
							       gdouble  pos);
static gdouble    gimp_gradient_calc_sphere_decreasing_factor (gdouble  middle,
							       gdouble  pos);


static GimpDataClass *parent_class = NULL;


GType
gimp_gradient_get_type (void)
{
  static GType gradient_type = 0;

  if (! gradient_type)
    {
      static const GTypeInfo gradient_info =
      {
        sizeof (GimpGradientClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_gradient_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpGradient),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_gradient_init,
      };

      gradient_type = g_type_register_static (GIMP_TYPE_DATA,
                                              "GimpGradient",
                                              &gradient_info, 0);
  }

  return gradient_type;
}

static void
gimp_gradient_class_init (GimpGradientClass *klass)
{
  GObjectClass      *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;
  GimpDataClass     *data_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  data_class        = GIMP_DATA_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize           = gimp_gradient_finalize;

  gimp_object_class->get_memsize   = gimp_gradient_get_memsize;

  viewable_class->default_stock_id = "gimp-tool-blend";
  viewable_class->get_preview_size = gimp_gradient_get_preview_size;
  viewable_class->get_popup_size   = gimp_gradient_get_popup_size;
  viewable_class->get_new_preview  = gimp_gradient_get_new_preview;

  data_class->dirty                = gimp_gradient_dirty;
  data_class->save                 = gimp_gradient_save;
  data_class->get_extension        = gimp_gradient_get_extension;
  data_class->duplicate            = gimp_gradient_duplicate;
}

static void
gimp_gradient_init (GimpGradient *gradient)
{
  gradient->segments     = NULL;
  gradient->last_visited = NULL;
}

static void
gimp_gradient_finalize (GObject *object)
{
  GimpGradient *gradient = GIMP_GRADIENT (object);

  if (gradient->segments)
    {
      gimp_gradient_segments_free (gradient->segments);
      gradient->segments = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_gradient_get_memsize (GimpObject *object,
                           gint64     *gui_size)
{
  GimpGradient        *gradient = GIMP_GRADIENT (object);
  GimpGradientSegment *segment;
  gint64               memsize  = 0;

  for (segment = gradient->segments; segment; segment = segment->next)
    memsize += sizeof (GimpGradientSegment);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_gradient_get_preview_size (GimpViewable *viewable,
                                gint          size,
                                gboolean      popup,
                                gboolean      dot_for_dot,
                                gint         *width,
                                gint         *height)
{
  *width  = size;
  *height = size / 2;
}

static gboolean
gimp_gradient_get_popup_size (GimpViewable *viewable,
                              gint          width,
                              gint          height,
                              gboolean      dot_for_dot,
                              gint         *popup_width,
                              gint         *popup_height)
{
  if (width < 128 || height < 32)
    {
      *popup_width  = 128;
      *popup_height =  32;

      return TRUE;
    }

  return FALSE;
}

static TempBuf *
gimp_gradient_get_new_preview (GimpViewable *viewable,
			       gint          width,
			       gint          height)
{
  GimpGradient *gradient = GIMP_GRADIENT (viewable);
  TempBuf      *temp_buf;
  guchar       *buf;
  guchar       *p;
  guchar       *row;
  gint          x, y;
  gdouble       dx, cur_x;
  GimpRGB       color;

  dx    = 1.0 / (width - 1);
  cur_x = 0.0;
  p     = row = g_malloc (width * 4);

  /* Create lines to fill the image */

  for (x = 0; x < width; x++)
    {
      gimp_gradient_get_color_at (gradient, cur_x, FALSE, &color);

      *p++ = color.r * 255.0;
      *p++ = color.g * 255.0;
      *p++ = color.b * 255.0;
      *p++ = color.a * 255.0;

      cur_x += dx;
    }

  temp_buf = temp_buf_new (width, height, 4, 0, 0, NULL);

  buf = temp_buf_data (temp_buf);

  for (y = 0; y < height; y++)
    memcpy (buf + (width * y * 4), row, width * 4);

  g_free (row);

  return temp_buf;
}

static GimpData *
gimp_gradient_duplicate (GimpData *data,
                         gboolean  stingy_memory_use)
{
  GimpGradient        *gradient;
  GimpGradientSegment *head, *prev, *cur, *orig;

  gradient = g_object_new (GIMP_TYPE_GRADIENT, NULL);

  prev = NULL;
  orig = GIMP_GRADIENT (data)->segments;
  head = NULL;

  while (orig)
    {
      cur = gimp_gradient_segment_new ();

      *cur = *orig;  /* Copy everything */

      cur->prev = prev;
      cur->next = NULL;

      if (prev)
	prev->next = cur;
      else
	head = cur;  /* Remember head */

      prev = cur;
      orig = orig->next;
    }

  gradient->segments = head;

  return GIMP_DATA (gradient);
}

GimpData *
gimp_gradient_new (const gchar *name,
                   gboolean     stingy_memory_use)
{
  GimpGradient *gradient;

  g_return_val_if_fail (name != NULL, NULL);

  gradient = g_object_new (GIMP_TYPE_GRADIENT,
                           "name", name,
                           NULL);

  gradient->segments = gimp_gradient_segment_new ();

  return GIMP_DATA (gradient);
}

GimpData *
gimp_gradient_get_standard (void)
{
  static GimpData *standard_gradient = NULL;

  if (! standard_gradient)
    {
      standard_gradient = gimp_gradient_new ("Standard", FALSE);

      standard_gradient->dirty = FALSE;
      gimp_data_make_internal (standard_gradient);

      g_object_ref (standard_gradient);
    }

  return standard_gradient;
}

GimpData *
gimp_gradient_load (const gchar  *filename,
                    gboolean      stingy_memory_use,
                    GError      **error)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  GimpGradientSegment *prev;
  gint                 num_segments;
  gint                 i;
  gint                 type, color;
  FILE                *file;
  gchar                line[1024];

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  file = fopen (filename, "rb");
  if (!file)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  fgets (line, 1024, file);
  if (strcmp (line, "GIMP Gradient\n") != 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in gradient file '%s': "
                     "Not a GIMP gradient file."),
                   gimp_filename_to_utf8 (filename));
      fclose (file);
      return NULL;
    }

  gradient = g_object_new (GIMP_TYPE_GRADIENT, NULL);

  fgets (line, 1024, file);
  if (! strncmp (line, "Name: ", strlen ("Name: ")))
    {
      gchar *utf8;

      utf8 = gimp_any_to_utf8 (&line[strlen ("Name: ")], -1,
                               _("Invalid UTF-8 string in gradient file '%s'."),
                               gimp_filename_to_utf8 (filename));
      g_strstrip (utf8);

      gimp_object_set_name (GIMP_OBJECT (gradient), utf8);
      g_free (utf8);

      fgets (line, 1024, file);
    }
  else /* old gradient format */
    {
      gchar *basename;
      gchar *utf8;

      basename = g_path_get_basename (filename);

      utf8 = g_filename_to_utf8 (basename, -1, NULL, NULL, NULL);
      g_free (basename);

      gimp_object_set_name (GIMP_OBJECT (gradient), utf8);
      g_free (utf8);
    }

  num_segments = atoi (line);

  if (num_segments < 1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in gradient file '%s': "
                     "File is corrupt."),
                   gimp_filename_to_utf8 (filename));
      g_object_unref (gradient);
      fclose (file);
      return NULL;
    }

  prev = NULL;

  for (i = 0; i < num_segments; i++)
    {
      gchar *end;

      seg = gimp_gradient_segment_new ();

      seg->prev = prev;

      if (prev)
	prev->next = seg;
      else
	gradient->segments = seg;

      fgets (line, 1024, file);

      seg->left = g_ascii_strtod (line, &end);
      if (end && errno != ERANGE)
        seg->middle = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->right = g_ascii_strtod (end, &end);

      if (end && errno != ERANGE)
        seg->left_color.r = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->left_color.g = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->left_color.b = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->left_color.a = g_ascii_strtod (end, &end);

      if (end && errno != ERANGE)
        seg->right_color.r = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->right_color.g = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->right_color.b = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->right_color.a = g_ascii_strtod (end, &end);

      if (errno != ERANGE &&
          sscanf (end, "%d %d", &type, &color) == 2)
        {
	  seg->type  = (GimpGradientSegmentType) type;
	  seg->color = (GimpGradientSegmentColor) color;
        }
      else
        {
	  g_message (_("Corrupt segment %d in gradient file '%s'."),
		     i, gimp_filename_to_utf8 (filename));
	}

      prev = seg;
    }

  fclose (file);

  return GIMP_DATA (gradient);
}

static void
gimp_gradient_dirty (GimpData *data)
{
  GimpGradient *gradient = GIMP_GRADIENT (data);

  gradient->last_visited = NULL;

  if (GIMP_DATA_CLASS (parent_class)->dirty)
    GIMP_DATA_CLASS (parent_class)->dirty (data);
}

static gboolean
gimp_gradient_save (GimpData  *data,
                    GError   **error)
{
  GimpGradient        *gradient = GIMP_GRADIENT (data);
  GimpGradientSegment *seg;
  gint                 num_segments;
  FILE                *file;

  file = fopen (data->filename, "wb");

  if (! file)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (data->filename),
		   g_strerror (errno));
      return FALSE;
    }

  /* File format is:
   *
   *   GIMP Gradient
   *   Name: name
   *   number_of_segments
   *   left middle right r0 g0 b0 a0 r1 g1 b1 a1 type coloring
   *   left middle right r0 g0 b0 a0 r1 g1 b1 a1 type coloring
   *   ...
   */

  fprintf (file, "GIMP Gradient\n");

  fprintf (file, "Name: %s\n", GIMP_OBJECT (gradient)->name);

  /* Count number of segments */
  num_segments = 0;
  seg          = gradient->segments;

  while (seg)
    {
      num_segments++;
      seg = seg->next;
    }

  /* Write rest of file */
  fprintf (file, "%d\n", num_segments);

  for (seg = gradient->segments; seg; seg = seg->next)
    {
      gchar buf[11][G_ASCII_DTOSTR_BUF_SIZE];

      g_ascii_formatd (buf[0],  G_ASCII_DTOSTR_BUF_SIZE, "%f", seg->left);
      g_ascii_formatd (buf[1],  G_ASCII_DTOSTR_BUF_SIZE, "%f", seg->middle);
      g_ascii_formatd (buf[2],  G_ASCII_DTOSTR_BUF_SIZE, "%f", seg->right);
      g_ascii_formatd (buf[3],  G_ASCII_DTOSTR_BUF_SIZE, "%f", seg->left_color.r);
      g_ascii_formatd (buf[4],  G_ASCII_DTOSTR_BUF_SIZE, "%f", seg->left_color.g);
      g_ascii_formatd (buf[5],  G_ASCII_DTOSTR_BUF_SIZE, "%f", seg->left_color.b);
      g_ascii_formatd (buf[6],  G_ASCII_DTOSTR_BUF_SIZE, "%f", seg->left_color.a);
      g_ascii_formatd (buf[7],  G_ASCII_DTOSTR_BUF_SIZE, "%f", seg->right_color.r);
      g_ascii_formatd (buf[8],  G_ASCII_DTOSTR_BUF_SIZE, "%f", seg->right_color.g);
      g_ascii_formatd (buf[9],  G_ASCII_DTOSTR_BUF_SIZE, "%f", seg->right_color.b);
      g_ascii_formatd (buf[10], G_ASCII_DTOSTR_BUF_SIZE, "%f", seg->right_color.a);

      fprintf (file, "%s %s %s %s %s %s %s %s %s %s %s %d %d\n",
               buf[0], buf[1], buf[2], buf[3], buf[4],
               buf[5], buf[6], buf[7], buf[8], buf[9], buf[10],
	       (gint) seg->type,
	       (gint) seg->color);
    }

  fclose (file);

  return TRUE;
}

static gchar *
gimp_gradient_get_extension (GimpData *data)
{
  return GIMP_GRADIENT_FILE_EXTENSION;
}

gboolean
gimp_gradient_save_as_pov (GimpGradient  *gradient,
                           const gchar   *filename,
                           GError       **error)
{
  FILE                *file;
  GimpGradientSegment *seg;
  gchar                buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar                color_buf[4][G_ASCII_DTOSTR_BUF_SIZE];

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = fopen (filename, "wb");

  if (! file)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }
  else
    {
      fprintf (file, "/* color_map file created by the GIMP */\n");
      fprintf (file, "/* http://www.gimp.org/               */\n");

      fprintf (file, "color_map {\n");

      for (seg = gradient->segments; seg; seg = seg->next)
	{
	  /* Left */
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           seg->left);
          g_ascii_formatd (color_buf[0], G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           seg->left_color.r);
          g_ascii_formatd (color_buf[1], G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           seg->left_color.g);
          g_ascii_formatd (color_buf[2], G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           seg->left_color.b);
          g_ascii_formatd (color_buf[3], G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           1.0 - seg->left_color.a);

	  fprintf (file, "\t[%s color rgbt <%s, %s, %s, %s>]\n",
		   buf,
                   color_buf[0], color_buf[1], color_buf[2], color_buf[3]);

	  /* Middle */
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           seg->middle);
          g_ascii_formatd (color_buf[0], G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           (seg->left_color.r + seg->right_color.r) / 2.0);
          g_ascii_formatd (color_buf[1], G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           (seg->left_color.g + seg->right_color.g) / 2.0);
          g_ascii_formatd (color_buf[2], G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           (seg->left_color.b + seg->right_color.b) / 2.0);
          g_ascii_formatd (color_buf[3], G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           1.0 - (seg->left_color.a + seg->right_color.a) / 2.0);

	  fprintf (file, "\t[%s color rgbt <%s, %s, %s, %s>]\n",
		   buf,
		   color_buf[0], color_buf[1], color_buf[2], color_buf[3]);

	  /* Right */
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           seg->right);
          g_ascii_formatd (color_buf[0], G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           seg->right_color.r);
          g_ascii_formatd (color_buf[1], G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           seg->right_color.g);
          g_ascii_formatd (color_buf[2], G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           seg->right_color.b);
          g_ascii_formatd (color_buf[3], G_ASCII_DTOSTR_BUF_SIZE, "%f",
                           1.0 - seg->right_color.a);

	  fprintf (file, "\t[%s color rgbt <%s, %s, %s, %s>]\n",
		   buf,
		   color_buf[0], color_buf[1], color_buf[2], color_buf[3]);
	}

      fprintf (file, "} /* color_map */\n");
      fclose (file);
    }

  return TRUE;
}

void
gimp_gradient_get_color_at (GimpGradient *gradient,
			    gdouble       pos,
                            gboolean      reverse,
			    GimpRGB      *color)
{
  gdouble              factor = 0.0;
  GimpGradientSegment *seg;
  gdouble              seg_len;
  gdouble              middle;
  GimpRGB              rgb;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (color != NULL);

  pos = CLAMP (pos, 0.0, 1.0);

  if (reverse)
    pos = 1.0 - pos;

  seg = gimp_gradient_get_segment_at (gradient, pos);

  seg_len = seg->right - seg->left;

  if (seg_len < EPSILON)
    {
      middle = 0.5;
      pos    = 0.5;
    }
  else
    {
      middle = (seg->middle - seg->left) / seg_len;
      pos    = (pos - seg->left) / seg_len;
    }

  switch (seg->type)
    {
    case GIMP_GRAD_LINEAR:
      factor = gimp_gradient_calc_linear_factor (middle, pos);
      break;

    case GIMP_GRAD_CURVED:
      factor = gimp_gradient_calc_curved_factor (middle, pos);
      break;

    case GIMP_GRAD_SINE:
      factor = gimp_gradient_calc_sine_factor (middle, pos);
      break;

    case GIMP_GRAD_SPHERE_INCREASING:
      factor = gimp_gradient_calc_sphere_increasing_factor (middle, pos);
      break;

    case GIMP_GRAD_SPHERE_DECREASING:
      factor = gimp_gradient_calc_sphere_decreasing_factor (middle, pos);
      break;

    default:
      g_warning ("%s: Unknown gradient type %d.",
		 G_STRFUNC, seg->type);
      break;
    }

  /* Calculate color components */

  if (seg->color == GIMP_GRAD_RGB)
    {
      rgb.r =
        seg->left_color.r + (seg->right_color.r - seg->left_color.r) * factor;

      rgb.g =
	seg->left_color.g + (seg->right_color.g - seg->left_color.g) * factor;

      rgb.b =
	seg->left_color.b + (seg->right_color.b - seg->left_color.b) * factor;
    }
  else
    {
      GimpHSV left_hsv;
      GimpHSV right_hsv;

      gimp_rgb_to_hsv (&seg->left_color,  &left_hsv);
      gimp_rgb_to_hsv (&seg->right_color, &right_hsv);

      left_hsv.s = left_hsv.s + (right_hsv.s - left_hsv.s) * factor;
      left_hsv.v = left_hsv.v + (right_hsv.v - left_hsv.v) * factor;

      switch (seg->color)
	{
	case GIMP_GRAD_HSV_CCW:
	  if (left_hsv.h < right_hsv.h)
	    {
	      left_hsv.h += (right_hsv.h - left_hsv.h) * factor;
	    }
	  else
	    {
	      left_hsv.h += (1.0 - (left_hsv.h - right_hsv.h)) * factor;

	      if (left_hsv.h > 1.0)
		left_hsv.h -= 1.0;
	    }
	  break;

	case GIMP_GRAD_HSV_CW:
	  if (right_hsv.h < left_hsv.h)
	    {
	      left_hsv.h -= (left_hsv.h - right_hsv.h) * factor;
	    }
	  else
	    {
	      left_hsv.h -= (1.0 - (right_hsv.h - left_hsv.h)) * factor;

	      if (left_hsv.h < 0.0)
		left_hsv.h += 1.0;
	    }
	  break;

	default:
	  g_warning ("%s: Unknown coloring mode %d",
		     G_STRFUNC, (gint) seg->color);
	  break;
	}

      gimp_hsv_to_rgb (&left_hsv, &rgb);
    }

  /* Calculate alpha */

  rgb.a =
    seg->left_color.a + (seg->right_color.a - seg->left_color.a) * factor;

  *color = rgb;
}

GimpGradientSegment *
gimp_gradient_get_segment_at (GimpGradient *gradient,
			      gdouble       pos)
{
  GimpGradientSegment *seg;

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL);

  /* handle FP imprecision at the edges of the gradient */
  pos = CLAMP (pos, 0.0, 1.0);

  if (gradient->last_visited)
    seg = gradient->last_visited;
  else
    seg = gradient->segments;

  while (seg)
    {
      if (pos >= seg->left)
	{
	  if (pos <= seg->right)
	    {
	      gradient->last_visited = seg; /* for speed */
	      return seg;
	    }
	  else
	    {
	      seg = seg->next;
	    }
	}
      else
	{
	  seg = seg->prev;
	}
    }

  /* Oops: we should have found a segment, but we didn't */
  g_warning ("%s: no matching segment for position %0.15f",
	     G_STRFUNC, pos);

  return NULL;
}

static gdouble
gimp_gradient_calc_linear_factor (gdouble middle,
				  gdouble pos)
{
  if (pos <= middle)
    {
      if (middle < EPSILON)
	return 0.0;
      else
	return 0.5 * pos / middle;
    }
  else
    {
      pos -= middle;
      middle = 1.0 - middle;

      if (middle < EPSILON)
	return 1.0;
      else
	return 0.5 + 0.5 * pos / middle;
    }
}

static gdouble
gimp_gradient_calc_curved_factor (gdouble middle,
				  gdouble pos)
{
  if (middle < EPSILON)
    middle = EPSILON;

  return pow(pos, log (0.5) / log (middle));
}

static gdouble
gimp_gradient_calc_sine_factor (gdouble middle,
				gdouble pos)
{
  pos = gimp_gradient_calc_linear_factor (middle, pos);

  return (sin ((-G_PI / 2.0) + G_PI * pos) + 1.0) / 2.0;
}

static gdouble
gimp_gradient_calc_sphere_increasing_factor (gdouble middle,
					     gdouble pos)
{
  pos = gimp_gradient_calc_linear_factor (middle, pos) - 1.0;

  return sqrt (1.0 - pos * pos); /* Works for convex increasing and concave decreasing */
}

static gdouble
gimp_gradient_calc_sphere_decreasing_factor (gdouble middle,
					     gdouble pos)
{
  pos = gimp_gradient_calc_linear_factor (middle, pos);

  return 1.0 - sqrt(1.0 - pos * pos); /* Works for convex decreasing and concave increasing */
}


/*  gradient segment functions  */

GimpGradientSegment *
gimp_gradient_segment_new (void)
{
  GimpGradientSegment *seg;

  seg = g_new (GimpGradientSegment, 1);

  seg->left   = 0.0;
  seg->middle = 0.5;
  seg->right  = 1.0;

  gimp_rgba_set (&seg->left_color,  0.0, 0.0, 0.0, 1.0);
  gimp_rgba_set (&seg->right_color, 1.0, 1.0, 1.0, 1.0);

  seg->type  = GIMP_GRAD_LINEAR;
  seg->color = GIMP_GRAD_RGB;

  seg->prev = seg->next = NULL;

  return seg;
}


void
gimp_gradient_segment_free (GimpGradientSegment *seg)
{
  g_return_if_fail (seg != NULL);

  g_free (seg);
}

void
gimp_gradient_segments_free (GimpGradientSegment *seg)
{
  GimpGradientSegment *tmp;

  g_return_if_fail (seg != NULL);

  while (seg)
    {
      tmp = seg->next;
      gimp_gradient_segment_free (seg);
      seg = tmp;
    }
}

GimpGradientSegment *
gimp_gradient_segment_get_last (GimpGradientSegment *seg)
{
  if (!seg)
    return NULL;

  while (seg->next)
    seg = seg->next;

  return seg;
}

GimpGradientSegment *
gimp_gradient_segment_get_first (GimpGradientSegment *seg)
{
  if (!seg)
    return NULL;

  while (seg->prev)
    seg = seg->prev;

  return seg;
}

void
gimp_gradient_segment_split_midpoint (GimpGradient         *gradient,
                                      GimpGradientSegment  *lseg,
                                      GimpGradientSegment **newl,
                                      GimpGradientSegment **newr)
{
  GimpRGB              color;
  GimpGradientSegment *newseg;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (lseg != NULL);
  g_return_if_fail (newl != NULL);
  g_return_if_fail (newr != NULL);

  /* Get color at original segment's midpoint */
  gimp_gradient_get_color_at (gradient, lseg->middle, FALSE, &color);

  /* Create a new segment and insert it in the list */

  newseg = gimp_gradient_segment_new ();

  newseg->prev = lseg;
  newseg->next = lseg->next;

  lseg->next = newseg;

  if (newseg->next)
    newseg->next->prev = newseg;

  /* Set coordinates of new segment */

  newseg->left   = lseg->middle;
  newseg->right  = lseg->right;
  newseg->middle = (newseg->left + newseg->right) / 2.0;

  /* Set coordinates of original segment */

  lseg->right  = newseg->left;
  lseg->middle = (lseg->left + lseg->right) / 2.0;

  /* Set colors of both segments */

  newseg->right_color = lseg->right_color;

  lseg->right_color.r = newseg->left_color.r = color.r;
  lseg->right_color.g = newseg->left_color.g = color.g;
  lseg->right_color.b = newseg->left_color.b = color.b;
  lseg->right_color.a = newseg->left_color.a = color.a;

  /* Set parameters of new segment */

  newseg->type  = lseg->type;
  newseg->color = lseg->color;

  /* Done */

  *newl = lseg;
  *newr = newseg;
}

void
gimp_gradient_segment_split_uniform (GimpGradient         *gradient,
                                     GimpGradientSegment  *lseg,
                                     gint                  parts,
                                     GimpGradientSegment **newl,
                                     GimpGradientSegment **newr)
{
  GimpGradientSegment *seg, *prev, *tmp;
  gdouble              seg_len;
  gint                 i;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (lseg != NULL);
  g_return_if_fail (newl != NULL);
  g_return_if_fail (newr != NULL);

  seg_len = (lseg->right - lseg->left) / parts; /* Length of divisions */

  seg  = NULL;
  prev = NULL;
  tmp  = NULL;

  for (i = 0; i < parts; i++)
    {
      seg = gimp_gradient_segment_new ();

      if (i == 0)
	tmp = seg; /* Remember first segment */

      seg->left   = lseg->left + i * seg_len;
      seg->right  = lseg->left + (i + 1) * seg_len;
      seg->middle = (seg->left + seg->right) / 2.0;

      gimp_gradient_get_color_at (gradient, seg->left,  FALSE, &seg->left_color);
      gimp_gradient_get_color_at (gradient, seg->right, FALSE, &seg->right_color);

      seg->type  = lseg->type;
      seg->color = lseg->color;

      seg->prev = prev;
      seg->next = NULL;

      if (prev)
	prev->next = seg;

      prev = seg;
    }

  /* Fix edges */

  tmp->left_color = lseg->left_color;

  seg->right_color = lseg->right_color;

  tmp->left  = lseg->left;
  seg->right = lseg->right; /* To squish accumulative error */

  /* Link in list */

  tmp->prev = lseg->prev;
  seg->next = lseg->next;

  if (lseg->prev)
    lseg->prev->next = tmp;
  else
    gradient->segments = tmp; /* We are on leftmost segment */

  if (lseg->next)
    lseg->next->prev = seg;

  gradient->last_visited = NULL; /* Force re-search */

  /* Done */

  *newl = tmp;
  *newr = seg;

  /* Delete old segment */

  gimp_gradient_segment_free (lseg);
}

void
gimp_gradient_segments_compress_range (GimpGradientSegment *range_l,
                                       GimpGradientSegment *range_r,
                                       gdouble              new_l,
                                       gdouble              new_r)
{
  gdouble              orig_l, orig_r;
  gdouble              scale;
  GimpGradientSegment *seg, *aseg;

  g_return_if_fail (range_l != NULL);
  g_return_if_fail (range_r != NULL);

  orig_l = range_l->left;
  orig_r = range_r->right;

  scale = (new_r - new_l) / (orig_r - orig_l);

  seg = range_l;

  do
    {
      seg->left   = new_l + (seg->left - orig_l) * scale;
      seg->middle = new_l + (seg->middle - orig_l) * scale;
      seg->right  = new_l + (seg->right - orig_l) * scale;

      /* Next */

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != range_r);
}

void
gimp_gradient_segments_blend_endpoints (GimpGradientSegment *lseg,
                                        GimpGradientSegment *rseg,
                                        GimpRGB             *rgb1,
                                        GimpRGB             *rgb2,
                                        gboolean             blend_colors,
                                        gboolean             blend_opacity)
{
  GimpRGB              d;
  gdouble              left, len;
  GimpGradientSegment *seg;
  GimpGradientSegment *aseg;

  g_return_if_fail (lseg != NULL);
  g_return_if_fail (rseg != NULL);

  d.r = rgb2->r - rgb1->r;
  d.g = rgb2->g - rgb1->g;
  d.b = rgb2->b - rgb1->b;
  d.a = rgb2->a - rgb1->a;

  left  = lseg->left;
  len   = rseg->right - left;

  seg = lseg;

  do
    {
      if (blend_colors)
	{
	  seg->left_color.r  = rgb1->r + (seg->left - left) / len * d.r;
	  seg->left_color.g  = rgb1->g + (seg->left - left) / len * d.g;
	  seg->left_color.b  = rgb1->b + (seg->left - left) / len * d.b;

	  seg->right_color.r = rgb1->r + (seg->right - left) / len * d.r;
	  seg->right_color.g = rgb1->g + (seg->right - left) / len * d.g;
	  seg->right_color.b = rgb1->b + (seg->right - left) / len * d.b;
	}

      if (blend_opacity)
	{
	  seg->left_color.a  = rgb1->a + (seg->left - left) / len * d.a;
	  seg->right_color.a = rgb1->a + (seg->right - left) / len * d.a;
	}

      aseg = seg;
      seg = seg->next;
    }
  while (aseg != rseg);
}
