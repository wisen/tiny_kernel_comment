#include "intel_renderstate.h"

static const u32 gen8_null_state_relocs[] = {
	0x00000048,
	0x00000050,
	0x00000060,
	0x000003ec,
	-1,
};

static const u32 gen8_null_state_batch[] = {
	0x69040000,
	0x61020001,
	0x00000000,
	0x00000000,
	0x79120000,
	0x00000000,
	0x79130000,
	0x00000000,
	0x79140000,
	0x00000000,
	0x79150000,
	0x00000000,
	0x79160000,
	0x00000000,
	0x6101000e,
	0x00000001,
	0x00000000,
	0x00000001,
	0x00000001,	 /* reloc */
	0x00000000,
	0x00000001,	 /* reloc */
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000001,	 /* reloc */
	0x00000000,
	0xfffff001,
	0x00001001,
	0xfffff001,
	0x00001001,
	0x78230000,
	0x000006e0,
	0x78210000,
	0x00000700,
	0x78300000,
	0x08010040,
	0x78330000,
	0x08000000,
	0x78310000,
	0x08000000,
	0x78320000,
	0x08000000,
	0x78240000,
	0x00000641,
	0x780e0000,
	0x00000601,
	0x780d0000,
	0x00000000,
	0x78180000,
	0x00000001,
	0x78520003,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78190009,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x781b0007,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78270000,
	0x00000000,
	0x782c0000,
	0x00000000,
	0x781c0002,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78160009,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78110008,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78290000,
	0x00000000,
	0x782e0000,
	0x00000000,
	0x781a0009,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x781d0007,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78280000,
	0x00000000,
	0x782d0000,
	0x00000000,
	0x78260000,
	0x00000000,
	0x782b0000,
	0x00000000,
	0x78150009,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78100007,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x781e0003,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78120002,
	0x00000000,
	0x00000000,
	0x00000000,
	0x781f0002,
	0x30400820,
	0x00000000,
	0x00000000,
	0x78510009,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78500003,
	0x00210000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78130002,
	0x00000000,
	0x00000000,
	0x00000000,
	0x782a0000,
	0x00000480,
	0x782f0000,
	0x00000540,
	0x78140000,
	0x00000800,
	0x78170009,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x7820000a,
	0x00000580,
	0x00000000,
	0x08080000,
	0x00000000,
	0x00000000,
	0x1f000002,
	0x00060000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x784d0000,
	0x40000000,
	0x784f0000,
	0x80000100,
	0x780f0000,
	0x00000740,
	0x78050006,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78070003,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78060003,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78040001,
	0x00000000,
	0x00000001,
	0x79000002,
	0xffffffff,
	0x00000000,
	0x00000000,
	0x78080003,
	0x00006000,
	0x000005e0,	 /* reloc */
	0x00000000,
	0x00000000,
	0x78090005,
	0x02000000,
	0x22220000,
	0x02f60000,
	0x11230000,
	0x02850004,
	0x11230000,
	0x784b0000,
	0x0000000f,
	0x78490001,
	0x00000000,
	0x00000000,
	0x7b000005,
	0x00000000,
	0x00000003,
	0x00000000,
	0x00000001,
	0x00000000,
	0x00000000,
	0x05000000,	 /* cmds end */
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x000004c0,	 /* state start */
	0x00000500,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000092,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x0060005a,
	0x21403ae8,
	0x3a0000c0,
	0x008d0040,
	0x0060005a,
	0x21603ae8,
	0x3a0000c0,
	0x008d0080,
	0x0060005a,
	0x21803ae8,
	0x3a0000d0,
	0x008d0040,
	0x0060005a,
	0x21a03ae8,
	0x3a0000d0,
	0x008d0080,
	0x02800031,
	0x2e0022e8,
	0x0e000140,
	0x08840001,
	0x05800031,
	0x200022e0,
	0x0e000e00,
	0x90031000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x06200000,
	0x00000002,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0xf99a130c,
	0x799a130c,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x3f800000,
	0x00000000,
	0x3f800000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,	 /* state end */
};

RO_RENDERSTATE(8);
