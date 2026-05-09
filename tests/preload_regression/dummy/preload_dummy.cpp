#if defined(_WIN32)
#define PRELOAD_DUMMY_API extern "C" __declspec(dllexport)
#else
#define PRELOAD_DUMMY_API extern "C"
#endif

PRELOAD_DUMMY_API int preload_dummy_add(int a, int b)
{
    return a + b;
}
