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

namespace onetable {

  class OnelevelHash {
    public:
      using T = pbbslib::sparse_table<unsigned __int128, long, hash128>;
      T table;
      int rr;
      template <class Graph>
      OnelevelHash(int r, Graph& DG, size_t max_deg) {
        rr = r;
        // count cliques
        timer t_pre; t_pre.start();
        size_t pre_count = 0;
        // Clique counting
        if (r == 3) pre_count = TriClique_count(DG, false, nullptr);
        else pre_count = Clique_count(DG, r, 5, true, false, 0, nullptr);
        std::cout << "Pre count " << r << ": " << pre_count << std::endl;
        double tt_pre = t_pre.stop();
        std::cout << "### Pre count: " << tt_pre << std::endl;

        std::cout << "Start table" << std::endl;
        table = pbbslib::sparse_table<unsigned __int128, long, hash128>(
          pre_count,
          std::make_tuple(static_cast<unsigned __int128>(0), long{0}), hash128{});
      }

      void insert(sequence<uintE>& base, int r, int k) {
        auto add_f = [&] (long* ct, const std::tuple<unsigned __int128, long>& tup) {
          pbbs::fetch_and_add(ct, (long)1);
        };
        // Sort base
        uintE base2[10];
        assert(10 > k);
        for(std::size_t i = 0; i < k + 1; i++) {
          base2[i] = base[i];
        }
        std::sort(base2, base2 + k + 1,std::less<uintE>());
        //sequence<uintE> base2(base);
        //pbbs::sample_sort_inplace(base2.slice(), std::less<uintE>());

        std::string bitmask(r+1, 1); // K leading 1's
        bitmask.resize(k+1, 0); // N-K trailing 0's

        do {
          unsigned __int128 key = 0;
          for (int i = 0; i < static_cast<int>(k)+1; ++i) {
            if (bitmask[i]) {
              key = key << 32;
              key |= static_cast<int>(base2[i]);
            }
          }
          table.insert_f(std::make_tuple(key, (long) 1), add_f);
          // TESTING EXTRACTION OF KEy
          /*for (int j = 0; j < static_cast<int>(r)+1; ++j) {
            int extract = (int) key;
            if (static_cast<uintE>(extract) >= DG.n) {
              std::cout << "Extract: " << static_cast<uintE>(extract) << ", n: " << DG.n << std::endl;
            }
            assert(static_cast<uintE>(extract) < DG.n);
          key = key >> 32;
        }*/
        } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
      }

      std::size_t return_total() {return table.m;}
      long get_count(std::size_t i) {
        if (std::get<0>((table.table)[i]) == 0) return 0;
        return std::get<1>((table.table)[i]);
      }
      size_t update_count(std::size_t i, size_t update){
        auto val = std::get<1>(table.table[i]) - update;
        table.table[i] = std::make_tuple(std::get<0>(table.table[i]), val);
        return val;
      }
      void clear_count(std::size_t index) {
        table.table[index] = std::make_tuple(std::get<0>(table.table[index]),0);
      }

      template<class I>
      void extract_indices(sequence<uintE>& base, I func, int r, int k) {
        // Sort base
        uintE base2[10];
        assert(10 > k);
        for(std::size_t i = 0; i < k + 1; i++) {
          base2[i] = base[i];
        }
        std::sort(base2, base2 + k + 1,std::less<uintE>());
        //sequence<uintE> base2(base);
        //pbbs::sample_sort_inplace(base2.slice(), std::less<uintE>());
        std::string bitmask(r+1, 1); // K leading 1's
        bitmask.resize(k+1, 0); // N-K trailing 0's

        do {
          unsigned __int128 key = 0;
          for (int i = 0; i < static_cast<int>(k)+1; ++i) {
            if (bitmask[i]) {
              key = key << 32;
              key |= static_cast<int>(base2[i]);
            }
          }
          auto index = table.find_index(key);
          func(index);
        } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
      }
    
    template<class S, class Graph>
    void extract_clique(S index, sequence<uintE>& base, Graph& G, int k) {
      auto vert = std::get<0>(table.table[index]);
      for (int j = 0; j < rr; ++j) {
        int extract = (int) vert; // vert & mask
        //if (static_cast<uintE>(extract) >= G.n) {
        //  std::cout << "Vert: " << static_cast<uintE>(extract) << ", n: " << G.n << std::endl;
        //}
        assert(static_cast<uintE>(extract) < G.n);
        if (j == rr - 1) base[0] = static_cast<uintE>(extract);
        else base[k - j] = static_cast<uintE>(extract);
        vert = vert >> 32;
      }
    }
  };

} // end namespace onetable

} //end namespace gbbs