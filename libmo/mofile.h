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

#pragma once

#include <gio/gio.h>
#include <glib-object.h>

#if !(defined(_IN_MO_H) || defined(MO_COMPILATION))
#error "mofile.h must not be included individually, include mo.h instead"
#endif

G_BEGIN_DECLS

/**
 * MoFileError:
 * @MO_FILE_INVALID_FILE_ERROR: The file exists but could not be parsed. It is not a valid .mo file.
 * @MO_FILE_NO_SUCH_FILE_ERROR: The file did not exist.
 *
 * Error codes for operations on #MoFiles.
 */
typedef enum {
        MO_FILE_INVALID_FILE_ERROR,
        MO_FILE_NO_SUCH_FILE_ERROR,
        MO_FILE_STRING_NOT_FOUND_ERROR,
} MoFileError;

/**
 * MO_TYPE_FILE:
 *
 * #GType for #MoFile.
 */
#define MO_TYPE_FILE (mo_file_get_type ())

/**
 * MoFile:
 *
 * All the data fields in the #MoFile class are private and should never be accessed directly.
 */
G_DECLARE_FINAL_TYPE (MoFile, mo_file, MO, FILE, GObject)

/**
 * MO_FILE_ERROR:
 *
 * The error domain for #MoFile errors.
 */
#define MO_FILE_ERROR (mo_file_error_quark ())
GQuark mo_file_error_quark (void) G_GNUC_CONST;

MoFile *mo_file_new (const gchar *filename, GError **error);
const gchar *mo_file_get_name (MoFile *self);

gchar *mo_file_get_translation (MoFile *self, const gchar *str, GError **error);

GHashTable *mo_file_get_translations (MoFile *self, GError **error);

G_END_DECLS
