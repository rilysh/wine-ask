#ifndef WINEASK_H
#define WINEASK_H

#define ARRAY_LEN(x) sizeof(x) / sizeof(x)[0]
#define BUFFS   1024
#define LBUFFS  300

struct Programs {
    char *name;
};

struct FilterBody {
    char url[LBUFFS];
    char name[LBUFFS];
};

#endif
