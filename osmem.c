// SPDX-License-Identifier: BSD-3-Clause
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "block_meta.h"
#include "osmem.h"

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define META_SIZE sizeof(struct block_meta)
#define MMAP_THRESHOLD (1024 * 128)
#define MIN_SPACE_BLOCK (8 + META_SIZE)
#define ALIGN_UP(size) (((size) + ALIGNMENT) & ~(ALIGNMENT - 1))
#define PAGE_SIZE getpagesize()

struct block_meta *heap_start;
int heap_preallocated;
int is_calloc;
int calloc_numerator;

struct block_meta *preallocate_heap(void)
{
	heap_start = (struct block_meta *)sbrk(MMAP_THRESHOLD);
	if (heap_start == (void *)-1)
		return NULL;
	heap_start->size = MMAP_THRESHOLD - META_SIZE;
	heap_start->next = NULL;
	heap_start->prev = NULL;
	heap_start->status = STATUS_ALLOC;

	return heap_start;
}

struct block_meta *coalesce(struct block_meta *block)
{
	struct block_meta *next_block = block->next;
	struct block_meta *prev_block = block->prev;

	if (prev_block != NULL && prev_block->status == STATUS_FREE) {
		prev_block->size += block->size + META_SIZE;
		prev_block->next = block->next;
		if (block->next != NULL)
			block->next->prev = prev_block;
		block = prev_block;
	}

	if (next_block != NULL && next_block->status == STATUS_FREE) {
		block->size += next_block->size + META_SIZE;
		block->next = next_block->next;
		if (next_block->next != NULL)
			next_block->next->prev = block;
	}

	return block;
}

struct block_meta *split(struct block_meta *block, size_t size)
{
	size = ALIGN(size);
	unsigned long free_space = block->size - size - META_SIZE;
	struct block_meta *new_block = (struct block_meta *)((char *)block + META_SIZE + size);

	new_block->size = free_space;
	new_block->status = STATUS_FREE;
	new_block->next = block->next;
	new_block->prev = block;
	new_block = coalesce(new_block);
	block->size = size;
	block->status = STATUS_ALLOC;
	block->next = new_block;
	return block;
}

struct block_meta *find_free_block(struct block_meta *heap_start, size_t size)
{
	struct block_meta *current = heap_start;
	struct block_meta *best_fit = NULL;

	size = ALIGN(size);
	while (current != NULL) {
		if (current->status == STATUS_FREE && current->size >= size) {
			if (best_fit == NULL || current->size < best_fit->size)
				best_fit = current;
		} else if (current->status == STATUS_FREE && current->size < size && current->next == NULL) {
			size_t extra_space = size - current->size;

			if (sbrk(extra_space) == (void *)-1)
				return NULL;
			current->size = size;
			return current;
		}
		current = current->next;
	}

	if (best_fit != NULL) {
		if (best_fit->size >= size + META_SIZE) {
			best_fit->status = STATUS_ALLOC;
			best_fit = split(best_fit, size);
		}
		return best_fit;
	}

	struct block_meta *new_block = (struct block_meta *)sbrk(size + META_SIZE);

	if (new_block == (void *)-1)
		return NULL;
	new_block->size = size;
	new_block->status = STATUS_ALLOC;
	new_block->next = NULL;
	new_block->prev = NULL;

	struct block_meta *aux = heap_start;

	while (aux->next != NULL)
		aux = aux->next;
	aux->next = new_block;
	new_block->prev = aux;
	return new_block;
}

void *os_malloc(size_t size)
{
	/* TODO: Implement os_malloc */
	if (size == 0)
		return 0;

	size = ALIGN(size);
	size_t threshold = is_calloc ? PAGE_SIZE : MMAP_THRESHOLD;

	if (is_calloc == 1)
		calloc_numerator++;

	if (heap_preallocated == 0 && size + META_SIZE < threshold) {
		heap_start = preallocate_heap();
		if (heap_start == NULL)
			return NULL;
		heap_preallocated = 1;
		return (void *)((char *)heap_start + META_SIZE);
	}

	if (heap_preallocated == 1 && size + META_SIZE < threshold) {
		struct block_meta *block = find_free_block(heap_start, size);

		if (block == NULL)
			return NULL;
		block->status = STATUS_ALLOC;
		return (void *)((char *)block + META_SIZE);
	} else if (size + META_SIZE >= threshold) {
		heap_start = (struct block_meta *)mmap(0, size + META_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
		if (heap_start == MAP_FAILED)
			return NULL;
		struct block_meta *block = heap_start;

		block->size = size + META_SIZE;
		block->status = STATUS_MAPPED;
		block->next = NULL;
		block->prev = NULL;
		return (void *)((char *)block + META_SIZE);
		}
	return NULL;
}

void os_free(void *ptr)
{
	/* TODO: Implement os_free */
	if (ptr == NULL)
		return;
	struct block_meta *block = (struct block_meta *)((char *)ptr - META_SIZE);

	if (block->status == STATUS_MAPPED) {
		block->status = STATUS_FREE;
		munmap((void *)block, block->size);
		return;
	}

	block->status = STATUS_FREE;
	block = coalesce(block);
}

void *os_calloc(size_t nmemb, size_t size)
{
	/* TODO: Implement os_calloc */
	if (nmemb == 0 || size == 0)
		return NULL;

	size_t total_size = nmemb * size;

	total_size = ALIGN(total_size);
	is_calloc = 1;
	void *ptr = os_malloc(total_size);

	if (ptr == NULL) {
		is_calloc = 0;
		return NULL;
	}

	is_calloc = 0;

	if (ptr != NULL)
		memset(ptr, 0, total_size);

	return ptr;
}

void *os_realloc(void *ptr, size_t size)
{
	/* TODO: Implement os_realloc */
	if (ptr == NULL)
		return os_malloc(size);

	if (size == 0) {
		os_free(ptr);
		return NULL;
	}

	size = ALIGN(size);

	struct block_meta *old_block = (struct block_meta *)((char *)ptr - META_SIZE);

	if (old_block->size >= size && size < MMAP_THRESHOLD && old_block->status == STATUS_ALLOC) {
		unsigned long free_space = old_block->size - size;

		if (free_space >= MIN_SPACE_BLOCK) {
			struct block_meta *new_block = (struct block_meta *)((char *)old_block + META_SIZE + size);

			new_block->size = free_space - META_SIZE;
			new_block->status = STATUS_FREE;
			new_block->next = old_block->next;
			new_block->prev = old_block;
			new_block = coalesce(new_block);

			if (old_block->next != NULL)
				old_block->next->prev = new_block;

			old_block->next = new_block;
			old_block->size = size;
		}
		return ptr;
	}
	if (old_block->size < size && old_block->next == NULL) {
		size_t extra_space = size - old_block->size;
			if (sbrk(extra_space) == (void *)-1)
				return NULL;
			old_block->size = size;
			return ptr;
	}

	if (old_block->size < size && size < MMAP_THRESHOLD && old_block->next->status == STATUS_ALLOC) {
		struct block_meta *current = old_block;
		size_t alloc_space = old_block->size;

		while (current->status == STATUS_ALLOC && current->next != NULL) {
			alloc_space = alloc_space + current->size;
			current = current->next;
		}

		if (alloc_space < size && current->next == NULL) {
			if (sbrk(size - alloc_space) == (void *)-1)
				return NULL;
			old_block->size = size;
			return ptr;
		}
	}

	if (old_block->size < size && size < MMAP_THRESHOLD && old_block->next != NULL) {
		struct block_meta *current = old_block;
		size_t alloc_space = old_block->size;

		while (current->next != NULL && current->status == STATUS_ALLOC) {
			current = current->next;
			if (current->status == STATUS_FREE) {
				alloc_space += current->size + META_SIZE;
				if (alloc_space >= size) {
					old_block->size = alloc_space;
					old_block->next = current->next;
					return ptr;
				}
			}
		}
		if (current->next == NULL) {
			if (sbrk(size - alloc_space) == (void *)-1)
				return NULL;
			old_block->size = size;
			return ptr;
		}
	}
	if (old_block->status == STATUS_MAPPED) {
		void *new_ptr = os_malloc(size);

		if (new_ptr == NULL)
			return NULL;
		munmap((void *)old_block, old_block->size);
		return new_ptr;
	}

	void *new_ptr = os_malloc(size);

	if (new_ptr == NULL)
		return NULL;
	memcpy(new_ptr, ptr, size);
	os_free(ptr);
	return new_ptr;
}
