#include <iostream>
#include <vector>
#include <algorithm>
#include <float.h>
#include <string.h>
#include <error.h>
#include <assert.h>
#include <stdarg.h>
#include <map>
#include "defs.h"

using namespace std;
char* inFilename_gr;
char* inFilename_result;
char* inFilename_valid;
weight_t* valid_weight_trees;
weight_t min_valid_weight;
vertex_id_t valid_num_trees;

void check(bool condition, const char* format, ...)
{
    if(!condition) {
        va_list args;

        va_start(args, format);
        vprintf(format, args);
        va_end(args);

        printf("\n");

        exit(1);
    }
}

void usage(int argc, char **argv)
{
    fprintf(stderr, "MST validation tool\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    %s -in_graph <input> -in_result <input> -in_valid <input> \n", argv[0]);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "    -in_graph <input> -- input graph filename\n");
    fprintf(stderr, "    -in_result <input> -- input filename with results of MST'\n");
    fprintf(stderr, "    -in_valid <input> -- input filename with validation information, which is generated by the gen_valid_info \n");
    exit(1);
}

void init (int argc, char** argv)
{
    inFilename_gr = inFilename_result = inFilename_valid = NULL;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-in_graph")) {
            inFilename_gr = argv[++i];
        }
        if (!strcmp(argv[i], "-in_result")) {
            inFilename_result = argv[++i];
        }
        if (!strcmp(argv[i], "-in_valid")) {
            inFilename_valid = argv[++i];
        }
    }
    if (!inFilename_gr || !inFilename_result || !inFilename_valid) usage(argc, argv);
}

int compare_double (double a, double b)
{
    if ( a <  b ) return -1;
    if ( a == b ) return 0;
    if ( a >  b ) return 1;
    return 0;
}

void sortTrees(vertex_id_t l, vertex_id_t r, double *sum_weight_of_tree, int (*compar)(double, double), forest_t *trees)
{
    if (l == r) return;
    vertex_id_t i = l, j = r;
    do
    {
        while ((*compar)(sum_weight_of_tree[i], sum_weight_of_tree[l]) == -1) i++;
        while ((*compar)(sum_weight_of_tree[j], sum_weight_of_tree[l]) == 1) j--;
        if (i <= j) {
            if (i < j) {
                double tmp = sum_weight_of_tree[i];
                sum_weight_of_tree[i] = sum_weight_of_tree[j];
                sum_weight_of_tree[j] = tmp;
                
                edge_id_t left_board = trees->p_edge_list[2*i+0];
                edge_id_t right_board = trees->p_edge_list[2*i+1];

                trees->p_edge_list[2*i+0] = trees->p_edge_list[2*j+0];
                trees->p_edge_list[2*j+0] = left_board;
                trees->p_edge_list[2*i+1] = trees->p_edge_list[2*j+1];
                trees->p_edge_list[2*j+1] = right_board;
            }
            i++;
            j--;
        }
    } while (i <= j);
    if (i < r) sortTrees(i, r, sum_weight_of_tree, compar, trees);
    if (l < j) sortTrees(l, j, sum_weight_of_tree, compar, trees);
}

void readTrees(forest_t *trees, char *filename)
{
    fprintf(stderr, "reading trees from %20s\t...  ",filename); fflush(NULL);
    FILE *F = fopen(filename, "rb");
    check(F, "Error opening file %s", filename);
    
    check(fread(&trees->numTrees, sizeof(vertex_id_t), 1, F) == 1, "No numTrees in the file");
    check(fread(&trees->numEdges, sizeof(edge_id_t), 1, F) == 1, "No numEdges in the file");
    trees->p_edge_list = (edge_id_t *)malloc((trees->numTrees*2) * sizeof(edge_id_t));
    check(trees->p_edge_list, "Cannot allocate memory for %d trees", trees->numTrees);
    check(fread(trees->p_edge_list, sizeof(edge_id_t), trees->numTrees*2, F) == (trees->numTrees*2), "Unable to read %d p_edge_list from %s", trees->numTrees, filename);
    
    trees->edge_id = (edge_id_t *)malloc((trees->numEdges) * sizeof(edge_id_t));
    check(trees->edge_id,"Cannot allocate memory for %d edges", trees->numEdges);
    check(fread(trees->edge_id, sizeof(edge_id_t), trees->numEdges, F) == (trees->numEdges), "Unable to read %d ");
    fprintf(stderr, " \t finished\n");
}

void readValidWeight(char *filename)
{
    fprintf(stderr, "reading weights from %18s\t...  ", filename); fflush(NULL);
    FILE *F = fopen(filename, "rb");
    if (!F) error(EXIT_FAILURE, 0, "Error in opening file %s", filename);

    assert(fread(&valid_num_trees, sizeof(vertex_id_t), 1, F) == 1);
    valid_weight_trees = (weight_t *)malloc(valid_num_trees * sizeof(weight_t));
    assert(valid_weight_trees);
    assert(fread(valid_weight_trees, sizeof(edge_id_t), valid_num_trees, F) == (valid_num_trees));
    fprintf(stderr, "\t finished\n");
}

extern "C" void validation(forest_t *trees, graph_t *G) 
{
    fprintf(stderr, "starting validation\t\t\t..."); fflush(NULL);
    weight_t *sum_weight_trees = (weight_t *)malloc(trees->numTrees * sizeof(weight_t));
    assert(sum_weight_trees);
    uint8_t *marked_vert_in_forest = (uint8_t *)malloc(G->n * sizeof(uint8_t));
    assert(marked_vert_in_forest);
    vertex_id_t *startV = (vertex_id_t *)malloc(G->rowsIndices[G->n] * sizeof(vertex_id_t));
    assert(startV);
    for (vertex_id_t i = 0; i < G->n; i++) marked_vert_in_forest[i] = 0;
   
    if (valid_num_trees != trees->numTrees) {
        printf("\terror\n");
        printf("number of trees is incorrect. Result: %d Valid: %d \n", trees->numTrees, valid_num_trees);
        exit(1);
    }

    /* calc and sort weight of each tree in forest */
    for (vertex_id_t i = 0; i < trees->numTrees; i++) {
        weight_t tmp_weight = 0;
        for (edge_id_t j = trees->p_edge_list[2*i+0]; j < trees->p_edge_list[2*i+1]; j++) {
            tmp_weight = tmp_weight + G->weights[trees->edge_id[j]];
        }
        sum_weight_trees[i] = tmp_weight;
    }
    sortTrees(0, trees->numTrees - 1, sum_weight_trees, compare_double, trees);
   
    
    for (vertex_id_t i = 0; i < G->n; i++) {
        for (edge_id_t j = G->rowsIndices[i]; j < G->rowsIndices[i+1]; j++) startV[j] = i;
    }
    for( uint64_t i = 0; i < valid_num_trees; i++) {
        min_valid_weight = valid_weight_trees[i]; // minimal non-zero tree's weight in the forest for error calculation 
        if( min_valid_weight != 0 ) break;
    }
    for (vertex_id_t i = 0; i < trees->numTrees; i++) {
        weight_t diff_weights = 0;
        if (sum_weight_trees[i] >= valid_weight_trees[i]) diff_weights = sum_weight_trees[i] - valid_weight_trees[i];    
        else diff_weights = valid_weight_trees[i] - sum_weight_trees[i];
        
        if (diff_weights / min_valid_weight < WEIGHT_ERROR ) {
            map <vertex_id_t, vertex_id_t> vertex_label;
            int64_t min_vertex = -1;
            // identification of vertex with minimal number and building of map with labels of verticis 
            for (edge_id_t j = trees->p_edge_list[2*i+0]; j <  trees->p_edge_list[2*i+1]; j++) {
                vertex_label[startV[trees->edge_id[j]]] = startV[trees->edge_id[j]];
                vertex_label[G->endV[trees->edge_id[j]]] = G->endV[trees->edge_id[j]];
            
                if (min_vertex == -1 || startV[trees->edge_id[j]] < min_vertex) min_vertex = startV[trees->edge_id[j]];
                if (G->endV[trees->edge_id[j]] < min_vertex) min_vertex = G->endV[trees->edge_id[j]];
            }

            bool label_no_change = false;  
            // connected components      
            while(!label_no_change) {
                label_no_change = true;
                for (edge_id_t j = trees->p_edge_list[2*i+0]; j <  trees->p_edge_list[2*i+1]; j++) {
                    if (vertex_label[startV[trees->edge_id[j]]] < vertex_label[G->endV[trees->edge_id[j]]]) {
                        vertex_label[vertex_label[G->endV[trees->edge_id[j]]]] = vertex_label[startV[trees->edge_id[j]]];
                    }
                    else if ( vertex_label[startV[trees->edge_id[j]]] > vertex_label[G->endV[trees->edge_id[j]]]) {
                        vertex_label[vertex_label[startV[trees->edge_id[j]]]] = vertex_label[G->endV[trees->edge_id[j]]];
                    }
                }
                for (map<vertex_id_t, vertex_id_t>::iterator it = vertex_label.begin(); it != vertex_label.end(); it++)
                {
                    if ((*it).second != vertex_label[(*it).second]) {
                        (*it).second = vertex_label[(*it).second];
                        label_no_change = false;
                    }
                }
            }
            for (map<vertex_id_t, vertex_id_t>::iterator it = vertex_label.begin(); it != vertex_label.end(); it++) {
                if ((*it).second != min_vertex) {
                    printf("\terror\n");
                    printf("there are more than one component in %d tree\n", i); 
                    exit(1);
                }
                marked_vert_in_forest[(*it).first] = 1;
            }
        }
        else {
            printf("\terror\n");
            printf("weight of %d tree is incorrect. Result: %.4f Valid: %.4f\n", i, sum_weight_trees[i], valid_weight_trees[i]);    
            exit(1);
        }
    }
    for (vertex_id_t i = 0; i < G->n; i++) {
        if(!marked_vert_in_forest[i]) {
            if (G->rowsIndices[i+1] - G->rowsIndices[i] != 0) {
                printf("\tthere is non-zero degree vertex <%d> in the graph, that are not included in any tree\n", i);
                exit(1);
            }
        }
    }
    free(sum_weight_trees);
    free(marked_vert_in_forest);
    free(startV);
    printf("\t ok\n");
}

void printGraph(graph_t *G)
{
    vertex_id_t i;
    edge_id_t j;
    for (i = 0; i < G->n; ++i) {
        printf("%d:", i);
        for (j=G->rowsIndices[i]; j < G->rowsIndices[i+1]; ++j)
            printf("%d (%f), ", G->endV[j], G->weights[j]);
        printf("\n");
    }
}


int main (int argc, char **argv)
{
    graph_t g;
    forest_t trees_result;
    init(argc, argv);
    readGraph(&g, inFilename_gr);
    readTrees(&trees_result, inFilename_result);
    readValidWeight(inFilename_valid);
    validation(&trees_result, &g);
    free(valid_weight_trees);
    freeGraph(&g);
    return 0;
}
