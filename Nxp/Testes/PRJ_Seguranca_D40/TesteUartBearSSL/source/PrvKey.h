/*
 * Prvkey.h
 *
 *  Created on: 15 de ago de 2025
 *      Author: carlos.oliveira
 */

#ifndef PRVKEY_H_
#define PRVKEY_H_

#include "bearssl_ec.h"
#include <stdbool.h>
#include <stdlib.h>


#define ECDH_PRV_P256_LEN	32

typedef struct sPrvKey
{
    uint8_t pvBuf[ECDH_PRV_P256_LEN];
    br_ec_private_key pvKey;
    bool ok;
}PrvKey;



PrvKey *New_PrvKey();
void Destroy_PrvKey(PrvKey *obj);

#endif /* PRVKEY_H_ */
