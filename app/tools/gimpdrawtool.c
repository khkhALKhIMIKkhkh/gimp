/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"
 
#include "display/gimpdisplay.h"

#include "gimpdrawtool.h"


enum
{
  DRAW,
  LAST_SIGNAL
};

static void   gimp_draw_tool_class_init (GimpDrawToolClass *klass);
static void   gimp_draw_tool_init       (GimpDrawTool      *draw_tool);

static void   gimp_draw_tool_finalize   (GObject           *object);

static void   gimp_draw_tool_control    (GimpTool          *tool,
					 ToolAction         action,
					 GimpDisplay       *gdisp);


static guint gimp_draw_tool_signals[LAST_SIGNAL] = { 0 };

static GimpToolClass *parent_class = NULL;



GType
gimp_draw_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpDrawToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_draw_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpDrawTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_draw_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TOOL,
					  "GimpDrawTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_draw_tool_class_init (GimpDrawToolClass *klass)
{
  GObjectClass  *object_class;
  GimpToolClass *tool_class;

  object_class = G_OBJECT_CLASS (klass);
  tool_class   = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_draw_tool_signals[DRAW] =
    g_signal_new ("draw",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpDrawToolClass, draw),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize = gimp_draw_tool_finalize;

  tool_class->control    = gimp_draw_tool_control;
}

static void
gimp_draw_tool_init (GimpDrawTool *tool)
{
  tool->draw_state   = INVISIBLE;
  tool->gc           = NULL;
  tool->paused_count = 0;
  tool->line_width   = 0;
  tool->line_style   = GDK_LINE_SOLID;
  tool->cap_style    = GDK_CAP_NOT_LAST;
  tool->join_style   = GDK_JOIN_MITER;
}

static void
gimp_draw_tool_finalize (GObject *object)
{
  GimpDrawTool *draw_tool;

  draw_tool = GIMP_DRAW_TOOL (object);

  if (draw_tool->gc)
    {
      g_object_unref (G_OBJECT (draw_tool->gc));
      draw_tool->gc = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_draw_tool_control (GimpTool    *tool,
			ToolAction   action,
			GimpDisplay *gdisp)
{
  GimpDrawTool *draw_tool;

  draw_tool = GIMP_DRAW_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      gimp_draw_tool_pause (draw_tool);
      break;

    case RESUME:
      gimp_draw_tool_resume (draw_tool);
      break;

    case HALT:
      gimp_draw_tool_stop (draw_tool);
      break;

    default:
      break;
    }

  if (GIMP_TOOL_CLASS (parent_class)->control)
    GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

void
gimp_draw_tool_start (GimpDrawTool *core,
		      GdkWindow    *win)
{
  GdkColor fg, bg;

  if (core->draw_state != INVISIBLE)
    gimp_draw_tool_stop (core);  /* this seems backwards ;) */

  core->win   = win;
  core->paused_count = 0;  /*  reset pause counter to 0  */

  /*  create a new graphics context  */
  if (! core->gc)
    core->gc = gdk_gc_new (win);

  gdk_gc_set_function (core->gc, GDK_INVERT);
  fg.pixel = 0xFFFFFFFF;
  bg.pixel = 0x00000000;
  gdk_gc_set_foreground (core->gc, &fg);
  gdk_gc_set_background (core->gc, &bg);
  gdk_gc_set_line_attributes (core->gc, core->line_width, core->line_style,
			      core->cap_style, core->join_style);

  g_signal_emit (G_OBJECT(core), gimp_draw_tool_signals[DRAW], 0);

  core->draw_state = VISIBLE;
}


void
gimp_draw_tool_stop (GimpDrawTool *core)
{
  if (core->draw_state == INVISIBLE)
    return;

  g_signal_emit (G_OBJECT(core), gimp_draw_tool_signals[DRAW], 0);

  core->draw_state = INVISIBLE;
}


void
gimp_draw_tool_resume (GimpDrawTool *core)
{
  core->paused_count = (core->paused_count > 0) ? core->paused_count - 1 : 0;

  if (core->paused_count == 0)
    {
      core->draw_state = VISIBLE;

      g_signal_emit(G_OBJECT(core), gimp_draw_tool_signals[DRAW], 0);
    }
}


void
gimp_draw_tool_pause (GimpDrawTool *core)
{
  if (core->paused_count == 0)
    {
      core->draw_state = INVISIBLE;

      g_signal_emit (G_OBJECT(core), gimp_draw_tool_signals[DRAW], 0);
    }

  core->paused_count++;
}


void
gimp_draw_tool_draw_handle (GimpDrawTool *draw_tool, 
			    gdouble x,
			    gdouble y,
			    gint size,
			    gint type)
{
  GimpTool *tool = GIMP_TOOL (draw_tool);
  gdouble hx, hy;
  gint filled;

  gdisplay_transform_coords_f (tool->gdisp, x, y, &hx, &hy, TRUE);

  hx = ROUND (hx);
  hy = ROUND (hy);

  filled = type % 2;

  if (type < 2)
    gdk_draw_rectangle (tool->gdisp->canvas->window,
			draw_tool->gc, filled,
			hx - size/2, hy - size/2,
			size, size); 
  else
    gdk_draw_arc       (tool->gdisp->canvas->window,
			draw_tool->gc, filled,
			hx - size/2, hy - size/2,
			size, size, 0, 360*64); 

}


void
gimp_draw_tool_draw_lines (GimpDrawTool *draw_tool, 
			   gdouble *points,
			   gint npoints,
			   gint filled)
{
  GimpTool *tool = GIMP_TOOL (draw_tool);
  GdkPoint *coords = g_new (GdkPoint, npoints);

  gint i;
  gdouble sx, sy;

  for (i=0; i < npoints ; i++)
  {
    gdisplay_transform_coords_f (tool->gdisp, points[i*2], points[i*2+1],
				 &sx, &sy, TRUE);
    coords[i].x = ROUND (sx);
    coords[i].y = ROUND (sy);
  }


  if (filled)
    gdk_draw_polygon (tool->gdisp->canvas->window,
	              draw_tool->gc, TRUE,
		      coords, npoints);
  else
    gdk_draw_lines (tool->gdisp->canvas->window,
	            draw_tool->gc,
		    coords, npoints);

  g_free (coords);

}
