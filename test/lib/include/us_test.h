/*! \file us_test.h
 * \brief things required for the testing system
 */
#ifndef US_TEST_H
#define US_TEST_H
#include <stdio.h>
#include <stdlib.h>

/*! 
 * \brief ensure that the expression is true, otherwise print the formatted message, and exit
 * \param expr the expression to check
 */
#define us_assert(expr,...) do { if (!(expr)) { printf("Assertion failed at %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); exit(-1); } } while(0)

#endif
