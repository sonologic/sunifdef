/***************************************************************************
 *   Copyright (C) 2004, 2006 Symbian Software Ltd.                        *
 *   All rights reserved.                                                  *
 *   Copyright (C) 2007, 2008 Mike Kinghan, imk@strudl.org                 *
 *   All rights reserved.                                                  *
 *                                                                         *
 *   Contributed originally by Mike Kinghan, imk@strudl.org                *
 *                                                                         *
 *   Redistribution and use in source and binary forms, with or without    *
 *   modification, are permitted provided that the following conditions    *
 *   are met:                                                              *
 *                                                                         *
 *   Redistributions of source code must retain the above copyright        *
 *   notice, this list of conditions and the following disclaimer.         *
 *                                                                         *
 *   Redistributions in binary form must reproduce the above copyright     *
 *   notice, this list of conditions and the following disclaimer in the   *
 *   documentation and/or other materials provided with the distribution.  *
 *                                                                         *
 *   Neither the name of Symbian Software Ltd. nor the names of its        *
 *   contributors may be used to endorse or promote products derived from  *
 *   this software without specific prior written permission.              *
 *                                                                         *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   *
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     *
 *   LIMITED TO, THE IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS    *
 *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE        *
 *   COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,   *
 *   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,  *
 *   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS *
 *   OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED    *
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,*
 *   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF *
 *   THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH  *
 *   DAMAGE.                                                               *
 *                                                                         *
 ***************************************************************************/

#include "file_tree.h"
#include "filesys.h"
#include "ptr_vector.h"
#include "platform.h"
#include "report.h"
#include <ctype.h>
#include <stdio.h>

/*!\ingroup file_tree_module file_tree_interface file_tree_internals
 *\file file_tree.c
 * This file implements the File Tree module
 */

/*! \addtogroup file_tree_internals */
/*@{*/

/*! Value of the \c parent field in \c struct \c file_tree
	indicating the the true value has yet to be set.
*/
#define PARENT_PENDING	((void *)-1)
/*!
	Test whether a file tree is not yet linked to its
	parent.
*/
#define IS_UNLINKED(ftree)	((ftree)->parent == PARENT_PENDING)
/*!
	A file tree is a root node if it has no parent
*/
#define IS_ROOT(ftree)	((ftree)->parent == NULL)
/*!
	A file tree represents a file if it has no list of
	children (not even an empty list).
*/
#define IS_FILE(ftree)	((ftree)->children == NULL)
/*!
	A file tree represents a directory if it does not
	represent a file.
*/
#define IS_DIR(ftree)	(!IS_FILE(ftree))

/*!
	\e flags for \c file_tree_count() specify that files are
	to be counted.
*/
#define COUNT_FILES(flags) (((flags) & FT_COUNT_FILES) == FT_COUNT_FILES)
/*!
	\e flags for \c file_tree_count() specify that only files are
	to be counted.
*/
#define COUNT_FILES_ONLY(flags) (((flags) & FT_COUNT_ALL) == FT_COUNT_FILES)
/*!
	\e flags for \c file_tree_count() specify that directories are
	to be counted.
*/
#define COUNT_DIRS(flags) (((flags) & FT_COUNT_DIRS) == FT_COUNT_DIRS)
/*!
	\e flags for \c file_tree_count() specify that only
	directories are	to be counted.
*/
#define COUNT_DIRS_ONLY(flags) (((flags) & FT_COUNT_ALL) == FT_COUNT_DIRS)
/*!
	\e flags for \c file_tree_count() specify that both
	directories and files are to be counted.
*/
#define COUNT_ALL(flags) (((flags) & FT_COUNT_ALL) == FT_COUNT_ALL)
/*!
	\e flags for \c file_tree_count() specify that only
	the immediate children of the file tree are to be
	counted
*/
#define COUNT_CHILDREN(flags) (((flags) & FT_COUNT_CHILDREN) == FT_COUNT_CHILDREN)

/*! Structure representing a file or directory in
	relation to its parent directory (if any) and
	contained files (if any)
*/
struct file_tree {
	union {
		heap_str leafname;	/*!< In non-root node, leafname of file
								or directory */
		file_filter_t filter;	/*!< In root-node, pointer to the
								filter function of the tree. */
	} var;
	struct file_tree * parent;	 /*!< NULL for root node. For
								non-root node, node representing
								parent directory */
	ptr_vector_h children;	/*!< Immediate children of directory node,
								or NULL */
	unsigned files; /*!< Total number of files (excluding directories)
						within this tree */
};

/*!
	The default filter function for a file tree to which
	no filter is attached. It accepts all files.
*/
static bool
null_filter(char const *leafname)
{
	return true;
}

/*!
	The default callback function for a file tree traversals
	where none is specified. It does nothing.
*/
static void
null_callback(	file_tree_h tree,
				char const *name,
				file_tree_traverse_state_t context)
{
}

/*!
	Construct a new file tree node.

	\param		leafname	Pointer to the name of the new node.
	\param		len			The length of the name at \c leafname.
				If 0, \c strlen(leafname) is used.

	\return	The new node.
*/
static file_tree_h
new_node(char const *leafname, size_t len)
{
	file_tree_h tree = callocate(1,sizeof(struct file_tree));
	if (!len) {
		len = strlen(leafname);
	}
	tree->parent = PARENT_PENDING;
	tree->var.leafname = allocate(len + 1);
	strncpy(tree->var.leafname,leafname,len);
	return tree;
}


/*!
	Construct a new file tree node to represent a file.

	\param		leafname	The name of the new node.
	\param		filter		Pointer to a function that
				shall be	applied to \c leafname to accept or
				reject it for constructing a node.
	\return	iIf \c leafname satisfies \c filter then
				a new node is returned; else NULL.
*/
static file_tree_h
new_file_node(char const *leafname, file_filter_t filter)
{
	file_tree_h tree = NULL;
	if (filter(leafname)) {
		tree = new_node(leafname,0);
		tree->files = 1;
	}
	return tree;
}

/*!
	Return the parent of a file tree.

	\param child		The file tree whose parent is required.
	\return	The parent of \e child, if any; NULL if
	\e child is a root node or is not yet linked.

*/
static file_tree_h
get_parent(file_tree_h child)
{
	return IS_UNLINKED(child) || IS_ROOT(child) ? NULL : child->parent;
}
/*!
	Link a child node to its parent node in a file tree.

	\param		parent		The parent node.
	\param		child		The child node.

	\e child is added to the children of \e child->parent
	and \e parent is assigned as the parent of \e child

*/
static void
link(file_tree_h parent, file_tree_h child)
{
	unsigned new_files;
	assert(!IS_ROOT(child));
	if (!parent->children) {
		parent->children = ptr_vector_new();
	}
	child->parent = parent;
	ptr_vector_append(parent->children,child);
	new_files = child->files;
	if (new_files) {
		for	(	;parent; child = parent,parent = get_parent(child)) {
			parent->files += new_files;
		}
	}
}

/* Forward declaration */
static void
file_tree_add_symlink(	file_tree_h root,
						fs_dir_t parent,
						char const *symlink,
						file_tree_callback_t callback);


/*!
	Construct a new file tree node to represent a directory

	\param		root	The root node of the file tree in which
						the new node will potentially be added.
	\param		leafname	The name of the new node.
	\param		dir			Handle to the directory of which
				\c leafname is the leafname.
	\param		filter		A pointer to a function that
				shall be	applied to the names of files
				(not directories) under \c dir	to accept or reject
				them for inclusion	under the new node.
	\param		callback	NULL, or a \c file_tree_callback_t to be
				called at entry	and exit for each directory node
				constructed and for each file added.
	\return	If any files are found recursively under \c dir
				that satisfy \c filter then a new node is returned
				representing the directory together with all such
				files and the subdirectories that contain them.
*/

static file_tree_h
new_dir_node(	file_tree_h root,
				char const *leafname,
				fs_dir_t dir,
				file_filter_t filter,
				file_tree_callback_t callback)
{
	file_tree_h tree = new_node(leafname,0);
	char const *cur_entry;
	char const *fullname = fs_cur_dir_entry(dir,&cur_entry);
	assert(cur_entry[0] == '\0');
	callback(tree,fullname,FT_ENTERING_DIR);
	tree->children = ptr_vector_new();
	while((leafname = fs_read_dir(dir,&fullname)) != NULL) {
		fs_obj_type_t obj_type = fs_obj_type(fullname);
		if (FS_IS_SLINK(obj_type)) {
			file_tree_add_symlink(tree,dir,fullname,callback);
		}
		else {
			file_tree_h child = NULL;
			fs_dir_t subdir = fs_open_dir(fullname,dir);
			if (subdir) {
				child = new_dir_node(root,leafname,subdir,filter,callback);
				fs_close_dir(subdir);
			}
			else {
				child = new_file_node(leafname,filter);
				if (child) {
					callback(child,fullname,FT_AT_FILE);
				}
			}
			if (child) {
				assert(child->files);
				link(tree,child);
			}
		}
	}
	fullname = fs_cur_dir_entry(dir,&cur_entry);
	assert(cur_entry[0] == '\0');
	callback(tree,fullname,FT_LEAVING_DIR);
	if (!tree->files) {
		file_tree_dispose(&tree);
	}
	return tree;
}

/*!		Return the root node of a file tree */
static file_tree_h
get_root(file_tree_h tree)
{
	if (IS_ROOT(tree)) {
		return tree;
	}
	return get_root(tree->parent);
}


static file_tree_h
seek_child(file_tree_h node, char const *childname)
{
	file_tree_h child = NULL;
	if (node->children) {
		file_tree_h *start =
			(file_tree_h *)ptr_vector_start(node->children);
		file_tree_h *end =
			(file_tree_h *)ptr_vector_end(node->children);
		for (	;start != end; ++start) {
			if (!strcmp(childname,(*start)->var.leafname)) {
				child = *start;
				break;
			}
		}
	}
	return child;
}


/*!
	Search for a path in a file tree.

	\param		tree	The file tree to be searched.
	\param		path	Pointer to the path to be searched for.

	\return	The deepest node in \c tree to which any
				initial subpath of \c *path is a key. The
				unmatched reminder of the path will be stored
				at \c path.

	If the path at \c *path is relative then then it is
	assumed to be relative to a path that is a key to the
	parent of \c tree. If \c *path is absolute then
	the function returns \c seek(root,path), where \c root
	is the root node of \c tree.

*/
static file_tree_h
seek(file_tree_h tree, char const **path)
{
	char const *posn = *path;
	char const *end = strchr(posn,PATH_DELIM);
 	fs_path_type_t path_type = fs_path_type(posn);
	if (!end) {
		end = posn + strlen(posn);
	}
	if (!IS_ROOT(tree)) {
		size_t elmlen = end - posn;
		if (fs_windows_path(path_type) && fs_absolute_path(path_type)) {
			path_type = 0;
		}
		if (tree->var.leafname[elmlen] =='\0' &&
				!strncmp(tree->var.leafname,posn,elmlen)) {
			*path = posn += elmlen;
		}
		else {
			return tree;
		}
	}
	if (*posn) {
		if (!fs_windows_path(path_type) || !fs_absolute_path(path_type)) {
			*path = posn = end + 1;
		}
		if (tree->children) {
			file_tree_h child = NULL;
			file_tree_h *start =
				(file_tree_h *)ptr_vector_start(tree->children);
			file_tree_h *end =
				(file_tree_h *)ptr_vector_end(tree->children);
			for (	;start != end && posn == *path; ++start) {
				child = seek(*start,&posn);
			}
			if (posn != *path) {
				*path = posn;
				tree = child;
			}
		}
	}
	return tree;
}

/*!
	Deepen a file tree by linking additional nodes
	to represent successive directory components of a
	path.

	\param		tree	The file tree to be deepened.
	\param		path	Pointer to the path to be parsed.

	\return	The final node that has been added to the
				file tree. The address of the final element
				of the parsed path is stored at \c path.
*/
static file_tree_h
deepen(file_tree_h tree, char const **path)
{
	char const *end = strchr(*path,PATH_DELIM);
	if (end) {
		file_tree_h child = new_node(*path,end - *path);
		child->children = ptr_vector_new();
		link(tree,child);
		*path = end + 1;
		tree = deepen(child,path);
	}
	return tree;
}

/*!
	Recursively traverse a file tree,
	executing a callback at each node.

	\param		tree	The file tree to traverse.
	\param		callback	The callback to execute at each node.
	\param		path_start	Pointer to the full pathname of
				\em tree
	\param		path_end		Pointer just past the end of the
				full pathname of \em tree.

	\em callback is executed at each file node reached,
	passing the full pathname of the file and the flag
	\c FT_AT_FILE. It is executed at entry and exit for each
	directory node, passing the full pathname of the directory
	and \c FT_ENTERING_DIR or \c FT_LEAVING_DIR respectively.
*/
static void
traverse(	file_tree_h tree,
			file_tree_callback_t callback,
			char *path_start,
			char *path_end)
{
	strcpy(path_end,tree->var.leafname);
	if (IS_DIR(tree)) {
		size_t leaflen = strlen(path_end);
		file_tree_h * start = (file_tree_h *)ptr_vector_start(tree->children);
		file_tree_h * end = (file_tree_h *)ptr_vector_end(tree->children);
		callback(tree,path_start,FT_ENTERING_DIR);
		path_end[leaflen++] = PATH_DELIM;
		for (	;start != end; ++start) {
			traverse(*start,callback,path_start,path_end + leaflen);
		}
		path_end[--leaflen] = '\0';
		callback(tree,path_start,FT_LEAVING_DIR);
	}
	else {
		callback(tree,path_start,FT_AT_FILE);
	}
}


/*! Add the filtered contents of a canonical path to a file tree.

	The function is the same as \e file_tree_add() with the
	restriction that the path parameter must be an absolute
	real path (not a symbolic link).
*/
static void
file_tree_add_canon(	file_tree_h root,
						char const *path,
						file_tree_callback_t callback)
{
	char const * fullpath = path;
	heap_str parent_path = fs_split_filename(path,NULL);
	fs_dir_t parent_dir = parent_path ? fs_open_dir(parent_path,NULL) : NULL;
	fs_dir_t dir = fs_open_dir(path,parent_dir);
	file_filter_t filter;
	file_tree_h lowest;
	if (parent_path) {
		free(parent_path);
	}
	root = get_root(root);
	filter = root->var.filter;
	if (!callback) {
		callback = null_callback;
	}
	lowest = seek(root,&path);
	if (*path) {
		file_tree_h child;
		lowest = deepen(lowest,&path);
		if (!dir) {
			child = new_file_node(path,filter);
			if (child) {
				callback(child,fullpath,FT_AT_FILE);
			}
		}
		else {
			child = new_dir_node(root,path,dir,filter,callback);
		}
		if (child) {
			link(lowest,child);
		}
	}
	else if (dir) {
		char const * leafname;
		char const *fullname;
		while((leafname = fs_read_dir(dir,&fullname)) != NULL) {
			fs_obj_type_t obj_type = fs_obj_type(fullname);
			if (FS_IS_SLINK(obj_type)) {
				file_tree_add_symlink(root,dir,fullname,callback);
			}
			else {
				file_tree_h child = seek_child(lowest,leafname);
				if (!child) {
					fs_dir_t subdir = fs_open_dir(fullname,dir);
					if (!subdir) {
						child = new_file_node(leafname,filter);
						if (child) {
							callback(child,fullname,FT_AT_FILE);
						}
					}
					else {
						child =
							new_dir_node(root,leafname,subdir,filter,callback);
						fs_close_dir(subdir);
					}
					if (child) {
						link(lowest,child);
					}
				}
			}
		}
	}
	if (parent_dir) {
		fs_close_dir(parent_dir);
	}
	if (dir) {
		fs_close_dir(dir);
	}
}


/*! Add a the contents of a symbolic link to a file tree.

	\param		root	The file tree to which a symbolic link is
						to be added.
	\param		dir		NULL, or the handle of the directory being
						traversed when the symlink was found.
	\param		symlink	The absolute name of the symlink.
	\return	True if the symlink is found to be eligible for
	addition to the file tree, else false.

	When a symbolic link is found among the inputs, it can
	represent a "jump" from one ostensible position in the
	filesystem to an arbitrary other position.

	The basic approach for handling such a jump is to
	"start from scratch", calling \em file_tree_add_canon()
	on the resolved real pathname \em R of the symbolic link.

	There is one snag with this approach. \em R might lie beneath
	some directory \em D that is still in play at the time when
	we wish to call \em file_tree_add_canon(R), i.e. we are
	in the process of traversing \em D and have not yet
	linked it into the input file tree. In this case a call
	to \em file_tree_add_canon(R) would force \em D to be
	linked while we are trying to determine whether it should
	be; and if we finally decided that \em D should be linked,
	we will end up linking it twice.

	The addition of a depuplication precautions to the linkage
	logic simply to cater for symbolic links would have very poor
	cost-benefit. Instead, we just determine whether \em R is
	a subdirectory of any of the directories that are in play
	when we encounter the symlink, by backtracking from the
	directory in which we found the symlink and comparing the
	successive directory pathnames with \em R until either we
	find one that is a parent of \em R or reach the root of
	of the directories currently open for traversal. Only
	if \em R is not found found to lie under any of the open
	directories is it necessary to call
	\em file_tree_add_canon(R), because otherwise \em R
	has either already been traversed or it is going to be
	traversed within the active call to \em file_tree_add_canon().

*/
static void
file_tree_add_symlink(	file_tree_h root,
						fs_dir_t dir,
						char const *symlink,
						file_tree_callback_t callback)
{
	heap_str realpath;
	realpath = fs_real_path(symlink,NULL);
	report(GRIPE_SYMLINK,NULL,"Resolved symbolic link \"%s\"",symlink);
	if (dir) {
		size_t shared_len = 0;
		char const *entry;
		char const * dirname = fs_cur_dir_entry(dir,&entry);
		char const *shared_path  =
			fs_path_comp(dirname,realpath,&shared_len);
		while (!(shared_path == dirname && dirname[shared_len] == 0)) {
			if ((dir = fs_get_parent(dir)) == NULL) {
				break;
			}
			dirname = fs_cur_dir_entry(dir,&entry);
			shared_path  = fs_path_comp(dirname,realpath,&shared_len);
		}
	}
	if (!dir) {
		file_tree_add_canon(root,realpath,callback);
	}
	free(realpath);
}

#ifdef DEBUG_FILE_TREE
static void
file_tree_print_name(file_tree_h tree, char const *name, int context)
{
	(void)context;
	puts(name);
}

void
file_tree_dump(file_tree_h file_tree)
{
	if (!IS_ROOT(file_tree)) {
		file_tree_dump(file_tree->parent);
	}
	else {
		file_tree_traverse(file_tree,file_tree_print_name);
	}
}

#endif

/*@}*/

/* API */

heap_str
file_tree_name(const file_tree_h file_tree)
{
	heap_str full_name;
	if (IS_ROOT(file_tree)) {
		full_name = callocate(1,1);
	}
	else {
		heap_str path_name = file_tree_name(file_tree->parent);
		full_name =
			fs_compose_filename(path_name,file_tree->var.leafname);
		free(path_name);
	}
	return full_name;
}

file_tree_h
file_tree_new(void)
{
	file_tree_h tree = callocate(1,sizeof(struct file_tree));
	tree->var.filter = null_filter;
	return tree;
}

bool
file_tree_set_filter(file_tree_h file_tree, file_filter_t filter)
{
	if (!file_tree_is_empty(file_tree)) {
		return false;
	}
	file_tree->var.filter = filter;
	return true;
}

void
file_tree_dispose(file_tree_h * file_tree)
{
	file_tree_h tree = *file_tree;
	if (tree) {
		if (tree->children) {
			file_tree_h * start = (file_tree_h *)ptr_vector_start(tree->children);
			file_tree_h * end = (file_tree_h *)ptr_vector_end(tree->children);
			for (	;start != end; ++start) {
				file_tree_dispose(start);
			}
			ptr_vector_dispose(&tree->children);
		}
		if (!IS_ROOT(tree) && tree->var.leafname) {
			free(tree->var.leafname);
		}
		release((void **)file_tree);
	}
}


void
file_tree_add(	file_tree_h root,
				char const *path,
				file_tree_callback_t callback)
{
	fs_obj_type_t obj_type = fs_file_or_dir(path);
	if (FS_IS_SLINK(obj_type)) {
		file_tree_add_symlink(root,NULL,path,callback);
	}
	else {
		heap_str fullpath = fs_real_path(path,NULL);
		file_tree_add_canon(root,fullpath,callback);
		free(fullpath);
	}
}

void
file_tree_traverse(	file_tree_h tree,
					file_tree_callback_t callback)
{
	callback(tree,NULL,FT_ENTERING_TREE);
	if (tree->children) {
		file_tree_h * start;
		file_tree_h * end;
		heap_str pathstack = callocate(1,PATH_MAX);
		strcpy(pathstack,FS_ROOT_PREFIX);
		start = (file_tree_h *)ptr_vector_start(tree->children);
		end = (file_tree_h *)ptr_vector_end(tree->children);
		for (	;start != end; ++start) {
			traverse(*start,callback,pathstack,pathstack + strlen(pathstack));
		}
		free(pathstack);
	}
	callback(tree,NULL,FT_LEAVING_TREE);
}

bool
file_tree_is_empty(file_tree_h file_tree)
{
	if (!IS_ROOT(file_tree)) {
		return false;
	}
	return !file_tree->children || ptr_vector_count(file_tree->children) == 0;
}

size_t
file_tree_count(const file_tree_h tree, unsigned flags, file_tree_count_t *counter)
{
	file_tree_count_t count = {0,0,0};
	int count_children = COUNT_CHILDREN(flags);
	int count_dirs = COUNT_DIRS(flags);
	int count_files = COUNT_FILES(flags);
	if (!count_children && count_files && !count_dirs) {
		count.files = tree->files;
	}
	else {
		if (!IS_ROOT(tree)) {
			if (!count_children) {
				if (IS_DIR(tree)) {
					count.dirs += count_dirs;
				}
				else {
					count.files += count_files;
				}
			}
		}
		if (IS_DIR(tree)) {
			file_tree_h * start = (file_tree_h *)ptr_vector_start(tree->children);
			file_tree_h * end = (file_tree_h *)ptr_vector_end(tree->children);
			for (	;start != end; ++ start) {
				if (count_children) {
					++count.children;
					if (IS_DIR(*start)) {
						count.dirs += count_dirs;
					}
					else {
						count.files += count_files;
					}
				}
				else {
					file_tree_count(*start,flags,&count);
				}
			}
		}
	}
	if (counter) {
		*counter = count;
	}
	return count.files + count.dirs;
}


file_tree_h
file_tree_child(file_tree_h file_tree, size_t which)
{
	file_tree_h child = NULL;
	if (IS_DIR(file_tree)) {
		size_t children = ptr_vector_count(file_tree->children);
		if (children) {
			if (which == FT_LAST) {
				which = children - 1;
			}
			if (which < children) {
				child = *(file_tree_h *)ptr_vector_at(file_tree->children,which);
			}
		}
	}
	return child;
}

/* EOF */
