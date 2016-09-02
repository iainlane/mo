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

#include "mo.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct {
        guint32 magic;
        guint32 revision;
        guint32 nstrings;
        guint32 orig_tab_offset;
        guint32 trans_tab_offset;
        guint32 hash_tab_size;
        guint32 hash_tab_offset;
} MoFileHeader;

struct _MoFile {
        GObject parent_instance;

        gchar *filename;
        GHashTable *translations_cache;
        MoFileHeader header;
        gboolean swapped;
        char *mmapped_file;
        off_t length;
};

enum {
        PROP_FILENAME = 1,
        N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* forward declarations */
static void mo_file_initable_init (GInitableIface *iface);
static gboolean read_mo_file (MoFile *self, GError **error);

G_DEFINE_TYPE_WITH_CODE (MoFile, mo_file, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                mo_file_initable_init))

GQuark
mo_file_error_quark ()
{
        return g_quark_from_static_string ("mo-file-error-quark");
}

static void
mo_file_get_property (GObject    *object,
                      guint       property_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
    MoFile *self = MO_FILE (object);

    switch (property_id)
    {
        case PROP_FILENAME:
            g_value_set_string (value, self->filename);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
mo_file_set_property (GObject      *object,
                      guint         property_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
    MoFile *self = MO_FILE (object);

    switch (property_id)
    {
        case PROP_FILENAME:
            self->filename = g_value_dup_string (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
clear_file (MoFile *self)
{
        g_return_if_fail (MO_IS_FILE (self));

        if (self->mmapped_file) {
                munmap (self->mmapped_file, self->length);
                memset (&self->header, 0, sizeof (MoFileHeader));
                self->mmapped_file = NULL;
        }

        g_free (self->filename);
        g_hash_table_remove_all (self->translations_cache);
}


static void
mo_file_dispose (GObject *object)
{
        G_OBJECT_CLASS (mo_file_parent_class)->dispose (object);
}

static void
mo_file_finalize (GObject *object)
{
        MoFile *self = MO_FILE (object);

        clear_file (self);

        G_OBJECT_CLASS (mo_file_parent_class)->finalize (object);
}

static gboolean
mo_file_initable_init_real (GInitable *init,
                            GCancellable *cancellable G_GNUC_UNUSED,
                            GError **error)
{
        MoFile *self;

        if (!MO_IS_FILE (init))
                return FALSE;

        self = MO_FILE (init);

        if (!self->filename)
                return FALSE;

        if (!g_file_test (self->filename, G_FILE_TEST_EXISTS) ||
            !g_file_test (self->filename, G_FILE_TEST_IS_REGULAR)) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_NO_SUCH_FILE_ERROR,
                             "'%s' does not exist.", self->filename,
                             NULL);
                g_assert (error == NULL || *error != NULL);
                return FALSE;
        }

        return read_mo_file (self, error);
}

static void
mo_file_initable_init (GInitableIface *iface)
{
        iface->init = mo_file_initable_init_real;
}

static void
mo_file_class_init (MoFileClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->set_property = mo_file_set_property;
        object_class->get_property = mo_file_get_property;
        object_class->dispose = mo_file_dispose;
        object_class->finalize = mo_file_finalize;

        obj_properties[PROP_FILENAME] =
                g_param_spec_string ("filename",
                                     "Filename",
                                     "Name of the .mo file to load.",
                                     NULL  /* default value */,
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_properties (object_class,
                                           N_PROPERTIES,
                                           obj_properties);
}

static void
mo_file_init (MoFile *self)
{
        self->translations_cache = g_hash_table_new_full (g_str_hash /* owned */,
                                                          g_str_equal,
                                                          g_free,
                                                          NULL); /* pointer to the mmapped file */
}

static gboolean
read_mo_file (MoFile *self, GError **error)
{
        g_return_val_if_fail (MO_IS_FILE (self), FALSE);

        int fd;
        struct stat sb;

        fd = open (self->filename, O_RDONLY);

        if ((fd = open (self->filename, O_RDONLY)) < 0) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "'%s' could not be opened: '%s'.", self->filename, strerror (errno),
                             NULL);
                goto fail;
        }

        if (fstat (fd, &sb) < 0) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "'%s' could not be statted: '%s'.", self->filename, strerror (errno),
                             NULL);
                goto fail;
        }


        if ((size_t) sb.st_size < sizeof (MoFileHeader)) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "'%s' doesn't contain a valid header, cannot read.", self->filename,
                             NULL);
                goto fail;
        }

        self->length = sb.st_size;

        if ((self->mmapped_file = mmap (NULL, self->length, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "Couldn't mmap '%s': '%s'", self->filename, strerror (errno),
                             NULL);
                goto fail;
        }

        close (fd);

        memcpy (&self->header, self->mmapped_file, sizeof (MoFileHeader));

        if (self->header.hash_tab_offset == 0) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "'%s' doesn't contain a hash table, cannot read.", self->filename,
                             NULL);
                goto fail;
        }

        if (self->header.magic == 0x950412de) {
                self->swapped = FALSE;
        } else if (self->header.magic == 0xde120495) {
                self->swapped = TRUE;
        } else {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "'%s' contains unrecognisable magic bits, cannot read.", self->filename,
                             NULL);
                goto fail;
        }

        return TRUE;

fail:
        memset (&self->header, 0, sizeof (MoFileHeader));
        return FALSE;

}

const gchar *
mo_file_get_name (MoFile *self)
{
        if (!MO_IS_FILE (self))
                return NULL;

        return self->filename;
}

// This is just the common hashpjw routine, pasted in:

#define HASHWORDBITS 32

static inline guint32 hashpjw (const gchar *str_param)
{
        g_return_val_if_fail (str_param != NULL, 0);

        guint32 hval = 0;
        guint32 g;
        const gchar *s = str_param;

        while (*s) {
                hval <<= 4;
                hval += (unsigned char) *s++;
                g = hval & ((guint32) 0xf << (HASHWORDBITS - 4));
                if (g != 0) {
                        hval ^= g >> (HASHWORDBITS - 8);
                        hval ^= g;
                }
        }

        return hval;
}

static inline guint32
get_uint32 (const gchar *data, size_t offset, gboolean swap)
{
        g_return_val_if_fail (data != NULL, 0);

        guint32 *ptr = (guint32 *) (data + offset);
        if (swap)
                return GUINT32_SWAP_LE_BE (*ptr);
        else
                return *ptr;
}

static inline const char *
get_string (const gchar *data, guint32 offset, guint32 index, gboolean swapped)
{

        g_return_val_if_fail (data != NULL, NULL);

        size_t s_offset = get_uint32 (data,
                        offset + index * sizeof (char *) + sizeof (guint32),
                        swapped);
        return data + s_offset;
}

static const gchar *
get_translation (MoFile *self,
                 const gchar *trans)
{
        g_return_val_if_fail (MO_IS_FILE (self), NULL);
        g_return_val_if_fail (self->filename != NULL, NULL);
        g_return_val_if_fail (self->header.hash_tab_offset != 0, NULL);

        unsigned long V = hashpjw (trans);
        int S = self->header.hash_tab_size;

        int hash_cursor = V % S;
        int orig_hash_cursor = hash_cursor;
        int increment = 1 + (V % (S - 2));

        int idx;

        while (1) {
                unsigned int index = get_uint32 (self->mmapped_file,
                                                 self->header.hash_tab_offset +
                                                        sizeof (guint32) * hash_cursor,
                                                 self->swapped);
                if (index == 0)
                        return NULL;

                index--;

                if (g_strcmp0 (get_string (self->mmapped_file,
                                           self->header.orig_tab_offset,
                                           index,
                                           self->swapped),
                               trans) == 0) {
                        idx = index;
                        break;
                }

                hash_cursor += increment;
                hash_cursor %= S;

                if (hash_cursor == orig_hash_cursor)
                        return NULL;
        }

        return get_string (self->mmapped_file,
                           self->header.trans_tab_offset,
                           idx,
                           self->swapped);
}

gchar *
mo_file_get_translation (MoFile *self, const gchar *str)
{
        if (!MO_IS_FILE (self) || !str || !self->filename || self->header.nstrings == 0)
                return NULL;

        gboolean found;

        const gchar *trans;

        found = g_hash_table_lookup_extended (self->translations_cache,
                                              str,
                                              NULL,
                                              (gpointer) &trans);

        if (!found) {
                trans = get_translation (self, str);
                g_hash_table_insert (self->translations_cache, g_strdup (str), (gchar *) trans);
        }

        return g_strdup (trans);
}

MoFile *
mo_file_new (const gchar *filename, GError **error)
{
        return MO_FILE (g_initable_new (MO_TYPE_FILE,
                                        NULL,
                                        error,
                                        "filename", filename,
                                        NULL));
}
