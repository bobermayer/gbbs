#pragma once

#include <math.h>
#include <limits>

// Library dependencies
#include "gbbs/bucket.h"
#include "gbbs/edge_map_reduce.h"
#include "gbbs/gbbs.h"
#include "gbbs/pbbslib/dyn_arr.h"
#include "gbbs/pbbslib/sparse_table.h"
#include "gbbs/pbbslib/sparse_additive_map.h"
#include "pbbslib/assert.h"
#include "pbbslib/list_allocator.h"
#include "pbbslib/integer_sort.h"

// Clique files
#include "benchmarks/CliqueCounting/intersect.h"
#include "benchmarks/CliqueCounting/induced_intersection.h"
#include "benchmarks/CliqueCounting/induced_neighborhood.h"
#include "benchmarks/CliqueCounting/induced_hybrid.h"
#include "benchmarks/CliqueCounting/induced_split.h"
#include "benchmarks/CliqueCounting/relabel.h"

#include "commontable.h"
  
namespace gbbs {

namespace twotable {

  struct EndTable {
    pbbslib::sparse_table<unsigned __int128, long, hash128> table;
    uintE vtx;
    //MidTable* up_table;
  };

  struct MidTable {
    pbbslib::sparse_table<uintE, EndTable*, std::hash<uintE>> table;
    sequence<EndTable*> arr;
  };
  
  class TwolevelHash {
    public:
      using T = pbbslib::sparse_table<unsigned __int128, long, hash128>;
      using X = std::tuple<unsigned __int128, long>;
      MidTable top_table;
      sequence<long> top_table_sizes;
      int rr;
      std::size_t total = 0;
      X* space = nullptr;
      bool contiguous_space;
  
      template <class Graph>
      TwolevelHash(int r, Graph& DG, size_t max_deg, bool _contiguous_space) {
        contiguous_space = _contiguous_space;
        rr = r;
        //top_table.up_table = nullptr;
        // How many vert in top level?
        // For each vert in top level, how many pairs of vert finish it?
        auto tmp_table = pbbslib::sparse_additive_map<uintE, long>(
          DG.n, std::make_tuple(UINT_E_MAX, long{0}));
        auto base_f = [&](sequence<uintE>& base){
          //auto min_vert = pbbslib::reduce_min(base);
          auto min_vert = base[0];
          auto tmp = std::make_tuple<uintE, long>(static_cast<uintE>(min_vert), long{1});
          tmp_table.insert(tmp);
        };
        auto init_induced = [&](HybridSpace_lw* induced) { induced->alloc(max_deg, r-1, DG.n, true, true); };
        auto finish_induced = [&](HybridSpace_lw* induced) { if (induced != nullptr) { delete induced; } };
        parallel_for_alloc<HybridSpace_lw>(init_induced, finish_induced, 0, DG.n, [&](size_t i, HybridSpace_lw* induced) {
          if (DG.get_vertex(i).getOutDegree() != 0) {
            induced->setup(DG, r-1, i);
            auto base = sequence<uintE>(r);
            base[0] = i;
            NKCliqueDir_fast_hybrid_rec(DG, 1, r-1, induced, base_f, base);
          }
        }, 1, false);
        auto top_table_sizes2 = tmp_table.entries();
        // Modify top_table_sizes2 to be appropriately oversized
        parallel_for(0, top_table_sizes2.size(), [&](std::size_t i) {
          auto m = (size_t)1 << pbbslib::log2_up((size_t)(1.1 * std::get<1>(top_table_sizes2[i])) + 1);
          top_table_sizes2[i] = std::make_tuple(std::get<0>(top_table_sizes2[i]), m);
        });
        // Do a scan inplace
        auto add_tuple_monoid = pbbs::make_monoid([](std::tuple<uintE, long> a, std::tuple<uintE, long> b){
          return std::make_tuple(std::get<0>(b), std::get<1>(a) + std::get<1>(b));
        }, std::make_tuple(UINT_E_MAX, 0));
        std::tuple<uintE, long> total_top_table_sizes2 = scan_inplace(top_table_sizes2.slice(), add_tuple_monoid);
        // Allocate space for the second level tables
        if (contiguous_space) space = pbbslib::new_array_no_init<X>(std::get<1>(total_top_table_sizes2));
        tmp_table.del();
  
        //*** for arr
        top_table.arr = sequence<EndTable*>(DG.n, [](std::size_t i){return nullptr;});
        top_table_sizes = sequence<long>(DG.n + 1, long{0});
        /*top_table.table = pbbslib::sparse_table<uintE, EndTable*, std::hash<uintE>>(
          top_table_sizes2.size(),
          std::make_tuple<uintE, EndTable*>(UINT_E_MAX, static_cast<EndTable*>(nullptr)),
          std::hash<uintE>());
        top_table_sizes = sequence<long>(top_table.table.m + 1, long{0});*/
  
        parallel_for(0, top_table_sizes2.size(), [&](std::size_t i){
          auto vtx = i == top_table_sizes2.size() - 1 ? std::get<0>(total_top_table_sizes2) : 
            std::get<0>(top_table_sizes2[i + 1]);
          auto upper_size = i == top_table_sizes2.size() - 1 ? std::get<1>(total_top_table_sizes2) : 
            std::get<1>(top_table_sizes2[i + 1]);
          auto size = upper_size - std::get<1>(top_table_sizes2[i]);
          EndTable* end_table = new EndTable();
          end_table->vtx = vtx;
          end_table->table = contiguous_space ? pbbslib::sparse_table<unsigned __int128, long, hash128>(
            size, 
            std::make_tuple<unsigned __int128, long>(static_cast<unsigned __int128>(0), static_cast<long>(0)),
            hash128{},
            space + std::get<1>(top_table_sizes2[i])
            ) :
            pbbslib::sparse_table<unsigned __int128, long, hash128>(
            size, 
            std::make_tuple<unsigned __int128, long>(static_cast<unsigned __int128>(0), static_cast<long>(0)),
            hash128{}, 1);
          /*top_table.table.insert(std::make_tuple(vtx, end_table));
          std::size_t l = top_table.table.find_index(vtx);
          top_table_sizes[l] = end_table->table.m;*/
          //***for arr
          top_table.arr[vtx] = end_table;
          top_table_sizes[vtx] = end_table->table.m;
        });
        total = scan_inplace(top_table_sizes.slice(), pbbs::addm<long>());
        /*for (std::size_t i = 1; i < top_table_sizes.size(); i++) {
          assert(top_table_sizes[i - 1] <= top_table_sizes[i]);
          EndTable* end_table = std::get<1>(top_table.table.table[i - 1]);
          if (end_table != nullptr) {
            if (top_table_sizes[i] - top_table_sizes[i - 1] != (end_table->table).m) {
              std::cout << "M: " << (end_table->table).m << std::endl;
              std::cout << "Diff: " << top_table_sizes[i] - top_table_sizes[i - 1] << std::endl;
              fflush(stdout);
            }
            assert(top_table_sizes[i] - top_table_sizes[i - 1] == (end_table->table).m);
          }
        }
        assert(top_table_sizes[0] == 0);*/
      }
  
      void insert(sequence<uintE>& base, int r, int k) {
        auto add_f = [&] (long* ct, const std::tuple<unsigned __int128, long>& tup) {
          pbbs::fetch_and_add(ct, (long)1);
        };
        // Sort base
        /*uintE base2[10];
        assert(10 > k);
        for(std::size_t i = 0; i < k + 1; i++) {
          base2[i] = base[i];
        }
        std::sort(base2, base2 + k + 1,std::less<uintE>());*/

        std::string bitmask(r+1, 1); // K leading 1's
        bitmask.resize(k+1, 0); // N-K trailing 0's

        do {
          bool use_vtx = false;
          uintE vtx = 0;
          unsigned __int128 key = 0;
          for (int i = 0; i < static_cast<int>(k)+1; ++i) {
            if (bitmask[i]) {
              if (!use_vtx) {
                use_vtx = true;
                vtx = base[i];
              } else {
                key = key << 32;
                key |= static_cast<int>(base[i]);
              }
            }
          }
          //EndTable* end_table = top_table.table.find(vtx, nullptr);
          //***for arr
          EndTable* end_table = top_table.arr[vtx];
          //assert(end_table != nullptr);
          (end_table->table).insert_f(std::make_tuple(key, (long) 1), add_f);
        } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
      }

      std::size_t return_total() {return total;}

      size_t get_top_index(std::size_t index) {
        // This gives the first i such that top_table_sizes[i] >= index
        auto idx = pbbslib::binary_search(top_table_sizes, long{index}, std::less<long>());
        if (idx >= top_table_sizes.size()) return top_table_sizes.size() - 1;
        if (idx == 0) return idx;
        if (top_table_sizes[idx] == index) {
          while(top_table_sizes[idx] == index) {
            idx--;
          }
          return idx++;
        }
        return idx - 1;
      }

      long get_count(std::size_t index) {
        if (contiguous_space) return std::get<1>(space[index]);
        //assert(index < total);
        size_t top_index = get_top_index(index);
        //assert(top_index != top_table_sizes.size());
        /*if (top_table_sizes[top_index] > index) {
          std::cout << "Size: " << top_table_sizes[top_index] << std::endl;
          std::cout << "Idx: " << index << std::endl;
          fflush(stdout);
        }
        assert(top_table_sizes[top_index] <= index);
        assert(top_index < top_table.table.m);
        if (top_index < top_table_sizes.size() - 1) {
          if ((top_table_sizes[top_index + 1] < index)) {
            std::cout << "Top: " << top_table_sizes[top_index + 1] << ", Idx: " << index << std::endl;
            fflush(stdout);
          }
          assert(top_table_sizes[top_index + 1] >= index);
        }*/
        //***for arr
        //EndTable* end_table = std::get<1>(top_table.table.table[top_index]);
        EndTable* end_table = top_table.arr[top_index];
        if (end_table == nullptr) return 0;
        size_t bottom_index = index - top_table_sizes[top_index];
        /*if (bottom_index >= (end_table->table).m) {
          std::cout << "Bottom: " << bottom_index << std::endl;
          std::cout << "M: " << (end_table->table).m << std::endl;
          fflush(stdout);
        }
        assert(bottom_index < (end_table->table).m);*/
        return std::get<1>((end_table->table).table[bottom_index]);
      }

      size_t update_count(std::size_t index, size_t update){
        if (contiguous_space) {
          auto val = std::get<1>(space[index]) - update;
          space[index] =
            std::make_tuple(std::get<0>(space[index]), val);
          return val;
        }
        size_t top_index = get_top_index(index);
        //EndTable* end_table = std::get<1>(top_table.table.table[top_index]);
        //***for arr
        EndTable* end_table = top_table.arr[top_index];
        size_t bottom_index = index - top_table_sizes[top_index];
        auto val = std::get<1>((end_table->table).table[bottom_index]) - update;
        (end_table->table).table[bottom_index] = std::make_tuple(
          std::get<0>((end_table->table).table[bottom_index]), val
        );
        return val;
      }

      void clear_count(std::size_t index) {
        if (contiguous_space) {
          space[index] = std::make_tuple(std::get<0>(space[index]), 0);
          return;
        }
        size_t top_index = get_top_index(index);
        //***for arr
        //EndTable* end_table = std::get<1>(top_table.table.table[top_index]);
        EndTable* end_table = top_table.arr[top_index];
        size_t bottom_index = index - top_table_sizes[top_index];
        (end_table->table).table[bottom_index] = std::make_tuple(
          std::get<0>((end_table->table).table[bottom_index]), 0
        );
      }

      template<class I>
      void extract_indices(sequence<uintE>& base, I func, int r, int k) {
        /*uintE base2[10];
        assert(10 > k);
        for(std::size_t i = 0; i < k + 1; i++) {
          base2[i] = base[i];
        }
        std::sort(base2, base2 + k + 1,std::less<uintE>());*/
        std::string bitmask(r+1, 1); // K leading 1's
        bitmask.resize(k+1, 0); // N-K trailing 0's
        do {
          bool use_vtx = false;
          uintE vtx = 0;
          unsigned __int128 key = 0;
          for (int i = 0; i < static_cast<int>(k)+1; ++i) {
            if (bitmask[i]) {
              if (!use_vtx) {
                use_vtx = true;
                vtx = base[i];
              } else {
                key = key << 32;
                key |= static_cast<int>(base[i]);
              }
            }
          }
          // First, find index in top_table
          // This should populate into a prefix sum of sizes
          /*auto top_index = top_table.table.find_index(vtx);
          auto prefix = top_table_sizes[top_index];
          EndTable* end_table = std::get<1>(top_table.table.table[top_index]);*/
          //***for arr
          EndTable* end_table = top_table.arr[vtx];
          auto prefix = top_table_sizes[vtx];
          auto index = (end_table->table).find_index(key);
          func(prefix + index);
        } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
      }

      // Given an index, get the clique
      template<class S, class Graph>
      void extract_clique(S index, sequence<uintE>& base, Graph& G, int k) {
        unsigned __int128 vert;
        if (!contiguous_space) {
          // First, do a binary search for index in prefix
          size_t top_index = get_top_index(index);
          /*base[0] = std::get<0>(top_table.table.table[top_index]);
          EndTable* end_table = std::get<1>(top_table.table.table[top_index]);*/
          //***for arr
          base[0] = top_index;
          EndTable* end_table = top_table.arr[top_index];
          size_t bottom_index = index - top_table_sizes[top_index];
          vert = std::get<0>((end_table->table).table[bottom_index]);
        } else {
          vert = std::get<0>(space[index]);
        }
        for (int j = 0; j < rr - 1; ++j) {
          int extract = (int) vert; // vert & mask
          /*if (static_cast<uintE>(extract) >= G.n) {
            std::cout << "Vert: " << static_cast<uintE>(extract) << ", n: " << G.n << std::endl;
          }*/
          assert(static_cast<uintE>(extract) < G.n);
          base[k - j] = static_cast<uintE>(extract);
          vert = vert >> 32;
        }
      }
  };

} // end namespace twotable

} //end namespace gbbs