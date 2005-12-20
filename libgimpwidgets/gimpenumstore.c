/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenumstore.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpenumstore.h"


static void   gimp_enum_store_finalize     (GObject      *object);

static void   gimp_enum_store_add_value    (GtkListStore *store,
                                            GEnumValue   *value);


G_DEFINE_TYPE (GimpEnumStore, gimp_enum_store, GIMP_TYPE_INT_STORE);

#define parent_class gimp_enum_store_parent_class


static void
gimp_enum_store_class_init (GimpEnumStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_enum_store_finalize;
}

static void
gimp_enum_store_init (GimpEnumStore *store)
{
}

static void
gimp_enum_store_finalize (GObject *object)
{
  GimpEnumStore *enum_store = GIMP_ENUM_STORE (object);

  if (enum_store->enum_class)
    g_type_class_unref (enum_store->enum_class);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_enum_store_add_value (GtkListStore *store,
                           GEnumValue   *value)
{
  GtkTreeIter  iter;
  const gchar *desc;

  desc = gimp_enum_value_get_desc (GIMP_ENUM_STORE (store)->enum_class, value);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      GIMP_INT_STORE_VALUE, value->value,
                      GIMP_INT_STORE_LABEL, desc,
                      -1);
}


/**
 * gimp_enum_store_new:
 * @enum_type: the #GType of an enum.
 *
 * Creates a new #GimpEnumStore, derived from #GtkListStore and fills
 * it with enum values. The enum needs to be registered to the type
 * system and should have translatable value names.
 *
 * Return value: a new #GimpEnumStore.
 *
 * Since: GIMP 2.4
 **/
GtkListStore *
gimp_enum_store_new (GType enum_type)
{
  GtkListStore *store;
  GEnumClass   *enum_class;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  enum_class = g_type_class_ref (enum_type);

  store = gimp_enum_store_new_with_range (enum_type,
                                          enum_class->minimum,
                                          enum_class->maximum);

  g_type_class_unref (enum_class);

  return store;
}

/**
 * gimp_enum_store_new_with_range:
 * @enum_type: the #GType of an enum.
 * @minimum: the minimum value to include
 * @maximum: the maximum value to include
 *
 * Creates a new #GimpEnumStore like gimp_enum_store_new() but allows
 * to limit the enum values to a certain range. Values smaller than
 * @minimum or larger than @maximum are not added to the store.
 *
 * Return value: a new #GimpEnumStore.
 *
 * Since: GIMP 2.4
 **/
GtkListStore *
gimp_enum_store_new_with_range (GType  enum_type,
                                gint   minimum,
                                gint   maximum)
{
  GtkListStore *store;
  GEnumValue   *value;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  store = g_object_new (GIMP_TYPE_ENUM_STORE, NULL);

  GIMP_ENUM_STORE (store)->enum_class = g_type_class_ref (enum_type);

  for (value = GIMP_ENUM_STORE (store)->enum_class->values;
       value->value_name;
       value++)
    {
      if (value->value < minimum || value->value > maximum)
        continue;

      gimp_enum_store_add_value (store, value);
    }

  return store;
}

/**
 * gimp_enum_store_new_with_values
 * @enum_type: the #GType of an enum.
 * @n_values:  the number of enum values to include
 * @...:       a list of enum values (exactly @n_values)
 *
 * Creates a new #GimpEnumStore like gimp_enum_store_new() but allows
 * to expliticely list the enum values that should be added to the
 * store.
 *
 * Return value: a new #GimpEnumStore.
 *
 * Since: GIMP 2.4
 **/
GtkListStore *
gimp_enum_store_new_with_values (GType enum_type,
                                 gint  n_values,
                                 ...)
{
  GtkListStore *store;
  va_list       args;

  va_start (args, n_values);

  store = gimp_enum_store_new_with_values_valist (enum_type,
                                                  n_values,
                                                  args);

  va_end (args);

  return store;
}

/**
 * gimp_enum_store_new_with_values_valist:
 * @enum_type: the #GType of an enum.
 * @n_values:  the number of enum values to include
 * @args:      a va_list of enum values (exactly @n_values)
 *
 * See gimp_enum_store_new_with_values().
 *
 * Return value: a new #GimpEnumStore.
 *
 * Since: GIMP 2.4
 **/
GtkListStore *
gimp_enum_store_new_with_values_valist (GType     enum_type,
                                        gint      n_values,
                                        va_list   args)
{
  GtkListStore *store;
  GEnumValue   *value;
  gint          i;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (n_values > 1, NULL);

  store = g_object_new (GIMP_TYPE_ENUM_STORE, NULL);

  GIMP_ENUM_STORE (store)->enum_class = g_type_class_ref (enum_type);

  for (i = 0; i < n_values; i++)
    {
      value = g_enum_get_value (GIMP_ENUM_STORE (store)->enum_class,
                                va_arg (args, gint));

      if (value)
        gimp_enum_store_add_value (store, value);
    }

  return store;
}

/**
 * gimp_enum_store_set_stock_prefix:
 * @store:        a #GimpEnumStore
 * @stock_prefix: a prefix to create icon stock ID from enum values
 *
 * Creates a stock ID for each enum value in the @store by appending
 * the value's nick to the given @stock_prefix, separated by a hyphen.
 *
 * See also: gimp_enum_combo_box_set_stock_prefix().
 *
 * Since: GIMP 2.4
 **/
void
gimp_enum_store_set_stock_prefix (GimpEnumStore *store,
                                  const gchar   *stock_prefix)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      iter_valid;

  g_return_if_fail (GIMP_IS_ENUM_STORE (store));

  model = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gchar *stock_id = NULL;

      if (stock_prefix)
        {
          GEnumValue *enum_value;
          gint        value;

          gtk_tree_model_get (model, &iter,
                              GIMP_INT_STORE_VALUE, &value,
                              -1);

          enum_value = g_enum_get_value (store->enum_class, value);

          stock_id = g_strconcat (stock_prefix, "-",
                                  enum_value->value_nick,
                                  NULL);
        }

      gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                          GIMP_INT_STORE_STOCK_ID, stock_id,
                          -1);

      if (stock_id)
        g_free (stock_id);
    }
}
