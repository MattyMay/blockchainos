/*
blockchain.c: method definitions for blockchain structure
Copyright (C) 2019 
maintainer: Carlos WM
email: cwmoreiras@gmail.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "blockchain.h"
#include "util.h"
#include <string.h>

#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

void blockchain_verify_block(Blockchain *this);
void blockchain_verify_chain(Blockchain *this);
void blockchain_root(Blockchain *this);
void blockchain_append(Blockchain *this,
                       uint8_t *record,
                       uint64_t record_sz);
void blockframe_print(uint8_t *this);
int blockchain_delete_front(void);

void blockframe_print(uint8_t *this) 
// -----------------------------------------------------------------------------
// Func: 
// Args:
// Retn:
// -----------------------------------------------------------------------------
{
  uint8_t prevhash[1000], hash[1000];
  uint64_t index, timestamp, record_sz;
  uint8_t record[1000];

  blockframe_decode(this, prevhash, hash,
    &index, &timestamp, &record_sz, record);

  printf("-------------------------------------------------------------------------\n");
  util_buf_print_hex(prevhash, SHA256_DIGEST_LENGTH, "phash", 1);
  util_buf_print_hex(hash, SHA256_DIGEST_LENGTH, "hash ", 1);

  printf("index: %lu\n", index);
  printf("tstmp: %lu\n", timestamp);
  printf("recsz: %lu\n", record_sz);

  util_buf_print_hex(record, record_sz, "recrd", 1);
  
}

void blockframe_decode(uint8_t *this,
                       uint8_t *prevhash, uint8_t *hash,
                       uint64_t *index, uint64_t *timestamp,
                       uint64_t *record_sz, uint8_t *record)
// -----------------------------------------------------------------------------
// Func: Inverse of block_frame function. Takes a block frame and extracts its 
//       contents to facilitate certain processing needs
// Args: this - a pointer to the block frame
//       prevhash - pointer to the previous hash
//       hash - a pointer to this block's hash
//       index - pointer to the index of this block
//       timestamp - pointer to this block's original timestamp
//       record_sz - pointer to the size of this block's record
//       record - points to the first element of the record
// Retn:
// -----------------------------------------------------------------------------
{
  memcpy(prevhash, &this[PREVHASH_POS], HASH_SZ);
  memcpy(hash, &this[CURRHASH_POS], HASH_SZ);
  *index = *(uint64_t *)&this[INDEX_POS];
  *timestamp = *(uint64_t *)&this[TS_POS];
  memcpy(record_sz, &this[RECORD_SZ_POS], WORD_SZ);
  memcpy(record, &this[RECORD_POS], *record_sz);
}

// frames a block (stores all members in a buffer with no padding)
void block_frame(Block *this, uint8_t *buf)
// -----------------------------------------------------------------------------
// Func: Store the contents of the block struct in an array to remove 0 buffer
// Args: block - a pointer to this block
//       buf - the buffer that the contents of the block will be copied to
// Retn: None
// -----------------------------------------------------------------------------
{
  memcpy(&buf[PREVHASH_POS], this->prevhash, HASH_SZ);
  memcpy(&buf[CURRHASH_POS], this->hash, HASH_SZ);
  memcpy(&buf[INDEX_POS], &this->index, WORD_SZ);
  memcpy(&buf[TS_POS], &this->timestamp, WORD_SZ);
  memcpy(&buf[RECORD_SZ_POS], &this->record_sz, WORD_SZ);
  memcpy(&buf[RECORD_POS], this->record, this->record_sz);
}

void block_hash(Block *this, uint8_t *hash)
// -----------------------------------------------------------------------------
// Func: Hash the the block after removing struct 0 padding
// Args: this - a pointer to the block
//       hash - a pointer to the hash of the block
// Retn: None
// -----------------------------------------------------------------------------
{
  uint8_t buf[BLOCK_HEADER_SZ + this->record_sz]; // record size is variable
  block_frame(this, buf);
  util_buf_hash(buf, BLOCK_HEADER_SZ + this->record_sz, hash);
}

void blockchain_root(Blockchain *this)
// -----------------------------------------------------------------------------
// Func: Construct the blockchain's root block (must be hardcoded)
// Args: this - the blockchain for which we need a root
// Retn: None
// -----------------------------------------------------------------------------
{
  Block block;
  uint8_t *record = (uint8_t *)"this is the first block"; // message can change
  uint64_t record_sz = strlen((char *)record)+1; // count the null character
  uint64_t blocksize = BLOCK_HEADER_SZ + record_sz;
  uint8_t buf[blocksize];
  uint8_t hash[HASH_SZ];

  block.record_sz = record_sz;
  block.record = malloc(block.record_sz);
  memcpy(block.record, record, block.record_sz);

  block.index = 0; // its the root block
  block.timestamp = time(NULL);

  memset(block.prevhash, 0, HASH_SZ);
  memset(block.hash, 0, HASH_SZ);

  // this will hash the whole block, with 0's in the prevhash and hash fields
  block_hash(&block, hash); 
  memcpy(&block.hash, hash, HASH_SZ); // fill the hash field with the result 
  block_frame(&block, buf); // frame it to remove 0-padding 

  this->insert_front(this, buf, blocksize); // add the framed block to the chain
  free(block.record); // free the local copy

}

void blockchain_append(Blockchain *this,
                       uint8_t *record,
                       uint64_t record_sz)
// -----------------------------------------------------------------------------
// Func: Append a record to the blockchain
// Args: this - a pointer to the blockchain
//       record - the record that we'd like to append
//       record_sz - the size of the record
// Retn: None
// -----------------------------------------------------------------------------
{
  Block block;
  uint64_t blocksize = BLOCK_HEADER_SZ + record_sz;
  uint8_t buf[blocksize];
  uint8_t hash[HASH_SZ];

  // get a pointer to the previous block
  Block *prevblock = (Block *)this->peek_front(this);

  block.record_sz = record_sz;
  block.record = malloc(block.record_sz);
  memcpy(block.record, record, block.record_sz); // copy record into block

  block.timestamp = time(NULL);
  block.index = prevblock->index+1; // increment index

  memcpy(block.prevhash, prevblock->hash, HASH_SZ); // copy the prev blocks hash
  memset(block.hash, 0, HASH_SZ); // set the hash field to 0

  block_hash(&block, hash); // hash the block
  memcpy(&block.hash, hash, HASH_SZ); // copy the hash into the hash field
  block_frame(&block, buf); // frame for storage
 
  this->insert_front(this, buf, blocksize); // append to the chain
  free(block.record); // free the local copy

}

int blockchain_delete_front(void) 
{
  return -1; // you can't delete blocks how you would in a linkedlist
}


void blockchain_init(Blockchain *blockchain)
// -----------------------------------------------------------------------------
// Func: Initialize a new blockchain
// Args: blockchain - a pointer to the new chain
// Retn: None
// -----------------------------------------------------------------------------
{
  linkedlist_init(blockchain); // blockchain is just a fancy linkedlist
  
  // we need to override some of the linkedlist functions
  // blockchain->insert_front = &blockchain_append;
  blockchain->append = &blockchain_append;
  // blockchain->delete_front = &blockchain_delete_front;

  blockchain_root(blockchain); // build and attach the root block

}

void blockchain_destroy(Blockchain *blockchain)
// -----------------------------------------------------------------------------
// Func: Destroy a blockchain object
// Args: blockchain - point to the blockchain object we'd like to destroy
// Retn: None
// -----------------------------------------------------------------------------
{
  linkedlist_destroy(blockchain); // just need to destroy the list
}
