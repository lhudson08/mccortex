#include "global.h"
#include <time.h> // srand

#include "cmd.h"
#include "util.h"
#include "file_util.h"
#include "db_graph.h"
#include "db_node.h"
#include "binary_kmer.h"
#include "graph_format.h"
#include "path_format.h"
#include "graph_walker.h"

static const char usage[] =
"usage: traversal [options] <input.ctx>\n"
"  Test graph traversal code\n";

int main(int argc, char **argv)
{
  CmdArgs args;
  cmd_alloc(&args, argc, argv);

  if(args.argc != 1) print_usage(usage, NULL);
  const char *input_ctx_path = args.argv[0];

  /* initialize random seed: */
  srand(time(NULL));

  // probe binary
  boolean is_binary = false;
  GraphFileHeader gheader = {.capacity = 0};

  if(!graph_file_probe(input_ctx_path, &is_binary, &gheader))
    print_usage(usage, "Cannot read binary file: %s", input_ctx_path);
  else if(!is_binary)
    print_usage(usage, "Input binary file isn't valid: %s", input_ctx_path);

  // probe paths file
  const char *input_paths_file = args.ctp_path;

  boolean valid_paths_file = false;
  PathFileHeader pheader = {.capacity = 0};

  if(input_paths_file == NULL)
    warn("No paths file given (-p <in.ctp>)");
  else if(!paths_file_probe(input_paths_file, &valid_paths_file, &pheader))
    print_usage(usage, "Cannot read .ctp file: %s", input_paths_file);
  else if(!valid_paths_file)
    die("Invalid .ctp file: %s", input_paths_file);

  // paths_compatibility_check(db_graph, pheader);

  // Get starting bkmer
  // BinaryKmer bkmer, bkey;
  // if(strlen(start_kmer) != kmer_size) die("length of kmer does not match kmer_size");
  // bkmer = binary_kmer_from_str(start_kmer, kmer_size);

  // Decide on memory
  size_t hash_kmers, req_num_kmers = gheader.num_of_kmers*(1.0/IDEAL_OCCUPANCY);
  size_t hash_mem = hash_table_mem(req_num_kmers, &hash_kmers);
  size_t path_mem = args.mem_to_use - hash_mem;

  dBGraph db_graph;
  GraphWalker wlk;

  db_graph_alloc(&db_graph, gheader.kmer_size, gheader.num_of_cols, req_num_kmers);
  graph_walker_alloc(&wlk);

  size_t node_bit_fields = round_bits_to_words64(db_graph.ht.capacity);

  db_graph.edges = calloc2(db_graph.ht.capacity, sizeof(Edges));
  db_graph.node_in_cols = calloc2(node_bit_fields, sizeof(uint64_t));
  db_graph.kmer_paths = malloc2(db_graph.ht.capacity * sizeof(uint64_t));
  memset((void*)db_graph.kmer_paths, 0xff, db_graph.ht.capacity * sizeof(uint64_t));

  uint64_t *visited = calloc2(2 * node_bit_fields, sizeof(uint64_t));

  uint8_t *path_store = malloc2(path_mem);
  path_store_init(&db_graph.pdata, path_store, path_mem, gheader.num_of_cols);

  // Load graph
  SeqLoadingStats *stats = seq_loading_stats_create(0);
  SeqLoadingPrefs prefs = {.into_colour = 0, .merge_colours = false,
                           .boolean_covgs = false,
                           .load_binaries = true,
                           .must_exist_in_graph = false,
                           .empty_colours = true,
                           .db_graph = &db_graph};

  graph_load(input_ctx_path, &prefs, stats, NULL);
  seq_loading_stats_free(stats);

  hash_table_print_stats(&db_graph.ht);

  // Load path file
  if(input_paths_file != NULL) {
    paths_format_read(input_paths_file, &pheader, &db_graph,
                      &db_graph.pdata, false);
  }

  // Find start node
  // hkey_t node;
  // Orientation orient;
  // db_node_get_key(bkmer, kmer_size, bkey);
  // node = hash_table_find(&db_graph.ht, bkey);
  // orient = db_node_get_orientation(bkmer, bkey);
  Nucleotide lost_nuc;

  // char bkmerstr[MAX_KMER_SIZE+1];

  hkey_t node;
  Orientation orient;

  size_t i, j, len, junc, prev_junc, num_repeats = 10000;
  size_t total_len = 0, total_junc = 0, dead_ends = 0, n = 0;
  size_t lengths[num_repeats], junctions[num_repeats];

  size_t path_cap = 4096;
  hkey_t *path = malloc2(path_cap * sizeof(hkey_t));

  for(i = 0; i < num_repeats; i++)
  {
    orient = rand() & 0x1;
    node = db_graph_rand_node(&db_graph);

    len = junc = 0;
    path[len++] = node;

    graph_walker_init(&wlk, &db_graph, 0, node, orient);
    lost_nuc = binary_kmer_first_nuc(wlk.bkmer, db_graph.kmer_size);
    prev_junc = edges_get_outdegree(db_graph.edges[node], orient) > 1;

    // binary_kmer_to_str(wlk.bkmer, kmer_size, bkmerstr);
    // printf("%3zu %s\n", len, bkmerstr);
    // graph_walker_print_state(&wlk);

    while(graph_traverse(&wlk) && !db_node_has_traversed(visited, wlk.node, wlk.orient))
    {
      db_node_set_traversed(visited, wlk.node, wlk.orient);
      graph_walker_node_add_counter_paths(&wlk, wlk.node, wlk.orient, lost_nuc);
      lost_nuc = binary_kmer_first_nuc(wlk.bkmer, db_graph.kmer_size);
      if(len == path_cap) {
        path_cap *= 2;
        path = realloc2(path, path_cap * sizeof(hkey_t));
      }
      path[len++] = wlk.node;
      junc += prev_junc;
      prev_junc = edges_get_outdegree(db_graph.edges[wlk.node], wlk.orient) > 1;
      // binary_kmer_to_str(wlk.bkmer, kmer_size, bkmerstr);
      // printf("%3zu %s\n", len, bkmerstr);
    }

    // printf(" len: %zu junc: %zu\n", len, junc);

    graph_walker_finish(&wlk);

    for(j = 0; j < len; j++)
      db_node_fast_clear_traversed(visited, path[j]);

    if(edges_get_outdegree(db_graph.edges[wlk.node], wlk.orient) == 0) dead_ends++;
    // {
      lengths[n] = len;
      junctions[n] = junc;
      total_len += len;
      total_junc += junc;
      n++;
    // }
    // else {
    //   dead_ends++;
    // }
    // break;
  }

  free(path);

  message("\n");
  message("total_len: %zu; total_junc: %zu\n", total_len, total_junc);
  message("dead ends: %zu / %zu\n", dead_ends, num_repeats);
  message("mean length: %.2f\n", (double)total_len / n);
  message("mean junctions: %.1f per contig, %.2f%% nodes (1 every %.1f nodes)\n",
          (double)total_junc / n, (100.0 * total_junc) / total_len,
          (double)total_len / total_junc);

  qsort(lengths, n, sizeof(size_t), cmp_size);
  qsort(junctions, n, sizeof(size_t), cmp_size);

  double median_len = MEDIAN(lengths, n);
  double median_junc = MEDIAN(junctions, n);

  message("Median contig length: %f\n", median_len);
  message("Median junctions per contig: %f\n", median_junc);

  free(visited);
  free(db_graph.edges);
  free(db_graph.node_in_cols);
  free((void*)db_graph.kmer_paths);
  free(path_store);

  graph_header_dealloc(&gheader);
  paths_header_dealloc(&pheader);
  graph_walker_dealloc(&wlk);
  db_graph_dealloc(&db_graph);

  cmd_free(&args);
  return EXIT_SUCCESS;
}