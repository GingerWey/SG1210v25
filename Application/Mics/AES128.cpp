//-----------------------------------------------------------------------------
/*
 File        : ASE128.h
 Version     : V1.10
 By          : 银网科技

 Description :实现ASE128算法
              the AES-128 as defined by the FIPS PUB 197:the official AES standard
        
 Date       : 2023.12.05
*/
//-----------------------------------------------------------------------------

#include "aes128.h"

#include <string.h>
#include <stdint.h>
//=============================================================================
// 本地宏
//-----------------------------------------------------------------------------
//#define _CIPHER_MODE_ECB
//#define _CIPHER_MODE_CBC
//#define _CIPHER_MODE_CFB
//#define _CIPHER_MODE_OFB
//#define _CIPHER_MODE_CTR

//=============================================================================
// 本地常量
//-----------------------------------------------------------------------------
// foreward sbox
const uint8_t sbox[256] = {
/* 0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F */
0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,   /* 0 */
0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,   /* 1 */
0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,   /* 2 */
0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,   /* 3 */
0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,   /* 4 */
0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,   /* 5 */
0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,   /* 6 */
0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,   /* 7 */
0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,   /* 8 */
0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,   /* 9 */
0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,   /* A */
0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,   /* B */
0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,   /* C */
0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,   /* D */
0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,   /* E */
0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 }; /* F */
//-----------------------------------------------------------------------------
// inverse sbox
const uint8_t rsbox[256] = {
/* 0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F */
  0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb    /* 0 */
, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb    /* 1 */
, 0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e    /* 2 */
, 0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25    /* 3 */
, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92    /* 4 */
, 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84    /* 5 */
, 0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06    /* 6 */
, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b    /* 7 */
, 0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73    /* 8 */
, 0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e    /* 9 */
, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b    /* A */
, 0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4    /* B */
, 0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f    /* C */
, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef    /* D */
, 0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61    /* E */
, 0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d }; /* F */

//-----------------------------------------------------------------------------
// round constant
const uint8_t rcon[10] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

//=============================================================================
// 本地方法
//-----------------------------------------------------------------------------
// multiply by 2 in the galois field
uint8_t galois_mul2(uint8_t value)
{
    signed char temp;
  
    /* cast to signed value */
    temp = (signed char) value;
  
    /* if MSB is 1, then this will signed extend and fill the temp variable with 1's */
    temp = temp >> 7;
  
    /* AND with the reduction variable */
    temp = temp & 0x1b;
  
    /* finally shift and reduce the value */
    return ((value << 1) ^ temp);
}

//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// AES encryption
// This function only implements AES-128 encryption and decryption 
// (AES-192 and AES-256 are not supported by this code)
void aes_encrypt(const uint8_t *key, const uint8_t *input, uint8_t *output)
{
    uint8_t buf1 = 0;
    uint8_t buf2 = 0;
    uint8_t buf3 = 0;
    uint8_t buf4 = 0;
    uint8_t round = 0;
    uint8_t i = 0;
    uint8_t key_buf[16] = {0};
    
    for (i = 0; i < 16; i++)
    {
        key_buf[i] = key[i];
        output[i] = input[i];
    }

    /* main loop */
    for (round = 0; round < 10; round++)
    {

        for (i = 0; i < 16; i++)
        {
            /* with shiftrow i+5 mod 16 */
            output[i] = sbox[output[i] ^ key_buf[i]];
        }
        
        /* shift rows */
        buf1       = output[1];
        output[1]  = output[5];
        output[5]  = output[9];
        output[9]  = output[13];
        output[13] = buf1;

        buf1       = output[2];
        buf2       = output[6];
        output[2]  = output[10];
        output[6]  = output[14];
        output[10] = buf1;
        output[14] = buf2;

        buf1       = output[15];
        output[15] = output[11];
        output[11] = output[7];
        output[7]  = output[3];
        output[3]  = buf1;
   
        /* mixcol - inv mix */
        if ( round < 9 )
        {
            for (i = 0; i < 4; i++)
            {
                buf4 = (i << 2);
                    
                /* in all cases */
                buf1 = output[buf4] ^ output[buf4 + 1] ^ output[buf4 + 2] ^ output[buf4 + 3];
                buf2 = output[buf4];
                buf3 = output[buf4] ^ output[buf4 + 1]; 
                buf3 = galois_mul2(buf3);
                
                output[buf4] = output[buf4] ^ buf3 ^ buf1;
                
                buf3 = output[buf4 + 1] ^ output[buf4 + 2];
                buf3 = galois_mul2(buf3);
                
                output[buf4 + 1] = output[buf4 + 1] ^ buf3 ^ buf1;
                
                buf3 = output[buf4 + 2] ^ output[buf4 + 3];
                buf3 = galois_mul2(buf3); 

                output[buf4 + 2] = output[buf4 + 2] ^ buf3 ^ buf1;
                
                buf3 = output[buf4 + 3] ^ buf2;     
                buf3 = galois_mul2(buf3);
                
                output[buf4 + 3] = output[buf4 + 3] ^ buf3 ^ buf1;
            }
        }
    
        /* key schedule */
        key_buf[0] = sbox[key_buf[13]] ^ key_buf[0] ^ rcon[round];
        key_buf[1] = sbox[key_buf[14]] ^ key_buf[1];
        key_buf[2] = sbox[key_buf[15]] ^ key_buf[2];
        key_buf[3] = sbox[key_buf[12]] ^ key_buf[3];
        for (i = 4; i < 16; i++) 
        {
            key_buf[i] = key_buf[i] ^ key_buf[i - 4];
        }
    }
            
    /* last Addroundkey */
    for (i = 0; i < 16; i++)
    {
        /* with shiftrow i+5 mod 16 */
        output[i] = output[i] ^ key_buf[i];
    }
}
//-----------------------------------------------------------------------------
// AES decryption function
// This function only implements AES-128 encryption and decryption (AES-192 and 
// AES-256 are not supported by this code) 
void aes_decrypt(const uint8_t *key, const uint8_t *input, uint8_t *output)
{
    uint8_t buf1 = 0;
    uint8_t buf2 = 0;
    uint8_t buf3 = 0;
    uint8_t buf4 = 0;
    uint8_t round = 0;
    uint8_t i = 0;
    uint8_t key_buf[16] = {0};

    for (i = 0; i < 16; i++)
    {
        key_buf[i] = key[i];
        output[i] = input[i];
    }
   
    /* In case of decryption */
    /* compute the last key of encryption before starting the decryption */
    for (round = 0 ; round < 10; round++) 
    {
        /* key_buf schedule */
        key_buf[0] = sbox[key_buf[13]] ^ key_buf[0] ^ rcon[round];
        key_buf[1] = sbox[key_buf[14]] ^ key_buf[1];
        key_buf[2] = sbox[key_buf[15]] ^ key_buf[2];
        key_buf[3] = sbox[key_buf[12]] ^ key_buf[3];
            
        for (i = 4; i < 16; i++) 
        {
            key_buf[i] = key_buf[i] ^ key_buf[i - 4];
        }
    }
    
    /* first Addroundkey */
    for (i = 0; i < 16; i++)
    {
        output[i] = output[i] ^ key_buf[i];
    }
  
    /* main loop */
    for (round = 0; round < 10; round++)
    {
        /* Inverse key_buf schedule */
        for (i = 15; i > 3; --i) 
        {
            key_buf[i] = key_buf[i] ^ key_buf[i - 4];
        }  
        key_buf[0] = sbox[key_buf[13]] ^ key_buf[0] ^ rcon[9 - round];
        key_buf[1] = sbox[key_buf[14]] ^ key_buf[1];
        key_buf[2] = sbox[key_buf[15]] ^ key_buf[2];
        key_buf[3] = sbox[key_buf[12]] ^ key_buf[3]; 

        /* mixcol - inv mix */
        if (round > 0)
        {
            for (i = 0; i < 4; i++)
            {
                buf4 = (i << 2);
                
                /* precompute for decryption */
                buf1 = galois_mul2( galois_mul2(output[buf4] ^ output[buf4 + 2]) );
                buf2 = galois_mul2( galois_mul2(output[buf4 + 1] ^ output[buf4 + 3]) );
                    
                output[buf4]     ^= buf1; 
                output[buf4 + 1] ^= buf2; 
                output[buf4 + 2] ^= buf1; 
                output[buf4 + 3] ^= buf2; 

                /* in all cases */
                buf1 = output[buf4] ^ output[buf4 + 1] ^ output[buf4 + 2] ^ output[buf4 + 3];
                buf2 = output[buf4];
                buf3 = output[buf4] ^ output[buf4 + 1]; 
                buf3 = galois_mul2(buf3);
                
                output[buf4] = output[buf4] ^ buf3 ^ buf1;
                
                buf3 = output[buf4 + 1] ^ output[buf4 + 2];
                buf3 = galois_mul2(buf3);
                
                output[buf4 + 1] = output[buf4 + 1] ^ buf3 ^ buf1;
                
                buf3 = output[buf4 + 2] ^ output[buf4 + 3];
                buf3 = galois_mul2(buf3); 

                output[buf4 + 2] = output[buf4 + 2] ^ buf3 ^ buf1;
                
                buf3 = output[buf4 + 3] ^ buf2;     
                buf3 = galois_mul2(buf3);
                
                output[buf4 + 3] = output[buf4 + 3] ^ buf3 ^ buf1;
            }
        }
    
        /* Inv shift rows */
        /* Row 1 */
        buf1     = output[13];
        output[13] = output[9];
        output[9]  = output[5];
        output[5]  = output[1];
        output[1]  = buf1;
            
        /* Row 2 */
        buf1     = output[10];
        buf2     = output[14];
        output[10] = output[2];
        output[14] = output[6];
        output[2]  = buf1;
        output[6]  = buf2;
            
        /* Row 3 */
        buf1     = output[3];
        output[3]  = output[7];
        output[7]  = output[11];
        output[11] = output[15];
        output[15] = buf1;         
           
        for (i = 0; i < 16; i++)
        {
            /* with shiftrow i+5 mod 16 */
            output[i] = rsbox[output[i]] ^ key_buf[i];
        } 
    }
}
//-----------------------------------------------------------------------------
#if defined(_CIPHER_MODE_ECB)
/**
 * AES-ECB block encryption/decryption
 *
 * @param key      AES key
 * @param mode     AES_ENCRYPT or AES_DECRYPT
 * @param input    16-byte input block
 * @param output   16-byte output block
 *
 * @return         0 if successful
 */
int aes_crypt_ecb( uint8_t *key,
                          int     mode,
                          uint32_t  length,
                          uint8_t *input,
                          uint8_t *output )
{
    if( length % 16 )
    {
        return( -1 );
    }

    if( mode == 1 ) /* Decrypt */
    {
        /* ECB mode operates in a block-by-block fashion */
        while(length >= 16)
        {
            /* Decrypt current block */
            aes_decrypt( key, input, output );

            /* Next block */
            input  += 16;
            output += 16;
            length -= 16;
        }
    }
    else if ( mode == 0 ) /* Encrypt */
    {
        /* ECB mode operates in a block-by-block fashion */
        while(length >= 16)
        {
            /* Encrypt current block */
            aes_encrypt( key, input, output );

            /* Next block */
            input  += 16;
            output += 16;
            length -= 16;
        }
    }
    return( 0 );
}

#endif /* _CIPHER_MODE_ECB */

//-----------------------------------------------------------------------------
#if defined(_CIPHER_MODE_CBC)
/**
 * AES-CBC buffer encryption/decryption
 * Length should be a multiple of the block
 * size (16 bytes)
 *
 * @param key      AES key
 * @param mode     AES_ENCRYPT or AES_DECRYPT
 * @param length   length of the input data
 * @param iv       initialization vector (updated after use)
 * @param input    buffer holding the input data
 * @param output   buffer holding the output data
 *
 * @return         0 if successful
 */
int aes_crypt_cbc( uint8_t *key,
                          int mode,
                          uint32_t length,
                          uint8_t iv[16],
                          uint8_t *input,
                          uint8_t *output )
{
    uint32_t i = 0;

    if( length % 16 )
    {
        return( -1 );
    }  

    if( mode == 1 ) /* Decrypt */
    {
        uint8_t temp[16] = {0};
        
        /* CBC mode operates in a block-by-block fashion */
        while(length >= 16)
        {
            /* Save input block */
            memcpy(temp, input, 16);

            /* Decrypt the current block */
            aes_decrypt( key, input, output);

            /* XOR output block with IV contents */
            for(i = 0; i < 16; i++)
            {
                output[i] ^= iv[i];
            }

            /* Update IV with input block contents */
            memcpy(iv, temp, 16);

            /* Next block */
            input  += 16;
            output += 16;
            length -= 16;
        }
    }
    else if ( mode == 0 ) /* Encrypt */
    {
        /* CBC mode operates in a block-by-block fashion */
        while(length >= 16)
        {
            /* XOR input block with IV contents */
            for(i = 0; i < 16; i++)
            output[i] = input[i] ^ iv[i];

            /* Encrypt the current block based upon the output */
            /* of the previous encryption */
            aes_encrypt( key, output, output);

            /* Update IV with output block contents */
            memcpy(iv, output, 16);

            /* Next block */
            input  += 16;
            output += 16;
            length -= 16;
        }
    }

    return( 0 );
}
#endif /* _CIPHER_MODE_CBC */

//-----------------------------------------------------------------------------
#if defined(_CIPHER_MODE_CFB)
/**
 * AES-CFB buffer encryption/decryption.
 *
 * @param key      AES key
 * @param mode     AES_ENCRYPT or AES_DECRYPT
 * @param length   length of the input data
 * @param s        Size of the plaintext and ciphertext segments
 * @param iv       initialization vector (updated after use)
 * @param input    buffer holding the input data
 * @param output   buffer holding the output data
 *
 * @return         0 if successful
 */
int aes_crypt_cfb( uint8_t *key,
                          int mode,
                          uint32_t length,
                          uint32_t s,
                          uint8_t iv[16],
                          uint8_t *input,
                          uint8_t *output )
{
    uint32_t i = 0;
    uint8_t temp[16] = {0};

    /* The parameter must be a multiple of 8 */
    if(s % 8)
    {
        return ( -1 );
    }

    /* Determine the size, in bytes, of the plaintext and ciphertext segments */
    s = s / 8;

    /* Check the resulting value */
    if(s < 1 || s > 16)
    {
        return ( -1 );
    }

    if( mode == 1 ) /* Decrypt */
    {
        while(length >= s)
        {
            /* Compute Te(j) = CIPH(Iv(j)) */
            aes_encrypt( key, iv, temp);

            /* Compute Iv(j+1) = LSB(Iv(j)) | In(j) */
            memcpy(iv, iv + s, 16 - s);
            memcpy(iv + 16 - s, input, s);

            /* Compute Ou(j) = In(j) XOR MSB(Te(j)) */
            for(i = 0; i < s; i++)
            {
                output[i] = input[i] ^ temp[i];
            }

            /* Next block */
            input  += s;
            output += s;
            length -= s;
        }
    }
    else if ( mode == 0 ) /* Encrypt */
    {
        /* Process each plaintext segment */
        while(length >= s)
        {
            /* Compute Te(j) = CIPH(Iv(j)) */
            aes_encrypt( key, iv, temp );

            /* Compute Ou(j) = In(j) XOR MSB(Te(j)) */
            for(i = 0; i < s; i++)
            {
                output[i] = input[i] ^ temp[i];
            }
        
            /* Compute I(j+1) = LSB(I(j)) | O(j) */
            memcpy(iv, iv + s, 16 - s);
            memcpy(iv + 16 - s, output, s);

            /* Next block */
            input  += s;
            output += s;
            length -= s;
        }
    }

    return( 0 );
}
#endif /* _CIPHER_MODE_CFB */

//-----------------------------------------------------------------------------
#if defined(_CIPHER_MODE_OFB)
/**
 * AES-OFB buffer encryption/decryption.
 *
 * @param key      AES key
 * @param mode     AES_ENCRYPT or AES_DECRYPT
 * @param length   length of the input data
 * @param s        Size of the plaintext and ciphertext segments
 * @param iv       initialization vector (updated after use)
 * @param input    buffer holding the input data
 * @param output   buffer holding the output data
 *
 * @return         0 if successful
 */
int aes_crypt_ofb( uint8_t *key,
                          int mode,
                          uint32_t length,
                          uint32_t s,
                          uint8_t iv[16],
                          uint8_t *input,
                          uint8_t *output )
{
    uint32_t i = 0;
    uint8_t temp[16] = {0};

    /* The parameter must be a multiple of 8 */
    if(s % 8)
    {
        return ( -1 );
    }

    /* Determine the size, in bytes, of the plaintext and ciphertext segments */
    s = s / 8;

    /* Check the resulting value */
    if(s < 1 || s > 16)
    {
        return ( -1 );
    }

    if( mode == 1 ) /* Decrypt */
    {
        /* Process each ciphertext segment */
        while(length >= s)
        {
            /* Compute Te(j) = CIPH(Iv(j)) */
            aes_encrypt(key, iv, temp);

            /* Compute Ou(j) = In(j) XOR MSB(Te(j)) */
            for(i = 0; i < s; i++)
            {
                output[i] = input[i] ^ temp[i];
            }

            /* Compute Iv(j+1) = LSB(Iv(j)) | Te(j) */
            memcpy(iv, iv + s, 16 - s);
            memcpy(iv + 16 - s, temp, s);

            /* Next block */
            output += s;
            input  += s;
            length -= s;
        }
    }
    else if ( mode == 0 ) /* Encrypt */
    {
        /* Process each plaintext segment */
        while(length >= s)
        {
            /* Compute Te(j) = CIPH(Iv(j)) */
            aes_encrypt( key, iv, temp);

            /* Compute Ou(j) = In(j) XOR MSB(Te(j)) */
            for(i = 0; i < s; i++)
            {
                output[i] = input[i] ^ temp[i];
            }

            /* Compute Iv(j+1) = LSB(Iv(j)) | Te(j) */
            memcpy(iv, iv + s, 16 - s);
            memcpy(iv + 16 - s, temp, s);

            /* Next block */
            input  += s;
            output += s;
            length -= s;
        }
    }
    return ( 0 );
}

#endif /* _CIPHER_MODE_OFB */

//-----------------------------------------------------------------------------
#if defined(_CIPHER_MODE_CTR)

/**
 * AES-CTR buffer encryption/decryption
 *
 * @param key           AES key
 * @param length        The length of the data
 * @param mode          AES_ENCRYPT or AES_DECRYPT
 * @param length        length of the input data
 * @param m             Size in bits of the specific part of the block to be incremented
 * @param t             Initial counter block
 * @param input         The input data stream
 * @param output        The output data stream
 *
 * @return              0 if successful
 */
int aes_crypt_ctr( uint8_t *key,
                          int mode,
                          uint32_t length,
                          uint32_t m,
                          uint8_t *t,
                          uint8_t *input,
                          uint8_t *output )
{
    uint32_t i = 0;
    uint32_t n = 0;
    uint8_t temp[16] = {0};

    /* The parameter must be a multiple of 8 */
    if(m % 8)
    {
        return ( -1 );
    }

    /* Determine the size, in bytes, of the specific part of */
    /* the block to be incremented */
    m = m / 8;

    /* Check the resulting value */
    if(m > 16)
    {
        return ( -1 );
    }

    if( mode == 1 ) /* Decrypt */
    {
        /* Process plaintext */
        while(length > 0)
        {
            /* CTR mode operates in a block-by-block fashion */
            n = (length < 16 ? length : 16);

            /* Compute Te(j) = CIPH(T(j)) */
            aes_encrypt( key, t, temp );

            /* Compute Ou(j) = In(j) XOR Te(j) */
            for(i = 0; i < n; i++)
            {
                output[i] = input[i] ^ temp[i];
            }

            /* Standard incrementing function */
            for(i = 0; i < m; i++)
            {
                /* Increment the current byte and propagate the carry if necessary */
                if( ++( t[16 - 1 - i] ) != 0 )
                {
                    break;
                }
            }

            /* Next block */
            output += n;
            input  += n;
            length -= n;
        }
    }
    else if ( mode == 0 ) /* Encrypt */
    {
        /* Process plaintext */
        while(length > 0)
        {
            /* CTR mode operates in a block-by-block fashion */
            n = (length < 16 ? length : 16);

            /* Compute Te(j) = CIPH(T(j)) */
            aes_encrypt( key, t, temp );

            /* Compute Ou(j) = In(j) XOR Te(j) */
            for(i = 0; i < n; i++)
            {
                output[i] = input[i] ^ temp[i];
            }

            /* Standard incrementing function */
            for(i = 0; i < m; i++)
            {
                /* Increment the current byte and propagate the carry if necessary */
                if( ++( t[16 - 1 - i] ) != 0 )
                {
                    break;
                }
            }

            /* Next block */
            input  += n;
            output += n;
            length -= n;
        }
    }

    /* Successful encryption */
    return ( 0 );
}

#endif /* _CIPHER_MODE_CTR */
//-----------------------------------------------------------------------------
