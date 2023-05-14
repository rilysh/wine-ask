#define TB_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <curl/curl.h>

#include "lib/termbox2.h"
#include "wineask.h"

struct Programs programs[10] = {0};
struct FilterBody fb = {0};

static void perr(const char *err)
{
    tb_shutdown(); /* Exit from any opened instance */
    perror(err);
    exit(EXIT_FAILURE);
}

static unsigned long write_data(
    void *ptr, unsigned long size,
    unsigned long nmemb, void *stream
)
{
    unsigned long sz;

    sz = fwrite(ptr, size, nmemb, (FILE *)stream);

    return sz;
}

static void iter_dir(void)
{
    DIR *d;
    struct dirent *dir;
    int i;

    d = opendir("./progs");
    i = 0;

    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 ||
            strcmp(dir->d_name, "..") == 0)
            continue;

        programs[i].name = dir->d_name;
        tb_printf(2, i, TB_GREEN, 0, "[] %s", programs[i].name);

        i++;
    }

    closedir(d);
}

static void wask_sfmt(char *dst, const char *src, ...)
{
    va_list vlist;

    va_start(vlist, src);
    vsprintf(dst, src, vlist);
    va_end(vlist);
}

static void filter_key(char *file)
{
    FILE *fp;
    char buf[BUFFS];
    char url[LBUFFS], name[LBUFFS];
    int i, k, j;
    long sz;

    memset(url, '\0', sizeof(url));
    /* GCC is complaning about truncating, for now this will patch it */
    wask_sfmt(url, "./progs/%s", file);

    /* url is here pointing the file */
    fp = fopen(url, "r");

    if (fp == NULL)
        perr("fopen()");

    memset(buf, '\0', sizeof(buf));
    memset(url, '\0', sizeof(url));
    memset(name, '\0', sizeof(name));

    sz = fread(buf, 1L, sizeof(buf), fp);
    fclose(fp);

    /* Ugly hacks ever.
       Need to fix this someday.
    */
    for (i = 0, k = 0; i < sz; i++) {
        if (buf[i] == 'U' && buf[i + 1] == 'R' &&
            buf[i + 2] == 'L') {

            for (j = 0; j < 4; j++)
                buf[i++]++;

            while (buf[i] != '\n')
                strncat(url, &buf[i++], 1);

        }
        if (buf[i] == 'N' && buf[i + 1] == 'A' &&
            buf[i + 2] == 'M' && buf[i + 3] == 'E') {

            for (j = 0; j < 5; j++)
                buf[i++]++;

            while (buf[i] != '\n') {
                name[k++] = buf[i];
                i++;
            }

            name[k] = '\0';
        }
    }

    memset(fb.name, '\0', sizeof(fb.name));
    memset(fb.url, '\0', sizeof(fb.url));
    strcat(fb.name, name);
    strcat(fb.url, url);
}

static void wask_download(char *file, char *url)
{
    CURL *curl;
    CURLcode ret;
    FILE *fp;

    curl = curl_easy_init();

    if (curl == NULL)
        perr("curl_easy_init()");

    fp = fopen(file, "wb");

    if (fp == NULL)
        perr("fopen()");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_FTP_SKIP_PASV_IP, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    ret = curl_easy_perform(curl);

    if (ret != CURLE_OK)
        perr("curl_easy_perform()");

    fclose(fp);
    curl_easy_cleanup(curl);
}

int main()
{
    struct tb_event ev;
    unsigned long i, k;
    char buf[BUFFS];

    i = k = 0;

    tb_init();
    iter_dir();
    tb_printf(
        2, k, TB_RED, 0,
        "[x] %s", programs[0].name
    );
    tb_present();

    memset(buf, '\0', sizeof(buf));

    while (tb_poll_event(&ev) == TB_OK) {
        switch (ev.key) {
        case TB_KEY_ARROW_UP:
            tb_clear();
            tb_present();

            for (i = 0; i < ARRAY_LEN(programs); i++)
                tb_printf(
                    2, i, TB_GREEN, 0, "[] %s",
                    programs[i].name
                );

            if (k == 0)
                k = ARRAY_LEN(programs) - 1;
            else
                k--;

            sprintf(buf, "%s", programs[k].name);
            tb_printf(2, k, TB_RED, 0, "[x] %s", programs[k].name);
            tb_present();
            break;

        case TB_KEY_ARROW_DOWN:
            tb_clear();
            tb_present();

            for (i = 0; i < ARRAY_LEN(programs); i++)
                tb_printf(
                    2, i, TB_GREEN, 0, "[] %s",
                    programs[i].name
                );

            if (k == ARRAY_LEN(programs) - 1)
                k = 0;
            else
                k++;

            sprintf(buf, "%s", programs[k].name);
            tb_printf(2, k, TB_RED, 0, "[x] %s", programs[k].name);
            tb_present();
            break;

        case TB_KEY_ENTER:
            filter_key(buf);
            tb_clear();
            tb_present();
            tb_shutdown();
            wask_download(fb.name, fb.url);
            break;

        default:
            tb_shutdown();
        }
    }

    tb_shutdown();
}
