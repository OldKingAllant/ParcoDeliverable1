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

# Results

The program runs the different version of the algorithm 10 times for each number of threads and computes
the average of the wall clock time

Example run:
````
Testing for 8192 rows and columns
Which means 67108864 elements
For a total 0.262144 GB
Base symm check took 0.0005 ms
Matrix is not symmetric
checkSymImp took 5.1e-05 ms
checkSymOMP with 2 threads took 73.1731 ms
checkSymOMP with 4 threads took 42.5234 ms
checkSymOMP with 8 threads took 74.527 ms
checkSymOMP with 16 threads took 102.034 ms
Base transpose took 586.433 ms
Imp transpose took 92.3785 ms
OMP transpose with 2 threads took 75.9511 ms
OMP transpose with 4 threads took 75.7223 ms
OMP transpose with 8 threads took 86.6244 ms
OMP transpose with 16 threads took 104.492 ms
Oblivious transpose took 61.9979 ms
Oblivious OMP transpose with 2 threads took 40.7665 ms
Oblivious OMP transpose with 4 threads took 39.747 ms
Oblivious OMP transpose with 8 threads took 41.7035 ms
Oblivious OMP transpose with 16 threads took 40.5148 ms
````

The above results need some explanation:
- The unoptimized and improved symmetry check simply bail out as soon as
  a different value is found, which explains why they are so fast
- On the other hand, checkSymOMP checks the entire matrix
- Base transpose has wildly varying timings, probably due to the
  entire state of the system
- This example is taken from a run performed on my system