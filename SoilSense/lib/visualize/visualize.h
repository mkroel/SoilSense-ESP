#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void visualize_init(void);
void visualize_update(int status);   // 0=ok, 1=pump, 2=tank empty

#ifdef __cplusplus
}
#endif