/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * strings.c
 *
 * As always, we let e2fsck lead the way.  A bitmap for
 * inodes with a single i_count (the vast majority), and a
 * tree of inode numbers with a greater count. 
 *
 * Copyright (C) 2004 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License, version 2,  as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 *
 * Authors: Zach Brown
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "ocfs2.h"

#include "fsck.h"
#include "strings.h"
#include "util.h"

struct string_entry {
	struct rb_node	s_node;
	size_t		s_strlen;
	char		s_string[0]; /* null terminated */
};

errcode_t o2fsck_strings_insert(o2fsck_strings *strings, char *string,
			   size_t strlen, int *is_dup)
{
	struct rb_node ** p = &strings->s_root.rb_node;
	struct rb_node * parent = NULL;
	struct string_entry *se;
	int cmp;

	*is_dup = 0;

	while (*p)
	{
		parent = *p;
		se = rb_entry(parent, struct string_entry, s_node);

		/* we don't actually care about lexographical sorting */
		cmp = strlen - se->s_strlen;
		if (cmp == 0)
			cmp = memcmp(string, se->s_string, strlen);

#if 0
		printf("%.*s %s %.*s\n", (int)strlen, string, 
			cmp < 0 ? "<" : (cmp > 0 ? ">" : "==" ),
			(int)se->s_strlen, se->s_string);
#endif

		if (cmp < 0)
			p = &(*p)->rb_left;
		else if (cmp > 0)
			p = &(*p)->rb_right;
		else {
			*is_dup = 1;
			return 0;
		}
	}

	se = malloc(offsetof(struct string_entry, s_string[strlen]));
	if (se == NULL)
		return OCFS2_ET_NO_MEMORY;

	se->s_strlen = strlen;
	memcpy(se->s_string, string, strlen);

	rb_link_node(&se->s_node, parent, p);
	rb_insert_color(&se->s_node, &strings->s_root);

	return 0;
}

void o2fsck_strings_init(o2fsck_strings *strings)
{
	strings->s_root = RB_ROOT;
}

void o2fsck_strings_free(o2fsck_strings *strings)
{
	struct string_entry *se;
	struct rb_node *node;

	while((node = rb_first(&strings->s_root)) != NULL) {
		se = rb_entry(node, struct string_entry, s_node);
		rb_erase(node, &strings->s_root);
		free(se);
	}
}
