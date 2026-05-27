//
//  Created by Matt Hartley on 29/04/2026.
//  Copyright 2026 GiantJelly. All rights reserved.
//

#include "core/core.h"
#include <core/json.h>


allocator_t memory;

file_data_t* LoadFile(char* path)
{
	file_t file = sys_open(path);
	if (!file) {
		return NULL;
	}

	stat_t info = sys_fstat(file);
	file_data_t* result = alloc_memory(&memory, info.size + sizeof(stat_t));
	result->stat = info;
	sys_read(file, 0, result->data, info.size);
	sys_close(file);
	return result;
}

int main()
{
	memory = virtual_bump_allocator(GB(1), MB(1));
}
