#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

/*

file layout:

main header
array of section headers
sections (with headers)

main header:

00-0f: name
30-3f: md5 of all headers (calculated with this field 0)
40-4f: unknown, not used in older firmware
50-53: length of a header (always 0x80 so doesn't matter which)
54-57: length of a different header (also always 0x80)
58-5b: length of entire update binary
5c-??: firmware version number, same as primary firmware section version
7f-7f: number of blobs

section header:

04-07: magic identifier number?
08-08: index?
0c-0f: header length (always 0x80)
10-1f: md5 of this section's data
20-23: section data length
28-??: section version number
7c-7f: section header checksum

*/

int main( int argc, char * * argv )
{
    (void)argc;
    (void)argv;
    FILE * fwfile = fopen( "pap2-3-1-23-LS.bin", "rb" );
    fseek( fwfile, 0, SEEK_END );
    long size = ftell( fwfile );
    rewind( fwfile );
    uint8_t * fw = malloc( size );
    fread( fw, size, 1, fwfile );
    uint32_t o = 0x200;
    for( int i = 0; i < 3; i++ )
    {
        uint32_t ssize;
        memcpy( &ssize, &fw[o + 0x20], sizeof(uint32_t) );
        ssize = __builtin_bswap32( ssize );
        printf( "Section length: 0x%"PRIx32"\n", ssize );
        o += 0x80;
        uint32_t soffset;
        memcpy( &soffset, &fw[o + ssize - 4], sizeof(uint32_t) );
        soffset = __builtin_bswap32( soffset );
        printf( "Main header offset: 0x%"PRIx32"\n", soffset );
        soffset = o + ssize - soffset;
        uint32_t foffset, entries;
        memcpy( &foffset, &fw[soffset + 4], sizeof(uint32_t) );
        foffset = __builtin_bswap32( foffset );
        memcpy( &entries, &fw[soffset + 8], sizeof(uint32_t) );
        entries = __builtin_bswap32( entries );
        printf( "Segment table offset: 0x%"PRIx32", %"PRIu32" entries\n", foffset, entries );
        foffset = soffset + foffset;
        for( uint32_t j = 0; j < entries; j++ )
        {
            uint32_t segment[5];
            memcpy( &segment[0], &fw[foffset], sizeof(segment) );
            for( uint32_t k = 0; k < 5; k++ )
            {
                segment[k] = __builtin_bswap32( segment[k] );
            }
            foffset += 0x14;
            printf( " %08"PRIx32" %08"PRIx32" %08"PRIx32" %08"PRIx32" %08"PRIx32, segment[0], segment[1], segment[2], segment[3], segment[4] );
            switch( segment[0] )
            {
                case 0x1:
                {
                    printf( " - copy 0x%"PRIx32" bytes from 0x%"PRIx32" to 0x%"PRIx32"\n", (uint32_t)(segment[3] * 4), segment[1], segment[2] );
                    break;
                }
                case 0x2:
                {
                    printf( " - clear 0x%"PRIx32" bytes from 0x%"PRIx32"\n", (uint32_t)(segment[2] * 4), segment[1] );
                    break;
                }
                case 0x3:
                {
                    printf( " - jump to 0x%"PRIx32"\n", (uint32_t)(segment[1] * 4) );
                    break;
                }
                // case 0x4: call segment[1]
                case 0x5:
                {
                    printf( " - r24 (code segment?) 0x%"PRIx32" (0x%"PRIx32")\n", segment[1], (uint32_t)(segment[1] * 4) );
                    break;
                }
                case 0x6:
                {
                    printf( " - r25 (data segment?) 0x%"PRIx32"\n", segment[1] );
                    break;
                }
                case 0x7:
                {
                    printf( " - r23 (code segment?) 0x%"PRIx32" (0x%"PRIx32")\n", segment[1], (uint32_t)(segment[1] * 4) );
                    break;
                }
                // case 0x8: r4 (a0?) = segment[1]
                // case 0x9: r5 (a1?) = segment[1]
                // case 0xa: r22 = segment[1]
                // case 0xb: r29 (sp?) = segment[1]
                // case 0xc: r19 = segment[1]
                case 0x10:
                {
                    printf( " - decompress 0x%"PRIx32" to 0x%"PRIx32" (", segment[1], segment[2] );
                    uint32_t temp;
                    memcpy( &temp, &fw[soffset + segment[1]], sizeof(uint32_t) );
                    printf( "0x%"PRIx32", ", __builtin_bswap32( temp ) );
                    memcpy( &temp, &fw[soffset + segment[1] + 4], sizeof(uint32_t) );
                    printf( "0x%"PRIx32") - ", __builtin_bswap32( temp ) );
                    char name[22];
                    sprintf( name, "%08"PRIx32"_%08"PRIx32".bin", i, j );
                    FILE * outfile = fopen( name, "wb" );
                    uint8_t byte;
                    z_stream stream = { &fw[soffset + segment[1] + 8], size - (soffset + segment[1] + 8), 0, &byte, 1, 0, NULL, NULL, NULL, NULL, NULL, 0, 0, 0 };
                    inflateInit2( &stream, -15 );
                    int zret = inflate( &stream, Z_SYNC_FLUSH );
                    while( zret == Z_OK )
                    {
                        fputc( byte, outfile );
                        stream.next_out = &byte;
                        stream.avail_out = 1;
                        zret = inflate( &stream, Z_SYNC_FLUSH );
                    }
                    if( zret == Z_STREAM_END )
                    {
                        fputc( byte, outfile );
                    }
                    //printf( "%d\n", zret );
                    printf( "0x%lx\n", stream.total_in );
                    inflateEnd( &stream );
                    break;
                }
                default:
                {
                    printf( "\n" );
                    break;
                }
            }
        }
            
        o += ssize;
    }
    return 0;
}