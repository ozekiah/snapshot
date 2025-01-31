#include <time.h>
#include <stdio.h>
#include <openssl/sha.h>

void timestamp(char *buffer)
{
        time_t timer;
        struct tm *tm_info;
        timer = time(NULL);
        tm_info = localtime(&timer);
        strftime(buffer, 26, "%Y%m%_d%H%M%S", tm_info);
}

void print_sha1(const unsigned char *hash)
{
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
                printf("%02x", hash[i]);
        }
}
