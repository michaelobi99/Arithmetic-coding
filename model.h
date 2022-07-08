#pragma once
using USHORT = unsigned short;
#define END_OF_STREAM 256
#define END_OF_COUNT 0


struct Symbol {
	USHORT low_count;
	USHORT high_count;
	USHORT scale;
};

USHORT totals[258];

void buildTotals(unsigned char counts[]) {
	totals[0] = 0;
	for (unsigned char index : std::ranges::iota_view(0, END_OF_STREAM)) {
		totals[index + 1] = totals[index] + counts[index];
	}
	totals[END_OF_STREAM + 1] = totals[END_OF_STREAM] + 1;
}