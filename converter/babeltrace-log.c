/*
 * babeltrace-log.c
 *
 * BabelTrace - Convert Text Log to CTF
 *
 * Copyright 2010, 2011 - Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
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
 *
 * Depends on glibc 2.10 for getline().
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <uuid/uuid.h>
#include <string.h>
#include <endian.h>

#include <babeltrace/babeltrace.h>
#include <babeltrace/ctf/types.h>

#ifndef UUID_STR_LEN
#define UUID_STR_LEN	37	/* With \0 */
#endif

/* Metadata format string */
static const char metadata_fmt[] =
"typealias integer { size = 8; align = 8; signed = false; } := uint8_t;\n"
"typealias integer { size = 32; align = 32; signed = false; } := uint32_t;\n"
"\n"
"trace {\n"
"	major = %u;\n"			/* major (e.g. 0) */
"	minor = %u;\n"			/* minor (e.g. 1) */
"	uuid = \"%s\";\n"		/* UUID */
"	byte_order = %s;\n"		/* be or le */
"	packet.header := struct {\n"
"		uint32_t magic;\n"
"		uint8_t  trace_uuid[16];\n"
"	};\n"
"};\n"
"\n"
"stream {\n"
"	packet.context := struct {\n"
"		uint32_t content_size;\n"
"		uint32_t packet_size;\n"
"	};\n"
"};\n"
"\n"
"event {\n"
"	name = string;\n"
"	fields := struct { string str; };\n"
"};\n";

int babeltrace_debug, babeltrace_verbose;

static uuid_t s_uuid;

static
void print_metadata(FILE *fp)
{
	char uuid_str[UUID_STR_LEN];

	uuid_unparse(s_uuid, uuid_str);
	fprintf(fp, metadata_fmt,
		BABELTRACE_VERSION_MAJOR,
		BABELTRACE_VERSION_MINOR,
		uuid_str,
		BYTE_ORDER == LITTLE_ENDIAN ? "le" : "be");
}


static
void write_packet_header(struct ctf_stream_pos *pos, uuid_t uuid)
{
	struct ctf_stream_pos dummy;

	/* magic */
	ctf_dummy_pos(pos, &dummy);
	ctf_align_pos(&dummy, sizeof(uint32_t) * CHAR_BIT);
	ctf_move_pos(&dummy, sizeof(uint32_t) * CHAR_BIT);
	assert(!ctf_pos_packet(&dummy));
	
	ctf_align_pos(pos, sizeof(uint32_t) * CHAR_BIT);
	*(uint32_t *) ctf_get_pos_addr(pos) = 0xC1FC1FC1;
	ctf_move_pos(pos, sizeof(uint32_t) * CHAR_BIT);

	/* trace_uuid */
	ctf_dummy_pos(pos, &dummy);
	ctf_align_pos(&dummy, sizeof(uint8_t) * CHAR_BIT);
	ctf_move_pos(&dummy, 16 * CHAR_BIT);
	assert(!ctf_pos_packet(&dummy));

	ctf_align_pos(pos, sizeof(uint8_t) * CHAR_BIT);
	memcpy(ctf_get_pos_addr(pos), uuid, 16);
	ctf_move_pos(pos, 16 * CHAR_BIT);
}

static
void write_packet_context(struct ctf_stream_pos *pos)
{
	struct ctf_stream_pos dummy;

	/* content_size */
	ctf_dummy_pos(pos, &dummy);
	ctf_align_pos(&dummy, sizeof(uint32_t) * CHAR_BIT);
	ctf_move_pos(&dummy, sizeof(uint32_t) * CHAR_BIT);
	assert(!ctf_pos_packet(&dummy));
	
	ctf_align_pos(pos, sizeof(uint32_t) * CHAR_BIT);
	*(uint32_t *) ctf_get_pos_addr(pos) = -1U;	/* Not known yet */
	pos->content_size_loc = (uint32_t *) ctf_get_pos_addr(pos);
	ctf_move_pos(pos, sizeof(uint32_t) * CHAR_BIT);

	/* packet_size */
	ctf_dummy_pos(pos, &dummy);
	ctf_align_pos(&dummy, sizeof(uint32_t) * CHAR_BIT);
	ctf_move_pos(&dummy, sizeof(uint32_t) * CHAR_BIT);
	assert(!ctf_pos_packet(&dummy));
	
	ctf_align_pos(pos, sizeof(uint32_t) * CHAR_BIT);
	*(uint32_t *) ctf_get_pos_addr(pos) = pos->packet_size;
	ctf_move_pos(pos, sizeof(uint32_t) * CHAR_BIT);
}

static
void trace_string(char *line, struct ctf_stream_pos *pos, size_t len)
{
	struct ctf_stream_pos dummy;
	int attempt = 0;

	printf_debug("read: %s\n", line);
retry:
	ctf_dummy_pos(pos, &dummy);
	ctf_align_pos(&dummy, sizeof(uint8_t) * CHAR_BIT);
	ctf_move_pos(&dummy, len * CHAR_BIT);
	if (ctf_pos_packet(&dummy)) {
		ctf_pos_pad_packet(pos);
		write_packet_header(pos, s_uuid);
		write_packet_context(pos);
		if (attempt++ == 1) {
			fprintf(stdout, "[Error] Line too large for packet size (%zukB) (discarded)\n",
				pos->packet_size / CHAR_BIT / 1024);
			return;
		}
		goto retry;
	}

	ctf_align_pos(pos, sizeof(uint8_t) * CHAR_BIT);
	memcpy(ctf_get_pos_addr(pos), line, len);
	ctf_move_pos(pos, len * CHAR_BIT);
}

static
void trace_text(FILE *input, int output)
{
	struct ctf_stream_pos pos;
	ssize_t len;
	char *line = NULL, *nl;
	size_t linesize;

	ctf_init_pos(&pos, output, O_RDWR);

	write_packet_header(&pos, s_uuid);
	write_packet_context(&pos);
	for (;;) {
		len = getline(&line, &linesize, input);
		if (len < 0)
			break;
		nl = strrchr(line, '\n');
		if (nl)
			*nl = '\0';
		trace_string(line, &pos, nl - line + 1);
	}
	ctf_fini_pos(&pos);
}

static void usage(FILE *fp)
{
	fprintf(fp, "BabelTrace Log Converter %u.%u\n",
		BABELTRACE_VERSION_MAJOR,
		BABELTRACE_VERSION_MINOR);
	fprintf(fp, "\n");
	fprintf(fp, "Convert for a text log (read from standard input) to CTF.\n");
	fprintf(fp, "\n");
	fprintf(fp, "usage : babeltrace-log OUTPUT\n");
	fprintf(fp, "\n");
	fprintf(fp, "  OUTPUT                         Output trace path\n");
	fprintf(fp, "\n");
}

int main(int argc, char **argv)
{
	int fd, metadata_fd, ret;
	char *outputname;
	DIR *dir;
	int dir_fd;
	FILE *metadata_fp;

	if (argc < 2) {
		usage(stdout);
		goto error;
	}
	outputname = argv[1];

	ret = mkdir(outputname, S_IRWXU|S_IRWXG);
	if (ret) {
		perror("mkdir");
		goto error;
	}

	dir = opendir(outputname);
	if (!dir) {
		perror("opendir");
		goto error_rmdir;
	}
	dir_fd = dirfd(dir);
	if (dir_fd < 0) {
		perror("dirfd");
		goto error_closedir;
	}

	fd = openat(dir_fd, "datastream", O_RDWR|O_CREAT,
		    S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
	if (fd < 0) {
		perror("openat");
		goto error_closedirfd;
	}

	metadata_fd = openat(dir_fd, "metadata", O_RDWR|O_CREAT,
			     S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
	if (fd < 0) {
		perror("openat");
		goto error_closedatastream;
	}
	metadata_fp = fdopen(metadata_fd, "w");
	if (!metadata_fp) {
		perror("fdopen");
		goto error_closemetadatafd;
	}

	uuid_generate(s_uuid);
	print_metadata(metadata_fp);
	trace_text(stdin, fd);

	close(fd);
	exit(EXIT_SUCCESS);

	/* error handling */
error_closemetadatafd:
	ret = close(metadata_fd);
	if (ret)
		perror("close");
error_closedatastream:
	ret = close(fd);
	if (ret)
		perror("close");
error_closedirfd:
	ret = close(dir_fd);
	if (ret)
		perror("close");
error_closedir:
	ret = closedir(dir);
	if (ret)
		perror("closedir");
error_rmdir:
	ret = rmdir(outputname);
	if (ret)
		perror("rmdir");
error:
	exit(EXIT_FAILURE);
}