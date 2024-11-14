#ifndef PARCO_UTILS
#define PARCO_UTILS

#include "Defs.h"

/// <summary>
/// Inits random number generation
/// </summary>
void InitRand();

/// <summary>
/// Allocates N*N contiguous memory and
/// initializes it with random numbers
/// </summary>
/// <param name="N">N rows and columns</param>
/// <param name="N_THREADS">Number of threads for init (useful for NUMA)</param>
/// <returns></returns>
Matrix CreateRandomMatrix(uint32_t N, uint32_t N_THREADS);

/// <summary>
/// Print matrix to console
/// </summary>
/// <param name="mat">Pointer to matrix</param>
/// <param name="N">N rows and cols</param>
void PrintMatrix(MatType* mat, uint32_t N);

/// <summary>
/// Checks if the two matrices are one and the same
/// </summary>
/// <param name="M">Left</param>
/// <param name="T">Right</param>
/// <param name="N">N rows and columns</param>
/// <returns> >0 if M!=T</returns>
int IsSameMatrix(MatType const* M, MatType const* T, uint32_t N);

/// <summary>
/// Attempts to parse a uint32 from
/// the given parameter and prints 
/// err_msg to the console if it fails
/// (after that it simply stops execution)
/// </summary>
/// <param name="str">Origin string</param>
/// <param name="err_msg">Error message</param>
/// <returns>The number</returns>
uint32_t TryParseUint32(const char* str, const char* err_msg);

////////////////////////////
//Ignore these two functions

MatType* AllocateAndInit(uint32_t N);

bool VerifyNestedAvail();

#endif // !PARCO_UTILS
