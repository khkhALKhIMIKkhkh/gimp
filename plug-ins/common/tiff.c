/* tiff loading and saving for the GIMP
 *  -Peter Mattis
 * The TIFF loading code has been completely revamped by Nick Lamb
 * njl195@zepler.org.uk -- 18 May 1998
 * And it now gains support for tiles (and doubtless a zillion bugs)
 * njl195@zepler.org.uk -- 12 June 1999
 * LZW patent fuss continues :(
 * njl195@zepler.org.uk -- 20 April 2000
 * The code for this filter is based on "tifftopnm" and "pnmtotiff",
 *  2 programs that are a part of the netpbm package.
 * khk@khk.net -- 13 May 2000
 * Added support for ICCPROFILE tiff tag. If this tag is present in a
 * TIFF file, then a parasite is created and vice versa.
 * peter@kirchgessner.net -- 29 Oct 2002
 * Progress bar only when run interactive
 */

/*
** tifftopnm.c - converts a Tagged Image File to a portable anymap
**
** Derived by Jef Poskanzer from tif2ras.c, which is:
**
** Copyright (c) 1990 by Sun Microsystems, Inc.
**
** Author: Patrick J. Naughton
** naughton@wind.sun.com
**
** This file is provided AS IS with no warranties of any kind.  The author
** shall have no liability with respect to the infringement of copyrights,
** trade secrets or any patents by this file or any part thereof.  In no
** event will the author be liable for any lost revenue or profits or
** other special, indirect and consequential damages.
*/

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <tiffio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


typedef struct
{
  gint      compression;
  gint      fillorder;
} TiffSaveVals;

typedef struct
{
  gboolean  run;
} TiffSaveInterface;

typedef struct
{
  gint32        ID;
  GimpDrawable *drawable;
  GimpPixelRgn  pixel_rgn;
  guchar       *pixels;
  guchar       *pixel;
} channel_data;

/* Declare some local functions.
 */
static void   query   (void);
static void   run     (const gchar      *name,
		       gint              nparams,
		       const GimpParam  *param,
		       gint             *nreturn_vals,
		       GimpParam       **return_vals);

static gint32 load_image    (const gchar  *filename);

static void   load_rgba     (TIFF         *tif,
			     channel_data *channel);
static void   load_lines    (TIFF         *tif,
			     channel_data *channel,
			     gushort       bps,
			     gushort       photomet,
			     gint          alpha,
			     gint          extra);
static void   load_tiles    (TIFF         *tif,
			     channel_data *channel,
			     gushort       bps,
			     gushort       photomet,
			     gint          alpha,
			     gint          extra);

static void   read_separate (guchar       *source,
			     channel_data *channel,
                             gushort       bps,
			     gushort       photomet,
                             gint          startcol,
			     gint          startrow,
			     gint          rows,
			     gint          cols,
                             gint          alpha,
			     gint          extra,
			     gint          sample);
static void   read_16bit    (guchar       *source,
			     channel_data *channel,
			     gushort       photomet,
			     gint          startcol,
			     gint          startrow,
			     gint          rows,
			     gint          cols,
			     gint          alpha,
			     gint          extra,
			     gint          align);
static void   read_8bit     (guchar       *source,
			     channel_data *channel,
			     gushort       photomet,
			     gint          startcol,
			     gint          startrow,
			     gint          rows,
			     gint          cols,
			     gint          alpha,
			     gint          extra,
			     gint          align);
static void   read_default  (guchar       *source,
			     channel_data *channel,
			     gushort       bps,
			     gushort       photomet,
			     gint          startcol,
			     gint          startrow,
			     gint          rows,
			     gint          cols,
			     gint          alpha,
			     gint          extra,
			     gint          align);

static gboolean  save_image             (const gchar *filename,
                                         gint32       image,
                                         gint32       drawable,
                                         gint32       orig_image);

static gboolean  save_dialog            (void);

static void      save_ok_callback       (GtkWidget   *widget,
                                         gpointer     data);
static void      comment_entry_callback (GtkWidget   *widget,
                                         gpointer     data);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static TiffSaveVals tsvals =
{
  COMPRESSION_NONE,    /*  compression  */
};

static TiffSaveInterface tsint =
{
  FALSE               /*  run  */
};

static gchar       *image_comment = NULL;
static GimpRunMode  run_mode      = GIMP_RUN_INTERACTIVE;

MAIN ()

static void
query (void)
{
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename", "The name of the file to load" },
    { GIMP_PDB_STRING, "raw_filename", "The name of the file to load" }
  };
  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  static GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Drawable to save" },
    { GIMP_PDB_STRING, "filename", "The name of the file to save the image in" },
    { GIMP_PDB_STRING, "raw_filename", "The name of the file to save the image in" },
    { GIMP_PDB_INT32, "compression", "Compression type: { NONE (0), LZW (1), PACKBITS (2), DEFLATE (3), JPEG (4)" }
  };

  gimp_install_procedure ("file_tiff_load",
                          "loads files of the tiff file format",
                          "FIXME: write help for tiff_load",
                          "Spencer Kimball, Peter Mattis & Nick Lamb",
                          "Nick Lamb <njl195@zepler.org.uk>",
                          "1995-1996,1998-2000",
                          "<Load>/Tiff",
			  NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_install_procedure ("file_tiff_save",
                          "saves files in the tiff file format",
                          "Saves files in the Tagged Image File Format.  "
			  "The value for the saved comment is taken "
			  "from the 'gimp-comment' parasite.",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1996,2000",
                          "<Save>/Tiff",
			  "RGB*, GRAY*, INDEXED",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_tiff_load",
				    "tif,tiff",
				    "",
				    "0,string,II*\\0,0,string,MM\\0*");
  gimp_register_save_handler       ("file_tiff_save",
				    "tif,tiff",
				    "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam      values[2];
  GimpPDBStatusType     status = GIMP_PDB_SUCCESS;
  GimpParasite         *parasite;
  gint32                image;
  gint32                drawable;
  gint32                orig_image;
  GimpExportReturnType  export = GIMP_EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, "file_tiff_load") == 0)
    {
      image = load_image (param[1].data.d_string);

      if (image != -1)
	{
	  *nreturn_vals = 2;
	  values[1].type         = GIMP_PDB_IMAGE;
	  values[1].data.d_image = image;
	}
      else
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, "file_tiff_save") == 0)
    {
      image = orig_image = param[1].data.d_int32;
      drawable = param[2].data.d_int32;

      /* Do this right this time, if POSSIBLE query for parasites, otherwise
	 or if there isn't one, choose the default comment from the gimprc. */

      /*  eventually export the image */
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_ui_init ("tiff", FALSE);
	  export = gimp_export_image (&image, &drawable, "TIFF",
				      (GIMP_EXPORT_CAN_HANDLE_RGB |
				       GIMP_EXPORT_CAN_HANDLE_GRAY |
				       GIMP_EXPORT_CAN_HANDLE_INDEXED |
				       GIMP_EXPORT_CAN_HANDLE_ALPHA ));
	  if (export == GIMP_EXPORT_CANCEL)
	    {
	      values[0].data.d_status = GIMP_PDB_CANCEL;
	      return;
	    }
	  break;
	default:
	  break;
	}

      parasite = gimp_image_parasite_find (orig_image, "gimp-comment");
      if (parasite)
        image_comment = g_strdup (parasite->data);
      gimp_parasite_free (parasite);

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_tiff_save", &tsvals);

	  parasite = gimp_image_parasite_find (orig_image, "tiff-save-options");
	  if (parasite)
	    {
	      tsvals.compression =
		((TiffSaveVals *) parasite->data)->compression;
	    }
	  gimp_parasite_free (parasite);

	  /*  First acquire information with a dialog  */
	  if (! save_dialog ())
	    status = GIMP_PDB_CANCEL;
	  break;

	case GIMP_RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 6)
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	    }
	  else
	    {
	      switch (param[5].data.d_int32)
		{
		case 0: tsvals.compression = COMPRESSION_NONE;     break;
		case 1: tsvals.compression = COMPRESSION_LZW;      break;
		case 2: tsvals.compression = COMPRESSION_PACKBITS; break;
		case 3: tsvals.compression = COMPRESSION_DEFLATE;  break;
		case 4: tsvals.compression = COMPRESSION_JPEG;  break;
		default: status = GIMP_PDB_CALLING_ERROR; break;
		}
	    }
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_tiff_save", &tsvals);

	  parasite = gimp_image_parasite_find (orig_image, "tiff-save-options");
	  if (parasite)
	    {
	      tsvals.compression =
		((TiffSaveVals *) parasite->data)->compression;
	    }
	  gimp_parasite_free (parasite);
	  break;

	default:
	  break;
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  if (save_image (param[3].data.d_string, image, drawable, orig_image))
	    {
	      /*  Store mvals data  */
	      gimp_set_data ("file_tiff_save", &tsvals, sizeof (TiffSaveVals));
	    }
	  else
	    {
	      status = GIMP_PDB_EXECUTION_ERROR;
	    }
	}

      if (export == GIMP_EXPORT_EXPORT)
	gimp_image_delete (image);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static void
tiff_warning(const gchar *module,
             const gchar *fmt,
             va_list      ap)
{
  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, fmt, ap);
}

static void
tiff_error (const gchar *module,
            const gchar *fmt,
            va_list      ap)
{
  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, fmt, ap);
}

static gint32
load_image (const gchar *filename)
{
  TIFF    *tif;
  gushort  bps, spp, photomet;
  guint16 orientation;
  gint     cols, rows, alpha;
  gint     image, image_type = GIMP_RGB;
  gint     layer, layer_type = GIMP_RGB_IMAGE;
  gushort  extra, *extra_types;
  channel_data *channel= NULL;

  gushort *redmap, *greenmap, *bluemap;
  GimpRGB  color;
  guchar   cmap[768];

  gint     i, j;
  gboolean worst_case = FALSE;
  gchar   *name;

  TiffSaveVals save_vals;
  GimpParasite *parasite;
  guint16  tmp;
#ifdef TIFFTAG_ICCPROFILE
  uint32   profile_size;
  guchar  *icc_profile;
#endif

  gboolean flip_horizontal = FALSE;
  gboolean flip_vertical = FALSE;

  gimp_rgb_set (&color, 0.0, 0.0, 0.0);

  TIFFSetWarningHandler (tiff_warning);
  TIFFSetErrorHandler (tiff_error);

  tif = TIFFOpen (filename, "r");
  if (!tif)
    {
      g_message (_("Can't open '%s':\n%s"), filename, g_strerror (errno));
      gimp_quit ();
    }

  name = g_strdup_printf (_("Opening '%s'..."), filename);
  gimp_progress_init (name);
  g_free (name);

  TIFFGetFieldDefaulted (tif, TIFFTAG_BITSPERSAMPLE, &bps);

  if (bps > 8 && bps != 16)
    worst_case = TRUE; /* Wrong sample width => RGBA */

  TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &spp);

  if (!TIFFGetField (tif, TIFFTAG_EXTRASAMPLES, &extra, &extra_types))
    extra = 0;

  if (!TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &cols))
    {
      g_message ("Can't get image width");
      gimp_quit ();
    }

  if (!TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &rows))
    {
      g_message ("Can't get image length");
      gimp_quit ();
    }

  if (!TIFFGetField (tif, TIFFTAG_PHOTOMETRIC, &photomet))
    {
      g_message ("Can't get photometric\nAssuming min-is-black");
      /* old AppleScan software misses out the photometric tag (and
       * incidentally assumes min-is-white, but xv assumes min-is-black,
       * so we follow xv's lead.  It's not much hardship to invert the
       * image later). */
      photomet = PHOTOMETRIC_MINISBLACK;
    }

  /* test if the extrasample represents an associated alpha channel... */
  if (extra > 0 && (extra_types[0] == EXTRASAMPLE_ASSOCALPHA))
    {
      alpha = 1;
      --extra;
    }
  else
    {
      alpha = 0;
    }

  if (photomet == PHOTOMETRIC_RGB && spp > 3 + extra)
    {
      alpha= 1;
      extra= spp - 4;
    }
  else if (photomet != PHOTOMETRIC_RGB && spp > 1 + extra)
    {
      alpha= 1;
      extra= spp - 2;
    }

  switch (photomet)
    {
    case PHOTOMETRIC_MINISBLACK:
    case PHOTOMETRIC_MINISWHITE:
      image_type = GIMP_GRAY;
      layer_type = (alpha) ? GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE;
      break;

    case PHOTOMETRIC_RGB:
      image_type = GIMP_RGB;
      layer_type = (alpha) ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE;
      break;

    case PHOTOMETRIC_PALETTE:
      image_type = GIMP_INDEXED;
      layer_type = (alpha) ? GIMP_INDEXEDA_IMAGE : GIMP_INDEXED_IMAGE;
      break;

    default:
      worst_case = TRUE;
      break;
    }

  if (worst_case)
    {
      image_type = GIMP_RGB;
      layer_type = GIMP_RGBA_IMAGE;
    }

  if ((image = gimp_image_new (cols, rows, image_type)) == -1)
    {
      g_message ("Can't create a new image");
      gimp_quit ();
    }

  gimp_image_set_filename (image, filename);

  /* attach a parasite containing an ICC profile - if found in the TIFF file */

#ifdef TIFFTAG_ICCPROFILE
  /* If TIFFTAG_ICCPROFILE is defined we are dealing with a libtiff version
   * that can handle ICC profiles. Otherwise just ignore this section. */
  if (TIFFGetField (tif, TIFFTAG_ICCPROFILE, &profile_size, &icc_profile))
    {
      parasite = gimp_parasite_new ("icc-profile", 0,
                                    profile_size, icc_profile);
      gimp_image_parasite_attach (image, parasite);
      gimp_parasite_free (parasite);
    }
#endif

  /* attach a parasite containing the compression */
  if (!TIFFGetField (tif, TIFFTAG_COMPRESSION, &tmp))
    save_vals.compression = COMPRESSION_NONE;
  else
    save_vals.compression = tmp;

  parasite = gimp_parasite_new ("tiff-save-options", 0,
				sizeof (save_vals), &save_vals);
  gimp_image_parasite_attach (image, parasite);
  gimp_parasite_free (parasite);

  /* Attach a parasite containing the image description.  Pretend to
   * be a gimp comment so other plugins will use this description as
   * an image comment where appropriate. */
  {
    const gchar *img_desc;

    if (TIFFGetField (tif, TIFFTAG_IMAGEDESCRIPTION, &img_desc) &&
        g_utf8_validate (img_desc, -1, NULL))
      {
        parasite = gimp_parasite_new ("gimp-comment",
                                      GIMP_PARASITE_PERSISTENT,
                                      strlen (img_desc) + 1, img_desc);
        gimp_image_parasite_attach (image, parasite);
        gimp_parasite_free (parasite);
      }
  }

  /* any resolution info in the file? */
  {
    gfloat   xres = 72.0, yres = 72.0;
    gushort  read_unit;
    GimpUnit unit = GIMP_UNIT_PIXEL; /* invalid unit */

    if (TIFFGetField (tif, TIFFTAG_XRESOLUTION, &xres))
      {
        if (TIFFGetField (tif, TIFFTAG_YRESOLUTION, &yres))
          {

            if (TIFFGetFieldDefaulted (tif, TIFFTAG_RESOLUTIONUNIT, &read_unit))
              {
                switch (read_unit)
                  {
                  case RESUNIT_NONE:
                    /* ImageMagick writes files with this silly resunit */
                    g_message ("Warning: resolution units meaningless");
                    break;

                  case RESUNIT_INCH:
                    unit = GIMP_UNIT_INCH;
                    break;

                  case RESUNIT_CENTIMETER:
                    xres *= 2.54;
                    yres *= 2.54;
                    unit = GIMP_UNIT_MM; /* this is our default metric unit */
                    break;

                  default:
                    g_message ("File error: unknown resolution unit type %d, "
                               "assuming dpi", read_unit);
                    break;
                  }
              }
            else
              { /* no res unit tag */
                /* old AppleScan software produces these */
                g_message ("Warning: resolution specified without "
                           "any units tag, assuming dpi");
              }
          }
        else
          { /* xres but no yres */
            g_message ("Warning: no y resolution info, assuming same as x");
            yres = xres;
          }

        /* now set the new image's resolution info */

        /* If it is invalid, instead of forcing 72dpi, do not set the resolution
           at all. Gimp will then use the default set by the user */
        if (read_unit != RESUNIT_NONE)
          {
            gimp_image_set_resolution (image, xres, yres);
            if (unit != GIMP_UNIT_PIXEL)
              gimp_image_set_unit (image, unit);
          }
      }

    /* no x res tag => we assume we have no resolution info, so we
     * don't care.  Older versions of this plugin used to write files
     * with no resolution tags at all. */

    /* TODO: haven't caught the case where yres tag is present, but
       not xres.  This is left as an exercise for the reader - they
       should feel free to shoot the author of the broken program
       that produced the damaged TIFF file in the first place. */
  }

  /* Install colormap for INDEXED images only */
  if (image_type == GIMP_INDEXED)
    {
      if (!TIFFGetField (tif, TIFFTAG_COLORMAP, &redmap, &greenmap, &bluemap))
	{
	  g_message ("Can't get colormaps");
	  gimp_quit ();
	}

      for (i = 0, j = 0; i < (1 << bps); i++)
	{
	  cmap[j++] = redmap[i] >> 8;
	  cmap[j++] = greenmap[i] >> 8;
	  cmap[j++] = bluemap[i] >> 8;
	}
      gimp_image_set_cmap (image, cmap, (1 << bps));
    }

  /* Allocate channel_data for all channels, even the background layer */
  channel = g_new (channel_data, extra + 1);
  layer = gimp_layer_new (image, _("Background"),
			  cols, rows, layer_type, 100, GIMP_NORMAL_MODE);
  channel[0].ID= layer;
  gimp_image_add_layer (image, layer, 0);
  channel[0].drawable= gimp_drawable_get(layer);

  if (extra > 0 && !worst_case)
    {
      /* Add alpha channels as appropriate */
      for (i = 1; i <= extra; ++i)
        {
          channel[i].ID= gimp_channel_new (image, _("TIFF Channel"), cols, rows,
                                           100.0, &color);
          gimp_image_add_channel(image, channel[i].ID, 0);
          channel[i].drawable= gimp_drawable_get (channel[i].ID);
        }
    }

  if (bps == 16)
    g_message (_("Warning:\n"
		 "The image you are loading has 16 bits per channel. GIMP "
		 "can only handle 8 bit, so it will be converted for you. "
		 "Information will be lost because of this conversion."));

  if (worst_case)
    {
      load_rgba (tif, channel);
    }
  else if (TIFFIsTiled(tif))
    {
      load_tiles (tif, channel, bps, photomet, alpha, extra);
    }
  else
    { /* Load scanlines in tile_height chunks */
      load_lines (tif, channel, bps, photomet, alpha, extra);
    }

  if (TIFFGetField (tif, TIFFTAG_ORIENTATION, &orientation))
    {
      switch (orientation)
        {
        case ORIENTATION_TOPLEFT:
          flip_horizontal = FALSE;
          flip_vertical   = FALSE;
          break;
        case ORIENTATION_TOPRIGHT:
          flip_horizontal = TRUE;
          flip_vertical   = FALSE;
          break;
        case ORIENTATION_BOTRIGHT:
          flip_horizontal = TRUE;
          flip_vertical   = TRUE;
          break;
        case ORIENTATION_BOTLEFT:
          flip_horizontal = FALSE;
          flip_vertical   = TRUE;
          break;
        default:
          flip_horizontal = FALSE;
          flip_vertical   = FALSE;
          g_warning ("Orientation %d not handled yet!", orientation);
          break;
        }

      if (flip_horizontal || flip_vertical)
        gimp_image_undo_disable (image);

      if (flip_horizontal)
        gimp_flip (layer, GIMP_ORIENTATION_HORIZONTAL);

      if (flip_vertical)
        gimp_flip (layer, GIMP_ORIENTATION_VERTICAL);

      if (flip_horizontal || flip_vertical)
        gimp_image_undo_enable (image);
    }

  for (i = 0; !worst_case && i < extra; ++i)
    {
      gimp_drawable_flush (channel[i].drawable);
      gimp_drawable_detach (channel[i].drawable);
    }

  return image;
}

static void
load_rgba (TIFF         *tif,
           channel_data *channel)
{
  uint32  imageWidth, imageLength;
  uint32  row;
  uint32 *buffer;

  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &imageLength);

  gimp_pixel_rgn_init (&(channel[0].pixel_rgn), channel[0].drawable,
                       0, 0, imageWidth, imageLength, TRUE, FALSE);

  buffer = g_new (uint32, imageWidth * imageLength);
  channel[0].pixels = (guchar*) buffer;

  if (!TIFFReadRGBAImage (tif, imageWidth, imageLength, buffer, 0))
    g_message ("Unsupported layout, no RGBA loader");

  for (row = 0; row < imageLength; ++row)
    {
      gimp_pixel_rgn_set_rect (&(channel[0].pixel_rgn),
                               channel[0].pixels + row * imageWidth * 4,
                               0, imageLength -row -1, imageWidth, 1);

      gimp_progress_update ((gdouble) row / (gdouble) imageLength);
    }
}

static void
load_tiles (TIFF           *tif,
            channel_data   *channel,
            unsigned short  bps,
            unsigned short  photomet,
            int             alpha,
            int             extra)
{
  uint16  planar= PLANARCONFIG_CONTIG;
  uint32  imageWidth, imageLength;
  uint32  tileWidth, tileLength;
  uint32  x, y, rows, cols;
  guchar *buffer;
  gdouble progress= 0.0, one_row;
  gint    i;

  TIFFGetField (tif, TIFFTAG_PLANARCONFIG, &planar);
  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &imageLength);
  TIFFGetField (tif, TIFFTAG_TILEWIDTH, &tileWidth);
  TIFFGetField (tif, TIFFTAG_TILELENGTH, &tileLength);

  one_row = (gdouble) tileLength / (gdouble) imageLength;
  buffer = g_malloc (TIFFTileSize (tif));

  for (i = 0; i <= extra; ++i)
    {
      channel[i].pixels = g_new (guchar,
				 tileWidth * tileLength *
				 channel[i].drawable->bpp);
    }

  for (y = 0; y < imageLength; y += tileLength)
    {
      for (x = 0; x < imageWidth; x += tileWidth)
        {
          gimp_progress_update (progress + one_row *
                                ( (gdouble) x / (gdouble) imageWidth));

          TIFFReadTile (tif, buffer, x, y, 0, 0);

          cols = MIN (imageWidth - x, tileWidth);
          rows = MIN (imageLength - y, tileLength);

          if (bps == 16)
            {
              read_16bit (buffer, channel, photomet, y, x, rows, cols, alpha,
                          extra, tileWidth - cols);
            }
          else if (bps == 8)
            {
              read_8bit (buffer, channel, photomet, y, x, rows, cols, alpha,
                         extra, tileWidth - cols);
            }
          else
            {
              read_default (buffer, channel, bps, photomet, y, x, rows, cols,
                            alpha, extra, tileWidth - cols);
            }
        }
      progress+= one_row;
    }

  for (i = 0; i <= extra; ++i)
    g_free(channel[i].pixels);

  g_free(buffer);
}

static void
load_lines (TIFF           *tif,
            channel_data   *channel,
	    unsigned short  bps,
            unsigned short  photomet,
	    int             alpha,
            int             extra)
{
  uint16  planar= PLANARCONFIG_CONTIG;
  uint32  imageLength, lineSize, cols, rows;
  guchar *buffer;
  gint    i, y;
  gint    tile_height = gimp_tile_height ();

  TIFFGetField (tif, TIFFTAG_PLANARCONFIG, &planar);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &imageLength);
  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &cols);

  lineSize= TIFFScanlineSize (tif);

  for (i = 0; i <= extra; ++i)
    {
      channel[i].pixels= g_new (guchar,
                                tile_height * cols * channel[i].drawable->bpp);
    }

  buffer = g_malloc (lineSize * tile_height);

  if (planar == PLANARCONFIG_CONTIG)
    {
      for (y = 0; y < imageLength; y+= tile_height )
        {
          gimp_progress_update ((gdouble) y / (gdouble) imageLength);

          rows = MIN (tile_height, imageLength - y);

          for (i = 0; i < rows; ++i)
            TIFFReadScanline (tif, buffer + i * lineSize, y + i, 0);

          if (bps == 16)
            {
              read_16bit (buffer, channel, photomet, y, 0, rows, cols,
                          alpha, extra, 0);
            }
          else if (bps == 8)
            {
              read_8bit (buffer, channel, photomet, y, 0, rows, cols,
                         alpha, extra, 0);
            }
          else
            {
              read_default (buffer, channel, bps, photomet, y, 0, rows, cols,
                            alpha, extra, 0);
            }
        }
    }
  else
    { /* PLANARCONFIG_SEPARATE  -- Just say "No" */
      uint16 s, samples;

      TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samples);

      for (s = 0; s < samples; ++s)
        {
          for (y = 0; y < imageLength; y+= tile_height )
            {
              gimp_progress_update ((gdouble) y / (gdouble) imageLength);

              rows = MIN (tile_height, imageLength - y);
              for (i = 0; i < rows; ++i)
                TIFFReadScanline(tif, buffer + i * lineSize, y + i, s);

              read_separate (buffer, channel, bps, photomet,
                             y, 0, rows, cols, alpha, extra, s);
            }
        }
    }

  for (i = 0; i <= extra; ++i)
    g_free(channel[i].pixels);

  g_free(buffer);
}

static void
read_16bit (guchar       *source,
	    channel_data *channel,
	    gushort       photomet,
	    gint          startrow,
	    gint          startcol,
	    gint          rows,
	    gint          cols,
	    gint          alpha,
            gint          extra,
            gint          align)
{
  guchar *dest;
  gint    gray_val, red_val, green_val, blue_val, alpha_val;
  gint    col, row, i;

  for (i = 0; i <= extra; ++i)
    {
      gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                           startcol, startrow, cols, rows, TRUE, FALSE);
    }

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  source++; /* offset source once, to look at the high byte */
#endif

  for (row = 0; row < rows; ++row)
    {
      dest= channel[0].pixels + row * cols * channel[0].drawable->bpp;

      for (i = 1; i <= extra; ++i)
        channel[i].pixel= channel[i].pixels + row * cols;

      for (col = 0; col < cols; col++)
        {
          switch (photomet)
            {
            case PHOTOMETRIC_MINISBLACK:
              if (alpha)
                {
                  gray_val  = *source; source += 2;
                  alpha_val = *source; source += 2;
                  gray_val  = MIN (gray_val, alpha_val);

                  if (alpha_val)
                    *dest++ = gray_val * 255 / alpha_val;
                  else
                    *dest++ = 0;

                  *dest++ = alpha_val;
                }
              else
                {
                  *dest++ = *source; source += 2;
                }
              break;

            case PHOTOMETRIC_MINISWHITE:
              if (alpha)
                {
                  gray_val  = *source; source += 2;
                  alpha_val = *source; source += 2;
                  gray_val  = MIN (gray_val, alpha_val);

                  if (alpha_val)
                    *dest++ = ((alpha_val - gray_val) * 255) / alpha_val;
                  else
                    *dest++ = 0;

                  *dest++ = alpha_val;
                }
              else
                {
                  *dest++ = ~(*source); source += 2;
                }
              break;

            case PHOTOMETRIC_PALETTE:
              *dest++= *source; source += 2;
              if (alpha) *dest++= *source; source += 2;
              break;

            case PHOTOMETRIC_RGB:
              if (alpha)
                {
                  red_val   = *source; source += 2;
                  green_val = *source; source += 2;
                  blue_val  = *source; source += 2;
                  alpha_val = *source; source += 2;
                  red_val   = MIN (red_val,   alpha_val);
                  green_val = MIN (green_val, alpha_val);
                  blue_val  = MIN (blue_val,  alpha_val);
                  if (alpha_val)
                    {
                      *dest++ = (red_val   * 255) / alpha_val;
                      *dest++ = (green_val * 255) / alpha_val;
                      *dest++ = (blue_val  * 255) / alpha_val;
                    }
                  else
                    {
                      *dest++ = 0;
                      *dest++ = 0;
                      *dest++ = 0;
                    }
                  *dest++ = alpha_val;
                }
              else
                {
                  *dest++ = *source; source += 2;
                  *dest++ = *source; source += 2;
                  *dest++ = *source; source += 2;
                }
              break;

            default:
              /* This case was handled earlier */
              g_assert_not_reached();
            }

          for (i = 1; i <= extra; ++i)
	    {
	      *channel[i].pixel++ = *source; source += 2;
	    }
	}

      if (align)
        {
          switch (photomet)
            {
            case PHOTOMETRIC_MINISBLACK:
            case PHOTOMETRIC_MINISWHITE:
            case PHOTOMETRIC_PALETTE:
              source += align * (1 + alpha + extra) * 2;
              break;
            case PHOTOMETRIC_RGB:
              source += align * (3 + alpha + extra) * 2;
              break;
            }
        }
    }

  for (i = 0; i <= extra; ++i)
    gimp_pixel_rgn_set_rect (&(channel[i].pixel_rgn), channel[i].pixels,
			     startcol, startrow, cols, rows);
}

static void
read_8bit (guchar       *source,
	   channel_data *channel,
	   gushort       photomet,
	   gint          startrow,
	   gint          startcol,
	   gint          rows,
	   gint          cols,
	   gint          alpha,
	   gint          extra,
	   gint          align)
{
  guchar *dest;
  gint    gray_val, red_val, green_val, blue_val, alpha_val;
  gint    col, row, i;

  for (i = 0; i <= extra; ++i)
    {
      gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                           startcol, startrow, cols, rows, TRUE, FALSE);
    }

  for (row = 0; row < rows; ++row)
    {
      dest= channel[0].pixels + row * cols * channel[0].drawable->bpp;

      for (i = 1; i <= extra; ++i)
        channel[i].pixel = channel[i].pixels + row * cols;

      for (col = 0; col < cols; col++)
        {
          switch (photomet)
            {
            case PHOTOMETRIC_MINISBLACK:
              if (alpha)
                {
                  gray_val= *source++;
                  alpha_val= *source++;
                  gray_val= MIN(gray_val, alpha_val);
                  if (alpha_val)
                    *dest++ = gray_val * 255 / alpha_val;
                  else
                    *dest++ = 0;
                  *dest++ = alpha_val;
                }
              else
                {
                  *dest++ = *source++;
                }
              break;

            case PHOTOMETRIC_MINISWHITE:
              if (alpha)
                {
                  gray_val  = *source++;
                  alpha_val = *source++;
                  gray_val  = MIN (gray_val, alpha_val);

                  if (alpha_val)
                    *dest++ = ((alpha_val - gray_val) * 255) / alpha_val;
                  else
                    *dest++ = 0;
                  *dest++ = alpha_val;
                }
              else
                {
                  *dest++ = ~(*source++);
                }
              break;

            case PHOTOMETRIC_PALETTE:
              *dest++= *source++;
              if (alpha) *dest++= *source++;
              break;

            case PHOTOMETRIC_RGB:
              if (alpha)
                {
                  red_val   = *source++;
                  green_val = *source++;
                  blue_val  = *source++;
                  alpha_val = *source++;
                  red_val   = MIN (red_val, alpha_val);
                  blue_val  = MIN (blue_val, alpha_val);
                  green_val = MIN (green_val, alpha_val);

                  if (alpha_val)
                    {
                      *dest++ = (red_val   * 255) / alpha_val;
                      *dest++ = (green_val * 255) / alpha_val;
                      *dest++ = (blue_val  * 255) / alpha_val;
                    }
                  else
                    {
                      *dest++ = 0;
                      *dest++ = 0;
                      *dest++ = 0;
                    }
                  *dest++ = alpha_val;
                }
              else
                {
                  *dest++ = *source++;
                  *dest++ = *source++;
                  *dest++ = *source++;
                }
              break;

            default:
              /* This case was handled earlier */
              g_assert_not_reached();
            }

          for (i = 1; i <= extra; ++i)
            *channel[i].pixel++ = *source++;
        }

    if (align)
      {
        switch (photomet)
          {
          case PHOTOMETRIC_MINISBLACK:
          case PHOTOMETRIC_MINISWHITE:
          case PHOTOMETRIC_PALETTE:
            source += align * (1 + alpha + extra);
            break;
          case PHOTOMETRIC_RGB:
            source += align * (3 + alpha + extra);
            break;
          }
      }
    }

  for (i = 0; i <= extra; ++i)
    gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                            startcol, startrow, cols, rows);
}

/* Step through all <= 8-bit samples in an image */

#define NEXTSAMPLE(var)                       \
  {                                           \
      if (bitsleft == 0)                      \
      {                                       \
	  source++;                           \
	  bitsleft = 8;                       \
      }                                       \
      bitsleft -= bps;                        \
      var = ( *source >> bitsleft ) & maxval; \
  }

static void
read_default (guchar       *source,
	      channel_data *channel,
	      gushort       bps,
	      gushort       photomet,
	      gint          startrow,
	      gint          startcol,
	      gint          rows,
	      gint          cols,
	      gint          alpha,
	      gint          extra,
              gint          align)
{
  guchar *dest;
  gint    gray_val, red_val, green_val, blue_val, alpha_val;
  gint    col, row, i;
  gint    bitsleft = 8, maxval = (1 << bps) - 1;

  for (i = 0; i <= extra; ++i)
    {
      gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                           startcol, startrow, cols, rows, TRUE, FALSE);
    }

  for (row = 0; row < rows; ++row)
    {
      dest= channel[0].pixels + row * cols * channel[0].drawable->bpp;

      for (i = 1; i <= extra; ++i)
        channel[i].pixel= channel[i].pixels + row * cols;

      for (col = 0; col < cols; col++)
        {
          switch (photomet)
            {
            case PHOTOMETRIC_MINISBLACK:
              NEXTSAMPLE (gray_val);
              if (alpha)
                {
                  NEXTSAMPLE (alpha_val);
                  gray_val= MIN (gray_val, alpha_val);

                  if (alpha_val)
                    *dest++ = (gray_val * 65025) / (alpha_val * maxval);
                  else
                    *dest++ = 0;

                  *dest++ = alpha_val;
                }
              else
                {
                  *dest++ = (gray_val * 255) / maxval;
                }
              break;

            case PHOTOMETRIC_MINISWHITE:
              NEXTSAMPLE (gray_val);
              if (alpha)
                {
                  NEXTSAMPLE (alpha_val);
                  gray_val= MIN (gray_val, alpha_val);

                  if (alpha_val)
                    *dest++ = ((maxval - gray_val) * 65025) / (alpha_val * maxval);
                  else
                    *dest++ = 0;

                  *dest++ = alpha_val;
                }
              else
                {
                  *dest++ = ((maxval - gray_val) * 255) / maxval;
                }
              break;

            case PHOTOMETRIC_PALETTE:
              NEXTSAMPLE (*dest++);
              if (alpha)
                NEXTSAMPLE (*dest++);
              break;

            case PHOTOMETRIC_RGB:
              NEXTSAMPLE (red_val);
              NEXTSAMPLE (green_val);
              NEXTSAMPLE (blue_val);
              if (alpha)
                {
                  NEXTSAMPLE (alpha_val);
                  red_val   = MIN (red_val, alpha_val);
                  blue_val  = MIN (blue_val, alpha_val);
                  green_val = MIN (green_val, alpha_val);

                  if (alpha_val)
                    {
                      *dest++ = (red_val   * 255) / alpha_val;
                      *dest++ = (green_val * 255) / alpha_val;
                      *dest++ = (blue_val  * 255) / alpha_val;
                    }
                  else
                    {
                      *dest++ = 0;
                      *dest++ = 0;
                      *dest++ = 0;
                    }

                  *dest++ = alpha_val;
                }
              else
                {
                  *dest++ = red_val;
                  *dest++ = green_val;
                  *dest++ = blue_val;
                }
              break;

            default:
              /* This case was handled earlier */
              g_assert_not_reached();
            }

          for (i = 1; i <= extra; ++i)
            {
              NEXTSAMPLE(alpha_val);
              *channel[i].pixel++ = alpha_val;
            }
        }

      if (align)
        {
          switch (photomet)
            {
            case PHOTOMETRIC_MINISBLACK:
            case PHOTOMETRIC_MINISWHITE:
            case PHOTOMETRIC_PALETTE:
              for (i = 0; i < align * (1 + alpha + extra); ++i)
                NEXTSAMPLE (alpha_val);
              break;
            case PHOTOMETRIC_RGB:
              for (i = 0; i < align * (3 + alpha + extra); ++i)
                NEXTSAMPLE (alpha_val);
              break;
            }
        }

      bitsleft= 0;
    }

  for (i = 0; i <= extra; ++i)
    gimp_pixel_rgn_set_rect (&(channel[i].pixel_rgn), channel[i].pixels,
                             startcol, startrow, cols, rows);
}

static void
read_separate (guchar       *source,
	       channel_data *channel,
               gushort       bps,
	       gushort       photomet,
               gint          startrow,
	       gint          startcol,
	       gint          rows,
	       gint          cols,
               gint          alpha,
	       gint          extra,
	       gint          sample)
{
  guchar *dest;
  gint    col, row, c;
  gint    bitsleft = 8, maxval = (1 << bps) - 1;

  if (bps > 8)
    {
      g_message ("Unsupported layout");
      gimp_quit ();
    }

  if (sample < channel[0].drawable->bpp)
    {
      c = 0;
    }
  else
    {
      c = (sample - channel[0].drawable->bpp) + 4;
      photomet = PHOTOMETRIC_MINISBLACK;
    }

  gimp_pixel_rgn_init (&(channel[c].pixel_rgn), channel[c].drawable,
                         startcol, startrow, cols, rows, TRUE, FALSE);

  gimp_pixel_rgn_get_rect (&(channel[c].pixel_rgn), channel[c].pixels,
                           startcol, startrow, cols, rows);

  for (row = 0; row < rows; ++row)
    {
      dest = channel[c].pixels + row * cols * channel[c].drawable->bpp;

      if (c == 0)
        {
          for (col = 0; col < cols; ++col)
            NEXTSAMPLE(dest[col * channel[0].drawable->bpp + sample]);
        }
      else
        {
          for (col = 0; col < cols; ++col)
            NEXTSAMPLE(dest[col]);
        }
    }

  gimp_pixel_rgn_set_rect (&(channel[c].pixel_rgn), channel[c].pixels,
                           startcol, startrow, cols, rows);
}

/*
** pnmtotiff.c - converts a portable anymap to a Tagged Image File
**
** Derived by Jef Poskanzer from ras2tif.c, which is:
**
** Copyright (c) 1990 by Sun Microsystems, Inc.
**
** Author: Patrick J. Naughton
** naughton@wind.sun.com
**
** This file is provided AS IS with no warranties of any kind.  The author
** shall have no liability with respect to the infringement of copyrights,
** trade secrets or any patents by this file or any part thereof.  In no
** event will the author be liable for any lost revenue or profits or
** other special, indirect and consequential damages.
*/

static gboolean
save_image (const gchar *filename,
	    gint32       image,
	    gint32       layer,
	    gint32       orig_image)  /* the export function might have created a duplicate */
{
  TIFF          *tif;
  gushort        red[256];
  gushort        grn[256];
  gushort        blu[256];
  gint           cols, col, rows, row, i;
  glong          rowsperstrip;
  gushort        compression;
  gushort        extra_samples[1];
  gint           alpha;
  gshort         predictor;
  gshort         photometric;
  gshort         samplesperpixel;
  gshort         bitspersample;
  gint           bytesperrow;
  guchar        *t, *src, *data;
  guchar        *cmap;
  gint           colors;
  gint           success;
  GimpDrawable     *drawable;
  GimpImageType  drawable_type;
  GimpPixelRgn      pixel_rgn;
  gint           tile_height;
  gint           y, yend;
  gchar         *name;

  compression = tsvals.compression;

  /* Disabled because this isn't in older releases of libtiff, and it
     wasn't helping much anyway */
#if 0
  if (TIFFFindCODEC((uint16) compression) == NULL)
    compression = COMPRESSION_NONE; /* CODEC not available */
#endif

  predictor = 0;
  tile_height = gimp_tile_height ();
  rowsperstrip = tile_height;

  TIFFSetWarningHandler (tiff_warning);
  TIFFSetErrorHandler (tiff_error);

  tif = TIFFOpen (filename, "w");
  if (!tif)
    {
      g_message (_("Can't open '%s' for writing:\n%s"),
                 filename, g_strerror (errno));
      return FALSE;
    }

  name = g_strdup_printf (_("Saving '%s'..."), filename);
  gimp_progress_init (name);
  g_free (name);

  drawable = gimp_drawable_get (layer);
  drawable_type = gimp_drawable_type (layer);
  gimp_pixel_rgn_init (&pixel_rgn, drawable,
		       0, 0, drawable->width, drawable->height, FALSE, FALSE);

  cols = drawable->width;
  rows = drawable->height;

  switch (drawable_type)
    {
    case GIMP_RGB_IMAGE:
      predictor = 2;
      samplesperpixel = 3;
      bitspersample = 8;
      photometric = PHOTOMETRIC_RGB;
      bytesperrow = cols * 3;
      alpha = 0;
      break;
    case GIMP_GRAY_IMAGE:
      samplesperpixel = 1;
      bitspersample = 8;
      photometric = PHOTOMETRIC_MINISBLACK;
      bytesperrow = cols;
      alpha = 0;
      break;
    case GIMP_RGBA_IMAGE:
      predictor = 2;
      samplesperpixel = 4;
      bitspersample = 8;
      photometric = PHOTOMETRIC_RGB;
      bytesperrow = cols * 4;
      alpha = 1;
      break;
    case GIMP_GRAYA_IMAGE:
      samplesperpixel = 2;
      bitspersample = 8;
      photometric = PHOTOMETRIC_MINISBLACK;
      bytesperrow = cols * 2;
      alpha = 1;
      break;
    case GIMP_INDEXED_IMAGE:
      samplesperpixel = 1;
      bitspersample = 8;
      photometric = PHOTOMETRIC_PALETTE;
      bytesperrow = cols;
      alpha = 0;

      cmap = gimp_image_get_cmap (image, &colors);

      for (i = 0; i < colors; i++)
	{
	  red[i] = *cmap++ * 65535 / 255;
	  grn[i] = *cmap++ * 65535 / 255;
	  blu[i] = *cmap++ * 65535 / 255;
	}
      break;
    case GIMP_INDEXEDA_IMAGE:
    default:
       return FALSE;
    }

  /* Set TIFF parameters. */
  TIFFSetField (tif, TIFFTAG_SUBFILETYPE, 0);
  TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, cols);
  TIFFSetField (tif, TIFFTAG_IMAGELENGTH, rows);
  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, bitspersample);
  TIFFSetField (tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField (tif, TIFFTAG_COMPRESSION, compression);
  if ((compression == COMPRESSION_LZW || compression == COMPRESSION_DEFLATE)
     && (predictor != 0)) {
    TIFFSetField (tif, TIFFTAG_PREDICTOR, predictor);
  }
  if (alpha) {
      extra_samples [0] = EXTRASAMPLE_ASSOCALPHA;
      TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, 1, extra_samples);
  }
  TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, photometric);
  TIFFSetField (tif, TIFFTAG_DOCUMENTNAME, filename);
  TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
  /* TIFFSetField( tif, TIFFTAG_STRIPBYTECOUNTS, rows / rowsperstrip ); */
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  /* resolution fields */
  {
    gdouble  xresolution;
    gdouble  yresolution;
    gushort  save_unit = RESUNIT_INCH;
    GimpUnit unit;
    gfloat   factor;

    gimp_image_get_resolution (orig_image, &xresolution, &yresolution);
    unit = gimp_image_get_unit (orig_image);
    factor = gimp_unit_get_factor (unit);

    /*  if we have a metric unit, save the resolution as centimeters
     */
    if ((ABS (factor - 0.0254) < 1e-5) ||  /* m  */
	(ABS (factor - 0.254) < 1e-5) ||   /* dm */
	(ABS (factor - 2.54) < 1e-5) ||    /* cm */
	(ABS (factor - 25.4) < 1e-5))      /* mm */
      {
	save_unit = RESUNIT_CENTIMETER;
	xresolution /= 2.54;
	yresolution /= 2.54;
      }

    if (xresolution > 1e-5 && yresolution > 1e-5)
      {
	TIFFSetField (tif, TIFFTAG_XRESOLUTION, xresolution);
	TIFFSetField (tif, TIFFTAG_YRESOLUTION, yresolution);
	TIFFSetField (tif, TIFFTAG_RESOLUTIONUNIT, save_unit);
      }
  }

  /* The TIFF spec explicitely says ASCII for the image description. */
  if (image_comment)
    {
      const gchar *c = image_comment;
      gint         len;

      for (len = strlen (c); len; c++, len--)
        {
          if ((guchar) *c > 127)
            {
              g_message (_("The TIFF format only supports comments in\n"
                           "7bit ASCII encoding. No comment is saved."));

              g_free (image_comment);
              image_comment = NULL;

              break;
            }
        }
    }

  /* do we have a comment?  If so, create a new parasite to hold it,
   * and attach it to the image. The attach function automatically
   * detaches a previous incarnation of the parasite. */
  if (image_comment && *image_comment)
    {
      GimpParasite *parasite;

      TIFFSetField (tif, TIFFTAG_IMAGEDESCRIPTION, image_comment);
      parasite = gimp_parasite_new ("gimp-comment",
				    GIMP_PARASITE_PERSISTENT,
				    strlen (image_comment) + 1, image_comment);
      gimp_image_parasite_attach (orig_image, parasite);
      gimp_parasite_free (parasite);
    }

  /* do we have an ICC profile? If so, write it to the TIFF file */
#ifdef TIFFTAG_ICCPROFILE
  {
    GimpParasite *parasite;
    uint32        profile_size;
    const guchar *icc_profile;

    parasite = gimp_image_parasite_find (orig_image, "icc-profile");
    if (parasite)
      {
        profile_size = gimp_parasite_data_size (parasite);
	icc_profile = gimp_parasite_data (parasite);

	TIFFSetField (tif, TIFFTAG_ICCPROFILE, profile_size, icc_profile);
        gimp_parasite_free (parasite);
      }
  }
#endif

  if (drawable_type == GIMP_INDEXED_IMAGE)
    TIFFSetField (tif, TIFFTAG_COLORMAP, red, grn, blu);

  /* array to rearrange data */
  src = g_new (guchar, bytesperrow * tile_height);
  data = g_new (guchar, bytesperrow);

  /* Now write the TIFF data. */
  for (y = 0; y < rows; y = yend)
    {
      yend = y + tile_height;
      yend = MIN (yend, rows);

      gimp_pixel_rgn_get_rect (&pixel_rgn, src, 0, y, cols, yend - y);

      for (row = y; row < yend; row++)
	{
	  t = src + bytesperrow * (row - y);

	  switch (drawable_type)
	    {
	    case GIMP_INDEXED_IMAGE:
	      success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
	      break;
	    case GIMP_GRAY_IMAGE:
	      success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
	      break;
	    case GIMP_GRAYA_IMAGE:
	      for (col = 0; col < cols*samplesperpixel; col+=samplesperpixel)
		{
		  /* pre-multiply gray by alpha */
		  data[col + 0] = (t[col + 0] * t[col + 1]) / 255;
		  data[col + 1] = t[col + 1];  /* alpha channel */
		}
	      success = (TIFFWriteScanline (tif, data, row, 0) >= 0);
	      break;
	    case GIMP_RGB_IMAGE:
	      success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
	      break;
	    case GIMP_RGBA_IMAGE:
	      for (col = 0; col < cols*samplesperpixel; col+=samplesperpixel)
		{
		  /* pre-multiply rgb by alpha */
		  data[col+0] = t[col + 0] * t[col + 3] / 255;
		  data[col+1] = t[col + 1] * t[col + 3] / 255;
		  data[col+2] = t[col + 2] * t[col + 3] / 255;
		  data[col+3] = t[col + 3];  /* alpha channel */
		}
	      success = (TIFFWriteScanline (tif, data, row, 0) >= 0);
	      break;
	    default:
	      success = FALSE;
	      break;
	    }

	  if (!success)
            {
              g_message ("Failed a scanline write on row %d", row);
              return FALSE;
            }
	}

      gimp_progress_update ((gdouble) row / (gdouble) rows);
    }

  TIFFFlushData (tif);
  TIFFClose (tif);

  gimp_drawable_detach (drawable);
  g_free (data);

  return TRUE;
}

static gboolean
save_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;

  dlg = gimp_dialog_new ( _("Save as TIFF"), "tiff",
			 gimp_standard_help_func, "filters/tiff.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 GTK_STOCK_CANCEL, gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,
			 GTK_STOCK_OK, save_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,

			 NULL);

  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, FALSE, TRUE, 0);

  /*  compression  */
  frame = gimp_radio_group_new2 (TRUE, _("Compression"),
                                 G_CALLBACK (gimp_radio_button_update),
                                 &tsvals.compression,
                                 GINT_TO_POINTER (tsvals.compression),

                                 _("_None"),
                                 GINT_TO_POINTER (COMPRESSION_NONE), NULL,

                                 _("_LZW"),
                                 GINT_TO_POINTER (COMPRESSION_LZW), NULL,

                                 _("_Pack Bits"),
                                 GINT_TO_POINTER (COMPRESSION_PACKBITS), NULL,

                                 _("_Deflate"),
                                 GINT_TO_POINTER (COMPRESSION_DEFLATE), NULL,

                                 _("_JPEG"),
                                 GINT_TO_POINTER (COMPRESSION_JPEG), NULL,

                                 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* comment entry */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new ( _("Comment:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), image_comment);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (comment_entry_callback),
                    NULL);

  gtk_widget_show (frame);

  gtk_widget_show (vbox);
  gtk_widget_show (dlg);

  gtk_main ();

  return tsint.run;
}

static void
save_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  tsint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
comment_entry_callback (GtkWidget *widget,
			gpointer   data)
{
  const gchar *text;

  text = gtk_entry_get_text (GTK_ENTRY (widget));

  g_free (image_comment);
  image_comment = g_strdup (text);

  /* g_print ("COMMENT: %s\n", image_comment); */
}
