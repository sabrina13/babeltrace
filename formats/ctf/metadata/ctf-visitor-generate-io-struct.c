/*
 * ctf-visitor-generate-io-struct.c
 *
 * Common Trace Format Metadata Visitor (generate I/O structures).
 *
 * Copyright 2010 - Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <glib.h>
#include <inttypes.h>
#include <errno.h>
#include <babeltrace/list.h>
#include "ctf-scanner.h"
#include "ctf-parser.h"
#include "ctf-ast.h"

#define fprintf_dbg(fd, fmt, args...)	fprintf(fd, "%s: " fmt, __func__, ## args)

#define _cds_list_first_entry(ptr, type, member)	\
	cds_list_entry((ptr)->next, type, member)

static
int ctf_visitor_print_type_specifier(FILE *fd, int depth, struct ctf_node *node)
{
	print_tabs(fd, depth);
	fprintf(fd, "<type_specifier \"");

	switch (node->u.type_specifier.type) {
	case TYPESPEC_VOID:
		fprintf(fd, "void");
		break;
	case TYPESPEC_CHAR:
		fprintf(fd, "char");
		break;
	case TYPESPEC_SHORT:
		fprintf(fd, "short");
		break;
	case TYPESPEC_INT:
		fprintf(fd, "int");
		break;
	case TYPESPEC_LONG:
		fprintf(fd, "long");
		break;
	case TYPESPEC_FLOAT:
		fprintf(fd, "float");
		break;
	case TYPESPEC_DOUBLE:
		fprintf(fd, "double");
		break;
	case TYPESPEC_SIGNED:
		fprintf(fd, "signed");
		break;
	case TYPESPEC_UNSIGNED:
		fprintf(fd, "unsigned");
		break;
	case TYPESPEC_BOOL:
		fprintf(fd, "bool");
		break;
	case TYPESPEC_COMPLEX:
		fprintf(fd, "complex");
		break;
	case TYPESPEC_CONST:
		fprintf(fd, "const");
		break;
	case TYPESPEC_ID_TYPE:
		fprintf(fd, "%s", node->u.type_specifier.id_type);
		break;

	case TYPESPEC_UNKNOWN:
	default:
		fprintf(stderr, "[error] %s: unknown type specifier %d\n", __func__,
			(int) node->u.type_specifier.type);
		return -EINVAL;
	}
	fprintf(fd, "\"/>\n");
	return 0;
}

static
int ctf_visitor_print_type_declarator(FILE *fd, int depth, struct ctf_node *node)
{
	int ret = 0;
	struct ctf_node *iter;

	print_tabs(fd, depth);
	fprintf(fd, "<type_declarator>\n");
	depth++;

	if (!cds_list_empty(&node->u.type_declarator.pointers)) {
		print_tabs(fd, depth);
		fprintf(fd, "<pointers>\n");
		cds_list_for_each_entry(iter, &node->u.type_declarator.pointers,
					siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</pointers>\n");
	}

	switch (node->u.type_declarator.type) {
	case TYPEDEC_ID:
		if (node->u.type_declarator.u.id) {
			print_tabs(fd, depth);
			fprintf(fd, "<id \"");
			fprintf(fd, "%s", node->u.type_declarator.u.id);
			fprintf(fd, "\" />\n");
		}
		break;
	case TYPEDEC_NESTED:
		if (node->u.type_declarator.u.nested.type_declarator) {
			print_tabs(fd, depth);
			fprintf(fd, "<type_declarator>\n");
			ret = ctf_visitor_print_xml(fd, depth + 1,
				node->u.type_declarator.u.nested.type_declarator);
			if (ret)
				return ret;
			print_tabs(fd, depth);
			fprintf(fd, "</type_declarator>\n");
		}
		if (node->u.type_declarator.u.nested.length) {
			print_tabs(fd, depth);
			fprintf(fd, "<length>\n");
			ret = ctf_visitor_print_xml(fd, depth + 1,
				node->u.type_declarator.u.nested.length);
			if (ret)
				return ret;
			print_tabs(fd, depth);
			fprintf(fd, "</length>\n");
		}
		if (node->u.type_declarator.u.nested.abstract_array) {
			print_tabs(fd, depth);
			fprintf(fd, "<length>\n");
			print_tabs(fd, depth);
			fprintf(fd, "</length>\n");
		}
		if (node->u.type_declarator.bitfield_len) {
			print_tabs(fd, depth);
			fprintf(fd, "<bitfield_len>\n");
			ret = ctf_visitor_print_xml(fd, depth + 1,
				node->u.type_declarator.bitfield_len);
			if (ret)
				return ret;
			print_tabs(fd, depth);
			fprintf(fd, "</bitfield_len>\n");
		}
		break;
	case TYPEDEC_UNKNOWN:
	default:
		fprintf(stderr, "[error] %s: unknown type declarator %d\n", __func__,
			(int) node->u.type_declarator.type);
		return -EINVAL;
	}

	depth--;
	print_tabs(fd, depth);
	fprintf(fd, "</type_declarator>\n");
	return 0;
}

/*
 * String returned must be freed by the caller.
 */
static
char *concatenate_unary_strings(struct list_head *head)
{

}

static
int get_unary_unsigned(struct list_head *head, uint64_t *value)
{

}

static
int get_unary_uuid(struct list_head *head, uuid_t *uuid)
{

}

static
struct ctf_stream *trace_stream_lookup(struct ctf_trace *trace, uint64_t stream_id)
{
	if (trace->streams->len <= stream_id)
		return NULL;
	return g_ptr_array_index(trace->streams, stream_id);
}

/*
 * Also add named variant, struct or enum to the current type scope.
 */
static
struct ctf_type *ctf_type_specifier_visit(FILE *fd,
					  int depth, struct list_head *head,
					  struct type_scope *type_scope,
					  struct declaration_scope *declaration_scope)
{
	struct ctf_type *type;
	struct node *first;

	first = _cds_list_first_entry(head, struct node, siblings);

	switch (first->type) {
	case NODE_STRUCT:
		/* For named struct (without body), lookup in type scope */
		/* For named struct (with body), create type and add to type scope */
		/* For unnamed struct, create type */
		break;
	case NODE_VARIANT:
		/* For named variant (without body), lookup in type scope */
		/* For named variant (with body), create type and add to type scope */
		/* For unnamed variant, create type */
		break;
	case NODE_ENUM:
		/* For named enum (without body), lookup in type scope */
		/* For named enum (with body), create type and add to type scope */
		/* For unnamed enum, create type */
		break;
	case NODE_INTEGER:
		break;
	case NODE_FLOATING_POINT:
		break;
	case NODE_TYPE_SPECIFIER:
		break;

	}
}

static
struct ctf_declaration *ctf_declaration_specifier_visit(FILE *fd,
					int depth, struct list_head *head,
					struct type_scope *type_scope,
					struct declaration_scope *declaration_scope)
{
	struct ctf_declaration *declaration;
	struct ctf_type *type;

	type = ctf_type_specifier_visit(fd, depth, head, type_scope,
					declaration_scope);
	declaration = type->declaration_new(type, declaration_scope);
	if (type->id == CTF_TYPE_VARIANT) {
		struct declaration_variant *variant =
			container_of(declaration, struct declaration_variant, p);
		struct declaration *enum_tag =
			lookup_field_declaration(enum_tag_name, declaration_scope);
		if (!enum_tag) {
			fprintf(fd, "[error]: expected enumeration tag field %s for variant\n",
				enum_tag_name);
			goto error;
		}
		/* TODO find enum tag */
		ret = variant_declaration_set_tag(variant, enum_tag);
	}
	return declaration;
error:
	declaration_unref(declaration);
	type_unref(type);
	return NULL;
}

static
int ctf_typedef_declarator_visit(FILE *fd, int depth,
				struct list_head *declaration_specifier,
				struct node *type_declarator, struct type_scope *type_scope,
				struct declaration_scope *declaration_scope)
{
	/*
	 * Build the type using declaration specifier (creating
	 * declaration from type_specifier), then apply type declarator,
	 * add the resulting type to the current type scope.
	 */
	cds_list_for_each_entry(iter, declaration_specifier, siblings) {


	}
	return 0;
}

static
int ctf_typedef_visit(FILE *fd, int depth,
		      struct list_head *declaration_specifier,
		      struct list_head *type_declarators,
		      struct type_scope *type_scope,
		      struct declaration_scope *declaration_scope)
{
	struct ctf_node *iter;

	cds_list_for_each_entry(iter, type_declarators, siblings) {
		ret = ctf_typedef_declarator_visit(fd, depth + 1,
			&node->u._typedef.declaration_specifier, iter,
			type_scope, declaration_scope);
		if (ret)
			return ret;
	}
	return 0;
}

static
int ctf_typealias_visit(FILE *fd, int depth, struct ctf_node *target,
			struct ctf_node *alias, struct type_scope *type_scope,
			struct declaration_scope *declaration_scope)
{
	/* Build target type, check that it is reachable in current type scope. */
	/* Only one type declarator is allowed */

	/* Build alias type, add to current type scope. */
	/* Only one type declarator is allowed */
}

static
int ctf_event_declaration_visit(FILE *fd, int depth, struct ctf_node *node, struct ctf_event *event, struct ctf_trace *trace)
{
	int ret = 0;

	switch (node->type) {
	case NODE_TYPEDEF:
		ret = ctf_typedef_visit(fd, depth + 1,
					&node->u._typedef.declaration_specifier,
					&node->u._typedef.type_declarators,
					event->type_scope, event->declaration_scope);
		if (ret)
			return ret;
		break;
	case NODE_TYPEALIAS:
		ret = ctf_typealias_visit(fd, depth + 1,
				&node->u.typealias.target, &node->u.typealias.alias
				event->type_scope, event->declaration_scope);
		if (ret)
			return ret;
		break;
	case NODE_CTF_EXPRESSION:
	{
		char *left;

		left = concatenate_unary_strings(&node->u.ctf_expression.left);
		if (!strcmp(left, "name")) {
			char *right;

			if (CTF_EVENT_FIELD_IS_SET(event, name))
				return -EPERM;
			right = concatenate_unary_strings(&node->u.ctf_expression.right);
			if (!right) {
				fprintf(stderr, "[error] %s: unexpected unary expression for event name\n", __func__);
				return -EINVAL;
			}
			event->name = g_quark_from_string(right);
			free(right);
			CTF_EVENT_SET_FIELD(event, name);
		} else if (!strcmp(left, "id")) {
			if (CTF_EVENT_FIELD_IS_SET(event, id))
				return -EPERM;
			ret = get_unary_unsigned(&node->u.ctf_expression.right, &event->id);
			if (ret) {
				fprintf(stderr, "[error] %s: unexpected unary expression for event id\n", __func__);
				return -EINVAL;
			}
			CTF_EVENT_SET_FIELD(event, id);
		} else if (!strcmp(left, "stream_id")) {
			if (CTF_EVENT_FIELD_IS_SET(event, stream_id))
				return -EPERM;
			ret = get_unary_unsigned(&node->u.ctf_expression.right, &event->stream_id);
			if (ret) {
				fprintf(stderr, "[error] %s: unexpected unary expression for event stream_id\n", __func__);
				return -EINVAL;
			}
			event->stream = trace_stream_lookup(trace, event->stream_id);
			if (!event->stream) {
				fprintf(stderr, "[error] %s: stream id %" PRIu64 " cannot be found\n", __func__, event->stream_id);
				return -EINVAL;
			}
			event->declaration_scope = new_declaration_scope(stream->declaration_scope);
			if (!event->declaration_scope) {
				fprintf(stderr, "[error] %s: Error allocating declaration scope\n", __func__);
				return -EPERM;
			}
			CTF_EVENT_SET_FIELD(event, stream_id);
		} else if (!strcmp(left, "context")) {
			struct declaration *declaration;

			if (!event->declaration_scope)
				return -EPERM;
			declaration = ctf_declaration_specifier_visit(fd, depth,
					&node->u.ctf_expression.right,
					event->type_scope, event->declaration_scope);
			if (!declaration)
				return -EPERM;
			if (declaration->type->id != CTF_TYPE_STRUCT)
				return -EPERM;
			event->context = container_of(declaration, struct declaration_struct, p);
		} else if (!strcmp(left, "fields")) {
			struct declaration *declaration;

			if (!event->declaration_scope)
				return -EPERM;
			declaration = ctf_declaration_specifier_visit(fd, depth,
					&node->u.ctf_expression.right,
					event->type_scope, event->declaration_scope);
			if (!declaration)
				return -EPERM;
			if (declaration->type->id != CTF_TYPE_STRUCT)
				return -EPERM;
			event->fields = container_of(declaration, struct declaration_struct, p);
		}
		free(left);
		break;
	}
	default:
		return -EPERM;
	/* TODO: declaration specifier should be added. */
	}

	return 0;
}

static
int ctf_event_visit(FILE *fd, int depth, struct ctf_node *node,
		    struct type_scope *parent_type_scope, struct ctf_trace *trace)
{
	int ret = 0;
	struct ctf_node *iter;
	struct ctf_event *event;

	event = g_new0(struct ctf_event, 1);
	event->type_scope = new_type_scope(parent_type_scope);
	cds_list_for_each_entry(iter, &node->u.event.declaration_list, siblings) {
		ret = ctf_event_declaration_visit(fd, depth + 1, iter, event, trace);
		if (ret)
			goto error;
	}
	if (!CTF_EVENT_FIELD_IS_SET(event, name)) {
		ret = -EPERM;
		goto error;
	}
	if (!CTF_EVENT_FIELD_IS_SET(event, id)) {
		ret = -EPERM;
		goto error;
	}
	if (!CTF_EVENT_FIELD_IS_SET(event, stream_id)) {
		ret = -EPERM;
		goto error;
	}
	if (event->stream->events_by_id->len <= event->id)
		g_ptr_array_set_size(event->stream->events_by_id, event->id + 1);
	g_ptr_array_index(event->stream->events_by_id, event->id) = event;
	g_hash_table_insert(event->stream->event_quark_to_id,
			    (gpointer)(unsigned long) event->name,
			    &event->id);
	return 0;

error:
	declaration_unref(event->fields);
	declaration_unref(event->context);
	free_declaration_scope(event->declaration_scope);
	free_type_scope(event->type_scope);
	g_free(event);
	return ret;
}

 
static
int ctf_stream_declaration_visit(FILE *fd, int depth, struct ctf_node *node, struct ctf_stream *stream, struct ctf_trace *trace)
{
	int ret = 0;

	switch (node->type) {
	case NODE_TYPEDEF:
		ret = ctf_typedef_visit(fd, depth + 1,
					&node->u._typedef.declaration_specifier,
					&node->u._typedef.type_declarators,
					stream->type_scope, stream->declaration_scope);
		if (ret)
			return ret;
		break;
	case NODE_TYPEALIAS:
		ret = ctf_typealias_visit(fd, depth + 1,
				&node->u.typealias.target, &node->u.typealias.alias
				stream->type_scope, stream->declaration_scope);
		if (ret)
			return ret;
		break;
	case NODE_CTF_EXPRESSION:
	{
		char *left;

		left = concatenate_unary_strings(&node->u.ctf_expression.left);
		if (!strcmp(left, "stream_id")) {
			if (CTF_EVENT_FIELD_IS_SET(event, stream_id))
				return -EPERM;
			ret = get_unary_unsigned(&node->u.ctf_expression.right, &event->stream_id);
			if (ret) {
				fprintf(stderr, "[error] %s: unexpected unary expression for event stream_id\n", __func__);
				return -EINVAL;
			}
			CTF_EVENT_SET_FIELD(event, stream_id);
		} else if (!strcmp(left, "event_header")) {
			struct declaration *declaration;

			declaration = ctf_declaration_specifier_visit(fd, depth,
					&node->u.ctf_expression.right,
					stream->type_scope, stream->declaration_scope);
			if (!declaration)
				return -EPERM;
			if (declaration->type->id != CTF_TYPE_STRUCT)
				return -EPERM;
			stream->event_header = container_of(declaration, struct declaration_struct, p);
		} else if (!strcmp(left, "event_context")) {
			struct declaration *declaration;

			declaration = ctf_declaration_specifier_visit(fd, depth,
					&node->u.ctf_expression.right,
					stream->type_scope, stream->declaration_scope);
			if (!declaration)
				return -EPERM;
			if (declaration->type->id != CTF_TYPE_STRUCT)
				return -EPERM;
			stream->event_context = container_of(declaration, struct declaration_struct, p);
		} else if (!strcmp(left, "packet_context")) {
			struct declaration *declaration;

			declaration = ctf_declaration_specifier_visit(fd, depth,
					&node->u.ctf_expression.right,
					stream->type_scope, stream->declaration_scope);
			if (!declaration)
				return -EPERM;
			if (declaration->type->id != CTF_TYPE_STRUCT)
				return -EPERM;
			stream->packet_context = container_of(declaration, struct declaration_struct, p);
		}
		free(left);
		break;
	}
	default:
		return -EPERM;
	/* TODO: declaration specifier should be added. */
	}

	return 0;
}

static
int ctf_stream_visit(FILE *fd, int depth, struct ctf_node *node,
		     struct type_scope *parent_type_scope, struct ctf_trace *trace)
{
	int ret = 0;
	struct ctf_node *iter;
	struct ctf_stream *stream;

	stream = g_new0(struct ctf_stream, 1);
	stream->type_scope = new_type_scope(parent_type_scope);
	stream->declaration_scope = new_declaration_scope(trace->declaration_scope);
	stream->events_by_id = g_ptr_array_new();
	stream->event_quark_to_id = g_hash_table_new(g_int_hash, g_int_equal);
	cds_list_for_each_entry(iter, &node->u.stream.declaration_list, siblings) {
		ret = ctf_stream_declaration_visit(fd, depth + 1, iter, stream, trace);
		if (ret)
			goto error;
	}
	if (!CTF_EVENT_FIELD_IS_SET(stream, stream_id)) {
		ret = -EPERM;
		goto error;
	}
	if (trace->streams->len <= stream->stream_id)
		g_ptr_array_set_size(trace->streams, stream->stream_id + 1);
	g_ptr_array_index(trace->streams, stream->stream_id) = stream;
	return 0;

error:
	declaration_unref(stream->event_header);
	declaration_unref(stream->event_context);
	declaration_unref(stream->packet_context);
	g_ptr_array_free(stream->events_by_id, TRUE);
	g_hash_table_free(stream->event_quark_to_id);
	free_declaration_scope(stream->declaration_scope);
	free_type_scope(stream->type_scope);
	g_free(stream);
	return ret;
}

static
int ctf_trace_declaration_visit(FILE *fd, int depth, struct ctf_node *node, struct ctf_trace *trace)
{
	int ret = 0;

	switch (node->type) {
	case NODE_TYPEDEF:
		ret = ctf_typedef_visit(fd, depth + 1,
					&node->u._typedef.declaration_specifier,
					&node->u._typedef.type_declarators,
					trace->type_scope, trace->declaration_scope);
		if (ret)
			return ret;
		break;
	case NODE_TYPEALIAS:
		ret = ctf_typealias_visit(fd, depth + 1,
				&node->u.typealias.target, &node->u.typealias.alias
				trace->type_scope, trace->declaration_scope);
		if (ret)
			return ret;
		break;
	case NODE_CTF_EXPRESSION:
	{
		char *left;

		left = concatenate_unary_strings(&node->u.ctf_expression.left);
		if (!strcmp(left, "major")) {
			if (CTF_EVENT_FIELD_IS_SET(trace, major))
				return -EPERM;
			ret = get_unary_unsigned(&node->u.ctf_expression.right, &trace->major);
			if (ret) {
				fprintf(stderr, "[error] %s: unexpected unary expression for trace major number\n", __func__);
				return -EINVAL;
			}
			CTF_EVENT_SET_FIELD(trace, major);
		} else if (!strcmp(left, "minor")) {
			if (CTF_EVENT_FIELD_IS_SET(trace, minor))
				return -EPERM;
			ret = get_unary_unsigned(&node->u.ctf_expression.right, &trace->minor);
			if (ret) {
				fprintf(stderr, "[error] %s: unexpected unary expression for trace minor number\n", __func__);
				return -EINVAL;
			}
			CTF_EVENT_SET_FIELD(trace, minor);
		} else if (!strcmp(left, "word_size")) {
			if (CTF_EVENT_FIELD_IS_SET(trace, word_size))
				return -EPERM;
			ret = get_unary_unsigned(&node->u.ctf_expression.right, &trace->word_size);
			if (ret) {
				fprintf(stderr, "[error] %s: unexpected unary expression for trace word_size\n", __func__);
				return -EINVAL;
			}
			CTF_EVENT_SET_FIELD(trace, word_size);
		} else if (!strcmp(left, "uuid")) {
			if (CTF_EVENT_FIELD_IS_SET(trace, uuid))
				return -EPERM;
			ret = get_unary_uuid(&node->u.ctf_expression.right, &trace->uuid);
			if (ret) {
				fprintf(stderr, "[error] %s: unexpected unary expression for trace uuid\n", __func__);
				return -EINVAL;
			}
			CTF_EVENT_SET_FIELD(trace, uuid);
		}
		free(left);
		break;
	}
	default:
		return -EPERM;
	/* TODO: declaration specifier should be added. */
	}

	return 0;
}


static
int ctf_trace_visit(FILE *fd, int depth, struct ctf_node *node, struct ctf_trace *trace)
{
	int ret = 0;
	struct ctf_node *iter;

	if (trace->type_scope)
		return -EEXIST;
	trace->type_scope = new_type_scope(trace->root_type_scope);
	trace->declaration_scope = new_declaration_scope(trace->root_declaration_scope);
	trace->streams = g_ptr_array_new();
	cds_list_for_each_entry(iter, &node->u.trace.declaration_list, siblings) {
		ret = ctf_trace_declaration_visit(fd, depth + 1, iter, trace);
		if (ret)
			goto error;
	}
	if (!CTF_EVENT_FIELD_IS_SET(trace, major)) {
		ret = -EPERM;
		goto error;
	}
	if (!CTF_EVENT_FIELD_IS_SET(trace, minor)) {
		ret = -EPERM;
		goto error;
	}
	if (!CTF_EVENT_FIELD_IS_SET(trace, uuid)) {
		ret = -EPERM;
		goto error;
	}
	if (!CTF_EVENT_FIELD_IS_SET(trace, word_size)) {
		ret = -EPERM;
		goto error;
	}
	return 0;

error:
	g_ptr_array_free(trace->streams, TRUE);
	free_declaration_scope(stream->declaration_scope);
	free_type_scope(stream->type_scope);
	return ret;
}

int _ctf_visitor(FILE *fd, int depth, struct ctf_node *node, struct ctf_trace *trace)
{
	int ret = 0;
	struct ctf_node *iter;

	switch (node->type) {
	case NODE_ROOT:
		cds_list_for_each_entry(iter, &node->u.root._typedef,
					siblings) {
			ret = ctf_typedef_visit(fd, depth + 1,
						&iter->u._typedef.declaration_specifier,
						&iter->u._typedef.type_declarators,
						trace->type_scope, trace->declaration_scope);
			if (ret)
				return ret;
		}
		cds_list_for_each_entry(iter, &node->u.root.typealias,
					siblings) {
			ret = ctf_typealias_visit(fd, depth + 1,
					&iter->u.typealias.target, &iter->u.typealias.alias
					trace->type_scope, trace->declaration_scope);
			if (ret)
				return ret;
		}
		cds_list_for_each_entry(iter, &node->u.root.declaration_specifier, siblings) {
			ret = ctf_declaration_specifier_visit(fd, depth, iter,
					trace->root_type_scope,
					trace->root_declaration_scope);
			if (ret)
				return ret;
		}
		cds_list_for_each_entry(iter, &node->u.root.trace, siblings) {
			ret = ctf_trace_visit(fd, depth + 1, iter, trace);
			if (ret)
				return ret;
		}
		cds_list_for_each_entry(iter, &node->u.root.stream, siblings) {
			ret = ctf_stream_visit(fd, depth + 1, iter,
		    			       trace->type_scope, trace);
			if (ret)
				return ret;
		}
		cds_list_for_each_entry(iter, &node->u.root.event, siblings) {
			ret = ctf_event_visit(fd, depth + 1, iter,
		    			      trace->type_scope, trace);
			if (ret)
				return ret;
		}
		break;

	case NODE_TYPEALIAS_TARGET:
		print_tabs(fd, depth);
		fprintf(fd, "<target>\n");
		depth++;

		print_tabs(fd, depth);
		fprintf(fd, "<declaration_specifier>\n");
		cds_list_for_each_entry(iter, &node->u.typealias_target.declaration_specifier, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</declaration_specifier>\n");

		print_tabs(fd, depth);
		fprintf(fd, "<type_declarators>\n");
		cds_list_for_each_entry(iter, &node->u.typealias_target.type_declarators, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</type_declarators>\n");

		depth--;
		print_tabs(fd, depth);
		fprintf(fd, "</target>\n");
		break;
	case NODE_TYPEALIAS_ALIAS:
		print_tabs(fd, depth);
		fprintf(fd, "<alias>\n");
		depth++;

		print_tabs(fd, depth);
		fprintf(fd, "<declaration_specifier>\n");
		cds_list_for_each_entry(iter, &node->u.typealias_alias.declaration_specifier, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</declaration_specifier>\n");

		print_tabs(fd, depth);
		fprintf(fd, "<type_declarators>\n");
		cds_list_for_each_entry(iter, &node->u.typealias_alias.type_declarators, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</type_declarators>\n");

		depth--;
		print_tabs(fd, depth);
		fprintf(fd, "</alias>\n");
		break;
	case NODE_TYPEALIAS:
		print_tabs(fd, depth);
		fprintf(fd, "<typealias>\n");
		ret = ctf_visitor_print_xml(fd, depth + 1, node->u.typealias.target);
		if (ret)
			return ret;
		ret = ctf_visitor_print_xml(fd, depth + 1, node->u.typealias.alias);
		if (ret)
			return ret;
		print_tabs(fd, depth);
		fprintf(fd, "</typealias>\n");
		break;

	case NODE_TYPE_SPECIFIER:
		ret = ctf_visitor_print_type_specifier(fd, depth, node);
		if (ret)
			return ret;
		break;
	case NODE_POINTER:
		print_tabs(fd, depth);
		if (node->u.pointer.const_qualifier)
			fprintf(fd, "<const_pointer />\n");
		else
			fprintf(fd, "<pointer />\n");
		break;
	case NODE_TYPE_DECLARATOR:
		ret = ctf_visitor_print_type_declarator(fd, depth, node);
		if (ret)
			return ret;
		break;

	case NODE_FLOATING_POINT:
		print_tabs(fd, depth);
		fprintf(fd, "<floating_point>\n");
		cds_list_for_each_entry(iter, &node->u.floating_point.expressions, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</floating_point>\n");
		break;
	case NODE_INTEGER:
		print_tabs(fd, depth);
		fprintf(fd, "<integer>\n");
		cds_list_for_each_entry(iter, &node->u.integer.expressions, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</integer>\n");
		break;
	case NODE_STRING:
		print_tabs(fd, depth);
		fprintf(fd, "<string>\n");
		cds_list_for_each_entry(iter, &node->u.string.expressions, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</string>\n");
		break;
	case NODE_ENUMERATOR:
		print_tabs(fd, depth);
		fprintf(fd, "<enumerator");
		if (node->u.enumerator.id)
			fprintf(fd, " id=\"%s\"", node->u.enumerator.id);
		fprintf(fd, ">\n");
		cds_list_for_each_entry(iter, &node->u.enumerator.values, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</enumerator>\n");
		break;
	case NODE_ENUM:
		print_tabs(fd, depth);
		if (node->u._struct.name)
			fprintf(fd, "<enum name=\"%s\">\n",
				node->u._enum.enum_id);
		else
			fprintf(fd, "<enum >\n");
		depth++;

		if (node->u._enum.container_type) {
			print_tabs(fd, depth);
			fprintf(fd, "<container_type>\n");
			ret = ctf_visitor_print_xml(fd, depth + 1, node->u._enum.container_type);
			if (ret)
				return ret;
			print_tabs(fd, depth);
			fprintf(fd, "</container_type>\n");
		}

		print_tabs(fd, depth);
		fprintf(fd, "<enumerator_list>\n");
		cds_list_for_each_entry(iter, &node->u._enum.enumerator_list, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</enumerator_list>\n");

		depth--;
		print_tabs(fd, depth);
		fprintf(fd, "</enum>\n");
		break;
	case NODE_STRUCT_OR_VARIANT_DECLARATION:
		print_tabs(fd, depth);
		fprintf(fd, "<declaration_specifier>\n");
		cds_list_for_each_entry(iter, &node->u.struct_or_variant_declaration.declaration_specifier, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</declaration_specifier>\n");

		print_tabs(fd, depth);
		fprintf(fd, "<type_declarators>\n");
		cds_list_for_each_entry(iter, &node->u.struct_or_variant_declaration.type_declarators, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</type_declarators>\n");
		break;
	case NODE_VARIANT:
		print_tabs(fd, depth);
		fprintf(fd, "<variant");
		if (node->u.variant.name)
			fprintf(fd, " name=\"%s\"", node->u.variant.name);
		if (node->u.variant.choice)
			fprintf(fd, " choice=\"%s\"", node->u.variant.choice);
		fprintf(fd, ">\n");
		cds_list_for_each_entry(iter, &node->u.variant.declaration_list, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</variant>\n");
		break;
	case NODE_STRUCT:
		print_tabs(fd, depth);
		if (node->u._struct.name)
			fprintf(fd, "<struct name=\"%s\">\n",
				node->u._struct.name);
		else
			fprintf(fd, "<struct>\n");
		cds_list_for_each_entry(iter, &node->u._struct.declaration_list, siblings) {
			ret = ctf_visitor_print_xml(fd, depth + 1, iter);
			if (ret)
				return ret;
		}
		print_tabs(fd, depth);
		fprintf(fd, "</struct>\n");
		break;

	case NODE_UNKNOWN:
	default:
		fprintf(stderr, "[error] %s: unknown node type %d\n", __func__,
			(int) node->type);
		return -EINVAL;
	}
	return ret;
}