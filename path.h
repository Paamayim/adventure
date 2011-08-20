#ifndef ADVENTURE_PATH_H
#define ADVENTURE_PATH_H

#define MAX_WAYPOINTS 32

extern POINT *waypoints[MAX_WAYPOINTS];
extern unsigned int waypoint_connections[MAX_WAYPOINTS];
extern int waypoint_count;
extern int enabled_paths[256];

void connect_waypoints(int, int);
int is_pathable(int, int, int, int);
void build_waypoints();
int get_neighbors(lua_State*);
int get_waypoint(lua_State*);
int get_closest_waypoint(lua_State*);
int is_walkable(int, int);
int closest_waypoint(int, int);
void register_path();

#endif
