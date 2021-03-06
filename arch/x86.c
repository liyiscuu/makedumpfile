/*
 * x86.c
 *
 * Copyright (C) 2006, 2007, 2008  NEC Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifdef __x86__

#include "../print_info.h"
#include "../elf_info.h"
#include "../makedumpfile.h"

int
get_machdep_info_x86(void)
{
	unsigned long vmlist, vmap_area_list, vmalloc_start;

	/* PAE */
	if ((vt.mem_flags & MEMORY_X86_PAE)
	    || ((SYMBOL(pkmap_count) != NOT_FOUND_SYMBOL)
	      && (SYMBOL(pkmap_count_next) != NOT_FOUND_SYMBOL)
	      && ((SYMBOL(pkmap_count_next)-SYMBOL(pkmap_count))/sizeof(int))
	      == 512)) {
		DEBUG_MSG("\n");
		DEBUG_MSG("PAE          : ON\n");
		vt.mem_flags |= MEMORY_X86_PAE;
		info->max_physmem_bits  = _MAX_PHYSMEM_BITS_PAE;
	} else {
		DEBUG_MSG("\n");
		DEBUG_MSG("PAE          : OFF\n");
		info->max_physmem_bits  = _MAX_PHYSMEM_BITS;
	}
	info->page_offset = __PAGE_OFFSET;

	if (SYMBOL(_stext) == NOT_FOUND_SYMBOL) {
		ERRMSG("Can't get the symbol of _stext.\n");
		return FALSE;
	}
	info->kernel_start = SYMBOL(_stext) & ~KVBASE_MASK;
	DEBUG_MSG("kernel_start : %lx\n", info->kernel_start);

	/*
	 * Get vmalloc_start value from either vmap_area_list or vmlist.
	 */
	if ((SYMBOL(vmap_area_list) != NOT_FOUND_SYMBOL)
	    && (OFFSET(vmap_area.va_start) != NOT_FOUND_STRUCTURE)
	    && (OFFSET(vmap_area.list) != NOT_FOUND_STRUCTURE)) {
		if (!readmem(VADDR, SYMBOL(vmap_area_list) + OFFSET(list_head.next),
			     &vmap_area_list, sizeof(vmap_area_list))) {
			ERRMSG("Can't get vmap_area_list.\n");
			return FALSE;
		}
		if (!readmem(VADDR, vmap_area_list - OFFSET(vmap_area.list) +
			     OFFSET(vmap_area.va_start), &vmalloc_start,
			     sizeof(vmalloc_start))) {
			ERRMSG("Can't get vmalloc_start.\n");
			return FALSE;
		}
	} else if ((SYMBOL(vmlist) != NOT_FOUND_SYMBOL)
		   && (OFFSET(vm_struct.addr) != NOT_FOUND_STRUCTURE)) {
		if (!readmem(VADDR, SYMBOL(vmlist), &vmlist, sizeof(vmlist))) {
			ERRMSG("Can't get vmlist.\n");
			return FALSE;
		}
		if (!readmem(VADDR, vmlist + OFFSET(vm_struct.addr), &vmalloc_start,
			     sizeof(vmalloc_start))) {
			ERRMSG("Can't get vmalloc_start.\n");
			return FALSE;
		}
	} else {
		/*
		 * For the compatibility, makedumpfile should run without the symbol
		 * used to get vmalloc_start value if they are not necessary.
		 */
		return TRUE;
	}
	info->vmalloc_start = vmalloc_start;
	DEBUG_MSG("vmalloc_start: %lx\n", vmalloc_start);

	return TRUE;
}

int
get_versiondep_info_x86(void)
{
	/*
	 * SECTION_SIZE_BITS of PAE has been changed to 29 from 30 since
	 * linux-2.6.26.
	 */
	if (vt.mem_flags & MEMORY_X86_PAE) {
		if (info->kernel_version < KERNEL_VERSION(2, 6, 26))
			info->section_size_bits = _SECTION_SIZE_BITS_PAE_ORIG;
		else
			info->section_size_bits = _SECTION_SIZE_BITS_PAE_2_6_26;
	} else
		info->section_size_bits = _SECTION_SIZE_BITS;

	return TRUE;
}

int get_xen_basic_info_x86(void)
{
	if (SYMBOL(pgd_l2) == NOT_FOUND_SYMBOL &&
	    SYMBOL(pgd_l3) == NOT_FOUND_SYMBOL) {
		ERRMSG("Can't get pgd.\n");
		return FALSE;
	}

	if (SYMBOL(pgd_l3) == NOT_FOUND_SYMBOL) {
		ERRMSG("non-PAE not support right now.\n");
		return FALSE;
	}

	if (SYMBOL(frame_table) != NOT_FOUND_SYMBOL) {
		unsigned long frame_table_vaddr;

		if (!readmem(VADDR_XEN, SYMBOL(frame_table),
		    &frame_table_vaddr, sizeof(frame_table_vaddr))) {
			ERRMSG("Can't get the value of frame_table.\n");
			return FALSE;
		}
		info->frame_table_vaddr = frame_table_vaddr;
	} else
		info->frame_table_vaddr = FRAMETABLE_VIRT_START;

	if (!info->xen_crash_info.com ||
	    info->xen_crash_info.com->xen_major_version < 4) {
		unsigned long xen_end;

		if (SYMBOL(xenheap_phys_end) == NOT_FOUND_SYMBOL) {
			ERRMSG("Can't get the symbol of xenheap_phys_end.\n");
			return FALSE;
		}
		if (!readmem(VADDR_XEN, SYMBOL(xenheap_phys_end), &xen_end,
		    sizeof(xen_end))) {
			ERRMSG("Can't get the value of xenheap_phys_end.\n");
			return FALSE;
		}
		info->xen_heap_start = 0;
		info->xen_heap_end   = paddr_to_pfn(xen_end);
	}

	return TRUE;
}

int get_xen_info_x86(void)
{
	int i;

	/*
	 * pickled_id == domain addr for x86
	 */
	for (i = 0; i < info->num_domain; i++) {
		info->domain_list[i].pickled_id =
			info->domain_list[i].domain_addr;
	}

	return TRUE;
}
#endif /* x86 */

