/*
dynarray.c: method definitions for dynarray structure
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

#include "dynarray.h"
#include <stdint.h>
#include <stdlib.h>

// private functions, access through dynarray object
int dynarray_insert(DynArray *this, uint8_t element, uint64_t index);
uint8_t dynarray_remove(DynArray *this, uint64_t index, int *valid);
int dynarray_set(DynArray *this, uint8_t element, uint64_t index);
uint8_t dynarray_get(DynArray *this, uint64_t index, int *valid);
int dynarray_grow(DynArray *this);

void dynarray_init(DynArray *this, uint64_t cap)
// -----------------------------------------------------------------------------
// Func: Initialize the dynarray object
// Args: this - a pointer to this dynarray object
// Retn: None
// -----------------------------------------------------------------------------
{
  this->buf = malloc(cap*sizeof(uint8_t));
  this->sz = 0; // current size
  this->cap = cap; // current capacity

  // set all function pointers
  this->insert = &dynarray_insert;
  this->remove = &dynarray_remove;
  this->set = &dynarray_set;
  this->get = &dynarray_get;
}

void dynarray_destroy(DynArray *this)
// -----------------------------------------------------------------------------
// Func: Destroy the dynarray object by freeing the buffer memory
// Args: this - a pointer to this dynarray object
// Retn: None
// -----------------------------------------------------------------------------
{
  free(this->buf);
  this->sz = 0;
  this->cap = 0;
  this->insert = NULL;
  this->remove = NULL;
  this->set = NULL;
  this->get = NULL;
}

int dynarray_grow(DynArray *this)
// -----------------------------------------------------------------------------
// Func: Double the size of the dynarray buffer
// Args: this - a pointer to this dynarray object
// Retn: error code
// -----------------------------------------------------------------------------
{
  this->cap *= 2; // double the size of the buffer for amortized cost
  if ((this->buf = realloc(this->buf, this->cap*sizeof(void*))) == NULL) {
    return -1; // TODO error stuff
  }

  return 0;
}

// TODO - do we really want the dynarray to operate on bytes?
//        consider a larger unit, or MAYBE void pointers
int dynarray_insert(DynArray *this, uint8_t element, uint64_t index)
// -----------------------------------------------------------------------------
// Func: Insert an element into an arbitrary location
// Args: this - a pointer to this dynarray object
//       element - the byte being inserted
// Retn: error code
// -----------------------------------------------------------------------------
{
  uint64_t i;

  if (index > this->sz) {
    return -1; // TODO index out of range
  }

  // if size == cap and grow fails, return -1
  if ((this->sz == this->cap) && dynarray_grow(this)) {
    return -1;
  }

  // TODO shift all elements right
  for (i = this->sz; i > index; i--) {
    this->buf[i] = this->buf[i-1];
  }

  this->buf[index] = element;
  this->sz++;

  return 0;
}

// TODO - return an error code, and fill a buffer with a copy of the memory that
//        is being removed, instead of what's happening now, which is
//        the opposite
// returns a pointer to the memory that is begin removed from the list
// valid contains the error code
uint8_t dynarray_remove(DynArray *this, uint64_t index, int *valid)
// -----------------------------------------------------------------------------
// Func: Remove an element from an arbitrary location
// Args: this - a pointer to this dynarray object
//       element - the index of the element to be deleted
//       valid - when the function returns, will be set to 1 if a valid index
//               was given, 0 otherwise. the caller should check this value to
//               see if their data was valid.
// Retn: the element being removed
// -----------------------------------------------------------------------------
{
  uint64_t i;
  uint8_t element;

  // TODO - don't like how this is happening
  if (index >= this->sz)
    *valid = 0;
  *valid = 1;
  element = this->buf[index];

  for (i = index; i < this->sz-1; i++) {
    this->buf[i] = this->buf[i+1];
  }
  this->sz--;

  return element;
}

int dynarray_set(DynArray *this, uint8_t element, uint64_t index)
// -----------------------------------------------------------------------------
// Func: Set the value of an element that currently exists in the list
// Args: this - a pointer to this dynarray object
//       element - the new value
//       index - the index being updated
// Retn: error code
// -----------------------------------------------------------------------------
{
  if (index > this->sz) {
    return -1; // TODO index out of range
  }

  this->buf[index] = element;

  return 0;
}

// TODO - return an error code, and fill a buffer with a copy of the memory that
//        is being removed, instead of what's happening now, which is
//        the opposite
uint8_t dynarray_get(DynArray *this, uint64_t index, int *valid)
// -----------------------------------------------------------------------------
// Func: Retrieve a pointer to the element at the given index
// Args: this - a pointer to this dynarray object
//       index - the index of the element to return
//       valid - 1 if the index is in range, else 0
// Retn: a pointer to the requested element
// -----------------------------------------------------------------------------
{
  if (index >= this->sz)
    *valid = 0;
  *valid = 1;

  return this->buf[index];
}
