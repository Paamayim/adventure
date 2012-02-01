#include "adventure.h"

lua_State *script;

int script_load_room(lua_State *L) {
    int i;
    
    if (lua_gettop(L) != 2 || !lua_isuserdata(L, 1) || !lua_isuserdata(L, 2)) {
        lua_pushstring(L, "__load_room expects (bitmap, bitmap)");
        lua_error(L);
    }

    room_art = *(BITMAP**)lua_touserdata(L, 1);
    room_hot = *(BITMAP**)lua_touserdata(L, 2);

    build_waypoints();
    
    lua_setconstant(L, "room_width", number, room_art->w);
    lua_setconstant(L, "room_height", number, room_art->h);
        
    return 0;
}

int script_panic(lua_State *L) {
    lua_Debug debug;
    lua_getstack(L, 1, &debug);
    lua_getinfo(L, "nS", &debug);
    
    printf("LUA ERROR: %s\nat %s\n", lua_tostring(L, 1), debug.name);

	return 0;
}

int script_which_hotspot(lua_State *L) {
    int x, y;
    
    if (lua_gettop(L) != 1 || !lua_istable(L, 1)) {
        lua_pushstring(L, "which_hotspot expects (vec)");
        lua_error(L);
    }
    
    lua_pushstring(L, "x");
    lua_gettable(L, -2);
    x = lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    lua_pushstring(L, "y");
    lua_gettable(L, -2);
    y = lua_tonumber(L, -1);
    
    lua_pushnumber(L, (getpixel(room_hot, x, y) & (255 << 16)) >> 16);
    return 1;
}

void update_mouse() {
    lua_getglobal(script, "engine");
    lua_pushstring(script, "mouse");
    lua_gettable(script, -2);
    
    lua_pushstring(script, "pos");
    lua_gettable(script, -2);
    lua_pushstring(script, "x");
    lua_pushnumber(script, mouse_x);
    lua_settable(script, -3);
    lua_pushstring(script, "y");
    lua_pushnumber(script, mouse_y);
    lua_settable(script, -3);
    lua_pop(script, 1);
    
    lua_pushstring(script, "buttons");
    lua_gettable(script, -2);
    lua_pushstring(script, "left");
    lua_pushboolean(script, mouse_b & 1);
    lua_settable(script, -3);
    lua_pushstring(script, "right");
    lua_pushboolean(script, mouse_b & 2);
    lua_settable(script, -3);
    lua_pushstring(script, "middle");
    lua_pushboolean(script, mouse_b & 4);
    lua_settable(script, -3);
    lua_pop(script, 3);
}

int script_get_key(lua_State *L) {
    if (lua_gettop(L) != 1 || !lua_isstring(L, 1)) {
        lua_pushstring(L, "get_key expects (string)");
        lua_error(L);
    }
    
    lua_pushboolean(L, key[(int)lua_tonumber(L, 1)]);
    return 1;
}

void init_script() {
    script = lua_open();
    luaL_openlibs(script);
    lua_atpanic(script, script_panic);
    
    lua_newtable(script);
    lua_setregister(script, "render_obj");
    lua_newtable(script);
    lua_setregister(script, "render_inv");
    
    lua_register(script, "set_room_data", &script_load_room);
    lua_register(script, "which_hotspot", &script_which_hotspot);
    lua_register(script, "get_key", &script_get_key);
    
    register_path();
    register_drawing();

    if (luaL_dofile(script, "scripts/environment.lua") != 0) {
		printf("%s\n", lua_tostring(script, -1));
	}
}

void init_keys() {
    lua_getglobal(script, "engine");
    lua_pushstring(script, "keys");
    lua_gettable(script, -2);
    
    lua_pushstring(script, "_names");
    
    lua_newtable(script);
    lua_setkey(A);
    lua_setkey(B);
    lua_setkey(C);
    lua_setkey(D);
    lua_setkey(E);
    lua_setkey(F);
    lua_setkey(G);
    lua_setkey(H);
    lua_setkey(I);
    lua_setkey(J);
    lua_setkey(K);
    lua_setkey(L);
    lua_setkey(M);
    lua_setkey(N);
    lua_setkey(O);
    lua_setkey(P);
    lua_setkey(Q);
    lua_setkey(R);
    lua_setkey(S);
    lua_setkey(T);
    lua_setkey(U);
    lua_setkey(V);
    lua_setkey(W);
    lua_setkey(X);
    lua_setkey(Y);
    lua_setkey(Z);
    lua_setkey(SPACE);
    lua_setkey(LEFT);
    lua_setkey(RIGHT);
    lua_setkey(UP);
    lua_setkey(DOWN);
    
    lua_settable(script, -3);
    lua_pop(script, 1);
}

void boot_module() {
    string initcode = "module = dofile(\"module.lua\")\n"
                      "dofile(module .. \"/boot.lua\")\n"
                      "readonly.locks[\"module\"] = module";
    
    if (luaL_dostring(script, initcode.c_str()) != 0) {
		printf("%s\n", lua_tostring(script, -1));
	}
}