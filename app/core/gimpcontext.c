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

#include <gtk/gtk.h>

#include "gimpcontext.h"
#include "gimpsignal.h"

#define context_return_if_fail(context) \
        g_return_if_fail ((context) != NULL); \
        g_return_if_fail (GIMP_IS_CONTEXT (context));

#define context_return_val_if_fail(context, val) \
        g_return_val_if_fail ((context) != NULL, (val)); \
        g_return_val_if_fail (GIMP_IS_CONTEXT (context), (val));

#define context_check_current(context) \
        ((context) = (context) ? (context) : current_context)

#define context_find_defined(context, field_defined) \
        while (!((context)->field_defined) && (context)->parent) \
          (context) = (context)->parent

enum {
  OPACITY_CHANGED,
  PAINT_MODE_CHANGED,
  IMAGE_CHANGED,
  DISPLAY_CHANGED,
  LAST_SIGNAL
};

static guint gimp_context_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass * parent_class = NULL;

/*  the currently active context  */
static GimpContext * current_context = NULL;

/*  the context user by the interface  */
static GimpContext * user_context = NULL;

/*  the default context which is initialized from gimprc  */
static GimpContext * default_context = NULL;

/*  the hardcoded standard context  */
static GimpContext * standard_context = NULL;


/*  private functions  ******************************************************/

static void
gimp_context_destroy (GtkObject *object)
{
  GimpContext *context;

  context_return_if_fail (object);

  context = GIMP_CONTEXT (object);

  if (context->name)
    g_free (context->name);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gimp_context_class_init (GimpContextClass *klass)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS (klass);

  parent_class = gtk_type_class (gimp_object_get_type ());

  gimp_context_signals[OPACITY_CHANGED] =
    gimp_signal_new ("opacity_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					opacity_changed),
		     gimp_sigtype_void);

  gimp_context_signals[PAINT_MODE_CHANGED] =
    gimp_signal_new ("paint_mode_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					paint_mode_changed),
		     gimp_sigtype_void);

  gimp_context_signals[IMAGE_CHANGED] =
    gimp_signal_new ("image_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					image_changed),
		     gimp_sigtype_void);

  gimp_context_signals[DISPLAY_CHANGED] =
    gimp_signal_new ("display_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					display_changed),
		     gimp_sigtype_void);

  gtk_object_class_add_signals (object_class, gimp_context_signals,
				LAST_SIGNAL);

  klass->opacity_changed    = NULL;
  klass->paint_mode_changed = NULL;
  klass->image_changed      = NULL;
  klass->display_changed    = NULL;

  object_class->destroy = gimp_context_destroy;
}

static void
gimp_context_init (GimpContext *context)
{
  context->name = NULL;
  context->parent = NULL;

  /*  Values defined by default  */

  context->opacity_defined = TRUE;
  context->opacity = 1.0;

  context->paint_mode_defined = TRUE;
  context->paint_mode = 0;

  /*  Values to be taken from the parent context by default  */

  context->image_defined = FALSE;
  context->image = NULL;

  context->display_defined = FALSE;
  context->display = NULL;
}

/*  public functions  *******************************************************/

GtkType
gimp_context_get_type (void)
{
  static GtkType context_type = 0;

  if(! context_type)
    {
      GtkTypeInfo context_info =
      {
	"GimpContext",
	sizeof(GimpContext),
	sizeof(GimpContextClass),
	(GtkClassInitFunc) gimp_context_class_init,
	(GtkObjectInitFunc) gimp_context_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL
      };

      context_type = gtk_type_unique (gimp_object_get_type (), &context_info);
    }

  return context_type;
}

GimpContext *
gimp_context_new (gchar       *name,
		  GimpContext *template,
		  GimpContext *parent)
{
  GimpContext *context;

  g_return_val_if_fail (!template || GIMP_IS_CONTEXT (template), NULL);
  g_return_val_if_fail (!parent || GIMP_IS_CONTEXT (parent), NULL);

  context = gtk_type_new (gimp_context_get_type ());

  /*  FIXME: need unique (translated??) names here
   */
  context->name = g_strdup (name ? name : "Unnamed");
  context->parent = parent;

  if (template)
    {
      context->opacity    = gimp_context_get_opacity (template);
      context->paint_mode = gimp_context_get_paint_mode (template);
      context->image      = gimp_context_get_image (template);
      context->display    = gimp_context_get_display (template);

      context->opacity_defined    = template->opacity_defined;
      context->paint_mode_defined = template->paint_mode_defined;
      context->image_defined      = template->image_defined;
      context->display_defined    = template->display_defined;
    }

  if (!parent)
    {
      context->opacity_defined    = TRUE;
      context->paint_mode_defined = TRUE;
      context->image_defined      = TRUE;
      context->display_defined    = TRUE;
    }

  return context;
}

/*  getting/setting the special contexts  ***********************************/

GimpContext *
gimp_context_get_current (void)
{
  return current_context;
}

void
gimp_context_set_current (GimpContext *context)
{
  current_context = context;
}

GimpContext *
gimp_context_get_user (void)
{
  return user_context;
}

void
gimp_context_set_user (GimpContext *context)
{
  user_context = context;
}

GimpContext *
gimp_context_get_default (void)
{
  return default_context;
}

void
gimp_context_set_default (GimpContext *context)
{
  default_context = context;
}

GimpContext *
gimp_context_get_standard (void)
{
  if (! standard_context)
    {
      standard_context = gimp_context_new ("Standard", NULL, NULL);

      gtk_quit_add_destroy (TRUE, GTK_OBJECT (standard_context));
    }

  return standard_context;
}

/*  functions manipulating a single context  ********************************/

/* FIXME: - this is UGLY code duplication
 *        - gimp_context_*_defined and _define_* sounds very ugly, too
 * TODO:  - implement a generic way or alternatively
 *        - write some macros which will fold one of the following
 *          functions into a single macro call
 */

/*  opacity  */

gdouble
gimp_context_get_opacity (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, 1.0);
  context_find_defined (context, opacity_defined);

  return context->opacity;
}

void
gimp_context_set_opacity (GimpContext *context,
			  gdouble      opacity)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, opacity_defined);

  context->opacity = opacity;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[OPACITY_CHANGED]);
}

gboolean
gimp_context_opacity_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->opacity_defined;
}

void
gimp_context_define_opacity (GimpContext *context,
			     gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    context->opacity = gimp_context_get_opacity (context);

  context->opacity_defined = defined;
}

/*  paint mode  */

gint
gimp_context_get_paint_mode (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, 0);
  context_find_defined (context, paint_mode_defined);

  return context->paint_mode;
}

void
gimp_context_set_paint_mode (GimpContext *context,
			     gint         paint_mode)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, paint_mode_defined);

  context->paint_mode = paint_mode;
  gtk_signal_emit (GTK_OBJECT(context),
		   gimp_context_signals[PAINT_MODE_CHANGED]);
}

gboolean
gimp_context_paint_mode_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->paint_mode_defined;
}

void
gimp_context_define_paint_mode (GimpContext *context,
				gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    context->paint_mode = gimp_context_get_paint_mode (context);

  context->paint_mode_defined = defined;
}

/*  image  */

GimpImage *
gimp_context_get_image (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);
  context_find_defined (context, image_defined);

  return context->image;
}

void
gimp_context_set_image (GimpContext *context,
			GimpImage   *image)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, image_defined);

  context->image = image;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[IMAGE_CHANGED]);
}

gboolean
gimp_context_image_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->image_defined;
}

void
gimp_context_define_image (GimpContext *context,
			   gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    context->image = gimp_context_get_image (context);

  context->image_defined = defined;
}

/*  display  */

GDisplay *
gimp_context_get_display (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);
  context_find_defined (context, display_defined);

  return context->display;
}

void
gimp_context_set_display (GimpContext *context,
			  GDisplay    *display)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, display_defined);

  context->display = display;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[DISPLAY_CHANGED]);
}

gboolean
gimp_context_display_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->display_defined;
}

void
gimp_context_define_display (GimpContext *context,
			     gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    context->display = gimp_context_get_display (context);

  context->display_defined = defined;
}
