/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2011, 2013-2015 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file ump_kernel_descriptor_mapping.h
 */

#ifndef __UMP_KERNEL_DESCRIPTOR_MAPPING_H__
#define __UMP_KERNEL_DESCRIPTOR_MAPPING_H__

#include "mali_osk.h"

/**
 * The actual descriptor mapping table, never directly accessed by clients
 */
typedef struct ump_descriptor_table {
	u32 *usage;  /**< Pointer to bitpattern indicating if a descriptor is valid/used or not */
	void **mappings; /**< Array of the pointers the descriptors map to */
} ump_descriptor_table;

/**
 * The descriptor mapping object
 * Provides a separate namespace where we can map an integer to a pointer
 */
typedef struct ump_descriptor_mapping {
	_mali_osk_mutex_rw_t *lock; /**< Lock protecting access to the mapping object */
	int max_nr_mappings_allowed; /**< Max number of mappings to support in this namespace */
	int current_nr_mappings; /**< Current number of possible mappings */
	ump_descriptor_table *table;  /**< Pointer to the current mapping table */
} ump_descriptor_mapping;

/**
 * Create a descriptor mapping object
 * Create a descriptor mapping capable of holding init_entries growable to max_entries
 * @param init_entries Number of entries to preallocate memory for
 * @param max_entries Number of entries to max support
 * @return Pointer to a descriptor mapping object, NULL on failure
 */
ump_descriptor_mapping *ump_descriptor_mapping_create(int init_entries, int max_entries);

/**
 * Destroy a descriptor mapping object
 * @param map The map to free
 */
void ump_descriptor_mapping_destroy(ump_descriptor_mapping *map);

/**
 * Allocate a new mapping entry (descriptor ID)
 * Allocates a new entry in the map.
 * @param map The map to allocate a new entry in
 * @param target The value to map to
 * @return The descriptor allocated, a negative value on error
 */
int ump_descriptor_mapping_allocate_mapping(ump_descriptor_mapping *map, void *target);

/**
 * Get the value mapped to by a descriptor ID
 * @param map The map to lookup the descriptor id in
 * @param descriptor The descriptor ID to lookup
 * @param target Pointer to a pointer which will receive the stored value
 * @return 0 on successful lookup, negative on error
 */
int ump_descriptor_mapping_get(ump_descriptor_mapping *map, int descriptor, void **target);

/**
 * Set the value mapped to by a descriptor ID
 * @param map The map to lookup the descriptor id in
 * @param descriptor The descriptor ID to lookup
 * @param target Pointer to replace the current value with
 * @return 0 on successful lookup, negative on error
 */
int ump_descriptor_mapping_set(ump_descriptor_mapping *map, int descriptor, void *target);

/**
 * Free the descriptor ID
 * For the descriptor to be reused it has to be freed
 * @param map The map to free the descriptor from
 * @param descriptor The descriptor ID to free
 */
void ump_descriptor_mapping_free(ump_descriptor_mapping *map, int descriptor);

#endif /* __UMP_KERNEL_DESCRIPTOR_MAPPING_H__ */
