#pragma once
/* this file is to be modified depending on your usecase,
 * this is the version for the tests
 */

/* if enabled, loading png's from file are supported */
#define UPNG_USE_STDIO

/* memory interface */
void* test_upng_malloc(unsigned size, const char* file, int line);
void test_upng_free(void* ptr);
#define UPNG_MEM_ALLOC(size) test_upng_malloc((size), __FILE__, __LINE__)
#define UPNG_MEM_FREE test_upng_free
