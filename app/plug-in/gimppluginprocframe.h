/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginprocframe.h
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

#ifndef __GIMP_PLUG_IN_PROC_FRAME_H__
#define __GIMP_PLUG_IN_PRON_FRAME_H__


struct _GimpPlugInProcFrame
{
  gint           ref_count;

  GimpContext   *main_context;
  GList         *context_stack;

  GimpProcedure *procedure;
  GMainLoop     *main_loop;

  GValueArray   *return_vals;

  GimpProgress  *progress;
  gboolean       progress_created;
  gulong         progress_cancel_id;

  /*  lists of things to clean up on dispose  */
  GList         *cleanups;
};


GimpPlugInProcFrame * gimp_plug_in_proc_frame_new     (GimpContext         *context,
                                                       GimpProgress        *progress,
                                                       GimpPlugInProcedure *procedure);
void                  gimp_plug_in_proc_frame_init    (GimpPlugInProcFrame *proc_frame,
                                                       GimpContext         *context,
                                                       GimpProgress        *progress,
                                                       GimpPlugInProcedure *procedure);

void                  gimp_plug_in_proc_frame_dispose (GimpPlugInProcFrame *proc_frame,
                                                       GimpPlugIn          *plug_in);

GimpPlugInProcFrame * gimp_plug_in_proc_frame_ref     (GimpPlugInProcFrame *proc_frame);
void                  gimp_plug_in_proc_frame_unref   (GimpPlugInProcFrame *proc_frame,
                                                       GimpPlugIn          *plug_in);

GValueArray * gimp_plug_in_proc_frame_get_return_vals (GimpPlugInProcFrame *proc_frame);


#endif /* __GIMP_PLUG_IN_PROC_FRAME_H__ */
