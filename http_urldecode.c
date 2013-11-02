/*************************************************************************
    > File Name: http_urldecode.c
    > Author: onerhao
# mail: onerhao@gmail.com
    > Created Time: Tue 29 Oct 2013 09:00:40 PM CST
 ************************************************************************/

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <stdio.h>

/*
* decode encd with URL, the result saved to decd.
* return point to be decoded string.
*/

char *urldecode( char *encd, char decd[] )
{
    if( encd == NULL )
        return (char* )0;

    int i, j=0;
    char *cd = encd;
    char p[2];
    unsigned int num;

    for( i = 0; i < (int)strlen( cd ); i++ )
    {
        memset( p, '\0', 2 );
        if( cd[i] != '%' )
        {
            decd[j++] = cd[i];
            continue;
        }
        p[0] = cd[++i];
        p[1] = cd[++i];

        sscanf( p, "%x", &num );
        sprintf(p, "%c", num );
        decd[j++] = p[0];
    }
    decd[j] = '\0';

    return decd;
}
