/*****************************************************************************/
/* huffman.cpp                       Copyright (c) Ladislav Zezula 1998-2003 */
/*---------------------------------------------------------------------------*/
/* This module contains Huffmann (de)compression methods                     */
/*                                                                           */
/* Authors : Ladislav Zezula (ladik@zezula.net)                              */
/*           ShadowFlare     (BlakFlare@hotmail.com)                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* xx.xx.xx  1.00  Lad  The first version of dcmp.cpp                        */
/* 03.05.03  1.00  Lad  Added compression methods                            */
/* 19.11.03  1.01  Dan  Big endian handling                                  */
/* 08.12.03  2.01  Dan  High-memory handling (> 0x80000000)                  */
/* 09.01.13  3.00  Lad  Refactored, beautified, documented :-)               */
/*****************************************************************************/

#include <assert.h>
#include <string.h>

#include "huff.h"

//-----------------------------------------------------------------------------
// Local defined

#define HUFF_DECOMPRESS_ERROR   0x1FF

//-----------------------------------------------------------------------------
// Table of byte-to-weight values

// Table for (de)compression. Every compression type has 258 entries
static unsigned char ByteToWeight_00[] =
{
    0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x00
};

// Data for compression type 0x01
static unsigned char ByteToWeight_01[] =
{
    0x54, 0x16, 0x16, 0x0D, 0x0C, 0x08, 0x06, 0x05, 0x06, 0x05, 0x06, 0x03, 0x04, 0x04, 0x03, 0x05,
    0x0E, 0x0B, 0x14, 0x13, 0x13, 0x09, 0x0B, 0x06, 0x05, 0x04, 0x03, 0x02, 0x03, 0x02, 0x02, 0x02,
    0x0D, 0x07, 0x09, 0x06, 0x06, 0x04, 0x03, 0x02, 0x04, 0x03, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02,
    0x09, 0x06, 0x04, 0x04, 0x04, 0x04, 0x03, 0x02, 0x03, 0x02, 0x02, 0x02, 0x02, 0x03, 0x02, 0x04,
    0x08, 0x03, 0x04, 0x07, 0x09, 0x05, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02,
    0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x02, 0x01, 0x02, 0x02,
    0x06, 0x0A, 0x08, 0x08, 0x06, 0x07, 0x04, 0x03, 0x04, 0x04, 0x02, 0x02, 0x04, 0x02, 0x03, 0x03,
    0x04, 0x03, 0x07, 0x07, 0x09, 0x06, 0x04, 0x03, 0x03, 0x02, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x0A, 0x02, 0x02, 0x03, 0x02, 0x02, 0x01, 0x01, 0x02, 0x02, 0x02, 0x06, 0x03, 0x05, 0x02, 0x03,
    0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x01, 0x01, 0x01,
    0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x04, 0x04, 0x04, 0x07, 0x09, 0x08, 0x0C, 0x02,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x03,
    0x04, 0x01, 0x02, 0x04, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01,
    0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x02, 0x02, 0x02, 0x06, 0x4B,
    0x00, 0x00
};

// Data for compression type 0x02
static unsigned char ByteToWeight_02[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x27, 0x00, 0x00, 0x23, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x06, 0x0E, 0x10, 0x04,
    0x06, 0x08, 0x05, 0x04, 0x04, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03, 0x01, 0x01, 0x02, 0x01, 0x01,
    0x01, 0x04, 0x02, 0x04, 0x02, 0x02, 0x02, 0x01, 0x01, 0x04, 0x01, 0x01, 0x02, 0x03, 0x03, 0x02,
    0x03, 0x01, 0x03, 0x06, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x02, 0x01, 0x01,
    0x01, 0x29, 0x07, 0x16, 0x12, 0x40, 0x0A, 0x0A, 0x11, 0x25, 0x01, 0x03, 0x17, 0x10, 0x26, 0x2A,
    0x10, 0x01, 0x23, 0x23, 0x2F, 0x10, 0x06, 0x07, 0x02, 0x09, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
};

// Data for compression type 0x03
static unsigned char ByteToWeight_03[] =
{
    0xFF, 0x0B, 0x07, 0x05, 0x0B, 0x02, 0x02, 0x02, 0x06, 0x02, 0x02, 0x01, 0x04, 0x02, 0x01, 0x03,
    0x09, 0x01, 0x01, 0x01, 0x03, 0x04, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01,
    0x05, 0x01, 0x01, 0x01, 0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x01, 0x01, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01,
    0x0A, 0x04, 0x02, 0x01, 0x06, 0x03, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x01,
    0x05, 0x02, 0x03, 0x04, 0x03, 0x03, 0x03, 0x02, 0x01, 0x01, 0x01, 0x02, 0x01, 0x02, 0x03, 0x03,
    0x01, 0x03, 0x01, 0x01, 0x02, 0x05, 0x01, 0x01, 0x04, 0x03, 0x05, 0x01, 0x03, 0x01, 0x03, 0x03,
    0x02, 0x01, 0x04, 0x03, 0x0A, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x01, 0x0A, 0x02, 0x05, 0x01, 0x01, 0x02, 0x07, 0x02, 0x17, 0x01, 0x05, 0x01, 0x01,
    0x0E, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x06, 0x02, 0x01, 0x04, 0x05, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x07, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x11,
    0x00, 0x00
};

// Data for compression type 0x04
static unsigned char ByteToWeight_04[] =
{
    0xFF, 0xFB, 0x98, 0x9A, 0x84, 0x85, 0x63, 0x64, 0x3E, 0x3E, 0x22, 0x22, 0x13, 0x13, 0x18, 0x17,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
};

// Data for compression type 0x05
static unsigned char ByteToWeight_05[] =
{
    0xFF, 0xF1, 0x9D, 0x9E, 0x9A, 0x9B, 0x9A, 0x97, 0x93, 0x93, 0x8C, 0x8E, 0x86, 0x88, 0x80, 0x82,
    0x7C, 0x7C, 0x72, 0x73, 0x69, 0x6B, 0x5F, 0x60, 0x55, 0x56, 0x4A, 0x4B, 0x40, 0x41, 0x37, 0x37,
    0x2F, 0x2F, 0x27, 0x27, 0x21, 0x21, 0x1B, 0x1C, 0x17, 0x17, 0x13, 0x13, 0x10, 0x10, 0x0D, 0x0D,
    0x0B, 0x0B, 0x09, 0x09, 0x08, 0x08, 0x07, 0x07, 0x06, 0x05, 0x05, 0x04, 0x04, 0x04, 0x19, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
};

    // Data for compression type 0x06
static unsigned char ByteToWeight_06[] =
{
    0xC3, 0xCB, 0xF5, 0x41, 0xFF, 0x7B, 0xF7, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xBF, 0xCC, 0xF2, 0x40, 0xFD, 0x7C, 0xF7, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x7A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
};

// Data for compression type 0x07
static unsigned char ByteToWeight_07[] =
{
    0xC3, 0xD9, 0xEF, 0x3D, 0xF9, 0x7C, 0xE9, 0x1E, 0xFD, 0xAB, 0xF1, 0x2C, 0xFC, 0x5B, 0xFE, 0x17,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xBD, 0xD9, 0xEC, 0x3D, 0xF5, 0x7D, 0xE8, 0x1D, 0xFB, 0xAE, 0xF0, 0x2C, 0xFB, 0x5C, 0xFF, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x70, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
};

// Data for compression type 0x08
static unsigned char ByteToWeight_08[] =
{
    0xBA, 0xC5, 0xDA, 0x33, 0xE3, 0x6D, 0xD8, 0x18, 0xE5, 0x94, 0xDA, 0x23, 0xDF, 0x4A, 0xD1, 0x10,
    0xEE, 0xAF, 0xE4, 0x2C, 0xEA, 0x5A, 0xDE, 0x15, 0xF4, 0x87, 0xE9, 0x21, 0xF6, 0x43, 0xFC, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xB0, 0xC7, 0xD8, 0x33, 0xE3, 0x6B, 0xD6, 0x18, 0xE7, 0x95, 0xD8, 0x23, 0xDB, 0x49, 0xD0, 0x11,
    0xE9, 0xB2, 0xE2, 0x2B, 0xE8, 0x5C, 0xDD, 0x15, 0xF1, 0x87, 0xE7, 0x20, 0xF7, 0x44, 0xFF, 0x13,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5F, 0x9E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
};

static unsigned char * WeightTables[0x09] =
{
    ByteToWeight_00,
    ByteToWeight_01,
    ByteToWeight_02,
    ByteToWeight_03,
    ByteToWeight_04,
    ByteToWeight_05,
    ByteToWeight_06,
    ByteToWeight_07,
    ByteToWeight_08
};

//-----------------------------------------------------------------------------
// Debug/diagnostics

#ifdef _DEBUG
void DumpHuffmannTree(THTreeItem * pItem)
{
    THTreeItem * pChildLo;                          // Item with the lower weight
    THTreeItem * pChildHi;                          // Item with the higher weight

    // Get the lower-weight branch
    pChildLo = pItem->pChildLo;
    if(pChildLo != NULL)
    {
        // Get the higher-weight branch
        pChildHi = pChildLo->pPrev;

        // Parse the lower-weight branch
        DumpHuffmannTree(pChildHi);
        DumpHuffmannTree(pChildLo);
    }
}
#endif

//-----------------------------------------------------------------------------
// TInputStream functions

TInputStream::TInputStream(void * pvInBuffer, size_t cbInBuffer)
{
    pbInBufferEnd = (unsigned char *)pvInBuffer + cbInBuffer;
    pbInBuffer = (unsigned char *)pvInBuffer;
    BitBuffer = 0;
    BitCount = 0;
}

// Gets one bit from input stream
bool TInputStream::Get1Bit(unsigned int & BitValue)
{
    // Ensure that the input stream is reloaded, if there are no bits left
    if(BitCount == 0)
    {
        // Buffer overflow check
        if(pbInBuffer >= pbInBufferEnd)
            return false;

        // Refill the bit buffer
        BitBuffer = *pbInBuffer++;
        BitCount = 8;
    }

    // Copy the bit from bit buffer to the variable
    BitValue = (BitBuffer & 0x01);
    BitBuffer >>= 1;
    BitCount--;
    return true;
}

// Gets the whole byte from the input stream.
bool TInputStream::Get8Bits(unsigned int & ByteValue)
{
    unsigned int dwReloadByte = 0;

    // If there is not enough bits to get the value,
    // we have to add 8 more bits from the input buffer
    if(BitCount < 8)
    {
        // Buffer overflow check
        if(pbInBuffer >= pbInBufferEnd)
            return false;

        dwReloadByte = *pbInBuffer++;
        BitBuffer |= dwReloadByte << BitCount;
        BitCount += 8;
    }

    // Return the lowest 8 its
    ByteValue = (BitBuffer & 0xFF);
    BitBuffer >>= 8;
    BitCount -= 8;
    return true;
}

// Gets 7 bits from the stream. DOES NOT remove the bits from input stream
bool TInputStream::Peek7Bits(unsigned int & Value)
{
    unsigned int Value8Bits = 0;

    // If there is not enough bits to get the value,
    // we have to add 8 more bits from the input buffer
    if(BitCount < 7)
    {
        // Load additional 8 bits. Be careful if we have no more data
        if(pbInBuffer >= pbInBufferEnd)
            return false;
        Value8Bits = *pbInBuffer++;

        // Add these 8 bits to the bit buffer
        BitBuffer |= Value8Bits << BitCount;
        BitCount += 8;
    }

    // Return 7 bits of data. DO NOT remove them from the input stream
    Value = (BitBuffer & 0x7F);
    return true;
}

void TInputStream::SkipBits(unsigned int dwBitsToSkip)
{
    unsigned int dwReloadByte = 0;

    // If there is not enough bits in the buffer,
    // we have to add 8 more bits from the input buffer
    if(BitCount < dwBitsToSkip)
    {
        // Buffer overflow check
        if(pbInBuffer >= pbInBufferEnd)
            return;

        dwReloadByte = *pbInBuffer++;
        BitBuffer |= dwReloadByte << BitCount;
        BitCount += 8;
    }

    // Skip the remaining bits
    BitBuffer >>= dwBitsToSkip;
    BitCount -= dwBitsToSkip;
}

//-----------------------------------------------------------------------------
// TOutputStream functions

TOutputStream::TOutputStream(void * pvOutBuffer, size_t cbOutLength)
{
    pbOutBufferEnd = (unsigned char *)pvOutBuffer + cbOutLength;
    pbOutBuffer = (unsigned char *)pvOutBuffer;
    BitBuffer = 0;
    BitCount = 0;
}

void TOutputStream::PutBits(unsigned int dwValue, unsigned int nBitCount)
{
    BitBuffer |= (dwValue << BitCount);
    BitCount += nBitCount;

    // Flush completed bytes
    while(BitCount >= 8)
    {
        if(pbOutBuffer < pbOutBufferEnd)
            *pbOutBuffer++ = (unsigned char)BitBuffer;

        BitBuffer >>= 8;
        BitCount -= 8;
    }
}

void TOutputStream::Flush()
{
    while(BitCount != 0)
    {
        if(pbOutBuffer < pbOutBufferEnd)
            *pbOutBuffer++ = (unsigned char)BitBuffer;

        BitBuffer >>= 8;
        BitCount -= ((BitCount > 8) ? 8 : BitCount);
    }
}

//-----------------------------------------------------------------------------
// Methods of the THTreeItem struct

void THTreeItem::RemoveItem()
{
    if(pNext != NULL)
    {
        pPrev->pNext = pNext;
        pNext->pPrev = pPrev;
        pNext = pPrev = NULL;
    }
}

//-----------------------------------------------------------------------------
// THuffmannTree class functions

THuffmannTree::THuffmannTree(bool bCompression)
{
    pFirst = pLast = LIST_HEAD();
    MinValidValue = 1;
    ItemsUsed = 0;
    bIsCmp0 = 0;

    memset(ItemsByByte, 0, sizeof(ItemsByByte));

    // If we are going to decompress data, we need to invalidate all item links
    // We do so by zeroing their ValidValue, so it becomes lower MinValidValue
    if(bCompression == false)
    {
        memset(QuickLinks, 0, sizeof(QuickLinks));
    }
}

THuffmannTree::~THuffmannTree()
{
    // Our Huffmann tree does not use any memory allocations,
    // so we don't need to do eny code in the destructor
}

void THuffmannTree::LinkTwoItems(THTreeItem * pItem1, THTreeItem * pItem2)
{
    pItem2->pNext = pItem1->pNext;
    pItem2->pPrev = pItem1->pNext->pPrev;
    pItem1->pNext->pPrev = pItem2;
    pItem1->pNext = pItem2;
}

// Inserts item into the tree (?)
void THuffmannTree::InsertItem(THTreeItem * pNewItem, TInsertPoint InsertPoint, THTreeItem * pInsertPoint)
{
    // Remove the item from the tree
    pNewItem->RemoveItem();

    if(pInsertPoint == NULL)
        pInsertPoint = LIST_HEAD();

    switch(InsertPoint)
    {
        case InsertAfter:
            LinkTwoItems(pInsertPoint, pNewItem);
            return;

        case InsertBefore:
            pNewItem->pNext = pInsertPoint;             // Set next item (or pointer to pointer to first item)
            pNewItem->pPrev = pInsertPoint->pPrev;      // Set prev item (or last item in the tree)
            pInsertPoint->pPrev->pNext = pNewItem;
            pInsertPoint->pPrev = pNewItem;             // Set the next/last item
            return;
    }
}

THTreeItem * THuffmannTree::FindHigherOrEqualItem(THTreeItem * pItem, unsigned int Weight)
{
    // Parse all existing items
    if(pItem != NULL)
    {
        while(pItem != LIST_HEAD())
        {
            if(pItem->Weight >= Weight)
                return pItem;

            pItem = pItem->pPrev;
        }
    }

    // If not found, we just get the first item
    return LIST_HEAD();
}

THTreeItem * THuffmannTree::CreateNewItem(unsigned int DecompressedValue, unsigned int Weight, TInsertPoint InsertPoint)
{
    THTreeItem * pNewItem = NULL;

    // Don't let the item buffer run out of space
    if(ItemsUsed < HUFF_ITEM_COUNT)
    {
        // Allocate new item from the item pool
        pNewItem = &ItemBuffer[ItemsUsed++];

        // Insert this item to the top of the tree
        InsertItem(pNewItem, InsertPoint, NULL);

        // Fill the rest of the item
        pNewItem->DecompressedValue = DecompressedValue;
        pNewItem->Weight = Weight;
        pNewItem->pParent = NULL;
        pNewItem->pChildLo = NULL;
    }

    return pNewItem;
}

unsigned int THuffmannTree::FixupItemPosByWeight(THTreeItem * pNewItem, unsigned int MaxWeight)
{
    THTreeItem * pHigherItem;

    if(pNewItem->Weight < MaxWeight)
    {
        // Find an item that has higher weight than this one
        pHigherItem = FindHigherOrEqualItem(pLast, pNewItem->Weight);

        // Remove the item and put it to the new position
        pNewItem->RemoveItem();
        LinkTwoItems(pHigherItem, pNewItem);
    }
    else
    {
        MaxWeight = pNewItem->Weight;
    }

    // Return the (updated) maximum weight
    return MaxWeight;
}

// Builds Huffman tree. Called with the first 8 bits loaded from input stream
bool THuffmannTree::BuildTree(unsigned int CompressionType)
{
    THTreeItem * pNewItem;
    THTreeItem * pChildLo;
    THTreeItem * pChildHi;
    unsigned char * WeightTable;
    unsigned int MaxWeight;                     // [ESP+10] - The greatest character found in table

    // Clear all pointers in HTree item array
    memset(ItemsByByte, 0, sizeof(ItemsByByte));
    MaxWeight = 0;

    // Ensure that the compression type is in range
    if((CompressionType & 0x0F) > 0x08)
        return false;
    WeightTable  = WeightTables[CompressionType & 0x0F];

    // Build the linear list of entries that is sorted by byte weight
    for(unsigned int i = 0; i < 0x100; i++)
    {
        // Skip all the bytes which are zero.
        if(WeightTable[i] != 0)
        {
            // Create new tree item
            ItemsByByte[i] = pNewItem = CreateNewItem(i, WeightTable[i], InsertAfter);

            // We need to put the item to the right place in the list
            MaxWeight = FixupItemPosByWeight(pNewItem, MaxWeight);
        }
    }

    // Insert termination entries at the end of the list
    ItemsByByte[0x100] = CreateNewItem(0x100, 1, InsertBefore);
    ItemsByByte[0x101] = CreateNewItem(0x101, 1, InsertBefore);

    // Now we need to build the tree. We start at the last entry
    // and go backwards to the first one
    pChildLo = pLast;

    // Work as long as both children are valid
    // pChildHi is child with higher weight, pChildLo is the one with lower weight
    while(pChildLo != LIST_HEAD())
    {
        // Also get and verify the higher-weight child
        pChildHi = pChildLo->pPrev;
        if(pChildHi == LIST_HEAD())
            break;

        // Create new parent item for the children
        pNewItem = CreateNewItem(0, pChildHi->Weight + pChildLo->Weight, InsertAfter);
        if(pNewItem == NULL)
            return false;

        // Link both child items to their new parent
        pChildLo->pParent = pNewItem;
        pChildHi->pParent = pNewItem;
        pNewItem->pChildLo = pChildLo;

        // Fixup the item's position by its weight
        MaxWeight = FixupItemPosByWeight(pNewItem, MaxWeight);

        // Get the previous lower-weight child
        pChildLo = pChildHi->pPrev;
    }

    // Initialize the MinValidValue to 1, which invalidates all quick-link items
    MinValidValue = 1;
    return true;
}

void THuffmannTree::IncWeightsAndRebalance(THTreeItem * pItem)
{
    THTreeItem * pHigherItem;           // A previous item with greater or equal weight
    THTreeItem * pChildHi;              // The higher-weight child
    THTreeItem * pChildLo;              // The lower-weight child
    THTreeItem * pParent;

    // Climb up the tree and increment weight of each tree item
    for(; pItem != NULL; pItem = pItem->pParent)
    {
        // Increment the item's weight
        pItem->Weight++;

        // Find a previous item with equal or greater weight, which is not equal to this item
        pHigherItem = FindHigherOrEqualItem(pItem->pPrev, pItem->Weight);
        pChildHi = pHigherItem->pNext;

        // If the item is not equal to the tree item, we need to rebalance the tree
        if(pChildHi != pItem)
        {
            // Move the previous item to the RIGHT from the given item
            pChildHi->RemoveItem();
            LinkTwoItems(pItem, pChildHi);

            // Move the given item AFTER the greater-weight tree item
            pItem->RemoveItem();
            LinkTwoItems(pHigherItem, pItem);

            // We need to maintain the tree so that pChildHi->Weight is >= pChildLo->Weight.
            // Rebalance the tree accordingly.
            pChildLo = pChildHi->pParent->pChildLo;
            pParent = pItem->pParent;
            if(pParent->pChildLo == pItem)
                pParent->pChildLo = pChildHi;
            if(pChildLo == pChildHi)
                pChildHi->pParent->pChildLo = pItem;
            pParent = pItem->pParent;
            pItem->pParent = pChildHi->pParent;
            pChildHi->pParent = pParent;

            // Increment the global valid value. This invalidates all quick-link items.
            MinValidValue++;
        }
    }
}

bool THuffmannTree::InsertNewBranchAndRebalance(unsigned int Value1, unsigned int Value2)
{
    THTreeItem * pLastItem = pLast;
    THTreeItem * pChildHi;
    THTreeItem * pChildLo;

    // Create higher-weight child
    pChildHi = CreateNewItem(Value1, pLastItem->Weight, InsertBefore);
    if(pChildHi != NULL)
    {
        pChildHi->pParent = pLastItem;
        ItemsByByte[Value1] = pChildHi;

        // Create lower-weight child
        pChildLo = CreateNewItem(Value2, 0, InsertBefore);
        if(pChildLo != NULL)
        {
            pChildLo->pParent = pLastItem;
            pLastItem->pChildLo = pChildLo;
            ItemsByByte[Value2] = pChildLo;

            IncWeightsAndRebalance(pChildLo);
            return true;
        }
    }

    // No more space in the tree buffer
    return false;
}

void THuffmannTree::EncodeOneByte(TOutputStream * os, THTreeItem * pItem)
{
    THTreeItem * pParent = pItem->pParent;
    unsigned int BitBuffer = 0;
    unsigned int BitCount = 0;

    // Put 1's as long as there is parent
    while(pParent != NULL)
    {
        // Fill the bit buffer
        BitBuffer = (BitBuffer << 1) | ((pParent->pChildLo != pItem) ? 1 : 0);
        BitCount++;

        // Move to the parent
        pItem = pParent;
        pParent = pParent->pParent;
    }

    // Write the bits to the output stream
    os->PutBits(BitBuffer, BitCount);
}

unsigned int THuffmannTree::DecodeOneByte(TInputStream * is)
{
    THTreeItem * pItemLink = NULL;
    THTreeItem * pItem;
    unsigned int ItemLinkIndex;
    unsigned int BitCount = 0;
    bool bHasItemLinkIndex;

    // Try to retrieve quick link index
    bHasItemLinkIndex = is->Peek7Bits(ItemLinkIndex);

    // Is the quick-link item valid?
    if(bHasItemLinkIndex && QuickLinks[ItemLinkIndex].ValidValue > MinValidValue)
    {
        // If that item needs less than 7 bits, we can get decompressed value directly
        if(QuickLinks[ItemLinkIndex].ValidBits <= 7)
        {
            is->SkipBits(QuickLinks[ItemLinkIndex].ValidBits);
            return QuickLinks[ItemLinkIndex].DecompressedValue;
        }

        // Otherwise we cannot get decompressed value directly
        // but we can skip 7 levels of tree parsing
        pItem = QuickLinks[ItemLinkIndex].pItem;
        is->SkipBits(7);
    }
    else
    {
        // Just a sanity check
        if(pFirst == LIST_HEAD())
            return HUFF_DECOMPRESS_ERROR;

        // We don't have the quick-link item, we need to parse the tree from its root
        pItem = pFirst;
    }

    // Step down the tree until we find a terminal item
    while(pItem->pChildLo != NULL)
    {
        unsigned int BitValue = 0;

        // If the next bit in the compressed stream is set, we get the higher-weight
        // child. Otherwise, get the lower-weight child.
        if(!is->Get1Bit(BitValue))
            return HUFF_DECOMPRESS_ERROR;

        pItem = BitValue ? pItem->pChildLo->pPrev : pItem->pChildLo;
        BitCount++;

        // If the number of loaded bits reached 7,
        // remember the current item for storing into quick-link item array
        if(BitCount == 7)
            pItemLink = pItem;
    }

    // If we didn't get the item from the quick-link array,
    // set the entry in it
    if(bHasItemLinkIndex && QuickLinks[ItemLinkIndex].ValidValue < MinValidValue)
    {
        // If the current compressed byte was more than 7 bits,
        // set a quick-link item with pointer to tree item
        if(BitCount > 7)
        {
            QuickLinks[ItemLinkIndex].ValidValue = MinValidValue;
            QuickLinks[ItemLinkIndex].ValidBits = BitCount;
            QuickLinks[ItemLinkIndex].pItem = pItemLink;
        }
        else
        {
            // Limit the quick-decompress item to lower amount of bits
            // Coverity fix 84457: (x >> 32) has undefined behavior
            ItemLinkIndex = (BitCount != 0) ? ItemLinkIndex & (0xFFFFFFFF >> (32 - BitCount)) : 0;
            while(ItemLinkIndex < LINK_ITEM_COUNT)
            {
                // Fill the quick-decompress item
                QuickLinks[ItemLinkIndex].ValidValue = MinValidValue;
                QuickLinks[ItemLinkIndex].ValidBits  = BitCount;
                QuickLinks[ItemLinkIndex].DecompressedValue = pItem->DecompressedValue;

                // Increment the index
                ItemLinkIndex += (1 << BitCount);
            }
        }
    }

    // Return the decompressed value from the found item
    return pItem->DecompressedValue;
}

unsigned int THuffmannTree::Compress(TOutputStream * os, void * pvInBuffer, int cbInBuffer, int CompressionType)
{
    unsigned char * pbInBufferEnd = (unsigned char *)pvInBuffer + cbInBuffer;
    unsigned char * pbInBuffer = (unsigned char *)pvInBuffer;
    unsigned char * pbOutBuff = os->pbOutBuffer;
    unsigned char InputByte;

    if(!BuildTree(CompressionType))
        return 0;
    bIsCmp0 = (CompressionType == 0);

    // Store the compression type into output buffer
    os->PutBits(CompressionType, 8);

    // Process the entire input buffer
    while(pbInBuffer < pbInBufferEnd)
    {
        // Get the (next) byte from the input buffer
        InputByte = *pbInBuffer++;

        // Do we have an item for such input value?
        if(ItemsByByte[InputByte] == NULL)
        {
            // Encode the relationship
            EncodeOneByte(os, ItemsByByte[0x101]);

            // Store the loaded byte into output stream
            os->PutBits(InputByte, 8);

            if(!InsertNewBranchAndRebalance(pLast->DecompressedValue, InputByte))
                return 0;

            if(bIsCmp0)
            {
                IncWeightsAndRebalance(ItemsByByte[InputByte]);
                continue;
            }

            IncWeightsAndRebalance(ItemsByByte[InputByte]);
        }
        else
        {
            EncodeOneByte(os, ItemsByByte[InputByte]);
        }

        if(bIsCmp0)
        {
            IncWeightsAndRebalance(ItemsByByte[InputByte]);
        }
    }

    // Put the termination mark to the compressed stream
    EncodeOneByte(os, ItemsByByte[0x100]);

    // Flush the remaining bits
    os->Flush();
    return (unsigned int)(os->pbOutBuffer - pbOutBuff);
}

// Decompression using Huffman tree (1500E450)
unsigned int THuffmannTree::Decompress(void * pvOutBuffer, unsigned int cbOutLength, TInputStream * is)
{
    unsigned char * pbOutBufferEnd = (unsigned char *)pvOutBuffer + cbOutLength;
    unsigned char * pbOutBuffer = (unsigned char *)pvOutBuffer;
    unsigned int DecompressedValue = 0;
    unsigned int CompressionType = 0;

    // Test the output length. Must not be NULL.
    if(cbOutLength == 0)
        return 0;

    // Get the compression type from the input stream
    if(!is->Get8Bits(CompressionType))
        return 0;
    bIsCmp0 = (CompressionType == 0) ? 1 : 0;

    // Build the Huffman tree
    if(!BuildTree(CompressionType))
        return 0;

    // Process the entire input buffer until end of the stream
    while((DecompressedValue = DecodeOneByte(is)) != 0x100)
    {
        // Did an error occur?
        if(DecompressedValue == HUFF_DECOMPRESS_ERROR)
            return 0;

        // Huffman tree needs to be modified
        if(DecompressedValue == 0x101)
        {
            // The decompressed byte is stored in the next 8 bits
            if(!is->Get8Bits(DecompressedValue))
                return 0;

            if(!InsertNewBranchAndRebalance(pLast->DecompressedValue, DecompressedValue))
                return 0;

            if(bIsCmp0 == 0)
                IncWeightsAndRebalance(ItemsByByte[DecompressedValue]);
        }

        // Store the byte to the output stream
        if(pbOutBuffer >= pbOutBufferEnd)
            break;
        *pbOutBuffer++ = (unsigned char)DecompressedValue;

        if(bIsCmp0)
        {
            IncWeightsAndRebalance(ItemsByByte[DecompressedValue]);
        }
    }

    return (unsigned int)(pbOutBuffer - (unsigned char *)pvOutBuffer);
}

