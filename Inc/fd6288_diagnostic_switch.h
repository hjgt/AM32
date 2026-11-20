#pragma once
/* Pure observation only. Never wrap control state, output gates or deadlines. */
#ifdef TARGET_FD_NO_DIAGNOSTICS
#define FD_DIAG(...) ((void)0)
#else
#define FD_DIAG(...) do { __VA_ARGS__; } while (0)
#endif

/* Preserve recorder code for diagnostic builds; erase observer arguments too. */
#ifdef TARGET_FD_M1_PURE
#define FD_M1_DIAG(...) ((void)0)
#else
#define FD_M1_DIAG(...) do { __VA_ARGS__; } while (0)
#endif
