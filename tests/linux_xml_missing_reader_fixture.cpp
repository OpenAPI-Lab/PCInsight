// Test-only library: dlopen succeeds, but required reader symbols are absent.
extern "C" void* xmlReaderForMemory(const char*, int, const char*, const char*, int) { return nullptr; }
