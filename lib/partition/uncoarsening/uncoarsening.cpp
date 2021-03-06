/******************************************************************************
 * uncoarsening.cpp 
 *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 *
 ******************************************************************************
 * Copyright (C) 2013 Christian Schulz <christian.schulz@kit.edu>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#include "graph_partition_assertions.h"
#include "misc.h"
#include "quality_metrics.h"
#include "refinement/mixed_refinement.h"
#include "refinement/refinement.h"
#include "separator/vertex_separator_algorithm.h"
#include "uncoarsening.h"


uncoarsening::uncoarsening() {

}

uncoarsening::~uncoarsening() {

}

int uncoarsening::perform_uncoarsening(const PartitionConfig & config, graph_hierarchy & hierarchy) {
        int improvement = 0;

        PartitionConfig cfg     = config;
        refinement* refine      = new mixed_refinement();

        graph_access * coarsest = hierarchy.get_coarsest();
        PRINT(std::cout << "log>" << "unrolling graph with " << coarsest->number_of_nodes() << std::endl;)

        complete_boundary* finer_boundary   = NULL;
        complete_boundary* coarser_boundary = NULL;

        coarser_boundary = new complete_boundary(coarsest);
        coarser_boundary->build();

        improvement += (int)refine->perform_refinement(cfg, *coarsest, *coarser_boundary);

        NodeID coarser_no_nodes = coarsest->number_of_nodes();
        graph_access* finest    = NULL;
        graph_access* to_delete = NULL;

        while(!hierarchy.isEmpty()) {
                graph_access* G = hierarchy.pop_finer_and_project();

                PRINT(std::cout << "log>" << "unrolling graph with " << G->number_of_nodes()<<  std::endl;)
                
                finer_boundary = new complete_boundary(G); 
                finer_boundary->build_from_coarser(coarser_boundary, coarser_no_nodes, hierarchy.get_mapping_of_current_finer());

                //call refinement
                improvement += (int)refine->perform_refinement(cfg, *G, *finer_boundary);
                ASSERT_TRUE(graph_partition_assertions::assert_graph_has_kway_partition(config, *G));

                if(config.use_balance_singletons) {
                        finer_boundary->balance_singletons( config, *G );
                }

                // update boundary pointers
                delete coarser_boundary;
                coarser_boundary = finer_boundary;
                coarser_no_nodes = G->number_of_nodes();

		//clean up 
		if(to_delete != NULL) {
			delete to_delete;
		}
		if(!hierarchy.isEmpty()) {
			to_delete = G;
		}

                finest = G;
        }

        if(config.compute_vertex_separator) {
               PRINT(std::cout <<  "now computing a vertex separator from the given edge separator"  << std::endl;)
               vertex_separator_algorithm vsa;
               vsa.compute_vertex_separator(config, *finest, *finer_boundary); 
        }

        delete refine;
        if(finer_boundary != NULL) delete finer_boundary;
	delete coarsest;

        return improvement;
}


