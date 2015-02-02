#include <iostream>
#include <vector>
#include <algorithm>
#include <float.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "defs.h"
using namespace std;

typedef struct
{
    vertex_id_t startV;
    vertex_id_t endV;
    weight_t w;
    vertex_id_t edge_id;
} edge_type;

extern "C" void init_mst(graph_t *G)
{   
}   

/* MST reference implementation. Prim's algorithm 
 * NOTE: isolated vertex is also tree, such tree must be represented as separate element of trees vector with zero-length edges list */
void MST(graph_t *G, vector < vector < edge_id_t > > &trees)
{
    edge_type nearest_edge;
    vector <uint8_t> marked_vert_in_forest(G->n, 0);
    bool non_visited_tree = true;
    vertex_id_t root = 0;
    vertex_id_t numTrees = 0;
    while (non_visited_tree) {
        vector <vertex_id_t> q_marked;
        q_marked.push_back(root);
        marked_vert_in_forest[root] = 1;


        bool non_visited_vert_in_tree = true;
        trees.push_back(vector <edge_id_t> ());
        while (non_visited_vert_in_tree) {
            non_visited_vert_in_tree = false;
            nearest_edge.w = DBL_MAX;
            for (vertex_id_t i = 0; i < q_marked.size(); i++) {
                for (edge_id_t j = G->rowsIndices[q_marked[i]]; j < G->rowsIndices[q_marked[i]+1]; j++) {
                    if (!marked_vert_in_forest[G->endV[j]]) {
                        non_visited_vert_in_tree = true;
                        if (nearest_edge.w == DBL_MAX || G->weights[j] < nearest_edge.w) {
                            nearest_edge.startV = q_marked[i];
                            nearest_edge.endV = G->endV[j];
                            nearest_edge.w = G->weights[j];
                            nearest_edge.edge_id = j;
                        }
                    }
                }
            }
            if (nearest_edge.w != DBL_MAX) {
                marked_vert_in_forest[nearest_edge.endV] = 1;
                trees[numTrees].push_back(nearest_edge.edge_id);
                q_marked.push_back(nearest_edge.endV);
            }
        }
        non_visited_tree = false;
        for (vertex_id_t i = 0; i < G->n ; i++) {
            if (!marked_vert_in_forest[i]) {
                non_visited_tree = true;
                root = i;
                numTrees++;
                break;
            }
        }
    }
}

extern "C" void finalize_mst()
{
}

