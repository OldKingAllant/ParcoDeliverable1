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

To compile: g++ -std=c++11 -msse4.1 -fopenmp -lgomp -O3 ParcoDeliverable1/ParcoDeliverable1.cpp -o ParcoDeliverable1

# To run

./ParcoDeliverable1 <N>

Where N is the number of rows and columns 

# Results

The program runs the different version of the algorithm 100 times each and computes
the average of the wall clock time

Example run:
````
Testing for 8000 rows and columns
Which means 64000000 elements
For a total 0.25 GB
Base symm check took 0.0003 ms
Matrix is not symmetric
checkSymImp took 5e-05 ms
checkSymOMP took 19.6392 ms
Base transpose took 436.599 ms
Imp transpose took 46.3716 ms
OMP transpose took 37.8199 ms
````
