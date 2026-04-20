#include "BlackfinArchitecture.h"

BlackfinArchitecture *blackfin_arch_init(const char *name, int endian) {
	(void)name;
	(void)endian;
	static BlackfinArchitecture arch;
	return &arch;
}

void blackfin_arch_free(BlackfinArchitecture *arch) {
	(void)arch;
}