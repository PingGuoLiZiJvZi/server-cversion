#include "Channel_Map.h"
#include <stdlib.h>
#include <string.h>
struct Channel_Map *channel_map_init(int size)
{
	struct Channel_Map *map = (struct Channel_Map *)malloc(sizeof(struct Channel_Map));
	map->size = size;
	map->list = (struct Channel **)malloc(sizeof(struct Channel *) * size);
	memset(map->list, 0, size * sizeof(struct Channel *));
	return map;
}
void channel_map_clear(struct Channel_Map *map)
{
	if (map != NULL)
	{
		for (int i = 0; i < map->size; i++)
		{
			if (map->list[i] != NULL)
			{
				free(map->list[i]);
				map->list[i] = NULL;
			}
		}
		free(map->list);
		map->list = NULL;
	}
}
bool expand_channel_map(struct Channel_Map *map, int new_size, int unit_size)
{
	if (map->size > new_size)
	{
		return false;
	}
	int cur_size = map->size;
	while (cur_size <= new_size)
	{
		cur_size *= 2;
	}
	struct Channel **temp_ptr = realloc(map->list, cur_size * unit_size);
	if (temp_ptr == NULL)
	{
		return false;
	}
	map->list = temp_ptr;
	memset(&map->list[map->size], 0, (cur_size - map->size) * unit_size);
	map->size = cur_size;
	return true;
}
