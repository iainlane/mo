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
#error "mogroup.h must not be included individually, include mo.h instead"
#endif

G_BEGIN_DECLS

/**
 * MoGroupError:
 * @MO_GROUP_NO_SUCH_DIRECTORY_ERROR: The directory did not exist.
 *
 * Error codes for operations on #MoGroups.
 */
typedef enum {
        MO_GROUP_NO_SUCH_DIRECTORY_ERROR,
} MoGroupError;

/**
 * MO_TYPE_GROUP:
 *
 * #GType for #MoGroup.
 */
#define MO_TYPE_GROUP (mo_group_get_type ())

/**
 * MoGroup:
 *
 * All the data fields in the #MoGroup class are private and should never be accessed directly.
 */
G_DECLARE_FINAL_TYPE (MoGroup, mo_group, MO, GROUP, GObject)


/**
 * MO_GROUP_ERROR:
 *
 * The error domain for #MoGroup errors.
 */
#define MO_GROUP_ERROR (mo_group_error_quark ())
GQuark mo_group_error_quark (void) G_GNUC_CONST;

MoGroup *mo_group_new (const gchar *domain, GError **error);
MoGroup *mo_group_new_for_directory (const gchar *domain,
                                     const gchar *directory,
                                     GError **error);

const gchar *mo_group_get_directory (MoGroup *self);
const gchar *mo_group_get_domain (MoGroup *self);

GList *mo_group_get_languages (MoGroup *self);
GHashTable *mo_group_get_translations (MoGroup *self, const gchar *translation);
MoFile *mo_group_get_mo_file (MoGroup *self, const gchar *locale);
gchar *mo_group_get_translation (MoGroup *self,
                                 const gchar *locale,
                                 const gchar *translation,
                                 GError **err);

G_END_DECLS
