#ifndef MAIN_H
#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

typedef int32 bool32;

struct Render_State {
    bool32 hide_textures;
    bool32 show_low_res_wireframe;
};

struct Key_State {
    bool32 is_down;
    bool32 was_down;
};

struct Controller_State {
    real32 last_cursor_x;
    real32 last_cursor_y;
    Key_State w;
    Key_State a;
    Key_State s;
    Key_State d;
    Key_State t;
    Key_State z;
    Key_State shift;
};

struct Camera {
    real32 heading;
    real32 pitch;
    real32 near_y;
    real32 far_y;
    real32 fov_y_degrees;
    real32 aim_fov;
    int32 window_width;
    int32 window_height;
    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 up;
};

char *read_file(char *filename);
double get_seconds();

#define MAIN_H
#endif
