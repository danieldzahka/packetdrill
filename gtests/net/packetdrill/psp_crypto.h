#ifndef __PSP_CRYPTO_H__
#define __PSP_CRYPTO_H__

#include "types.h"
#include "psp.h"

int psp_encrypt(struct psp *psp, int psp_len, u8 *derived_key);
int psp_decrypt(struct psp *psp, int psp_len, u8 *derived_key);

#endif /* __PSP_CRYPTO_H__ */
