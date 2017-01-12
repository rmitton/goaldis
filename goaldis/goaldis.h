#define _HAS_ITERATOR_DEBUGGING 0
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NONSTDC_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <direct.h>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>

using namespace std;

struct MetaGoFile;

// Custom GOAL-based MIPS disassembler
void disasmFile(FILE *_fp, MetaGoFile *go, bool final_pass);

// DGO linker/loader
// Ported directly from the MIPS disassembly.
void link_and_exec(void *data, uint32_t size);


//
// Meta-information about the loaded GOAL objects.
// The actual GOAL runtime doesn't generally hang on to this stuff,
// they only use it during loading. However we keep ahold of it so
// that we can use it to print symbolic assembly out.
//

enum MetaRelocType
{
	REL_ADDR,
	REL_ADDR_LO,
	REL_ADDR_HI,
	REL_TYPE,
	REL_SYM,
	REL_SYMIDX_LO,
};

struct MetaRelocInfo
{
	MetaRelocType type;
	uint32_t *pair;
};

struct MetaSegment
{
	string name;
	uint32_t *start;
	uint32_t size;
};

struct MetaGoFile
{
	string name, fileName, dgoname;
	bool shouldDump;

	uint32_t *goalEntry;
	vector<uint8_t> rawdata;
	vector<uint32_t *> objects;
	vector<MetaSegment> segments;
};

void metaAddReloc(void *addr, MetaRelocType type, uint32_t *pair=NULL);
void metaAddObject(void *addr);
void metaAddSegment(const char *name, uint32_t *start, uint32_t size);

extern MetaGoFile *metaLoadingGo;
extern vector<MetaGoFile *> metaGoFiles;
extern map<void *, MetaRelocInfo> metaRelocTable;


//
// This represents the GOAL machine runtime.
// Most/all of these functions are direct replacements for their GOAL equivalents.
// They generally live in the C++ kernel.
//

#define KMALLOC_ZERO 0x1000 // memset data to zero
#define KMALLOC_HIGH 0x2000 // alloc from high end

enum FixedSym
{
	s_f,
	s_t,
	s_function,
	s_basic,
	s_string,
	s_symbol,
	s_type,
	s_object,
	s_link_block,
	s_integer,
	s_sinteger,
	s_uinteger,
	s_binteger,
	s_int8,
	s_int16,
	s_int32,
	s_int64,
	s_int128,
	s_uint8,
	s_uint16,
	s_uint32,
	s_uint64,
	s_uint128,
	s_float,
	s_process_tree,
	s_process,
	s_thread,
	s_structure,
	s_pair,
	s_pointer,
	s_number,
	s_array,
	s_vu_function,
	s_connectable,
	s_stack_frame,
	s_file_stream,
	s_kheap,
	s_nothing,
	s_delete_basic,
	s_static,
	s_globalheap,
	s_debug,
	s_loading_level,
	s_loading_package,
	s_process_level_heap,
	s_stack,
	s_scratch,
	s_scratch_top,
	s_zero_func,
	s_asize_of_basic_func,
	s_copy_basic,
	s_level,
	s_art_group,
	s_texture_page_dir,
	s_texture_page,
	s_sound,
	s_dgo,
	s_top_level,
};

struct kheapinfo
{
	uint32_t field_0;
	void *high;
	void *low;
};

struct symbol
{
	void *ptr;
	uint32_t _pad;
};

struct Type
{
	// type information:
	symbol *sym;				// 00
	Type *parent;				// 04
	uint16_t sizeA;				// 08
	uint16_t sizeB;				// 0a
	uint16_t heap_base;			// 0c
	uint16_t allocated_length;	// 0e

	// methods:
	void *new_fn;				// 10
	void *delete_fn;			// 14
	void *print_fn;				// 18
	void *inspect_fn;			// 1c
	void *unknown_fn;			// 20
	uint32_t field_24;			// 24
	void *copy_fn;				// 28
	uint32_t field_2c;			// 2c
	uint32_t field_30;			// 30
};

void InitMachine();
symbol *intern_from_c(const char *name);
void set_fixed_symbol(FixedSym id, const char *name, void *value);
Type *alloc_and_init_type(symbol *sym, int num_methods);
const char *kmalloc_name(void *addr);
char *sym_name(symbol *s);
void *kmalloc(kheapinfo *heapinfo, uint32_t size, uint32_t flags, const char *name);

extern symbol *s7;
