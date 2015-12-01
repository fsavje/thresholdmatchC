/* Copyright 2015 Fredrik Savje

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
==============================================================================*/


#include "../include/digraph.h"

#include <stdbool.h>
#include <stdlib.h>

#include "../include/config.h"


static inline scc_Arcref iscc_do_union(const scc_Vid vertices,
									   const size_t num_dgs,
									   const scc_Digraph* const * const dgs,
									   scc_Vid* restrict const row_markers,
									   const bool write,
									   scc_Arcref* restrict const out_tail_ptr,
									   scc_Vid* restrict const out_head) {
	scc_Arcref counter = 0;
	if (write) out_tail_ptr[0] = 0;
	for (scc_Vid v = 0; v < vertices; ++v) row_markers[v] = SCC_VID_MAX;

	for (scc_Vid v = 0; v < vertices; ++v) {
		for (size_t i = 0; i < num_dgs; ++i) {
			for (const scc_Vid* arc_i = dgs[i]->head + dgs[i]->tail_ptr[v];
					arc_i != dgs[i]->head + dgs[i]->tail_ptr[v + 1];
					++arc_i) {
				if (row_markers[*arc_i] != v) {
					row_markers[*arc_i] = v;
					if (write) out_head[counter] = *arc_i;
					++counter;
				}
			}
		}

		if (write) out_tail_ptr[v + 1] = counter;
	}

	return counter;
}


scc_Digraph scc_digraph_union(const size_t num_dgs, const scc_Digraph* const dgs[const static num_dgs]) {
	if (num_dgs == 0) return scc_empty_digraph(0, 0);
	if (!dgs || !dgs[0]) return scc_null_digraph();

	const scc_Vid vertices = dgs[0]->vertices;

	scc_Vid* const row_markers = malloc(sizeof(scc_Vid[vertices]));
	if (!row_markers) return scc_null_digraph();

	scc_Arcref out_arcs_write = 0;

	// Try greedy memory count first
	for (size_t i = 0; i < num_dgs; ++i) {
		if (!dgs[i] || !dgs[i]->tail_ptr || dgs[i]->vertices != vertices) return scc_null_digraph();
		out_arcs_write += dgs[i]->tail_ptr[vertices];
	}

	scc_Digraph dg_out = scc_init_digraph(vertices, out_arcs_write);
	if (!dg_out.tail_ptr) {
		// Could not allocate digraph with `out_arcs_write' arcs.
		// Do correct (but slow) memory count by doing
		// doing union without writing.
		out_arcs_write = iscc_do_union(vertices,
									   num_dgs, dgs,
									   row_markers,
									   false, NULL, NULL);

		// Try again. If fail, give up.
		dg_out = scc_init_digraph(vertices, out_arcs_write);
		if (!dg_out.tail_ptr) {
			free(row_markers);
			return dg_out;
		}
	}

	out_arcs_write = iscc_do_union(vertices,
								   num_dgs, dgs,
								   row_markers,
								   true, dg_out.tail_ptr, dg_out.head);

	free(row_markers);

	scc_change_arc_storage(&dg_out, out_arcs_write);

	return dg_out;
}


scc_Digraph scc_digraph_transpose(const scc_Digraph* const dg) {
	if (!dg || !dg->tail_ptr) return scc_null_digraph();
	if (dg->vertices == 0) return scc_empty_digraph(0, 0);

	scc_Vid* const row_count = calloc(dg->vertices + 1, sizeof(scc_Arcref));
	if (!row_count) return scc_null_digraph();

	scc_Digraph dg_out = scc_init_digraph(dg->vertices, dg->tail_ptr[dg->vertices]);
	if (!dg_out.tail_ptr) {
		free(row_count);
		return dg_out;
	}

	for (const scc_Vid* arc = dg->head;
			arc != dg->head + dg->tail_ptr[dg->vertices];
			++arc) {
		++row_count[*arc + 1];
	}

	dg_out.tail_ptr[0] = 0;
	for (scc_Vid v = 1; v <= dg->vertices; ++v) {
		row_count[v] += row_count[v - 1];
		dg_out.tail_ptr[v] = row_count[v];
	}

	for (scc_Vid v = 0; v < dg->vertices; ++v) {
		for (const scc_Vid* arc = dg->head + dg->tail_ptr[v];
				arc != dg->head + dg->tail_ptr[v + 1];
				++arc) {
			dg_out.head[row_count[*arc]] = v;
			++row_count[*arc];
		}
	}

	free(row_count);

	return dg_out;
}


static inline scc_Arcref iscc_do_adjacency_product(const scc_Vid vertices,
												   const scc_Arcref* const dg_a_tail_ptr,
												   const scc_Vid* const dg_a_head,
												   const scc_Arcref* const dg_b_tail_ptr,
												   const scc_Vid* const dg_b_head,
												   scc_Vid* restrict const row_markers,
												   const bool force_diagonal,
												   const bool ignore_diagonal,
												   const bool write,
												   scc_Arcref* restrict const out_tail_ptr,
												   scc_Vid* restrict const out_head) {
	scc_Arcref counter = 0;
	if (write) out_tail_ptr[0] = 0;
	for (scc_Vid v = 0; v < vertices; ++v) row_markers[v] = SCC_VID_MAX;

	for (scc_Vid v = 0; v < vertices; ++v) {
		if (force_diagonal) {
			for (const scc_Vid* arc_b = dg_b_head + dg_b_tail_ptr[v];
					arc_b != dg_b_head + dg_b_tail_ptr[v + 1];
					++arc_b) {
				if (row_markers[*arc_b] != v) {
					row_markers[*arc_b] = v;
					if (write) out_head[counter] = *arc_b;
					++counter;
				}
			}
		}
		for (const scc_Vid* arc_a = dg_a_head + dg_a_tail_ptr[v];
				arc_a != dg_a_head + dg_a_tail_ptr[v + 1];
				++arc_a) {
			if (*arc_a == v && (force_diagonal || ignore_diagonal)) continue;
			for (const scc_Vid* arc_b = dg_b_head + dg_b_tail_ptr[*arc_a];
					arc_b != dg_b_head + dg_b_tail_ptr[*arc_a + 1];
					++arc_b) {
				if (row_markers[*arc_b] != v) {
					row_markers[*arc_b] = v;
					if (write) out_head[counter] = *arc_b;
					++counter;
				}
			}
		}

		if (write) out_tail_ptr[v + 1] = counter;
	}

	return counter;
}


scc_Digraph scc_adjacency_product(const scc_Digraph* const dg_a, const scc_Digraph* const dg_b, const bool force_diagonal, const bool ignore_diagonal) {
	if (force_diagonal && ignore_diagonal) return scc_null_digraph();
	if (!dg_a || !dg_b || !dg_a->tail_ptr || !dg_b->tail_ptr) return scc_null_digraph();
	if (dg_a->vertices != dg_b->vertices) return scc_null_digraph();
	if (dg_a->vertices == 0) return scc_empty_digraph(0, 0);

	const scc_Vid vertices = dg_a->vertices;

	scc_Vid* const row_markers = malloc(sizeof(scc_Vid[vertices]));
	if (!row_markers) return scc_null_digraph();

	scc_Arcref out_arcs_write = 0;

	// Try greedy memory count first
	for (scc_Vid v = 0; v < vertices; ++v) {
		if (force_diagonal) {
			out_arcs_write += dg_b->tail_ptr[v + 1] - dg_b->tail_ptr[v];
		}
		for (const scc_Vid* arc_a = dg_a->head + dg_a->tail_ptr[v];
				arc_a != dg_a->head + dg_a->tail_ptr[v + 1];
				++arc_a) {
			if (*arc_a == v && (force_diagonal || ignore_diagonal)) continue;
			out_arcs_write += dg_b->tail_ptr[*arc_a + 1] - dg_b->tail_ptr[*arc_a];
		}
	}

	scc_Digraph dg_out = scc_init_digraph(vertices, out_arcs_write);
	if (!dg_out.tail_ptr) {
		// Could not allocate digraph with `out_arcs_write' arcs.
		// Do correct (but slow) memory count by doing
		// doing product without writing.
		out_arcs_write = iscc_do_adjacency_product(vertices,
												   dg_a->tail_ptr, dg_a->head,
												   dg_b->tail_ptr, dg_b->head,
												   row_markers,
												   force_diagonal, ignore_diagonal,
												   false, NULL, NULL);

		// Try again. If fail, give up.
		dg_out = scc_init_digraph(vertices, out_arcs_write);
		if (!dg_out.tail_ptr) {
			free(row_markers);
			return dg_out;
		}
	}

	out_arcs_write = iscc_do_adjacency_product(vertices,
											   dg_a->tail_ptr, dg_a->head,
											   dg_b->tail_ptr, dg_b->head,
											   row_markers,
											   force_diagonal, ignore_diagonal,
											   true, dg_out.tail_ptr, dg_out.head);

	free(row_markers);

	scc_change_arc_storage(&dg_out, out_arcs_write);

	return dg_out;
}
