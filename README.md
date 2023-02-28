# GBBS: Graph Based Benchmark Suite  ![Bazel build](https://github.com/paralg/gbbs/workflows/CI/badge.svg)

this fork makes python bindings for Hierarchical Agglomerative Clustering (more or less) usable

compile with:

```
bazel build //pybindings:gbbs_lib.so
```

and then use like this

```
sys.path.append(os.path.join(path/to/repo,'gbbs'))
sys.path.append(os.path.join(path/to/repo,'gbbs','bazel-bin','pybindings'))
import gbbs
import numpy as np

# with D a scipy sparse distance matrix
nz=D.nonzero()
m=np.vstack((nz[0],nz[1],D.data)).T
G=gbbs.numpyFloatEdgeListToSymmetricWeightedGraph(np.ascontiguousarray(m))
# this will work for single or complete linkage
L=G.HierarchicalAgglomerativeClustering('single',False)
```

Organization
--------

This repository contains code for the SPAA paper "Theoretically Efficient
Parallel Graph Algorithms Can Be Fast and Scalable" (SPAA'18). It includes
implementations of Graph-Based Hierarchical Agglomerative Clustering (Graph HAC)

The code for these applications is located in the `benchmark` directory. The
implementations are based on the Ligra/Ligra+/Julienne graph processing
frameworks. The framework code is located in the `src` directory.

If you use this work, please cite the [paper](https://arxiv.org/abs/1805.05208):

```
@inproceedings{dhulipala2018theoretically,
  author    = {Laxman Dhulipala and
               Guy E. Blelloch and
               Julian Shun},
  title     = {Theoretically Efficient Parallel Graph Algorithms Can Be Fast and
               Scalable},
  booktitle = {ACM Symposium on Parallelism in Algorithms and Architectures (SPAA)},
  year      = {2018},
}
```

Compilation
--------

Compiler:
* g++ &gt;= 7.4.0 with support for Cilk Plus
* g++ &gt;= 7.4.0 with pthread support (Homemade Scheduler)

Build system:
* [Bazel](https://docs.bazel.build/versions/master/install.html) 2.1.0
* Make --- though our primary build system is Bazel, we also maintain Makefiles
  for those who wish to run benchmarks without installing Bazel.

The default compilation uses a lightweight scheduler developed at CMU (Homemade)
for parallelism, which results in comparable performance to Cilk Plus. The
half-lengths for certain functions such as histogramming are lower using
Homemade, which results in better performance for codes like KCore.

The benchmark supports both uncompressed and compressed graphs. The uncompressed
format is identical to the uncompressed format in Ligra. The compressed format,
called bytepd_amortized (bytepda) is similar to the parallelByte format used in
Ligra+, with some additional functionality to support efficiently packs,
filters, and other operations over neighbor lists.

To compile codes for graphs with more than 2^32 edges, the `LONG` command-line
parameter should be set. If the graph has more than 2^32 vertices, the
`EDGELONG` command-line parameter should be set. Note that the codes have not
been tested with more than 2^32 vertices, so if any issues arise please contact
[Laxman Dhulipala](mailto:ldhulipa@cs.cmu.edu).

To compile with the Cilk Plus scheduler instead of the Homegrown scheduler, use
the Bazel configuration `--config=cilk`. To compile using OpenMP instead, use
the Bazel configuration `--config=openmp`. To compile serially instead, use the
Bazel configuration `--config=serial`. (For the Makefiles, instead set the
environment variables `CILK`, `OPENMP`, or `SERIAL` respectively.)

To build:
```sh
# Load external libraries as submodules. (This only needs to be run once.)
git submodule update --init

# For Bazel:
$ bazel build  //...  # compiles all benchmarks

# For Make:
# First set the appropriate environment variables, e.g., first run
# `export CILK=1` to compile with Cilk Plus.
# After that, build using `make`.
$ cd benchmarks/Clustering/SeqHAC  # go to a benchmark
$ make
```
Note that the default compilation mode in bazel is to build optimized binaries
(stripped of debug symbols). You can compile debug binaries by supplying `-c
dbg` to the bazel build command.

The following commands cleans the directory:
```sh
# For Bazel:
$ bazel clean  # removes all executables

# For Make:
$ make clean  # removes executables for the current directory
```

Input Formats
-----------
We support the adjacency graph format used by the [Problem Based Benchmark
suite](http://www.cs.cmu.edu/~pbbs/benchmarks/graphIO.html)
and [Ligra](https://github.com/jshun/ligra).

The adjacency graph format starts with a sequence of offsets one for each
vertex, followed by a sequence of directed edges ordered by their source vertex.
The offset for a vertex i refers to the location of the start of a contiguous
block of out edges for vertex i in the sequence of edges. The block continues
until the offset of the next vertex, or the end if i is the last vertex. All
vertices and offsets are 0 based and represented in decimal. The specific format
is as follows:

```
AdjacencyGraph
<n>
<m>
<o0>
<o1>
...
<o(n-1)>
<e0>
<e1>
...
<e(m-1)>
```

This file is represented as plain text.

Weighted graphs are represented in the weighted adjacency graph format. The file
should start with the string "WeightedAdjacencyGraph". The m edge weights
should be stored after all of the edge targets in the .adj file.
