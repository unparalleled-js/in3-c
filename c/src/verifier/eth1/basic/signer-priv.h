/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

#ifndef in3_signer_priv_h__
#define in3_signer_priv_h__
typedef enum {
  hasher_sha2,
  hasher_sha2d,
  hasher_sha2_ripemd,
  hasher_sha3,
  hasher_sha3k,
  hasher_blake,
  hasher_blaked,
  hasher_blake_ripemd,
  hasher_groestld_trunc, /* double groestl512 hasher truncated to 256 bits */
  hasher_overwinter_prevouts,
  hasher_overwinter_sequence,
  hasher_overwinter_outputs,
  hasher_overwinter_preimage,
  hasher_sapling_preimage,
} hasher_t;

#include "../../../core/client/context_internal.h"

in3_ret_t sign(d_signature_type_t type, bytes_t message, uint8_t* pk, uint8_t* dst);
in3_ret_t eth_sign(void* ctx, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst);
bytes_t   sign_tx(d_token_t* tx, in3_ctx_t* ctx);
in3_ret_t sign_hash(uint8_t* message, uint8_t len, uint8_t* pk, hasher_t hasher, uint8_t* dst);
in3_ret_t sign_raw(uint8_t* message, uint8_t* pk, uint8_t* dst);
#endif
