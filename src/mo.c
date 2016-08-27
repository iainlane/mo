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

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define GET_PRIVATE(x) (x)->priv;

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
        char *mmaped_file;
        off_t length;
};

enum {
        PROP_FILENAME = 1,
        N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (MoFile, mo_file, G_TYPE_OBJECT)

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
            mo_file_set_name (self,
                              (gchar *) g_value_get_string (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
clear_file (MoFile *self)
{
        if (self->mmaped_file) {
                munmap (self->mmaped_file, self->length);
                self->mmaped_file = NULL;
        }

        g_free (self->filename);
        g_hash_table_remove_all (self->translations_cache);
}


static void
mo_file_dispose (GObject *object)
{
        MoFile *self G_GNUC_UNUSED = MO_FILE (object);

        G_OBJECT_CLASS (mo_file_parent_class)->dispose (object);
}

static void
mo_file_finalize (GObject *object)
{
        MoFile *self = MO_FILE (object);

        clear_file (self);

        G_OBJECT_CLASS (mo_file_parent_class)->finalize (object);
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
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

        g_object_class_install_properties (object_class,
                                           N_PROPERTIES,
                                           obj_properties);
}

static void
mo_file_init (MoFile *self)
{
        self->translations_cache = g_hash_table_new_full (g_str_hash,
                                                          g_str_equal,
                                                          g_free,
                                                          NULL);
}

MoFile *
mo_file_new (const gchar *filename)
{
        return MO_FILE (g_object_new (MO_TYPE_FILE,
                                      "filename", filename,
                                      NULL));
}

static void
read_mo_file (MoFile *self)
{
        int fd;
        struct stat sb;

        /* XXX */
        fd = open (self->filename, O_RDONLY);

        /* XXX */
        fstat (fd, &sb);
        self->length = sb.st_size;

        self->mmaped_file = mmap (NULL, self->length, PROT_READ, MAP_PRIVATE, fd, 0);

        //close (fd);

        memcpy (&self->header, self->mmaped_file, sizeof (MoFileHeader));

        if (self->header.magic == 0x950412de)
                self->swapped = FALSE;
        else if (self->header.magic == 0xde120495)
                self->swapped = TRUE;
        else
                g_assert_not_reached ();
}

void
mo_file_set_name (MoFile *self, const gchar *filename)
{
        g_return_if_fail (MO_IS_FILE (self));
        g_return_if_fail (filename != NULL);

        if (self->filename != filename) {
                clear_file (self);
                self->filename = g_strdup (filename);
                read_mo_file (self);
                g_object_notify_by_pspec (G_OBJECT (self),
                                          obj_properties[PROP_FILENAME]);
        }
}

const gchar *
mo_file_get_name (MoFile *self)
{
        g_return_val_if_fail (MO_IS_FILE (self), NULL);

        return self->filename;
}

// This is just the common hashpjw routine, pasted in:

#define HASHWORDBITS 32

static inline guint32 hashpjw (const gchar *str_param)
{
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
        guint32 *ptr = (guint32 *) (data + offset);
        if (swap)
                return GUINT32_SWAP_LE_BE (*ptr);
        else
                return *ptr;
}

static inline const char *
get_string (const gchar *data, guint32 offset, guint32 index, gboolean swapped)
{

        size_t s_offset = get_uint32 (data,
                        offset + index * sizeof (char *) + sizeof (guint32),
                        swapped);
        return data + s_offset;
}

static const gchar *
get_translation (MoFile *self,
                 const gchar *trans)
{
        unsigned long V = hashpjw (trans);
        int S = self->header.hash_tab_size;

        int hash_cursor = V % S;
        int orig_hash_cursor = hash_cursor;
        int increment = 1 + (V % (S - 2));

        int idx;

        while (1) {
                unsigned int index = get_uint32 (self->mmaped_file,
                                                 self->header.hash_tab_offset +
                                                        sizeof (guint32) * hash_cursor,
                                                 self->swapped);
                if (index == 0)
                        return NULL;

                index--;

                if (g_strcmp0 (get_string (self->mmaped_file,
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

        return get_string (self->mmaped_file,
                           self->header.trans_tab_offset,
                           idx,
                           self->swapped);
}

gchar *
mo_file_get_translation (MoFile *self, const gchar *str)
{
        g_return_val_if_fail (MO_IS_FILE (self), NULL);
        g_return_val_if_fail (str != NULL, NULL);

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

int
main (int argc,
      char *argv[])
{
        MoFile *mofile = mo_file_new ("/usr/share/locale-langpack/fr/LC_MESSAGES/apt.mo");

        g_autofree gchar *trans = mo_file_get_translation (mofile,
                                                           "edit the source information file");

        g_printf ("%s\n", trans);

        g_object_unref (mofile);
}
