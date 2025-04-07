#include "psp-ynl.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <ynl.h>

#include "psp-user.h"
#include "psp.h"

struct psp_ynl_state {
	struct ynl_sock *ys;
	int psp_dev_id;
	u32 restore_ver_ena;
};

static int psp_dev_set_ena(struct ynl_sock *ys, u32 dev_id, u32 versions)
{
	struct psp_dev_set_req *sreq;
	struct psp_dev_set_rsp *srsp;

	fprintf(stdout, "Set PSP enable on device %d to 0x%x\n", dev_id,
		versions);

	sreq = psp_dev_set_req_alloc();

	psp_dev_set_req_set_id(sreq, dev_id);
	psp_dev_set_req_set_psp_versions_ena(sreq, versions);

	srsp = psp_dev_set(ys, sreq);
	psp_dev_set_req_free(sreq);
	if (!srsp)
		return STATUS_ERR;

	psp_dev_set_rsp_free(srsp);
	return STATUS_OK;
}

static int psp_ynl_init(bool enable_psp, struct psp_ynl_state *state)
{
	struct psp_dev_get_list *dev_list;
	struct ynl_error yerr;
	struct ynl_sock *ys;
	int first_id = 0;
	u32 ver_ena, ver_cap, ver_want;

	ys = ynl_sock_create(&ynl_psp_family, &yerr);
	if (!ys) {
		fprintf(stderr, "YNL: %s\n", yerr.msg);
		return STATUS_ERR;
	}

	dev_list = psp_dev_get_dump(ys);
	if (ynl_dump_empty(dev_list)) {
		if (ys->err.code)
			goto err_close;
		printf("No PSP devices\n");
		return STATUS_ERR;
	}

	ynl_dump_foreach(dev_list, d)
	{
		if (!first_id) {
			first_id = d->id;
			ver_ena = d->psp_versions_ena;
			ver_cap = d->psp_versions_cap;
		} else {
			fprintf(stderr, "Multiple PSP devices found\n");
			goto err_close_silent;
		}
	}
	psp_dev_get_list_free(dev_list);

	ver_want = enable_psp ? ver_cap : 0;
	if (ver_ena != ver_want) {
		if (psp_dev_set_ena(ys, first_id, ver_want))
			goto err_close;
	}

	state->ys = ys;
	state->psp_dev_id = first_id;
	state->restore_ver_ena = ver_ena;

	return STATUS_OK;

err_close:
	fprintf(stderr, "YNL: %s\n", ys->err.msg);
err_close_silent:
	ynl_sock_destroy(ys);
	return STATUS_ERR;
}

static void psp_ynl_uninit(struct psp_ynl_state *state)
{
	if (psp_dev_set_ena(state->ys, state->psp_dev_id,
			    state->restore_ver_ena))
		fprintf(stderr, "WARN: failed to set the PSP versions back\n");

	ynl_sock_destroy(state->ys);
}

struct psp_ynl_state *psp_ynl_new(bool enable_psp)
{
	struct psp_ynl_state *state = calloc(1, sizeof(*state));
	int rc;

	rc = psp_ynl_init(enable_psp, state);
	return rc ? NULL : state;
}
void psp_ynl_free(struct psp_ynl_state *state)
{
	if (!state)
		return;

	psp_ynl_uninit(state);
	free(state);
}

int psp_ynl_rx_spi_alloc(struct psp_ynl_state *state, int live_sock,
			 __be32 *live_spi, u8 *live_key)
{
	struct psp_rx_assoc_rsp *rsp;
	struct psp_rx_assoc_req *req;
	int key_len;

	req = psp_rx_assoc_req_alloc();
	psp_rx_assoc_req_set_dev_id(req, state->psp_dev_id);
	psp_rx_assoc_req_set_sock_fd(req, live_sock);
	psp_rx_assoc_req_set_version(req, PSP_VERSION_HDR0_AES_GCM_128);

	rsp = psp_rx_assoc(state->ys, req);
	psp_rx_assoc_req_free(req);

	if (!rsp)
		return STATUS_ERR;

	key_len = rsp->rx_key._len.key;
	*live_spi = rsp->rx_key.spi;
	memcpy(live_key, rsp->rx_key.key, key_len);
	psp_rx_assoc_rsp_free(rsp);
	return STATUS_OK;
}

int psp_ynl_tx_spi_set(struct psp_ynl_state *state, int live_sock, __be32 spi)
{
	struct psp_tx_assoc_rsp *tsp;
	struct psp_tx_assoc_req *teq;
	u8 key[PSP_V0_KEY] = { 1, 2,  3,  4,  5,  6,  7,  8,
			       9, 10, 11, 12, 13, 14, 15, 16 };

	teq = psp_tx_assoc_req_alloc();

	psp_tx_assoc_req_set_dev_id(teq, state->psp_dev_id);
	psp_tx_assoc_req_set_sock_fd(teq, live_sock);
	psp_tx_assoc_req_set_version(teq, PSP_VERSION_HDR0_AES_GCM_128);
	psp_tx_assoc_req_set_tx_key_spi(teq, spi);
	psp_tx_assoc_req_set_tx_key_key(teq, key, sizeof(key));

	tsp = psp_tx_assoc(state->ys, teq);
	psp_tx_assoc_req_free(teq);
	if (!tsp) {
		perror("ERROR: failed to Tx assoc");
		return STATUS_ERR;
	}
	psp_tx_assoc_rsp_free(tsp);

	return STATUS_OK;
}
