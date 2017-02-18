/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2016 Iain Lane <iain@orangesquash.org.uk>
 *
 * Licensed under the GNU Lesser General Public License Version 3
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include "mofile.h"
#include "mogroup.h"

#define DEFAULT_DIRECTORY "/usr/share/locale/"

/**
 * SECTION:mogroup
 * @short_description: Work with all translations for a domain.
 * @title: MoGroup
 * @stability: unstable
 * @include: libmo/mo.h
 *
 * #MoGroup allows for the reading of multiple installed languages for a given
 * translation domain at once.
 *
 * <example>
 * <title>Reading all translations for a given string.</title>
 *
 * <programlisting>
 *       GError *err = NULL;
 *       g_autoptr(GHashTable) ht = NULL;
 *       GHashTableIter iter;
 *       void *domain, *translation;
 *
 *       g_autoptr(MoGroup) mogroup = mo_group_new ("apt", &err);
 *
 *       if (!mogroup) {
 *               g_printerr ("couldn't create MoGroup: %s\n", err->message);
 *               g_clear_error (&err);
 *               return;
 *       }
 *
 *       ht = mo_group_get_translations (mogroup,
 *                                       "edit the source information file");
 *
 *       g_hash_table_iter_init (&iter, ht);
 *
 *       while (g_hash_table_iter_next (&iter, &domain, &translation))
 *               g_printf ("%s: '%s'\n", (gchar *) domain, (gchar *) translation);
 *
 *       return EXIT_SUCCESS;
 * </programlisting>
 * </example>
 */

struct _MoGroup {
        GObject parent_instance;

        gchar *directory;
        gchar *domain;
        GHashTable *mofiles;
};

enum {
        PROP_DOMAIN = 1,
        PROP_DIRECTORY,
        N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* forward declarations */
static void mo_group_initable_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MoGroup, mo_group, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                mo_group_initable_init))

GQuark
mo_group_error_quark (void)
{
        return g_quark_from_static_string ("mo-group-error-quark");
}

static void
mo_group_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
    MoGroup *self = MO_GROUP (object);

    switch (property_id)
    {
        case PROP_DOMAIN:
            g_value_set_string (value, self->domain);
            break;
        case PROP_DIRECTORY:
            g_value_set_string (value, self->directory);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
mo_group_set_property (GObject      *object,
                      guint         property_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
    MoGroup *self = MO_GROUP (object);

    switch (property_id)
    {
        case PROP_DOMAIN:
            self->domain = g_value_dup_string (value);
            break;
        case PROP_DIRECTORY:
            self->directory = g_value_dup_string (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
mo_group_dispose (GObject *object)
{
        MoGroup *self = MO_GROUP (object);

        /* drop all references to MoFiles */
        g_hash_table_remove_all (self->mofiles);

        G_OBJECT_CLASS (mo_group_parent_class)->dispose (object);
}

static void
mo_group_finalize (GObject *object)
{
        MoGroup *self = MO_GROUP (object);

        g_clear_pointer (&self->directory, g_free);
        g_clear_pointer (&self->domain, g_free);
        g_clear_pointer (&self->mofiles, g_hash_table_destroy);

        G_OBJECT_CLASS (mo_group_parent_class)->finalize (object);
}

static gboolean
mo_group_initable_init_real (GInitable *init,
                             GCancellable *cancellable G_GNUC_UNUSED,
                             GError **error)
{
        MoGroup *self;

        const gchar *current_directory;
        g_autoptr(GDir) dir = NULL;
        GError *local_error = NULL;
        MoFile *mofile;
        g_autofree gchar *mofilename = NULL;

        if (!MO_IS_GROUP (init))
                return FALSE;

        self = MO_GROUP (init);

        if (!self->directory)
                return FALSE;

        /* First check the directory exists */
        if (!g_file_test (self->directory, G_FILE_TEST_EXISTS) ||
            !g_file_test (self->directory, G_FILE_TEST_IS_DIR)) {
                g_set_error (error,
                             MO_GROUP_ERROR,
                             MO_GROUP_NO_SUCH_DIRECTORY_ERROR,
                             "'%s' does not exist.", self->directory,
                             NULL);
                g_assert (error == NULL || *error != NULL);
                return FALSE;
        }

        /* and then load all of the .mo files in it */
        dir = g_dir_open (self->directory, 0, &local_error);

        if (!dir) {
                g_assert (local_error != NULL);

                g_propagate_prefixed_error (error,
                                            local_error,
                                            "Opening directory '%s' failed",
                                            self->directory);
                return FALSE;
        }

        mofilename = g_strdup_printf ("%s.mo", self->domain);

        /* dir is okay, let's go */
        while ((current_directory = g_dir_read_name (dir))) {
                g_autofree gchar *current_filename;

                current_filename = g_build_filename (self->directory,
                                                     current_directory,
                                                     "LC_MESSAGES",
                                                     mofilename,
                                                     NULL);

                mofile = mo_file_new (current_filename, &local_error);

                if (!mofile) {
                        g_assert (local_error != NULL);

                        if (g_error_matches (local_error,
                                             MO_FILE_ERROR,
                                             MO_FILE_NO_SUCH_FILE_ERROR)) {
                                g_debug ("'%s' was not found.", current_filename);
                        } else {
                                g_warning ("Couldn't load '%s': %s",
                                           current_filename,
                                           local_error->message);
                        }

                        g_clear_error (&local_error);
                        continue;
                }

                g_hash_table_insert (self->mofiles,
                                     g_strdup (current_directory),
                                     mofile);
        }

        return TRUE;
}

static void
mo_group_initable_init (GInitableIface *iface)
{
        iface->init = mo_group_initable_init_real;
}

static void
mo_group_class_init (MoGroupClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->set_property = mo_group_set_property;
        object_class->get_property = mo_group_get_property;
        object_class->dispose = mo_group_dispose;
        object_class->finalize = mo_group_finalize;

        /**
         * MoGroup::domain:
         *
         * Domain that this #MoGroup contains translations for.
         */
        obj_properties[PROP_DOMAIN] =
                g_param_spec_string ("domain",
                                     "Domain",
                                     "Domain to load .mo files for",
                                     NULL  /* default value */,
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_STRINGS);
        /**
         * MoGroup::directory:
         *
         * Directory to load translations for this #MoGroup from.
         */
        obj_properties[PROP_DIRECTORY] =
                g_param_spec_string ("directory",
                                     "Directory",
                                     "Directory to load .mo files from",
                                     DEFAULT_DIRECTORY  /* default value */,
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_properties (object_class,
                                           N_PROPERTIES,
                                           obj_properties);
}

static void
mo_group_init (MoGroup *self)
{
        self->mofiles = g_hash_table_new_full (g_str_hash, /* hash_func */
                                               g_str_equal, /* key_equal_func */
                                               g_free, /* key_destroy_func */
                                               g_object_unref /* value_destroy_func */);
}

/**
 * mo_group_get_directory:
 * @self: An initialised #MoGroup.
 *
 * Get the directory whose .mo files are represented in this #MoGroup.
 *
 * Returns: (transfer none): The directory.
 */
const gchar *
mo_group_get_directory (MoGroup *self)
{
        if (!MO_IS_GROUP (self))
                return NULL;

        return self->directory;
}

/**
 * mo_group_get_domain:
 * @self: An initialised #MoGroup.
 *
 * Get the domain this #MoGroup contains translations for.
 *
 * Returns: (transfer none): The domain.
 */
const gchar *
mo_group_get_domain (MoGroup *self)
{
        if (!MO_IS_GROUP (self))
                return NULL;

        return self->domain;
}

/**
 * mo_group_get_languages:
 * @self: An initialised #MoGroup.
 *
 * Get the languages that the domain has any translations for.
 *
 * Returns: (element-type utf8) (transfer container): A #GList of translation
 * domains. Do not modify or free the contained strings. Free the list itself
 * with g_list_free.
 */
GList *
mo_group_get_languages (MoGroup *self)
{
        if (!MO_IS_GROUP (self))
                return NULL;

        return g_hash_table_get_keys (self->mofiles);
}

/**
 * mo_group_get_mo_file:
 * @self: An initialised #MoGroup.
 * @locale: A locale.
 *
 * Returns: (transfer full): The #MoFile containing translations for @locale.
 */
MoFile *
mo_group_get_mo_file (MoGroup *self,
                      const gchar *locale)
{
        MoFile *mofile;

        if (!MO_IS_GROUP (self) || !locale)
                return NULL;

        mofile = g_hash_table_lookup (self->mofiles, locale);

        if (!mofile)
                return NULL;

        return MO_FILE (g_object_ref (mofile));
}

typedef struct {
        const gchar *translation;
        GHashTable *dict;
} MoTranslationDictData;

static void
find_translation (gpointer key,
                  gpointer value,
                  gpointer user_data)
{
        MoTranslationDictData *data = user_data;
        gchar *lang = key;
        MoFile *mofile = value;
        gchar *translation;

        /* Ignoring errors - sensible? */
        translation = mo_file_get_translation (mofile, data->translation, NULL);

        if (!translation)
                return;

        g_hash_table_insert (data->dict,
                             g_strdup (lang),
                             translation);
}

/**
 * mo_group_get_translations:
 * @self: An initialised #MoGroup.
 * @translation: Untranslated (in the 'C' locale) string.
 *
 * Retrieve all translations for a string.
 *
 * Returns: (transfer full) (element-type utf8 utf8): A dictionary mapping
 * domains to translated values.
 */
GHashTable *
mo_group_get_translations (MoGroup *self,
                           const gchar *translation)
{
        GHashTable *ret;
        MoTranslationDictData data;

        if (!MO_IS_GROUP (self) || !translation)
                return NULL;

        ret = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

        data.translation = translation;
        data.dict = ret;

        g_hash_table_foreach (self->mofiles, find_translation, &data);

        return ret;
}

/**
 * mo_group_get_translation:
 * @self: An initialised #MoGroup.
 * @locale: The locale to retrieve the translation for.
 * @translation: Untranslated (in the 'C' locale) string.
 *
 * Retrieve the translated value of a string.
 *
 * Returns: (transfer full): the translated string, or NULL if a translation is not found.
 */
gchar *
mo_group_get_translation (MoGroup *self,
                          const gchar *locale,
                          const gchar *translation,
                          GError **err)
{
        g_autoptr(MoFile) mofile = NULL;

        if (!MO_IS_GROUP (self) || !locale || !translation)
                return NULL;

        mofile = g_object_ref (g_hash_table_lookup (self->mofiles, locale));

        if (!mofile)
                return NULL;

        return mo_file_get_translation (mofile, translation, err);
}

/**
 * mo_group_new_for_directory
 * @domain: Domain to creat this #MoGroup for.
 * @directory: Directory to load .mo files from.
 * @error: Return location for a GError, or NULL.
 *
 * Create a new #MoGroup, containing all available translations for @domain.
 * Translations will be loaded from @directory, which must exist (otherwise the
 * #MoGroup will not be created).
 *
 * Returns: The new #MoGroup, or NULL on error, in which case @error will be set.
 */
MoGroup *
mo_group_new_for_directory (const gchar *domain,
                            const gchar *directory,
                            GError **error)
{
        return MO_GROUP (g_initable_new (MO_TYPE_GROUP,
                                         NULL,
                                         error,
                                         "domain", domain,
                                         "directory", directory,
                                         NULL));
}

/**
 * mo_group_new:
 * @domain: Domain to create this #MoGroup for.
 * @error: Return location for a GError, or NULL.
 *
 * Create a new #MoGroup, containing all available translations for @domain.
 *
 * Returns: The new #MoGroup, or NULL on error, in which case @error will be set.
 */
MoGroup *
mo_group_new (const gchar *domain, GError **error)
{
        return MO_GROUP (g_initable_new (MO_TYPE_GROUP,
                                         NULL,
                                         error,
                                         "domain", domain,
                                         NULL));
}
