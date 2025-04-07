#include "psp_state.h"

#include <stdlib.h>

struct psp_state *psp_state_new()
{
	return calloc(1, sizeof(struct psp_state));
}

void psp_state_free(struct psp_state *state)
{
	assert(state);
	free(state);
}

int psp_state_add_spi(struct psp_state *state, __be32 script_spi,
		      __be32 live_spi, u8 *key, int key_len)
{
	int entries = ntohl(state->entries);

	if (entries >= PSP_LIVE_SPI_MAX_ENTRIES)
		return STATUS_ERR;

	state->rx_spi_table[entries].script_spi = script_spi;
	state->rx_spi_table[entries].live_spi = live_spi;
	memcpy(state->rx_spi_table[entries].key, key, key_len);
	++entries;

	state->entries = htonl(entries);
	return STATUS_OK;
}

int psp_to_live_spi(struct psp_state *state, __be32 script_spi,
		    __be32 *live_spi, u8 *key, int key_len)
{
	for (int i = 0; i < ntohl(state->entries); ++i) {
		if (script_spi == state->rx_spi_table[i].script_spi) {
			*live_spi = state->rx_spi_table[i].live_spi;
			memcpy(key, state->rx_spi_table[i].key, key_len);
			return STATUS_OK;
		}
	}

	fprintf(stderr, "script spi %u, not found in spi table\n",
		ntohl(script_spi));

	return STATUS_ERR;
}
