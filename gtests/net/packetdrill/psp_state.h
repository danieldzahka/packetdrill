#ifndef __PSP_STATE_H__
#define __PSP_STATE_H__

#include "types.h"
#include "psp.h"

#define PSP_LIVE_SPI_MAX_ENTRIES 10

struct psp_state {
	struct {
		__be32 script_spi;
		__be32 live_spi;
		u8 key[PSP_MAX_KEY];
	} rx_spi_table[PSP_LIVE_SPI_MAX_ENTRIES];
	__be32 entries;
};

struct psp_state *psp_state_new();
void psp_state_free(struct psp_state *state);
int psp_state_add_spi(struct psp_state *state, __be32 script_spi,
		      __be32 live_spi, u8 *key, int key_len);
int psp_to_live_spi(struct psp_state *state, __be32 script_spi,
		    __be32 *live_spi, u8 *key, int key_len);

#endif /* __PSP_STATE_H__ */
