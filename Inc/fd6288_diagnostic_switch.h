#pragma once
/* Pure observation only. Never wrap control state, output gates or deadlines. */
#ifdef TARGET_FD_NO_DIAGNOSTICS
#define FD_DIAG(...) ((void)0)
#else
#define FD_DIAG(...) do { __VA_ARGS__; } while (0)
#endif

/* M2 production removes observers, never control state or safety gates. */
#ifdef TARGET_FD_M2_PURE
#define FD_M2_DIAG(...) ((void)0)
#else
#define FD_M2_DIAG(...) do { __VA_ARGS__; } while (0)
#endif
