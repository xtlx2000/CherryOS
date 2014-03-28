#include "pg_data.h"

int zonelist_init()
{
}

int pg_data_init()
{
	int i, j;
	for(i = 0; i < NR_CPUS; i++){
	
		for(j = 0; j < MAX_NR_ZONES; j++){
			zone_init(&(pg[i].node_zones[j]));
		}

		for(j = 0; j < MAX_ZONELISTS; j++){
			zonelist_init(&(pg[i].node_zonelists[j]));
		}

		for(j = 0; j < MAX_NR_ZONES; j++){
			if(pg[i].node_zones[j].name){
				pg[i].nr_zones++;
			}
		}

		//node_mem_map and ..
		//TODO

	}
	
	return 0;
}

