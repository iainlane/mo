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

G_BEGIN_DECLS

typedef enum {
        MO_FILE_INVALID_FILE_ERROR,
        MO_FILE_NO_SUCH_FILE_ERROR,
} MoFileError;

#define MO_TYPE_FILE mo_file_get_type ()

G_DECLARE_FINAL_TYPE (MoFile, mo_file, MO, FILE, GObject)

#define MO_FILE_ERROR mo_file_error_quark ()

MoFile *mo_file_new (const gchar *filename);
gboolean mo_file_set_name (MoFile *self, const gchar *filename, GError **error);
const gchar *mo_file_get_name (MoFile *self);

gchar *mo_file_get_translation (MoFile *self, const gchar *str);

G_END_DECLS
