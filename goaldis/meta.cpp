#include "goaldis.h"

MetaGoFile *metaLoadingGo = NULL;
vector<MetaGoFile *> metaGoFiles;
map<void *, MetaRelocInfo> metaRelocTable;

static int numObjects = 0;

void metaAddReloc(void *addr, MetaRelocType type, uint32_t *pair)
{
	assert(addr);
	assert(((uint32_t)addr & 3) == 0);

	if (type == REL_ADDR_LO || type == REL_ADDR_HI)
		assert(pair);
	else
		assert(!pair);

	MetaRelocInfo info;
	info.type = type;
	info.pair = pair;
	metaRelocTable[addr] = info;
}

void metaAddObject(void *addr)
{
	assert(addr);
	assert(((uint32_t)addr & 3) == 0);

	assert(metaLoadingGo);
	metaLoadingGo->objects.push_back((uint32_t *)addr);
}

void metaAddSegment(const char *name, uint32_t *start, uint32_t size)
{
	assert(start);
	assert(((uint32_t)start & 3) == 0);

	assert(metaLoadingGo);
	MetaSegment seg;
	seg.name = name;
	seg.start = start;
	seg.size = size;
	metaLoadingGo->segments.push_back(seg);
}
