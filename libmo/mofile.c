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

#include <glib/gprintf.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * SECTION:mofile
 * @short_description: Work with binary translation .mo files.
 * @title: MoFile
 * @stability: unstable
 * @include: libmo/mo.h
 *
 * #MoFile is a class for reading .mo files, as generated by gettext's msgfmt
 * program. You can currently load and read translations from .mo files. More
 * functionality, including reading collections of files and maybe writing
 * files, is planned for the future.
 *
 * <example>
 * <title>Opening a .mo file and retrieving a translation from it.</title>
 *
 * <programlisting>
 *    GError *err = NULL;
 *
 *    g_autoptr(MoFile) mofile = mo_file_new ("/tmp/my-mo-file.mo", &err);
 *
 *    if (!mofile) {
 *            g_printerr ("couldn't get translation: %s\n", err->message);
 *    } else {
 *            g_autofree gchar *trans = mo_file_get_translation (mofile,
 *                                                               "edit the source information file");
 *            if (trans)
 *                    g_printf ("%s\n", trans);
 *            else
 *                    g_printerr ("no translation found\n");
 *    }
 * </programlisting>
 * </example>
 */

typedef struct {
        guint32 magic;
        guint32 revision;
        guint32 nstrings;
        guint32 orig_tab_offset;
        guint32 trans_tab_offset;
        guint32 hash_tab_size;
        guint32 hash_tab_offset;
        /* there are also 'sysdep' strings, which we don't handle currently */
} MoFileHeader;

struct _MoFile {
        GObject parent_instance;

        gchar *filename;
        GHashTable *translations_cache;
        MoFileHeader header;
        gboolean swapped;
        GBytes *bytes;
        guint8 *data;
        off_t length;
};

enum {
        PROP_FILENAME = 1,
        PROP_BYTES,
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
mo_file_error_quark (void)
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

        case PROP_BYTES:
            g_value_set_pointer (value, self->bytes);
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

        case PROP_BYTES:
            self->bytes = (GBytes *) g_value_get_pointer (value);
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

        if (self->data) {
                if (self->filename)
                        munmap (self->data, self->length);
                memset (&self->header, 0, sizeof (MoFileHeader));
                self->data = NULL;
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
        g_clear_pointer (&self->translations_cache, g_hash_table_destroy);

        G_OBJECT_CLASS (mo_file_parent_class)->finalize (object);
}

static gboolean
mo_file_initable_init_bytes (GInitable *init,
                             GCancellable *cancellable G_GNUC_UNUSED,
                             GError **error)
{
        MoFile *self;
        gsize length;
        gconstpointer b;

        g_assert (MO_IS_FILE (init));

        self = MO_FILE (init);

        g_assert (self->bytes);

        if (!(b = g_bytes_get_data (self->bytes, &length))) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "The bytes data must not be NULL.",
                             NULL);
                goto fail;
        }

        g_clear_pointer (&self->filename, g_free);

        if ((size_t) length <= sizeof (MoFileHeader)) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "'%s' doesn't contain a valid header, cannot read.", self->filename,
                             NULL);
                goto fail;
        }

        self->header = *(MoFileHeader *) b;

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

        self->length = (off_t) length;
        self->data = (guint8 *) b;

        return TRUE;

fail:
        memset (&self->header, 0, sizeof (MoFileHeader));
        return FALSE;
}

static gboolean
mo_file_initable_init_filename (GInitable *init,
                                GCancellable *cancellable G_GNUC_UNUSED,
                                GError **error)
{
        MoFile *self;

        g_assert (MO_IS_FILE (init));

        self = MO_FILE (init);

        g_assert (self->filename);

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

static gboolean
mo_file_initable_init_real (GInitable *init,
                            GCancellable *cancellable,
                            GError **error)
{
        MoFile *self;

        if (!MO_IS_FILE (init))
                return FALSE;

        self = MO_FILE (init);

        if (self->bytes)
                return mo_file_initable_init_bytes (init, cancellable, error);
        else if (self->filename)
                return mo_file_initable_init_filename (init, cancellable, error);

        return FALSE;
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

        /**
         * MoFile::filename:
         *
         * The filename that this #MoFile represents, if loaded from a file.
         */
        obj_properties[PROP_FILENAME] =
                g_param_spec_string ("filename",
                                     "Filename",
                                     "Name of the .mo file to load.",
                                     NULL  /* default value */,
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_STRINGS);

        /**
         * MoFile::bytes:
         *
         * The #GBytes that this #MoFile represents, if loaded from bytes in
         * memory.
         */
        obj_properties[PROP_BYTES] =
                g_param_spec_pointer ("bytes",
                                      "Bytes",
                                      "The bytes that this .mo file was loaded from.",
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
                                                          NULL); /* pointer to the mmapped file or in-memory bytes */
}

static gboolean
read_mo_file (MoFile *self, GError **error)
{
        int fd;
        struct stat sb;

        g_return_val_if_fail (MO_IS_FILE (self), FALSE);

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

        if ((self->data = mmap (NULL, self->length, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "Couldn't mmap '%s': '%s'", self->filename, strerror (errno),
                             NULL);
                goto fail;
        }

        close (fd);

        memcpy (&self->header, self->data, sizeof (MoFileHeader));

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

/**
 * mo_file_get_name:
 * @self: An initialised #MoFile.
 *
 * Get the filename of this #MoFile instance.
 *
 * Returns: (transfer none): The filename.
 */
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
        guint32 hval = 0;
        guint32 g;
        const gchar *s;

        g_return_val_if_fail (str_param != NULL, 0);

        s = str_param;

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

static inline size_t
osum (size_t a, size_t b)
{
        size_t sum = a + b;

        return sum >= a ? sum : G_MAXSIZE;
}

static inline size_t
osum3 (size_t a, size_t b, size_t c)
{
        return osum (osum (a, b), c);
}

static inline guint32
get_uint32 (const guint8 *data,
            size_t offset,
            gboolean swap,
            off_t length,
            GError **error)
{
        guint32 b0, b1, b2, b3, res;
        size_t sum;

        sum = osum (offset, sizeof(guint8) * 4);

        g_return_val_if_fail (length >= 0, G_MAXUINT32);

        if (sum == G_MAXSIZE || sum > (size_t) length) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "File is truncated.",
                             NULL);
                return G_MAXUINT32;
        }

        b0 = *(guint8 *) (data + offset);
        b1 = *(guint8 *) (data + offset + sizeof (guint8));
        b2 = *(guint8 *) (data + offset + sizeof (guint8) * 2);
        b3 = *(guint8 *) (data + offset + sizeof (guint8) * 3);

        res = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);

        if (swap)
                res = GUINT32_SWAP_LE_BE (res);

        if (res == G_MAXUINT32) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "File is truncated.",
                             NULL);
        }

        return res;
}

static inline const gchar *
get_string (const guint8 *data,
            guint32 offset,
            guint32 index,
            gboolean swapped,
            off_t length,
            size_t *lengthp,
            GError **error)
{
        size_t string_length, string_offset, string_end;

        g_return_val_if_fail (data != NULL, NULL);
        g_return_val_if_fail (length >= 0, NULL);

        string_length = get_uint32 (data, offset + index * sizeof (char *), swapped, length, error);
        if (string_length == G_MAXUINT) {
                return NULL;
        }

        string_offset = get_uint32 (data,
                                    offset + index * sizeof (char *) + sizeof (guint32),
                                    swapped,
                                    length,
                                    error);
        if (string_offset == G_MAXUINT) {
                return NULL;
        }

        /* See if we're overflowed or pointed off the end of the file */
        string_end = osum3 (string_offset, string_length, 1);

        if (string_end == G_MAXSIZE || string_end > (size_t) length) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "File is truncated.",
                             NULL);
                return NULL;
        }

        if (data[string_offset + string_length] != '\0') {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "File contains a non-NUL terminated string.",
                             NULL);
                return NULL;
        }

        /* We think we're ok now */
        if (lengthp)
                *lengthp = string_length + 1;

        return (const gchar *) data + string_offset;
}

static const gchar *
get_translation (MoFile *self,
                 const gchar *trans,
                 GError **error)
{
        int S, hash_cursor, orig_hash_cursor, increment, idx;
        unsigned int index;
        unsigned long V;
        const gchar *str;

        GError *err = NULL;

        g_return_val_if_fail (MO_IS_FILE (self), NULL);
        g_return_val_if_fail (self->filename != NULL || self->data != NULL, NULL);
        g_return_val_if_fail (self->header.hash_tab_offset != 0, NULL);

        V = hashpjw (trans);
        S = self->header.hash_tab_size;

        hash_cursor = V % S;
        orig_hash_cursor = hash_cursor;
        increment = 1 + (V % (S - 2));

        while (1) {
                index = get_uint32 (self->data,
                                    self->header.hash_tab_offset +
                                            sizeof (guint32) * hash_cursor,
                                    self->swapped,
                                    self->length,
                                    error);
                if (index == 0) {
                        g_set_error (error,
                                     MO_FILE_ERROR,
                                     MO_FILE_STRING_NOT_FOUND_ERROR,
                                     "Translation for '%s' not found in '%s'",
                                     trans,
                                     self->filename,
                                     NULL);
                        return NULL;
                }

                if (index == G_MAXUINT32)
                        return NULL;

                index--;

                str = get_string (self->data,
                                  self->header.orig_tab_offset,
                                  index,
                                  self->swapped,
                                  self->length,
                                  NULL, /* length */
                                  &err);
                if (err) {
                        g_propagate_error (error, err);
                        return NULL;
                }

                if (g_strcmp0 (str, trans) == 0) {
                        idx = index;
                        break;
                }

                hash_cursor += increment;
                hash_cursor %= S;

                if (hash_cursor == orig_hash_cursor) {
                        g_set_error (error,
                                     MO_FILE_ERROR,
                                     MO_FILE_STRING_NOT_FOUND_ERROR,
                                     "Translation for '%s' not found in '%s'",
                                     trans,
                                     self->filename,
                                     NULL);
                        return NULL;
                }
        }

        return get_string (self->data,
                           self->header.trans_tab_offset,
                           idx,
                           self->swapped,
                           self->length,
                           NULL, /* length */
                           error);
}

/**
 * mo_file_get_translation:
 * @self: An initialised #MoFile.
 * @str: Untranslated (in the 'C' locale) string.
 * @error: Return location for a GError, or NULL.
 *
 * Retrieve the translated value of a string.
 *
 * Returns (transfer full): the translated string, or NULL if a translation is
 * not found. If NULL is returned, @error will be set.
 */
gchar *
mo_file_get_translation (MoFile *self, const gchar *str, GError **error)
{
        gboolean found;
        const gchar *trans;

        if (!MO_IS_FILE (self) || !str || (!self->filename && !self->data)) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "The MoFile object is invalid.",
                             NULL);
                return NULL;
        }

        if (self->header.nstrings == 0) {
                g_set_error (error,
                             MO_FILE_ERROR,
                             MO_FILE_INVALID_FILE_ERROR,
                             "'%s' contains no strings",
                             self->filename,
                             NULL);
                return NULL;
        }

        found = g_hash_table_lookup_extended (self->translations_cache,
                                              str,
                                              NULL,
                                              (gpointer) &trans);

        if (!found) {
                trans = get_translation (self, str, error);
                g_hash_table_insert (self->translations_cache, g_strdup (str), (gchar *) trans);
        }

        return g_strdup (trans);
}

/**
 * mo_file_get_translations:
 * @self: An initialised #MoFile.
 *
 * Retrieve all translations.
 *
 * Returns: (element-type gchar* gchar*) (transfer full): A #GHashTable
 * containing a mapping from original to translated strings.
 */
GHashTable *
mo_file_get_translations (MoFile *self, GError **error)
{
        const gchar *orig, *trans;
        GHashTable *ret;

        if (!MO_IS_FILE (self) || !self->filename)
                return NULL;

        ret = g_hash_table_new_full (g_str_hash,
                                     g_str_equal,
                                     g_free,
                                     g_free);

        for (unsigned int i = 0; i < self->header.nstrings; ++i) {
                orig = get_string (self->data,
                                   self->header.orig_tab_offset,
                                   i,
                                   self->swapped,
                                   self->length,
                                   NULL,
                                   error);

                if (!orig) {
                        g_hash_table_unref (ret);
                        return NULL;
                }

                trans = get_string (self->data,
                                    self->header.trans_tab_offset,
                                    i,
                                    self->swapped,
                                    self->length,
                                    NULL,
                                    error);

                if (!trans) {
                        g_hash_table_unref (ret);
                        return NULL;
                }

                g_hash_table_insert (ret,
                                     g_strdup (orig),
                                     g_strdup (trans));
        }

        return ret;
}

/**
 * mo_file_new:
 * @filename: Filename of the .mo file to work with.
 * @error: Return location for a GError, or NULL.
 *
 * Create a new #MoFile, pointing to @filename.
 *
 * Returns: The new #MoFile.
 */
MoFile *
mo_file_new (const gchar *filename, GError **error)
{
        return MO_FILE (g_initable_new (MO_TYPE_FILE,
                                        NULL,
                                        error,
                                        "filename", filename,
                                        NULL));
}

MoFile *
mo_file_new_from_bytes (const GBytes *bytes, GError **error)
{
        return MO_FILE (g_initable_new (MO_TYPE_FILE,
                                        NULL,
                                        error,
                                        "bytes", bytes,
                                        NULL));
}
