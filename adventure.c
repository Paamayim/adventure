#include "adventure.h"

#define ACTION_TIME 0.5

BITMAP *buffer, *actionbar, *inventory;
BITMAP *room_art = NULL, *room_hot = NULL;

HOTSPOT *hotspots[256];
const char *object_name = NULL;
float life = 0;

STATE game_state = STATE_GAME;

int cursor = 0;

int last_mouse, last_key[KEY_MAX];
int quit = 0;
int fps = 0;
int in_console = 0;
int door_travel = 0;

volatile int ticks = 0;
sem_t semaphore_rest;

struct {
    int x, y, relevant, result;
    const char *type, *object;
    float started;
    STATE last_state;
    POINT* walkspot;
    int owns_walkspot;
} action_state;

struct {
    int active;
    const char *name;
    BITMAP *image;
} active_item;


// gets the hotspot id associated with a pixel
int hotspot(int x, int y) {
    return (getpixel(room_hot, x, y) & (255 << 16)) >> 16;
}

// is (x1, y1) < (x, y) < (x2, y2)?
int in_rect(int x, int y, int x1, int y1, int x2, int y2) {
    return x >= x1 && x < x2 && y >= y1 && y < y2;
}

// did we just click a button?
int is_click(int button) {
    return mouse_b & button && !(last_mouse & button);
}

// calculates pixel perfect detection
//TODO(sandy): i think this fails for animated sprites
int is_pixel_perfect(BITMAP *sheet, int x, int y, int width, int height, int xorigin, int yorigin, int flipped) {
    int direction = flipped ? -1 : 1;

    int relx = x - xorigin;

    if (flipped)
        xorigin += width;

    return getpixel(sheet, xorigin + relx * direction, y + yorigin) != ((255 << 16) | 255);
}

// sets the object of the action bar
void set_action(const char *type, const char *obj) {
    action_state.started = life;
    action_state.x = mouse_x;
    action_state.y = mouse_y;
    action_state.type = type;
    action_state.object = obj;
    action_state.relevant = 1;
}

// walks to a point and then fires a game event
void walk_and_fire_event(POINT *walkpoint, const char *type, const char *obj, const char *method, int flip) {
    if (walkpoint) {
        lua_getglobal(script, "walk");
        lua_getglobal(script, "player");
        lua_vector(script, walkpoint->x, walkpoint->y);
        lua_call(script, 2, 0);
    }

    lua_getglobal(script, "append_dispatch");
    lua_getglobal(script, "player");
    lua_pushstring(script, type);
    lua_pushstring(script, obj);
    lua_pushstring(script, method);
    lua_pushboolean(script, flip);
    lua_call(script, 5, 0);
}

// fires a game event
void fire_event(const char *type, const char *obj, const char *method) {
    lua_getglobal(script, "do_callback");
    lua_pushstring(script, type);
    lua_pushstring(script, obj);
    lua_pushstring(script, method);
    lua_call(script, 3, 0);
}

// performs a room exit
void do_exit(EXIT *exit) {
// TODO(sandy): do something about exits
    lua_getglobal(script, "rooms");
    lua_pushstring(script, exit->room);
    lua_gettable(script, -2);
    lua_pushstring(script, "switch");
    lua_gettable(script, -2);
    lua_call(script, 0, 0);
    lua_pop(script, 2);
}

// updates the regular game state
void update_game() {
    update_mouse();
    
    // update lua's game loop
    lua_getglobal(script, "events");
    lua_pushstring(script, "game");
    lua_gettable(script, -2);
    lua_pushstring(script, "tick");
    lua_gettable(script, -2);
    
    lua_pushstring(script, "game");
    lua_call(script, 1, 0);
    lua_pop(script, 2);
    
    return;

    object_name = "";
    cursor = 0;

    if (0) return; // disable input
    
    // did we just return from the actionbar?
    if (action_state.result) {
        const char *method;

        switch (action_state.result) {
            case 1: method = "look";  break;
            case 2: method = "talk";  break;
            case 3: method = "touch"; break;
        }

        // if we have a walkspot, use it
        if (action_state.walkspot)
            walk_and_fire_event(action_state.walkspot, action_state.type, action_state.object, method, action_state.walkspot->x > action_state.x );
        else
            fire_event(action_state.type, action_state.object, method);
        
        // if it is OUR walkspot, we can delete it
        if (action_state.owns_walkspot)
            free(action_state.walkspot);
        
        action_state.walkspot = NULL;
        action_state.result = 0;
    }

    // gets the list of rendered objects
    lua_getregister(script, "render_obj");
    int t = lua_gettop(script);

    int found = 0;

    lua_pushnil(script);
    while (lua_next(script, t) != 0) {
        // get the properties of this object
        lua_pushstring(script, "name");
        lua_gettable(script, -2);
        const char *name = lua_tostring(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "x");
        lua_gettable(script, -2);
        int x = lua_tonumber(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "y");
        lua_gettable(script, -2);
        int y = lua_tonumber(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "width");
        lua_gettable(script, -2);
        int width = lua_tonumber(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "height");
        lua_gettable(script, -2);
        int height = lua_tonumber(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "flipped");
        lua_gettable(script, -2);
        int flipped = lua_toboolean(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "xorigin");
        lua_gettable(script, -2);
        int xorigin = lua_tonumber(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "yorigin");
        lua_gettable(script, -2);
        int yorigin = lua_tonumber(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "sheet");
        lua_gettable(script, -2);
        BITMAP *sheet = lua_touserdata(script, -1);
        lua_pop(script, 1);

        // is our mouse on top of this object?
        if (in_rect(mouse_x, mouse_y, x , y , x  + width, y  + height)
                && is_pixel_perfect(sheet, mouse_x - x , mouse_y - y , width, height, xorigin, yorigin, flipped)) {
            object_name = name;
            found = 1;
            door_travel = 0;

            if (is_click(1))
                if (active_item.active) { // are we using an item?
                    fire_event("object", lua_tostring(script, -2), active_item.name);
                    active_item.active = 0;
                } else {
                    // nope, we are just using our action bar
                    lua_getglobal(script, "make_walkspot");
                    lua_pushstring(script, lua_tostring(script, -3));
                    lua_call(script, 1, 2);
                    POINT* walkspot = malloc(sizeof(POINT));
                    walkspot->x = (int)lua_tonumber(script, -2);
                    walkspot->y = (int)lua_tonumber(script, -1);
                    lua_pop(script, 2);
                    
                    set_action("object", lua_tostring(script, -2));
                    action_state.owns_walkspot = 1;
                    action_state.walkspot = walkspot;
                }
            
            // use the active cursor
            cursor = 5;
        }

        lua_pop(script, 1);
    } lua_pop(script, 1);

    // we didn't get a render object
    if (!found) {
        HOTSPOT *hs = hotspots[hotspot(mouse_x , mouse_y )];

        // but we are on a hotspot!
        if (hs) {
            cursor = hs->cursor;
            object_name = hs->display_name;

            POINT *walkspot = walkspots[hotspot(mouse_x , mouse_y )];
            
            if (is_click(1))
                if (active_item.active) { // item?
                    walk_and_fire_event(walkspot, "hotspot", hs->internal_name, active_item.name, walkspot->x > mouse_x );
                    active_item.active = 0;
                    door_travel = 0;
                } else if (hs->exit) { // no item, door
                    action_state.relevant = 0;
                    
                    // first click?
                    if (door_travel != hs->exit->door) {
                        door_travel = hs->exit->door;

                        walk_and_fire_event(walkspot, "door", hs->internal_name, "enter", walkspot->x > mouse_x );
                        
                        lua_getglobal(script, "append_switch");
                        lua_getglobal(script, "player");
                        lua_pushstring(script, hs->exit->room);
                        lua_pushnumber(script, hs->exit->door);
                        lua_call(script, 3, 0);
                    } else { // double click
                        door_travel = 0;
                        do_exit(hs->exit);
                    }
                } else { // use the action bar
                    set_action("hotspot", hs->internal_name);
                    action_state.owns_walkspot = 0;
                    action_state.walkspot = walkspot;
                    door_travel = 0;
                }
        } else if (is_click(1)) { // clicking on nothing
            door_travel = 0;
            
            if (is_walkable(mouse_x, mouse_y)) { // on walkable ground
                lua_getglobal(script, "player");
                int x, y;
                actor_position(&x, &y);

                // can we get there directly?
                if (is_pathable(x, y, mouse_x , mouse_y )) {
                    lua_getglobal(script, "player");
                    lua_pushstring(script, "goal");
                    lua_vector(script, mouse_x , mouse_y );
                    lua_settable(script, -3);
                    
                    lua_pushstring(script, "goals");
                    lua_newtable(script);
                    lua_settable(script, -3);
                    
                    lua_pop(script, 2);
                } else { // nope, do pathfinding
                    lua_getglobal(script, "player");
                    lua_pushstring(script, "walk");
                    lua_gettable(script, -2);
                    lua_vector(script, mouse_x , mouse_y );
                    lua_call(script, 1, 0);
                    lua_pop(script, 1);
                }
            }

            // you can't get an actionbar on nothing
            action_state.relevant = 0;
        }
    }

    // are we holding the mouse button?
    if (mouse_b & 1 && action_state.relevant) {
        // it's time to bring up the action bar
        if (life > action_state.started + ACTION_TIME) {
            game_state = STATE_ACTION;
            action_state.last_state = STATE_GAME;
        }
    } else if (is_click(2)) { // we want the inventory instead
        action_state.relevant = 0;
        if (active_item.active)
            active_item.active = 0;
        else
            game_state = STATE_INVENTORY;
    }
}

// see update_game(). it is very parallel
void update_inventory() {
    lua_getregister(script, "render_inv");
    int t = lua_gettop(script);

    int found = 0;
    cursor = 0;

    lua_pushnil(script);
    while (lua_next(script, t) != 0) {
        lua_pushstring(script, "name");
        lua_gettable(script, -2);
        const char *name = lua_tostring(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "x");
        lua_gettable(script, -2);
        int x = lua_tonumber(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "y");
        lua_gettable(script, -2);
        int y = lua_tonumber(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "width");
        lua_gettable(script, -2);
        int width = lua_tonumber(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "height");
        lua_gettable(script, -2);
        int height = lua_tonumber(script, -1);
        lua_pop(script, 1);

        if (in_rect(mouse_x, mouse_y, x, y, x + width, y + height)) {
            object_name = name;
            found = 1;
            cursor = 5;

            if (is_click(1))
                if (active_item.active) {
                    fire_event("combine", lua_tostring(script, -2), active_item.name);
                    active_item.active = 0;
                } else
                    set_action("item", lua_tostring(script, -2));
            else if (!(mouse_b & 1) && last_mouse & 1) {
                active_item.active = 1;

                active_item.name = lua_tostring(script, -2);

                lua_pushstring(script, "image");
                lua_gettable(script, -2);

                active_item.image = (BITMAP*)lua_touserdata(script, -1);
                action_state.relevant = 0;

                lua_pop(script, 1);
            }
        }

        lua_pop(script, 1);
    } lua_pop(script, 1);

    if (mouse_b & 1 && action_state.relevant) { // time to bring up the action menu
        if (life > action_state.started + ACTION_TIME) {
            game_state = STATE_ACTION | STATE_INVENTORY;
            action_state.last_state = STATE_GAME;
        }
    } else if (is_click(2)) {
        action_state.relevant = 0;
        if (active_item.active)
            active_item.active = 0;
        else
            game_state = STATE_GAME;
    } else if (is_click(1) && !in_rect(mouse_x, mouse_y, 270, 210, 270 + 741, 510)) {
        game_state = STATE_GAME;
    }
}

// update the actionbar gamestate
void update_action() {
    action_state.result = 0;
    cursor = 0;
    
    if (in_rect(mouse_x, mouse_y, action_state.x - 72, action_state.y - 24, action_state.x - 24, action_state.y + 24)) {
        action_state.result = 2;
        cursor = 5;
    } else if (in_rect(mouse_x, mouse_y, action_state.x - 24, action_state.y - 24, action_state.x + 24, action_state.y + 24)) {
        action_state.result = 1;
        cursor = 5;
    } else if (in_rect(mouse_x, mouse_y, action_state.x + 24, action_state.y - 24, action_state.x + 72, action_state.y + 24)) {
        action_state.result = 3;
        cursor = 5;
    }
    
    if (!(mouse_b & 1))
        game_state = action_state.last_state;
}

// returns the number of dialogue options available
// if 0, we are not in a conversation
int get_dialogue_count() {
    lua_getglobal(script, "table");
    lua_pushstring(script, "getn");
    lua_gettable(script, -2);
    lua_getglobal(script, "conversation");
    lua_pushstring(script, "options");
    lua_gettable(script, -2);
    lua_remove(script, -2);
    lua_call(script, 1, 1);

    int ret = lua_tonumber(script, -1);
    lua_pop(script, 2);
    return ret;
}

// we are in the dialogue gamestate
void update_dialogue() {
    lua_getglobal(script, "tick");
    lua_pushstring(script, "dialogue");
    lua_call(script, 1, 0);

    int dialogue = get_dialogue_count();

    // we have a click
    if (is_click(1)) {
        // but where??
        for (int i = 0; i < dialogue; i++) {
            int y =  695 - 14 * dialogue + 14 * i;

            if (in_rect(mouse_x, mouse_y, 0, y, 1280, y + 14)) {
                // this is it!
                
                lua_getglobal(script, "conversation");
                lua_pushstring(script, "continue");
                lua_gettable(script, -2);
                lua_pushnumber(script, i + 1);
                lua_call(script, 1, 0);
                lua_pop(script, 1);
            }
        }
    }

    action_state.relevant = 0;
}

// generic update - dispatches on gamestate
void update() {
    life += 1 / (float)FRAMERATE;

    if (key[KEY_ESC] && !last_key[KEY_ESC]) quit = 1;
    if (key[KEY_F10]) open_console();

    int dialogue = get_dialogue_count();
    
    if (game_state & STATE_GAME && !dialogue) 
        update_game();
    else if (dialogue) {
        update_dialogue();
    } else {
        if (game_state & STATE_ACTION) 
            update_action();
        else if (game_state & STATE_INVENTORY)
            update_inventory();
    }

    last_mouse = mouse_b;

    for (int i = 0; i < KEY_MAX; i++)
        last_key[i] = key[i];
}

// draws the foreground of a hot image
void draw_foreground(int level) {
    int col;
    for (int y = 0; y < SCREEN_HEIGHT; y++)
    for (int x = 0; x < SCREEN_WIDTH; x++)
        if ((getpixel(room_hot, x, y) & 255) == level)
            putpixel(buffer, x, y, getpixel(room_art, x, y));
}

// draws the game
void frame() {
    char cbuffer[10];

    acquire_bitmap(buffer);

    lua_getglobal(script, "engine");
    lua_pushstring(script, "events");
    lua_gettable(script, -2);
    
    lua_pushstring(script, "draw");
    lua_gettable(script, -2);
    
    lua_call(script, 0, 0);
    lua_pop(script, 2);
    
    char fps_buffer[10];
    sprintf(fps_buffer, "%d", fps);
    textout_ex(buffer, font, fps_buffer, SCREEN_WIDTH - 25, 25, makecol(255, 0, 0), -1);
    
    release_bitmap(buffer);
    blit(buffer, screen, 0, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    return;
    
    //clear_to_color(buffer, 0);
    

    lua_getglobal(script, "conversation");
    lua_pushstring(script, "words");
    lua_gettable(script, -2);
    int t = lua_gettop(script);

    lua_pushnil(script);
    while (lua_next(script, t) != 0) {
        lua_pushstring(script, "message");
        lua_gettable(script, -2);
        const char *msg = lua_tostring(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "x");
        lua_gettable(script, -2);
        int x = lua_tonumber(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "y");
        lua_gettable(script, -2);
        int y = lua_tonumber(script, -1);
        lua_pop(script, 1);

        lua_pushstring(script, "color");
        lua_gettable(script, -2);
        int color = lua_tonumber(script, -1);

        textprintf_centre_ex(buffer, font, x , y , color, -1, msg);

        lua_pop(script, 2);
    } lua_pop(script, 1);

    if (object_name)
        textout_ex(buffer, font, object_name, 10, 10, 0, -1);

    if (game_state & STATE_INVENTORY) {
        masked_blit(inventory, buffer, 0, 0, 270, 210, 741, 300);

        int i = 0;
        lua_getglobal(script, "player");
        lua_pushstring(script, "inventory");
        lua_gettable(script, -2);
        int t = lua_gettop(script);

        lua_pushnil(script);
        while (lua_next(script, t) != 0) {
            if (lua_isnil(script, -1)) {
                lua_pop(script, 1);
                continue;
            }

            lua_pushstring(script, "label");
            lua_gettable(script, -2);
            const char *name = lua_tostring(script, -1);
            lua_pop(script, 1);

            lua_pushstring(script, "image");
            lua_gettable(script, -2);

            int xpos = 270 + 75 * (i % 10);
            int ypos = 215 + 75 * (i / 10);

            BITMAP *bmp = *(BITMAP**)lua_touserdata(script, -1);
            masked_blit(bmp, buffer, 0, 0, xpos, ypos, 64, 64);

            lua_getregister(script, "render_inv");
            lua_pushvalue(script, -4);
            push_rendertable(name, xpos, ypos, 64, 64);
            lua_pushstring(script, "image");
            lua_pushlightuserdata(script, bmp);
            lua_settable(script, -3);
            lua_settable(script, -3);

            lua_pop(script, 3);
            i++;
        } lua_pop(script, 1);
    }

    if (game_state & STATE_ACTION)
        masked_blit(actionbar, buffer, 0, 0, action_state.x - 72, action_state.y - 24, 144, 48);

    int dialogue = get_dialogue_count();
    if (dialogue) {
        int i = 0;
        lua_getglobal(script, "conversation");
        lua_pushstring(script, "options");
        lua_gettable(script, -2);
        int t = lua_gettop(script);

        lua_pushnil(script);
        while (lua_next(script, t) != 0) {
            int y =  695 - 14 * dialogue + 14 * i ;
            if (in_rect(mouse_x, mouse_y, 0, y, 1280, y + 14))
                textout_ex(buffer, font, lua_tostring(script, -1), 25, y, makecol(255, 0, 0), -1);
            else
                textout_ex(buffer, font, lua_tostring(script, -1), 25, y, 0, -1);
            lua_pop(script, 1);
            i++;
        } lua_pop(script, 2);
    }
    
    if (!disable_input) {
        int cx, cy;
        //masked_blit(cursors, buffer, cursor * 32, 0, mouse_x - cx, mouse_y - cy, 32, 32);
    }

    if (active_item.active)
        masked_blit(active_item.image, buffer, 0, 0, mouse_x, mouse_y, 64, 64);
}

// interrupt for the sempahore ticker
void ticker() {
    if (!in_console) {
        sem_post(&semaphore_rest);
        ticks++;
    }
}
END_OF_FUNCTION(ticker);

int main(int argc, char* argv[]) {
    sem_init(&semaphore_rest, 0, 1);

    allegro_init();
    install_keyboard();
    install_mouse();
    install_timer();

    LOCK_VARIABLE(ticks);
    LOCK_FUNCTION(ticker);

    set_color_depth(32);
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);

    // load resources
    buffer = create_bitmap(SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // set state
    action_state.relevant = 0;
    action_state.walkspot = NULL;
    active_item.active = 0;
    enabled_paths[255] = 1;

    // set script state
    init_script();
    lua_setconstant(script, "screen_width", number, SCREEN_WIDTH);
    lua_setconstant(script, "screen_height", number, SCREEN_HEIGHT);
    lua_setconstant(script, "framerate", number, 60);
    
    luaL_dofile(script, "game/boot.lua");
    
    init_console(32);
    install_int_ex(&ticker, BPS_TO_TIMER(FRAMERATE));

    int frames_done = 0;
    float old_time = 0;

    while (!quit) {
        sem_wait(&semaphore_rest);

        if (life - old_time >= 1) {
            fps = frames_done;
            frames_done = 0;
            old_time = life;
            
            lua_getglobal(script, "engine");
            lua_pushstring(script, "fps");
            lua_pushnumber(script, fps);
            lua_settable(script, -3);
            lua_pop(script, 1);
        }

        while (ticks > 0) {
            int old_ticks = ticks;
            update();
            ticks--;

            if (old_ticks <= ticks) break;
        }

        frame();
        frames_done++;
    }

    remove_int(ticker);
    sem_destroy(&semaphore_rest);

    return 0;
}
END_OF_MAIN();
