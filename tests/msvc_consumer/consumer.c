#include <libeasymcp2221/mcp2221.h>

int main(void) {
	mcp2221_t *dev = 0;
	return (int)mcp2221_open(
		0x04D8u, 0x00DDu, 0, 0, 1, 0, 0, 0, &dev);
}
