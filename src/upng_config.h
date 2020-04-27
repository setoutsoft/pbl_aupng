#pragma once
/* this file is to be modified depending on your usecase,
 * this is the version for the tests
 */

/* if enabled, loading png's from file are supported */
#define UPNG_USE_STDIO

/* memory interface */
#define UPNG_MEM_ALLOC malloc
#define UPNG_MEM_FREE free
