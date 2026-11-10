#pragma once
#include <stddef.h>
#define MALLOC_CAP_8BIT 1U
#define MALLOC_CAP_INTERNAL 2U
#define MALLOC_CAP_DMA 4U
#define MALLOC_CAP_SPIRAM 8U
void *heap_caps_malloc(size_t size, unsigned capabilities);
void heap_caps_free(void *pointer);
