# Deliverable 1 for Parallel Computing at UniTn for year 24/25

This repository provides the files and build instructions
for the first deliverable. 

The folder ParcoDeliverable1 contains the source code

# Compilation

The source file has been modified to allow the usage of old compiler,
as initially it used features from C++17/20. Now, only C++11 is
necessary. 
Compiler: GCC/G++ 4.5+ or Clang 3.0+
I only tested using GCC, both on local machine and cluster

To compile (from the top level of the project): 
````
cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" .
make
````
This will build the executable and put it in the directory
ParcoDeliverable1, under the name ParcoDeliverable1

It is also possible to add -ffast-math and -fno-math-errno to the compile flags,
which produced a sensible speedup in the symmetry checks, but more or less
no gain to the transpose 

# To run (from top level)

OMP_SCHEDULE=static OMP_PROC_BIND=true ./ParcoDeliverable1/ParcoDeliverable1 N MAX_THREADS

Where N is the number of rows and columns and MAX_THREADS is
the maximum number of OMP threads used for the benchmarks

If you want to generate benchmark graphs, make sure
to install matplotlib and then use the python script present
in the top level directory:
````
python generate_graphs.py benchmark_file n_threads
````

Where benchmark_file is the output file of the executable
and n_threads selects a specific number of threads to compare
against the serial versions

# Results

The program runs the different version of the algorithm 10 times for each number of threads and computes
the average of the wall clock time.
Collected data is output to a file named bench.txt under a custom text format and can
be used by the python script to generate graphs