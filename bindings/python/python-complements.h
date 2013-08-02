/*
 * python-complements.h
 *
 * Babeltrace Python module complements header, required for Python bindings
 *
 * Copyright 2012 EfficiOS Inc.
 *
 * Author: Danny Serres <danny.serres@efficios.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#include <stdio.h>
#include <glib.h>
#include <babeltrace/babeltrace.h>
#include <babeltrace/format.h>
#include <babeltrace/ctf-ir/metadata.h>
#include <babeltrace/ctf/events.h>
#include <babeltrace/iterator-internal.h>
#include <babeltrace/ctf/events-internal.h>

/* File */
FILE *_bt_file_open(char *file_path, char *mode);
void _bt_file_close(FILE *fp);

/* ctf-field-list */
struct bt_definition **_bt_python_field_listcaller(
		const struct bt_ctf_event *ctf_event,
		const struct bt_definition *scope);
struct bt_definition *_bt_python_field_one_from_list(
		struct bt_definition **list, int index);

/* event_decl_list */
struct bt_ctf_event_decl **_bt_python_event_decl_listcaller(
		int handle_id, struct bt_context *ctx);
struct bt_ctf_event_decl *_bt_python_decl_one_from_list(
		struct bt_ctf_event_decl **list, int index);

/* decl_fields */
struct bt_ctf_field_decl **_by_python_field_decl_listcaller(
		struct bt_ctf_event_decl *event_decl,
		enum bt_ctf_scope scope);
struct bt_ctf_field_decl *_bt_python_field_decl_one_from_list(
		struct bt_ctf_field_decl **list, int index);
struct definition_sequence *_bt_python_from_def_to_sequence(
		struct bt_definition *field);
