/*
 * PubKey.h
 *
 *  Created on: 15 de ago de 2025
 *      Author: carlos.oliveira
 */

#ifndef PUBVKEY_H_
#define PUBKEY_H_

#include "bearssl_ec.h"
#include <stdbool.h>
#include <stdlib.h>


#define ECDH_PUB_P256_LEN	65

typedef struct sPubKey
{
    uint8_t pbBuf[ECDH_PUB_P256_LEN];
    br_ec_public_key pbKey;
    bool ok;
}PubKey;

PubKey *New_PubKey();
void Destroy_PubKey(PubKey *obj);



#endif /* PUBKEY_H_ */
