#ifndef __PSP_YNL_H__
#define __PSP_YNL_H__

#include "types.h"

#include <ynl.h>

struct psp_ynl_state;

struct psp_ynl_state *psp_ynl_new(bool enable_psp);
void psp_ynl_free(struct psp_ynl_state *state);
int psp_ynl_rx_spi_alloc(struct psp_ynl_state *state, int live_sock,
			 __be32 *live_spi, u8 *live_key);
int psp_ynl_tx_spi_set(struct psp_ynl_state *state, int live_sock, __be32 spi);
#endif /* __PSP_YNL_H__ */
