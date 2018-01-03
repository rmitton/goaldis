#include "goaldis.h"

struct Alloc
{
	void *addr;
	uint32_t size;
	uint32_t flags;
	const char *name;
};

int numSymbols = 0;
symbol *s7;
char **names;

#define HEAP_SIZE	100*1024*1024

uint32_t heapUsed = 0;
char *heap = NULL;

vector<Alloc> allocs;

void *kmalloc(kheapinfo *heapinfo, uint32_t size, uint32_t flags, const char *name)
{
	if (!heap)
		heap = (char *)malloc(HEAP_SIZE);

	// We can't just malloc each allocation individually as our labelling
	// algorithm requires the addresses to always be in ascending order,
	// if we want the results to match between different runs of the program.
	void *data = &heap[heapUsed];
	memset(data, 0xbb, size);
	heapUsed += ((size+15) & ~15);

	assert(heapUsed < HEAP_SIZE);

	Alloc a;
	a.addr = data;
	a.size = size;
	a.flags = flags;
	a.name = name;
	allocs.push_back(a);

	metaAddSegment(name, (uint32_t *)data, size);

	return data;
}

const char *kmalloc_name(void *addr)
{
	for (size_t n=0;n<allocs.size();n++)
	{
		uint32_t offset = (uint32_t)addr - (uint32_t)allocs[n].addr;
		if (offset < allocs[n].size)
			return allocs[n].name;
	}
	return "unknown";
}

char *sym_name(symbol *s)
{
	assert(s);
	int id = s - s7;
	assert(id >= 0 && id < numSymbols);
	char *n = names[id];
	assert(n);
	return n;
}

void set_fixed_symbol(FixedSym id, const char *name, void *value)
{
	if (id >= numSymbols)
		numSymbols = id+1;

	s7[id].ptr = value;
	names[id] = strdup(name);

	void **p = (void **)&s7[id];
	p[-1] = &s7[s_symbol];
}

symbol *intern_from_c(const char *name)
{
	assert(name && *name);

	for (int n=0;n<numSymbols;n++)
	{
		if (names[n] && !strcmp(names[n], name))
			return &s7[n];
	}

	int id = numSymbols;
	set_fixed_symbol((FixedSym)id, name, NULL);
	return &s7[id];
}

void *alloc_heap_object(kheapinfo *heap, Type *type, uint32_t size)
{
	void **ptr = (void **)malloc(size);
	memset(ptr, 0, size);
	*ptr = type;
	ptr++;
	return (void *)ptr;
}

void *make_nothing_func()
{
	void *func = alloc_heap_object((kheapinfo *)s7[s_globalheap].ptr, (Type *)s7[s_function].ptr, 0x14);
	uint32_t *p = (uint32_t *)func;
	*p++ = 0x3e00008; // MIPS "jr ra"
	*p++ = 0;
	return func;
}

void *make_zero_func()
{
	void *func = alloc_heap_object((kheapinfo *)s7[s_globalheap].ptr, (Type *)s7[s_function].ptr, 0x14);
	uint32_t *p = (uint32_t *)func;
	*p++ = 0x03e00008; // MIPS "jr ra"
	*p++ = 0x24020000; // MIPS "addiu r0, r2, #0"
	*p++ = 0;
	return func;
}

Type *alloc_and_init_type(symbol *sym, int num_methods)
{
	Type *type = (Type *)calloc(1, 0x14 + num_methods*4);
	type->sym = sym;
	type->allocated_length = num_methods;
	sym->ptr = type;
	return type;
}

void set_fixed_type(FixedSym id, const char *name, symbol *parentTypeSym, uint64_t flags, void *print_fn, void *inspect_fn)
{
	// type flags:
	// 0xaaaabbbbccccdddd
	// a = ?
	// b = allocated_length
	// c = heap_base
	// d = sizeA

	symbol *sym = &s7[id];
	Type *type = (Type *)sym->ptr;

	if (!type)
		type = alloc_and_init_type(sym, (flags>>32)&0xffff);

	Type *parent = (Type *)parentTypeSym->ptr;
	type->parent = parent;
	type->sizeA = flags&0xffff;
	type->sizeB = (type->sizeA+0xf)&0xfff0;
	type->heap_base = (flags>>16)&0xffff;

	int num_methods = (flags>>32)&0xffff;
	if (type->allocated_length < num_methods)
		type->allocated_length = num_methods;

	if (!print_fn)
		print_fn = parent->print_fn;
	if (!inspect_fn)
		inspect_fn = parent->inspect_fn;

	type->new_fn = parent->new_fn;
	type->delete_fn = parent->delete_fn;
	type->print_fn = print_fn;
	type->inspect_fn = inspect_fn;
	type->unknown_fn = s7[s_zero_func].ptr;
	type->field_24 = parent->field_24;
	type->copy_fn = parent->copy_fn;

	set_fixed_symbol(id, name, type);
}

void InitMachine()
{
	int maxSyms = 0x20000;
	s7 = new symbol[maxSyms];
	names = new char*[maxSyms];
	memset(s7, 0, sizeof(symbol)*maxSyms);
	memset(names, 0, sizeof(char*)*maxSyms);

	numSymbols = 0;

	set_fixed_symbol(s_f, "#f", &s7[s_f]);
	set_fixed_symbol(s_t, "#t", &s7[s_t]);
	set_fixed_symbol(s_nothing, "nothing", make_nothing_func());
	set_fixed_symbol(s_zero_func, "zero-func", make_zero_func());
	// TODO- the rest of these
	set_fixed_type(s_object, "object", &s7[s_object], (uint64_t(9)<<32) | 4, NULL, NULL);
	set_fixed_type(s_structure, "structure", &s7[s_object], (uint64_t(9)<<32) | 4, NULL, NULL);
	set_fixed_type(s_basic, "basic", &s7[s_structure], (uint64_t(9)<<32) | 4, NULL, NULL);
	set_fixed_type(s_string, "string", &s7[s_basic], (uint64_t(9)<<32) | 8, NULL, NULL);
	set_fixed_type(s_function, "function", &s7[s_basic], (uint64_t(9)<<32) | 4, NULL, NULL);
}
