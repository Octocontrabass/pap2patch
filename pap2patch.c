#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const uint8_t gzip_header[0xa] = { 0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03 };
static uint8_t firmware[0xba254];
static uint8_t data[0x654ec];

int main( int argc, char * * argv )
{
    if( argc > 2 )
    {
        printf( "Usage: %s pap2-3-1-23-LS.bin\n", argv[0] );
        return 0;
    }
    char * name = (argc == 2) ? argv[1] : "pap2-3-1-23-LS.bin";
    FILE * infile = fopen( name, "rb" );
    if( !infile )
    {
        printf( "Couldn't open input file: %s\n", name );
        return 0;
    }
    if( !fread( firmware, 0xba254, 1, infile ) )
    {
        printf( "Couldn't read input file: %s\n", name );
        fclose( infile );
        return 0;
    }
    fclose( infile );
    if( memcmp( &firmware[8], "FiRmWaRe", 8 ) )
    {
        printf( "Not a Sipura-formatted firmware update: %s\n", name );
        return 0;
    }
    if( memcmp( &firmware[0], "LiNkSyS ", 8 ) )
    {
        printf( "Not a PAP2 firmware update: %s\n", name );
        return 0;
    }
    if( strcmp( &firmware[0x5c], "3.1.23(LS)" ) )
    {
        printf( "Sorry, only version 3.1.23(LS) works.\n" );
        return 0;
    }
    uint32_t hoffset = (
        ((uint32_t)firmware[0x54] << 24) |
        ((uint32_t)firmware[0x55] << 16) |
        ((uint32_t)firmware[0x56] << 8) |
        (uint32_t)firmware[0x57]
    ) * (
        ((uint32_t)firmware[0x7c] << 24) |
        ((uint32_t)firmware[0x7d] << 16) |
        ((uint32_t)firmware[0x7e] << 8) |
        (uint32_t)firmware[0x7f]
    ) + 0x80;
    uint32_t base = hoffset + 0x80 + (
        ((uint32_t)firmware[hoffset + 0x20] << 24) |
        ((uint32_t)firmware[hoffset + 0x21] << 16) |
        ((uint32_t)firmware[hoffset + 0x22] << 8) |
        (uint32_t)firmware[hoffset + 0x23]
    );
    base = base - (
        ((uint32_t)firmware[base - 4] << 24) |
        ((uint32_t)firmware[base - 3] << 16) |
        ((uint32_t)firmware[base - 2] << 8) |
        (uint32_t)firmware[base - 1]
    );
    uint32_t offset = (
        ((uint32_t)firmware[base + 4] << 24) |
        ((uint32_t)firmware[base + 5] << 16) |
        ((uint32_t)firmware[base + 6] << 8) |
        (uint32_t)firmware[base + 7]
    );
    offset = (
        ((uint32_t)firmware[base + offset + 0x14 * 3 + 4] << 24) |
        ((uint32_t)firmware[base + offset + 0x14 * 3 + 5] << 16) |
        ((uint32_t)firmware[base + offset + 0x14 * 3 + 6] << 8) |
        (uint32_t)firmware[base + offset + 0x14 * 3 + 7]
    );
    int to_gzip[2];
    int from_gzip[2];
    if( pipe( to_gzip ) || pipe( from_gzip ) )
    {
        printf( "Unable to open pipes for decompression.\n" );
        return 0;
    }
    pid_t pid = fork();
    if( pid == -1 )
    {
        printf( "Unable to fork for decompression.\n" );
        return 0;
    }
    if( pid == 0 )
    {
        close( to_gzip[1] );
        close( from_gzip[0] );
        if( dup2( to_gzip[0], STDIN_FILENO ) == -1 || dup2( from_gzip[1], STDOUT_FILENO ) == -1 )
        {
            printf( "Unable to set pipes for decompression.\n" );
            return 0;
        }
        // todo: use stderr to display error messages and handle errors here
        execlp( "gzip", "gzip", "-d", (char *)NULL );
        return 0;
    }
    close( to_gzip[0] );
    close( from_gzip[1] );
    pid = fork();
    if( pid == -1 )
    {
        printf( "Unable to fork for decompression.\n" );
        return 0;
    }
    if( pid == 0 )
    {
        close( from_gzip[0] );
        if( write( to_gzip[1], gzip_header, 0xa ) != 0xa )
        {
            printf( "Unable to send data for decompression.\n" );
            return 0;
        }
        if( write( to_gzip[1], &firmware[base + offset + 8], 0x2f015 ) != 0x2f015 )
        {
            printf( "Unable to send data for decompression.\n" );
            return 0;
        }
        close( to_gzip[1] );
        return 0;
    }
    close( to_gzip[1] );
    uint8_t * dataptr = data;
    size_t remaining = 0x654ec;
    ssize_t complete = read( from_gzip[0], dataptr, remaining );
    while( complete != remaining )
    {
        if( complete < 0 )
        {
            printf( "Unable to receive data from decompression.\n" );
            return 0;
        }
        dataptr += complete;
        remaining -= complete;
        complete = read( from_gzip[0], dataptr, remaining );
    }
    close( from_gzip[0] );
    /*z_stream stream = { &firmware[base + offset + 8], 0x2f015, 0, data, 0x654ec, 0, NULL, NULL, NULL, NULL, NULL, 0, 0, 0 };
    if( inflateInit2( &stream, -15 ) != Z_OK )
    {
        printf( "Unable to initialize decompression: %s\n", stream.msg ? stream.msg : "(no error message provided)" );
        return 0;
    }
    if( inflate( &stream, Z_FINISH ) != Z_STREAM_END )
    {
        printf( "Unable to decompress: %s\n", stream.msg ? stream.msg : "(no error message provided)" );
        return 0;
    }
    inflateEnd( &stream );*/
    data[0x12c1] = 0x01; // replace "mov r2,r19" with "mov r0,r19"
    /*
    uint8_t temp[10];
    stream = (z_stream){ data, 0x654ec, 0, temp, 10, 0, NULL, NULL, NULL, NULL, NULL, 0, 0, 0 };
    if( deflateInit2( &stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY ) != Z_OK )
    {
        printf( "Unable to initialize compression: %s\n", stream.msg ? stream.msg : "(no error message provided)" );
        return 0;
    }
    if( deflate( &stream, Z_NO_FLUSH ) != Z_OK )
    {
        printf( "Unable to compress: %s\n", stream.msg ? stream.msg : "(no error message provided)" );
        return 0;
    }
    stream.next_out = &firmware[base + offset + 8];
    stream.avail_out = 0x2f015;
    if( deflate( &stream, Z_FINISH ) != Z_STREAM_END )
    {
        printf( "Unable to compress: %s\n", stream.msg ? stream.msg : "(no error message provided)" );
        return 0;
    }
    printf( "%lx\n", stream.total_out );
    */
    //okay... guess we're just invoking gzip directly? seems like the least dumb way to get what i want
    
    
    pipe( to_gzip );
    pipe( from_gzip );
    if( !fork() )
    {
        close( to_gzip[1] );
        close( from_gzip[0] );
        //printf( "go go gadget gzip\n" );
        dup2( to_gzip[0], STDIN_FILENO );
        dup2( from_gzip[1], STDOUT_FILENO );
        execlp( "gzip", "gzip", "-c", (char *)NULL );
        return 0;
    }
    close( to_gzip[0] );
    close( from_gzip[1] );
    if( !fork() )
    {
        close( from_gzip[0] );
        //printf( "start write\n" );
        write( to_gzip[1], data, 0x654ec );
        //printf( "end write\n" );
        close( to_gzip[1] );
        return 0;
    }
    close( to_gzip[1] );
    {
        uint8_t temp[10];
        read( from_gzip[0], temp, 10 );
    }
    dataptr = &firmware[base + offset + 8];
    remaining = 0x2f015;
    complete = read( from_gzip[0], dataptr, remaining );
    while( complete < remaining )
    {
        dataptr += complete;
        remaining -= complete;
        complete = read( from_gzip[0], dataptr, remaining );
    }
    FILE * outfile = fopen( "pap2-3-1-23-LS-hacked.bin", "wb" );
    fwrite( firmware, 0xba254, 1, outfile );
    return 0;
}
