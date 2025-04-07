#include "types.h"
#include "psp.h"

#include <gnutls/crypto.h>

typedef enum {
	PSP_ENCRYPT,
	PSP_DECRYPT,
} enc_dec_t;

enum {
	PSP_V0 = 0,
	PSP_V1,
} psp_versions;

#define CRYPT_OFF 4

static int psp_do_crypto(struct psp *psp, int psp_len, u8 *derived_key,
			 enc_dec_t action)
{
	uint8_t version = psp->version;

	gnutls_aead_cipher_hd_t handle;
	gnutls_datum_t key = {
		.data = derived_key,
		.size = version == PSP_V0 ? PSP_V0_KEY : PSP_V1_KEY,
	};

	gnutls_cipher_algorithm_t algorithm =
		version == PSP_V0 ? GNUTLS_CIPHER_AES_128_GCM :
				    GNUTLS_CIPHER_AES_256_GCM;
	int rc = gnutls_aead_cipher_init(&handle, algorithm, &key);
	if (rc)
		return rc;

	uint8_t nonce[12];
	memcpy(nonce, &psp->spi, sizeof(nonce));

	giovec_t auth = {
		.iov_base = psp,
		.iov_len = PSP_MINLEN + psp->crypt_offset * CRYPT_OFF,
	};
	int auth_vec_len = 1;

	if (auth.iov_len > psp_len)
		return STATUS_ERR;

	giovec_t ctext = {
		.iov_base = (uint8_t *)auth.iov_base + auth.iov_len,
		.iov_len = psp_len - auth.iov_len,
	};
	int ctext_vec_len = 1;

	size_t tag_size = PSP_TRL_SIZE;
	uint8_t *tag = (uint8_t *)ctext.iov_base + ctext.iov_len;

	return action == PSP_ENCRYPT ? gnutls_aead_cipher_encryptv2(
					       handle, nonce, sizeof(nonce),
					       &auth, auth_vec_len, &ctext,
					       ctext_vec_len, tag, &tag_size) :
				       gnutls_aead_cipher_decryptv2(
					       handle, nonce, sizeof(nonce),
					       &auth, auth_vec_len, &ctext,
					       ctext_vec_len, tag, tag_size);
}

int psp_encrypt(struct psp *psp, int psp_len, u8 *derived_key)
{
	int rc = psp_do_crypto(psp, psp_len - PSP_TRL_SIZE, derived_key,
			       PSP_ENCRYPT);
	return rc ? STATUS_ERR : STATUS_OK;
}

int psp_decrypt(struct psp *psp, int psp_len, u8 *derived_key)
{
	int rc = psp_do_crypto(psp, psp_len - PSP_TRL_SIZE, derived_key,
			       PSP_DECRYPT);
	return rc ? STATUS_ERR : STATUS_OK;
}
