/*
 * Loads a SourceMod extension the way srcds does (RTLD_NOW: every symbol must resolve)
 * and checks that it exports GetSMExtAPI. Built and run by scripts/check-binary.sh.
 */
#include <dlfcn.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s extension.so\n", argv[0]);
		return 2;
	}
	void *handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
	if (!handle) {
		fprintf(stderr, "dlopen failed: %s\n", dlerror());
		return 1;
	}
	if (!dlsym(handle, "GetSMExtAPI")) {
		fprintf(stderr, "GetSMExtAPI not exported: %s\n", argv[1]);
		return 1;
	}
	printf("dlopen OK: %s\n", argv[1]);
	return 0;
}
