// Custom GOAL-based MIPS disassembler

#include "goaldis.h"
#include <stdarg.h>

const char *reg[] = {
	"r0", "r1", "v0", "v1", "a0", "a1", "a2", "a3", 
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", 
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", 
	"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
};

string print(const char *fmt, ...)
{
    char tmp[1024];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(tmp, fmt, ap);
    va_end(ap);
	return tmp;
}

#define IMM(FOO) print("%i", FOO)
#define SYMBOL(FOO) print("%s", sym_name(FOO))

#define WRITE(...) do { if (final_pass) fprintf(fp, __VA_ARGS__); else (void)(__VA_ARGS__); } while(0)

static bool block_finished;
static bool pass_finished;
static bool final_pass;
static bool delay_slot;
static bool likely;
static FILE *fp;
static set<uint32_t *> done, todo;
static uint8_t *objbase;
static const char *fileName;

map<uint32_t *, string> labels;

static string addLabel(uint32_t *addr)
{
	map<uint32_t *, string>::iterator p = labels.find(addr);
	if (p != labels.end())
		return p->second;
	
	pass_finished = false;

	string lbl = "L1"; // we'll renumber later at the final pass
	labels[addr] = lbl;
	return lbl;
}

static string LABEL(uint32_t *addr)
{
	todo.insert(addr);
	return addLabel(addr);
}

static string DATALABEL(uint32_t *addr)
{
	return addLabel(addr);
}

static bool hasReloc(uint32_t *addr)
{
	if (metaRelocTable.find(addr) == metaRelocTable.end())
		return false;
	return true;
}

static string relocTarget(uint32_t *addr)
{
	if (metaRelocTable.find(addr) == metaRelocTable.end())
		return print("0x%x", *addr);

	MetaRelocInfo &info = metaRelocTable[addr];
	switch (info.type)
	{
	case REL_ADDR:
	{
		uint32_t *obj = (uint32_t *)(*addr);
		return DATALABEL(obj);
	}
	case REL_ADDR_LO:
	{
		uint32_t objAddr = ((*addr) & 0xffff);
		objAddr |= ((*info.pair) & 0xffff) << 16;
		uint32_t *obj = (uint32_t *)objAddr;
		return DATALABEL(obj);
	}
	case REL_ADDR_HI:
	{
		uint32_t objAddr = ((*addr) & 0xffff) << 16;
		objAddr |= (*info.pair) & 0xffff;
		uint32_t *obj = (uint32_t *)objAddr;
		return DATALABEL(obj);
	}
	case REL_TYPE:
	{
		Type *type = (Type *)(*addr);
		return sym_name(type->sym);
	}
	case REL_SYM:
	{
		return SYMBOL((symbol *)*addr);
	}
	case REL_SYMIDX_LO:
	{
		int16_t i = (int16_t)((*addr) & 0xffff);
		return SYMBOL(s7 + ((uint16_t)i)/sizeof(symbol));
	}
	default:				assert(0); return "";
	}
}

static string RIMM(uint32_t *addr)
{
	int16_t i = (int16_t)((*addr) & 0xffff);

	// Either a symbol/reloc or just an immediate, depending on if there's a reloc here.
	if (hasReloc(addr))
		return relocTarget(addr);
	else
		return IMM(i);
}

// Checks for FP-relative immediates.
static string RIMM_FP(uint32_t *addr, int rs)
{
	if (rs == 30) // FP
	{
		int16_t i = (int16_t)((*addr) & 0xffff);
		return DATALABEL((uint32_t *)(objbase+i)) + " - " + DATALABEL((uint32_t *)objbase);
	} else {
		return RIMM(addr);
	}
}

static void checkLabel(uint32_t *pos)
{
	if (labels.find(pos) != labels.end()) {
		WRITE("%s:\n", labels[pos].c_str());
	}
}

static uint32_t *pos, *pc;
static uint32_t base_reg;
static bool is_fp_offset;

static string str_op(uint32_t op) { return print("0x%02x", op); }
static string str_base(uint32_t rs) { return reg[rs]; }
static string str_rs(uint32_t rs) { return reg[rs]; }
static string str_rt(uint32_t rt) { return reg[rt]; }
static string str_rd(uint32_t rd) { return reg[rd]; }
static string str_sa(uint32_t sa) { return print("0x%02x", sa); }
static string str_of(uint32_t of) { return print("0x%02x", of); }
static string str_fs(uint32_t fs) { return print("f%i", fs); }
static string str_ft(uint32_t ft) { return print("f%i", ft); }
static string str_fd(uint32_t fd) { return print("f%i", fd); }
static string str_fcr(uint32_t fcr) { return print("fcr%i", fcr); }
static string str_vfs(uint32_t vfs) { return print("vf%i", vfs); }
static string str_vft(uint32_t vft) { return print("vf%i", vft); }
static string str_vfd(uint32_t vfd) { return print("vf%i", vfd); }
static string str_vis(uint32_t vis) { return print("vi%i", vis); }
static string str_vit(uint32_t vit) { return print("vi%i", vit); }
static string str_vid(uint32_t vid) { return print("vi%i", vid); }
static string str_wm(uint32_t wm) { return print("%s%s%s%s", (wm&1)?"x":"", (wm&2)?"y":"", (wm&4)?"z":"", (wm&4)?"w":""); }
static string str_offset(uint32_t offset) { return RIMM_FP(pos, base_reg); }
static string str_branch(uint32_t branch) { return LABEL(pc+(int16_t)branch); }
static string str_simm(uint32_t imm) { return hasReloc(pos) ? RIMM(pos) : is_fp_offset ? RIMM_FP(pos, base_reg) : print("%i", (int16_t)(imm&0xffff)); }
static string str_zimm(uint32_t imm) { return hasReloc(pos) ? RIMM(pos) : print("%i", (uint16_t)(imm&0xffff)); }
static string str_vcall(uint32_t imm) { return print("%i", (uint16_t)(imm&0x7fff)*8); }
static string str_code(uint32_t code) { return print("0x%x", code); }
static string str_hint(uint32_t hint) { return print("0x%x", hint); }
static string str_vimm(uint32_t imm) { return print("%i", imm); }
static string str_bc(uint32_t bc) { return print("%c", "xyzw"[bc]); }
static string str_fsf(uint32_t fsf) { return print("%c", "xyzw"[fsf]); }
static string str_ftf(uint32_t ftf) { return print("%c", "xyzw"[ftf]); }
static string str_cpr(uint32_t cpr) { return print("cpr%i", cpr); }

#define die(...) { fprintf(stderr, "%s: (%08x) ", fileName, *pos); fprintf(stderr, __VA_ARGS__); block_finished = true; delay_slot = false; return; }

static void handle_D() { delay_slot = true; }
static void handle_L() { likely = true; }
static void handle_E() { block_finished = true; }

static void decodeToAsm(uint32_t opcode)
{
#include "opcodes.h"
}

static void decodeInstr(uint32_t *&codeptr)
{
	pos = codeptr++;
	pc = codeptr;
	uint32_t opcode = *pos;

	bool was_delay_slot = delay_slot;
	delay_slot = false;
	likely = false;
	base_reg = (opcode >> 21) & 31;

	// Special handling for DADDIU with FP-relative offsets
	is_fp_offset = false;
	if ((opcode & 0xfc000000) == 0x64000000 && ((opcode >> 21) & 31) == 30)
		is_fp_offset = true;

	done.insert(pos);

	checkLabel(pos);

	decodeToAsm(opcode);

	if (was_delay_slot)
		WRITE("\n");
}

static uint32_t *doPass(uint32_t *code, void *segment_end)
{
	uint32_t *end = code;

	done.clear();
	todo.clear();
	todo.insert(code);

	while(!todo.empty())
	{
		uint32_t *pos = *todo.begin();
		todo.erase(pos);
		if (done.find(pos) != done.end())
			continue;

		delay_slot = false;
		block_finished = false;
		do {
			assert(pos < segment_end);
			decodeInstr(pos);
		} while (!block_finished || delay_slot);

		if (pos > end)
			end = pos;
	}
	return end;
}

void disasmBegin(FILE *_fp, MetaGoFile *go, bool _final_pass)
{
	fp = _fp;
	final_pass = _final_pass;
	fileName = go->fileName.c_str();

	labels[go->goalEntry] = "goal-entry";

	WRITE("; %s\n", go->name.c_str());
	WRITE("\n");
}

uint32_t *disasmFunc(uint32_t *obj, void *segment_end)
{
	uint32_t *end;

	string name = LABEL(obj);

	while (true)
	{
		pass_finished = true;
		end = doPass(obj, segment_end);
		if (pass_finished)
			break;
	}

	return end;
}

void disasmData(uint32_t *start, uint32_t *end, void *segment_end)
{
	if (start == end)
		return;

	assert(start < end);

	// Check for uninitialized data at the end of art-group files.
	// (TODO: detection not perfect)
	//if (end == segment_end && labels.find(pos) == labels.end() && final_pass)
	//{
	//	WRITE("; uninitialized data follows.\n");
	//	return;
	//}


	while (start+1 <= end)
	{
		checkLabel(start);
		WRITE("\t.word %s\n", relocTarget(start).c_str());
		start++;
	}

	// Check for any unaligned end bytes.
	uint8_t *p = (uint8_t *)start;
	uint32_t extra = (uint8_t *)end - p;
	while (extra--) {
		WRITE("\t.byte 0x%02x\n", *p++);
	}

	WRITE("\n");
}

uint32_t *disasmString(uint32_t *obj, uint32_t *segment_end)
{
	uint32_t *p = obj;
	uint32_t len = *p++;
	uint8_t *c = (uint8_t *)p;
	if (c + len > (uint8_t *)segment_end || len > 1000)
	{
		// if we erroneously detected a string
		disasmData(obj, obj+1, segment_end);
		return obj+1;
	}
	checkLabel(obj);
	WRITE("\t.word %i\n", len);
	WRITE("\t.ascii \"");
	while (len--) {
		char i = *c++;
		switch (i) {
		case '\\': WRITE("\\\\"); break;
		case '\"': WRITE("\\\""); break;
		case '\t': WRITE("\\t"); break;
		case '\n': WRITE("\\n"); break;
		default:   WRITE("%c", i); break;
		}		
	}
	WRITE("\"\n");
	while (((uint32_t)c)&3) c++;
	return (uint32_t *)c;
}

uint32_t *disasmObj(uint32_t *obj, uint32_t *segment_start, uint32_t *segment_end)
{
	Type *type = ((Type **)obj)[-1];
	int id = type->sym - s7;

	objbase = (uint8_t *)obj;

	if (id == s_function)
		WRITE("\n;------------------------------------------------------------------\n");

	checkLabel(obj - 1);

	WRITE("\t.type %s\n", sym_name(type->sym));

	uint32_t *end;
	switch (id)
	{
	case s_function: end = disasmFunc(obj, segment_end); break;
	case s_string: end = disasmString(obj, segment_end); break;
	default:
		end = obj; break;
	}
	WRITE("\n");
	return end;
}

void disasmEnd()
{
	WRITE("; end\n");

	if (final_pass)
		labels.clear();
}

void disasmFile(FILE *fp, MetaGoFile *go, bool final_pass)
{
	disasmBegin(fp, go, final_pass);

	for each (const MetaSegment &s in go->segments)
	{
		WRITE(";==================================================================\n");
		WRITE("\t.segment %s\n", s.name.c_str());
		WRITE(";==================================================================\n");
		WRITE("\n");

		uint32_t *end = (uint32_t *)((uint8_t *)s.start + s.size);
		uint32_t *prev = s.start;

		// Find all objects in this segment.
		vector<uint32_t *> objs;
		for each (uint32_t *obj in go->objects)
		{
			if (obj >= s.start && obj < end)
				objs.push_back(obj);
		}

		std::sort(objs.begin(), objs.end());

		for each (uint32_t *obj in objs)
		{
			if (obj < prev)
				continue;

			disasmData(prev, obj - 1, end);
			prev = disasmObj(obj, s.start, end);
			assert(prev <= end);
		}

		assert(prev <= end);
		disasmData(prev, end, end);
	}

	// Renumber labels in order.
	vector<uint32_t *> all;
	for each (pair<uint32_t *, string> x in labels)
		all.push_back(x.first);
	sort(all.begin(), all.end());
	int i = 1;
	for each (uint32_t *x in all)
	{
		// Check it's an auto-generated name.
		string s = labels[x];
		if (s.length() >= 2 && s[0] == 'L' && isdigit(s[1]))
		{
			labels[x] = print("L%i", i++);
		}
	}

	disasmEnd();
}