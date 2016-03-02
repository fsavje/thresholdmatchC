/* ==============================================================================
 * scclust -- A C library for size constrained clustering
 * https://github.com/fsavje/scclust
 *
 * Copyright (C) 2015-2016  Fredrik Savje -- http://fredriksavje.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * ============================================================================== */

#include "error.h"

#include <assert.h>
#include <stdio.h>
#include "../include/scclust.h"


// ==============================================================================
// Internal variables
// ==============================================================================

static const char* iscc_error_file = "unknown file";

static int iscc_error_line = -1;


// ==============================================================================
// External function implementations
// ==============================================================================

scc_ErrorCode iscc_make_error_func(const scc_ErrorCode ec,
                                   const char* const file,
                                   const int line)
{
	assert((ec > SCC_ER_OK) && (ec <= SCC_ER_NOT_IMPLEMENTED));

	iscc_error_file = file;
	iscc_error_line = line;

	return ec;
}


void iscc_reset_error(void)
{
	iscc_error_file = "unknown file";
	iscc_error_line = -1;
}


bool scc_get_error_message(const scc_ErrorCode ec,
                           const size_t len_error_message_buffer,
                           char error_message_buffer[const])
{
	if ((len_error_message_buffer == 0) || (error_message_buffer == NULL)) return false;

	if (ec == SCC_ER_OK) {
		snprintf(error_message_buffer, len_error_message_buffer, "%s", "No error.");
		return true;
	}

	const char* error_message;
	switch (ec) {
		case SCC_ER_NULL_INPUT:
			error_message = "A required input pointer is NULL.";
			break;
		case SCC_ER_INVALID_INPUT:
			error_message = "Inputted function parameters are invalid.";
			break;
		case SCC_ER_INVALID_CLUSTERING:
			error_message = "Inputted clustering is invalid.";
			break;
		case SCC_ER_EMPTY_CLUSTERING:
			error_message = "Empty clustering is inputted when non-empty is required.";
			break;
		case SCC_ER_INVALID_DATA_OBJ:
			error_message = "Inputted data object is invalid.";
			break;
		case SCC_ER_NO_MEMORY:
			error_message = "Cannot allocate required memory.";
			break;
		case SCC_ER_TOO_LARGE_PROBLEM:
			error_message = "The clustering problem is too large under the current configuration (either too many clusters or data points).";
			break;
		case SCC_ER_TOO_LARGE_DIGRAPH:
			error_message = "The clustering problem yields a digraph with too many arcs.";
			break;
		case SCC_ER_DIST_SEARCH_ERROR:
			error_message = "Failed to calculate distances.";
			break;
		case SCC_ER_NOT_IMPLEMENTED:
			error_message = "Requested functionality is not yet implemented.";
			break;
		default:
			error_message = "Unknown error code.";
			break;
	}

	snprintf(error_message_buffer, len_error_message_buffer, "(%s:%d) %s", iscc_error_file, iscc_error_line, error_message);

	return true;
}