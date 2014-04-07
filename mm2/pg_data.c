#include "pg_data.h"
#include "pm.h"

int zonelist_init()
{
}

int pg_data_init(long pg_data_size)
{
	int i, j;
	for(i = 0; i < NR_CPUS; i++){
	
		for(j = 0; j < MAX_NR_ZONES; j++){
			DEBUG("  cpu %d  zone %d init:\n", i, j);
			if( i == 0 && j == ZONE_NORMAL){
				zone_init(&(pg[i].node_zones[j]), 0, memory_size);
			}else{
				zone_init(&(pg[i].node_zones[j]), 0, 0);
			}
		}

		for(j = 0; j < MAX_ZONELISTS; j++){
			DEBUG("  cpu%d  zonelist %d init:\n", i, j);
			zonelist_init(&(pg[i].node_zonelists[j]));
		}

		for(j = 0; j < MAX_NR_ZONES; j++){
			if(pg[i].node_zones[j].name){
				pg[i].nr_zones++;
			}

		}
		DEBUG("  cpu%d have %d zones\n", i, pg[i].nr_zones);

		//node_mem_map and ..
		//TODO

	}
	
	return 0;
}

