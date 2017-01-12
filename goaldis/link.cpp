// DGO linker/loader
// Ported directly from the MIPS disassembly.

#include "goaldis.h"

static uint8_t Byte(void *ptr) { return *(uint8_t *)ptr; }
static uint16_t Word(void *ptr) { return *(uint16_t *)ptr; }
static uint32_t Dword(void *ptr) { return *(uint32_t *)ptr; }
static void PatchDword(void *ptr, uint32_t value) { *(uint32_t *)ptr = value; }

static uint32_t read_clock() { return 0; }

// The game normally throws away the debug segment for a retail build,
// but we'd like to keep it please to aid our debugging.
static int DebugSegment = 1;

static kheapinfo *kdebugheap;

struct link_block
{
	uint32_t length;
	uint32_t version;
};

struct link_block_v4 : link_block
{
	uint32_t chunkSize;
	uint32_t _unused;
};

struct v3_header
{
	uint32_t segment_count;
	char name[64];
};

struct segment_header
{
	uint8_t *relocs;
	uint8_t *data;
	uint32_t size;
	uint32_t _padding;
};

static void ultimate_memcpy(void *dest, void *src, uint32_t size)
{
	memcpy(dest, src, size);
}

static void call_method_of_type_arg2(void *obj, void *type, uint32_t methodIdx, void *heapinfo)
{
}

static void *intern_type_from_c(const char *name, uint8_t num_methods)
{
	symbol *sym = intern_from_c(name);
	if (sym->ptr)
	{
		Type *type = (Type *)sym->ptr;
		if (num_methods > type->allocated_length)
		{
			fprintf(stderr, "link: trying to redefine a type '%s' with %d methods when it had %d\n",
				name, num_methods, type->allocated_length);
		}
	} else {
		alloc_and_init_type(sym, num_methods);
	}
	return sym->ptr;
}

static void update_goal_fns()
{
}

static uint8_t *symlink2(void *segmentBase, void *interned_thing, uint8_t *pos, bool is_type)
{
	uint32_t *dest = (uint32_t *)segmentBase;
	while(1)
	{
		uint32_t offset = Byte(pos);
		pos++;
		if (offset & 3)
		{
			offset = offset | (Byte(pos) << 8);
			pos++;
			if (offset & 2)
			{
				offset = offset | (Byte(pos) << 16);
				pos++;
				if (offset & 1)
				{
					offset = offset | (Byte(pos) << 24);
					pos++;
				}
			}
		}

		dest = dest + (offset >> 2);
		uint32_t val = Dword(dest);

		if (val == 0xffffffff)
		{
			PatchDword(dest, (uint32_t)interned_thing);

			metaAddReloc(dest, is_type ? REL_TYPE : REL_SYM);

			if (is_type)
				metaAddObject(dest + 1);
		} else {
			uint32_t var_18 = (uint32_t)interned_thing + val - (uint32_t)s7;
			uint32_t target = (val & 0xffff0000) | (var_18 & 0xffff);
			PatchDword(dest, target);

			metaAddReloc(dest, REL_SYMIDX_LO);
		}

		if (Byte(pos) == 0)
			break;
	}

	pos++;
	return pos;
}

static uint8_t *symlink3(void *segmentBase, void *interned_thing, uint8_t *pos, bool is_type)
{
	uint32_t *dest = (uint32_t *)segmentBase;
	do {
		uint8_t c;
		do {
			c = Byte(pos);
			pos++;
			dest = dest + c;
		} while(c == 0xff);

		uint32_t val = Dword(dest);
		if (val == 0xffffffff)
		{
			PatchDword(dest, (uint32_t)interned_thing);

			if (is_type) {
				metaAddReloc(dest, REL_TYPE);
				metaAddObject(dest + 1);
			} else {
				metaAddReloc(dest, REL_SYM);
			}
		} else {
			uint32_t var_1c = (uint32_t)interned_thing + val - (uint32_t)s7;
			uint32_t target = (var_1c & 0xffff) | (val & 0xffff0000);
			PatchDword(dest, target);

			metaAddReloc(dest, REL_SYMIDX_LO);
		}
	} while(Byte(pos) != 0);

	pos++;
	return pos;
}

class link_control
{
	uint8_t *fileData;
	uint32_t fileSize;
	uint8_t *goalEntry;
	link_block *block;
	uint8_t *streamPtr;
	uint32_t streamSize;
	uint8_t *chunkPtr;
	uint32_t chunkSize, chunkDone;
	uint32_t state;
	kheapinfo *heapinfo;
	uint8_t *packptr;
	uint8_t *reloc_ptr;
	uint32_t field_80;
	void *load_address_base;
	uint32_t max_relocs_this_pass;
	segment_header *segments;

	bool work_v2()
	{
		uint32_t start_clock = read_clock();
		if (state == 0)
		{	
			// loc_10A648
			if (chunkDone == 0)
			{
				// Lots of memory, so allocate space for the chunk on the low heap.
				chunkPtr = streamPtr;
				streamPtr = (uint8_t *)kmalloc(heapinfo, chunkSize, 0, "data-segment");
			}
			
			// loc_10A69C
			uint8_t *memcpyDest = streamPtr;
			uint8_t *memcpySrc = chunkPtr;
			uint32_t memcpySize = chunkSize;
			memcpySrc = memcpySrc + chunkDone;
			memcpyDest = memcpyDest + chunkDone;			
			memcpySize = memcpySize - chunkDone;
			
			if (memcpySize >= 0x80000)
			{
				ultimate_memcpy(memcpyDest, memcpySrc, 0x80000);
				chunkDone = chunkDone + 0x80000;
				return 0;			
			}
		
			// loc_10A728
			if (memcpySize != 0)
			{
				ultimate_memcpy(memcpyDest, memcpySrc, memcpySize);
				if (chunkDone > 0)
				{
					state = 1;
					chunkDone = 0;
					return 0;
				}
			}
		
			state = 1;
			chunkDone = 0;
		}

		// loc_10A770:
		if (state == 1 && chunkDone == 0)
		{
			packptr = (uint8_t *)(block + 1);
			if (Byte(packptr) == 0)
			{
				state = 2;
				chunkDone = 0;
			} else {
				load_address_base = streamPtr;
				reloc_ptr = streamPtr;
				field_80 = 0;
				chunkDone = 1;
			}
		}
		
		// loc_10A7E0:
		if (state == 1)
		{
			max_relocs_this_pass = 0x400;
			// loc_10A7F8
			while(1)
			{
				uint32_t count = Byte(packptr);
				packptr++; // read next byte
				
				if (field_80 == 0)
				{
					// loc_10A878
					reloc_ptr = reloc_ptr + count*4;
				} else {
					uint32_t i = 0;
					// loc_10A820
					while (i < count)
					{
						// loc_10A83C
						uint32_t val = Dword(reloc_ptr);
						val = val + (uint32_t)load_address_base;
						PatchDword(reloc_ptr, val);

						metaAddReloc(reloc_ptr, REL_ADDR);

						reloc_ptr = reloc_ptr + 4; // next 4 bytes
						i++;
					}
				}
				
				// loc_10A890
				if (count != 0xff)
				{
					// loc_10A8D8
					field_80 = field_80 ^ 1;
					if (Byte(packptr) == 0)
						break;
					
					// loc_10A910
					max_relocs_this_pass--;
					if (max_relocs_this_pass == 0)
					{
						if ((read_clock() - start_clock) < 150000)
							return 0;
						
						max_relocs_this_pass = 0x400;
					}
				}
			
				if (Byte(packptr) == 0)
				{
					packptr++;
					field_80 = field_80 ^ 1;
				}		
			}

			packptr++;
			state = 2;
			chunkDone = 0;
		}

		// loc_10A964
		if (state != 2)
		{
			goalEntry = streamPtr + 4;
			return 1;
		}
		
		if (Byte(packptr) != 0)
		{
			// loc_10A998
			while(1)
			{
				void *interned_thing;
				const char *name;

				uint8_t code = Byte(packptr);
				packptr++;

				uint8_t num_methods = 0;

				if (code & 0x80)
				{
					num_methods = code & 0x7f;
					if (num_methods == 0)
						num_methods = 1;
					
					// loc_10A9E0
					name = (char *)packptr;
					interned_thing = intern_type_from_c(name, num_methods);
				} else {
					// loc_10A9F8
					if (code >= 10)
						packptr--;

					// loc_10AA14
					name = (char *)packptr;
					interned_thing = intern_from_c(name);
				}
				
				// loc_10AA20
				packptr = packptr + (strlen(name) + 1);
				
				packptr = symlink2(streamPtr, interned_thing, packptr, num_methods>0);

				if (Byte(packptr) == 0)
					break;
				
				// loc_10AA70
				uint32_t diff = read_clock() - start_clock;
				if (diff >= 150000)
					return 0;
			}
		}
		
		state = 3;
		chunkDone = 0;
		
		goalEntry = streamPtr + 4;
		return 1;
	}

	bool work_v3()
	{
		// state 0 sets up the segments
		// states 1-3 are like a loop then, one for each segment
		uint32_t start_clock = read_clock();
		uint8_t *old;
		uint32_t segment_count; // var_54
		
		if (state == 0)
		{
			v3_header *v3 = (v3_header *)(block + 1);
			segment_count = v3->segment_count;
			segments = (segment_header *)(v3 + 1);
			int32_t seg_idx;
			
			// loc_109BA0
			for (seg_idx = segment_count-1; seg_idx >= 0; seg_idx--)
			{
				// loc_109BB4
				segment_header *seg = &segments[seg_idx];

				seg->relocs += (uint32_t)block - 4;
				seg->data += (uint32_t)streamPtr;
							
				if (seg_idx == 0) {
					// loc_109C70
					if (seg->size == 0)
					{
						// loc_109D7C
						seg->data = NULL;
						continue;
					}
					
					old = seg->data;
					seg->data = (uint8_t *)kmalloc(heapinfo, seg->size, 0, "main-segment");
					
					// loc_109D3C
					ultimate_memcpy(seg->data, old, seg->size);
				} else if (seg_idx == 1) {
					// loc_109D9C
					if (DebugSegment == 0)
					{
						// loc_109EDC
						seg->data = NULL;
						seg->size = 0;
						continue;
					}
				
					if (seg->size == 0)
					{
						// loc_109EBC
						seg->data = NULL;
						continue;			
					}
					
					old = seg->data;
					seg->data = (uint8_t *)kmalloc(kdebugheap, seg->size, 0, "debug-segment");
					
					// loc_109E7C
					ultimate_memcpy(seg->data, old, seg->size);
				} else if (seg_idx == 2) {
					// loc_109F14
					if (seg->size == 0)
					{
						// loc_10A020
						seg->data = NULL;
						continue;
					}
					
					old = seg->data;
					seg->data = (uint8_t *)kmalloc(heapinfo, seg->size, KMALLOC_HIGH, "top-level-segment");
					
					// loc_109FE0
					ultimate_memcpy(seg->data, old, seg->size);
				} 
			}
			
			// loc_10A050
			state = 1;
			chunkDone = 0;
			uint32_t diff = read_clock() - start_clock;
			
			if (diff >= 200000)
				return 0;
		}
		
		//--------- states 1-3 -------------------------------------------------------------
		// loc_10A08C
		while(1)
		{	
			while (state < 4 && chunkDone == 0)
			{
				segment_header *prev = &segments[state-1];
				if (prev->data && prev->size)
				{
					packptr = prev->relocs;
					streamPtr = prev->data;
					load_address_base = streamPtr;
					reloc_ptr = streamPtr - 4;
					field_80 = 0;
					chunkDone = 1;
					break;
				}
				
				// loc_10A15C
				state++;
				chunkDone = 0;
			}
				
			// loc_10A170
			if (state >= 4)
			{
				// loc_10A554
				update_goal_fns();
				return 1;
			}
			
			if (chunkDone == 1)
			{
				max_relocs_this_pass = 0x400;
				
				if (Byte(packptr) != 0)
				{		
					// loc_10A1A8
					while(1)
					{
						if (field_80 == 0)
						{
							// loc_10A330
							reloc_ptr = reloc_ptr + Byte(packptr)*4;
						} else {	
							uint32_t i = 0; // var_54
							
							// loc_10A1B8
							while (i < Byte(packptr))
							{
								// loc_10A1D8
								uint32_t val = Dword(reloc_ptr);
								uint32_t v0 = val >> 24;
								if (v0 == 0)
								{
									// simple relocation
									val = val + (uint32_t)load_address_base;
									
									// TODO- the J&D linker doesn't do this,
									// why do I need to?
									val &= ~3;
	
									PatchDword(reloc_ptr, val);

									metaAddReloc(reloc_ptr, REL_ADDR);
								} else {
									// MIPS HI/LO relocation
									uint32_t segidx = (val >> 8) & 0xf;
									uint32_t lowoffset = (val >> 12) & 0xf;
									
									segment_header *tgtseg = &segments[segidx];
									
									uint32_t a1 = Dword(reloc_ptr + lowoffset*4);
									uint32_t a0 = val & 0xff;
									uint32_t v1 = (a0 << 8) | (a1 & 0xffff);
									uint32_t final_addr = (uint32_t)(tgtseg->data) + v1;
									
									// the (a0 << 8) part above doesn't make any sense!
									// why not <<16 so it doesn't overlap with a1?
									// to me, it'd make sense to use the 8-bit field field and the
									// 16-bit field to construct a 26-bit offset (with low 2 bits zero)
									assert(a0 == 0);

									// TODO- the J&D linker doesn't do this,
									// why do I need to? Does the game just write out
									// unaligned addresses sometimes by mistake, and the CPU
									// just silently masks off the unaligned accesses?
									final_addr &= ~3;
																	
									if (DebugSegment == 0)
									{
										if (segidx == 1)
											final_addr = 0;
									}
									
									// loc_10A288
									val = (val & 0xffff0000) | (final_addr >> 16);
									uint8_t *lo = reloc_ptr + lowoffset*4;
									PatchDword(reloc_ptr, val);
									PatchDword(lo, (a1 & 0xffff0000) | (final_addr & 0xffff));

									metaAddReloc(reloc_ptr, REL_ADDR_HI, (uint32_t *)lo);
									metaAddReloc(lo, REL_ADDR_LO, (uint32_t *)reloc_ptr);
								}
								
								// loc_10A30C
								reloc_ptr = reloc_ptr + 4;
								i++;
							}
						}

						// loc_10A34C
						if (Byte(packptr) != 0xff)
						{
							// loc_10A3A4
							packptr++;
							field_80 = field_80 ^ 1;
							
							// loc_10A3BC
							if (Byte(packptr) == 0)
								break;
							
							// loc_10A3D4
							max_relocs_this_pass = max_relocs_this_pass - 1;
							if (max_relocs_this_pass == 0)
							{
								uint32_t diff2 = read_clock() - start_clock;
								if (diff2 >= 200000)
									return 0;

								max_relocs_this_pass = 0x400;
							}
						} else {
							packptr++;
							if (Byte(packptr) == 0)
							{
								packptr++;
								field_80 = field_80 ^ 1;
							}
						}
					
						// loc_10A420
					}
				}
				
				// loc_10A428
				packptr++;
				chunkDone++;
			}

			// loc_10A440
			if (chunkDone != 2)
			{
				update_goal_fns();
				return 1;
			}
			 
			while(Byte(packptr) != 0)
			{
				// loc_10A468
				const char *name;
				void *interned_thing;
				
				uint8_t code = Byte(packptr);
				packptr++;

				uint8_t num_methods = 0;
				
				if (code & 0x80)
				{
					num_methods = code & 0x7f;
					assert(num_methods != 0);

					name = (char *)packptr;
					interned_thing = intern_type_from_c(name, num_methods);
				} else {
					packptr--;
					name = (char *)packptr;
					interned_thing = intern_from_c(name);
				}

				// loc_10A4CC
				uint32_t len = strlen(name)+1;
				packptr = packptr + len;

				packptr = symlink3(streamPtr, interned_thing, packptr, num_methods>0);
				
				uint32_t diff3 = read_clock() - start_clock;
				if (diff3 >= 200000)
				{
					update_goal_fns();
					return 1;
				}
			}
			
			// loc_10A534
			goalEntry = streamPtr + 4;
			state++;
			chunkDone = 0;
		}
	}

public:
	void begin(void *data, uint32_t size, kheapinfo *heap)
	{
		fileData = (uint8_t *)data;
		fileSize = size;
		heapinfo = heap;

		streamPtr = fileData;
		streamSize = fileSize;
		goalEntry = 0;
		block = (link_block *)(streamPtr + 4);
		chunkSize = 0;
		state = 0;
		chunkDone = 0;
		
		if (block->version < 4)
		{
			streamPtr = streamPtr + block->length;
			chunkSize = streamSize - block->length;
		} else {
			// loc_1099C0
			link_block_v4 *v4 = (link_block_v4 *)block;
			block = (link_block *)(streamPtr + 4 + sizeof(link_block_v4) + v4->chunkSize);
			streamPtr = streamPtr + sizeof(link_block_v4);
			chunkSize = v4->chunkSize;
		}
	}

	bool work()
	{
		//uint32_t *root = ((uint32_t *)block) - 1;
		//*root = s7[0x40];

		uint32_t version = block->version;
		
		bool ret;
		if (version == 2 || version == 4)
			ret = work_v2();
		else if (version == 3)
			ret = work_v3();
		else {
			fprintf(stderr, "link: Unknown version 0x%x\n", version);
			return true;
		}
		
		return ret;
	}

	void finish()
	{
		uint32_t version = block->version;

		if (version == 3)
		{
			assert(goalEntry != 0);
			//goalEntry();
		} else {
			void *type = ((void **)goalEntry)[-1];
			call_method_of_type_arg2(goalEntry, type, 7, heapinfo);
		}

		metaLoadingGo->goalEntry = (uint32_t *)goalEntry;
	}
};

void link_and_exec(void *data, uint32_t size)
{
	link_control link;

	link.begin(data, size, NULL);
	while(!link.work())
	{
	}

	link.finish();
}
