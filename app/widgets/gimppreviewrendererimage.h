/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppreviewrendererimage.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PREVIEW_RENDERER_IMAGE_H__
#define __GIMP_PREVIEW_RENDERER_IMAGE_H__

#include "gimppreviewrenderer.h"

#define GIMP_TYPE_PREVIEW_RENDERER_IMAGE            (gimp_preview_renderer_image_get_type ())
#define GIMP_PREVIEW_RENDERER_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PREVIEW_RENDERER_IMAGE, GimpPreviewRendererImage))
#define GIMP_PREVIEW_RENDERER_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PREVIEW_RENDERER_IMAGE, GimpPreviewRendererImageClass))
#define GIMP_IS_PREVIEW_RENDERER_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_PREVIEW_RENDERER_IMAGE))
#define GIMP_IS_PREVIEW_RENDERER_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PREVIEW_RENDERER_IMAGE))
#define GIMP_PREVIEW_RENDERER_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PREVIEW_RENDERER_IMAGE, GimpPreviewRendererImageClass))


typedef struct _GimpPreviewRendererImageClass  GimpPreviewRendererImageClass;

struct _GimpPreviewRendererImage
{
  GimpPreviewRenderer parent_instance;

  GimpChannelType     channel;
};

struct _GimpPreviewRendererImageClass
{
  GimpPreviewRendererClass  parent_class;
};


GType   gimp_preview_renderer_image_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_PREVIEW_RENDERER_IMAGE_H__ */
