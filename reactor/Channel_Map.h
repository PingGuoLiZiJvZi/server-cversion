#pragma once
#include<stdbool.h>

struct Channel_Map
{
    int size;
    struct Channel **list;
};
struct Channel_Map *channel_map_init(int size);
void channel_map_clear(struct Channel_Map *map);
bool expand_channel_map(struct Channel_Map *map,int new_size,int unit_size);
