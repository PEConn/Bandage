#ifndef SOFTBOUND_H
#define SOFTBOUND_H
void TableSetup();
void TableTeardown();
void TableLookup(void *key, void **base, void **bound);
void TableAssign(void *key, void *base, void *bound);
#endif
