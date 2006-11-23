/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help plug-in
 * Copyright (C) 1999-2004 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

#ifndef __GIMP_HELP_LOCALE_H__
#define __GIMP_HELP_LOCALE_H__


struct _GimpHelpLocale
{
  gchar      *locale_id;
  GHashTable *help_id_mapping;
  gchar      *help_missing;

  /* eek */
  GList      *toplevel_items;
};


GimpHelpLocale * gimp_help_locale_new        (const gchar     *locale_id);
void             gimp_help_locale_free       (GimpHelpLocale  *locale);

const gchar    * gimp_help_locale_map        (GimpHelpLocale  *locale,
                                              const gchar     *help_id);

gboolean         gimp_help_locale_parse      (GimpHelpLocale  *locale,
                                              const gchar     *filename,
                                              const gchar     *help_domain,
                                              GError         **error);


#endif /* __GIMP_HELP_LOCALE_H__ */
