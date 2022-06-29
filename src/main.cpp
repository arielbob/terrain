#include <Windows.h>
#include "include/glew.h"
#define GLFW_DLL
#define GLFW_INCLUDE_NONE
#include "include/glfw3.h"
#define GLM_FORCE_RADIANS
#include "include/glm/glm.hpp"
#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtc/type_ptr.hpp"
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"
#include "shaders.cpp"
#include "main.h"
#include "terrain.cpp"

Camera camera = {};
real64 last_frame_time_seconds;
Controller_State controller_state = {};

double get_seconds() {
#if defined(_WIN32)
    LARGE_INTEGER frequency, count_value;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&count_value);
    return (real64) count_value.QuadPart / (real64) frequency.QuadPart;
#else
    // TODO: linux support..
    return 0.0;
#endif
}

uint32 gl_read_and_load_texture(char *filename) {
    int32 width, height, num_channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(filename, &width, &height, &num_channels, 0);

    assert(data);
    
    uint32 texture_id;
    glGenTextures(1, &texture_id);

    glBindTexture(GL_TEXTURE_2D, texture_id);
    if (num_channels == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else if (num_channels == 1) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    } else {
        printf("Expected either 1 or 3 colour channels in texture.\n");
        assert(false);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    
    return texture_id;
}

char *read_file(char *filename) {
    FILE *fid;
    char *buffer;

    fopen_s(&fid, filename, "rb");
    assert(fid);

    fseek(fid, 0, SEEK_END);
    int32 length = ftell(fid);
    fseek(fid, 0, SEEK_SET);

    buffer = new char[length + 1];
    fread(buffer, sizeof(char), length, fid);
    buffer[length] = 0;

    fclose(fid);
    
    return buffer;
}

enum Shader_Type {
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_FRAGMENT
};

uint32 gl_create_shader(char *shader_contents, Shader_Type shader_type) {
    GLenum type = 0;
    if (shader_type == SHADER_TYPE_VERTEX) {
        type = GL_VERTEX_SHADER;
    } else if (shader_type == SHADER_TYPE_FRAGMENT) {
        type = GL_FRAGMENT_SHADER;
    }

    uint32 shader_id = glCreateShader(type);
    glShaderSource(shader_id, 1, &shader_contents, 0);
    glCompileShader(shader_id);

    int32 success;
    char buffer[256];
    char error_buffer[256];
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    glGetShaderInfoLog(shader_id, 256, NULL, error_buffer);
    if (!success) {
        snprintf(buffer, 256, "%s\n", error_buffer);
        printf(buffer);
        assert(success);
    }
    
    return shader_id;
}

uint32 gl_link_shaders(uint32 vertex_shader_id, uint32 fragment_shader_id) {
    uint32 program_id = glCreateProgram();
    assert(vertex_shader_id);
    assert(fragment_shader_id);
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);
    
    int32 success;
    char error_buffer[256];
    glGetShaderiv(program_id, GL_LINK_STATUS, &success);
    glGetShaderInfoLog(program_id, 256, NULL, error_buffer);
    if (!success) {
        snprintf(error_buffer, 256, "%s\n", error_buffer);
        printf(error_buffer);
        assert(success);
    }

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    return program_id;
}

void gl_init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.671f, 0.725f, 0.769f, 1.0f);
    glViewport(0, 0, camera.window_width, camera.window_height);
}

void gl_init_terrain(Terrain *terrain, char *vertex_shader_name, char *fragment_shader_name) {
    uint32 vbo, ebo;
    glGenVertexArrays(1, &terrain->vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(terrain->vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (3*terrain->num_vertices + 3*terrain->num_normals + 2*terrain->num_uvs) * sizeof(real32),
                 NULL, GL_STATIC_DRAW);
    int32 offset = 0;
    uint32 vertices_data_size = terrain->num_vertices * 3 * sizeof(real32);
    glBufferSubData(GL_ARRAY_BUFFER, offset,
                    vertices_data_size, terrain->vertices);
    offset += vertices_data_size;

    uint32 normals_data_size = terrain->num_normals * 3 * sizeof(real32);
    glBufferSubData(GL_ARRAY_BUFFER, offset,
                    normals_data_size, terrain->normals);
    offset += normals_data_size;

    uint32 uvs_data_size = terrain->num_uvs * 2 * sizeof(real32);
    glBufferSubData(GL_ARRAY_BUFFER, offset,
                    uvs_data_size, terrain->uvs);
    offset += uvs_data_size;
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrain->num_indices * sizeof(uint32),
                 terrain->indices, GL_STATIC_DRAW);
    
    uint32 vertex_shader_id = buildShader(GL_VERTEX_SHADER, vertex_shader_name);
    uint32 fragment_shader_id = buildShader(GL_FRAGMENT_SHADER, fragment_shader_name);
    terrain->shader_id = buildProgram(vertex_shader_id, fragment_shader_id, 0);
    
    // vertex positions
    int32 position_attrib = glGetAttribLocation(terrain->shader_id, "vertex_position");
    glVertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_attrib);
    
    // vertex normals
    int32 normal_attrib = glGetAttribLocation(terrain->shader_id, "vertex_normal");
    glVertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_FALSE, 0,
                          (void *) (terrain->num_vertices * 3 * sizeof(real32)));
    glEnableVertexAttribArray(normal_attrib);

    // vertex UVs
    int32 uv_attrib = glGetAttribLocation(terrain->shader_id, "vertex_uv");
    glVertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, 0,
                          (void *) ((terrain->num_vertices + terrain->num_normals) * 3 * sizeof(real32)));
    glEnableVertexAttribArray(uv_attrib);

    real64 start_time = get_seconds();
    terrain->grass_texture_id = gl_read_and_load_texture("../data/textures/mountains2_grass.png");
    terrain->stone_texture_id = gl_read_and_load_texture("../data/textures/mountains2_stone.png");
    terrain->snow_texture_id = gl_read_and_load_texture("../data/textures/mountains2_snow.png");
    terrain->transition_texture_id = gl_read_and_load_texture("../data/textures/mountains2_transition_mask.png");
    terrain->clouds_texture_id = gl_read_and_load_texture("../data/textures/mountains2_clouds.png");
    printf("Read and loaded textures in %f seconds.\n", get_seconds() - start_time);
    
    int32 grass_sampler_uniform = glGetUniformLocation(terrain->shader_id, "grass_texture");
    int32 stone_sampler_uniform = glGetUniformLocation(terrain->shader_id, "stone_texture");
    int32 snow_sampler_uniform = glGetUniformLocation(terrain->shader_id, "snow_texture");
    int32 transition_sampler_uniform = glGetUniformLocation(terrain->shader_id, "transition_texture");
    int32 clouds_uniform = glGetUniformLocation(terrain->shader_id, "clouds_texture");

    glUseProgram(terrain->shader_id);
    glUniform1i(grass_sampler_uniform, 0);
    glUniform1i(stone_sampler_uniform, 1);
    glUniform1i(snow_sampler_uniform, 2);
    glUniform1i(transition_sampler_uniform, 3);
    glUniform1i(clouds_uniform, 4);



    // NOTE: wireframe opengl setup
    glGenVertexArrays(1, &terrain->low_res_wireframe_vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(terrain->low_res_wireframe_vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (3*terrain->num_low_res_vertices) * sizeof(real32),
                 NULL, GL_STATIC_DRAW);
    vertices_data_size = terrain->num_low_res_vertices * 3 * sizeof(real32);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    vertices_data_size, terrain->low_res_vertices);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrain->num_low_res_indices * sizeof(uint32),
                 terrain->low_res_indices, GL_STATIC_DRAW);
    
    vertex_shader_id = buildShader(GL_VERTEX_SHADER, "../data/shaders/wireframe.vs");
    fragment_shader_id = buildShader(GL_FRAGMENT_SHADER, "../data/shaders/wireframe.fs");
    terrain->low_res_wireframe_shader_id = buildProgram(vertex_shader_id, fragment_shader_id, 0);

    position_attrib = glGetAttribLocation(terrain->low_res_wireframe_shader_id, "vertex_position");
    glVertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_attrib);

}

void gl_draw_terrain(Render_State *render_state, Terrain terrain, float t) {
    if (!controller_state.t.is_down && controller_state.t.was_down) {
        render_state->hide_textures = !render_state->hide_textures;
    }

    if (!controller_state.z.is_down && controller_state.z.was_down) {
        render_state->show_low_res_wireframe = !render_state->show_low_res_wireframe;
    }
    
    glm::vec3 scale_vector = glm::vec3(terrain.world_x_size / (terrain.x_resolution - 1),
                                       terrain.vertical_scale_factor,
                                       terrain.world_y_size / (terrain.y_resolution - 1));
    glm::mat4 model_matrix = glm::scale(glm::mat4(1), scale_vector);
    glm::mat4 view_matrix = glm::lookAt(camera.position,
                                        camera.position + camera.forward,
                                        camera.up);
        
    real32 aspect_ratio = (real32) camera.window_width / camera.window_height;
    glm::mat4 projection_matrix = glm::perspective(glm::radians(camera.fov_y_degrees),
                                                   aspect_ratio,
                                                   camera.near_y, camera.far_y);

    glm::mat4 normal_transform_matrix = glm::transpose(glm::inverse(model_matrix));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(terrain.shader_id);

    int32 model_uniform = glGetUniformLocation(terrain.shader_id, "model");
    glUniformMatrix4fv(model_uniform, 1, 0, glm::value_ptr(model_matrix));
    
    int32 view_uniform = glGetUniformLocation(terrain.shader_id, "view");
    glUniformMatrix4fv(view_uniform, 1, 0, glm::value_ptr(view_matrix));
    
    int32 projection_uniform = glGetUniformLocation(terrain.shader_id, "projection");
    glUniformMatrix4fv(projection_uniform, 1, 0, glm::value_ptr(projection_matrix));
    
    int32 normal_transform_uniform = glGetUniformLocation(terrain.shader_id, "normal_transform");
    glUniformMatrix4fv(normal_transform_uniform, 1, 0, glm::value_ptr(normal_transform_matrix));
    
    int32 max_height_uniform = glGetUniformLocation(terrain.shader_id, "max_height");
    glUniform1f(max_height_uniform, terrain.max_height);

    int32 t_uniform = glGetUniformLocation(terrain.shader_id, "t");
    glUniform1f(t_uniform, t);

    int32 hide_textures_uniform = glGetUniformLocation(terrain.shader_id, "hide_textures");
    glUniform1i(hide_textures_uniform, render_state->hide_textures);

    int32 camera_position_uniform = glGetUniformLocation(terrain.shader_id, "camera_position");
    glUniform3f(camera_position_uniform, camera.position.x, camera.position.y, camera.position.z);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, terrain.grass_texture_id);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, terrain.stone_texture_id);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, terrain.snow_texture_id);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, terrain.transition_texture_id);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, terrain.clouds_texture_id);
        
    glBindVertexArray(terrain.vao);
    glDrawElements(GL_TRIANGLES, terrain.num_indices, GL_UNSIGNED_INT, NULL);

    if (render_state->show_low_res_wireframe) {
        scale_vector = glm::vec3(terrain.world_x_size / (terrain.max_x - 1),
                                 terrain.vertical_scale_factor,
                                 terrain.world_y_size / (terrain.max_y - 1));
        model_matrix = glm::scale(glm::mat4(1), scale_vector);
        
        glUseProgram(terrain.low_res_wireframe_shader_id);
        
        model_uniform = glGetUniformLocation(terrain.low_res_wireframe_shader_id, "model");
        glUniformMatrix4fv(model_uniform, 1, 0, glm::value_ptr(model_matrix));
    
        view_uniform = glGetUniformLocation(terrain.low_res_wireframe_shader_id, "view");
        glUniformMatrix4fv(view_uniform, 1, 0, glm::value_ptr(view_matrix));
    
        projection_uniform = glGetUniformLocation(terrain.low_res_wireframe_shader_id, "projection");
        glUniformMatrix4fv(projection_uniform, 1, 0, glm::value_ptr(projection_matrix));

        // glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glBindVertexArray(terrain.low_res_wireframe_vao);
        glDrawElements(GL_TRIANGLES, terrain.num_low_res_indices, GL_UNSIGNED_INT, NULL);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        // glEnable(GL_DEPTH_TEST);
    }
}

/*
 *  Executed each time the window is resized,
 *  usually once at the start of the program.
 */
void framebufferSizeCallback(GLFWwindow *window, int width, int height) {

    // Prevent a divide by zero, when window is too short
    // (you cant make a window of zero width).

    if (height == 0) {
        height = 1;
    }

    camera.window_width = width;
    camera.window_height = height;
    real32 aspect_ratio = (real32) width / height;

    glfwMakeContextCurrent(window);
    glViewport(0, 0, width, height);
}

void do_movement() {
    real32 dt = (real32) (glfwGetTime() - last_frame_time_seconds);

    glm::vec3 displacement = glm::vec3();
    bool32 did_move = false;

    glm::vec3 camera_right = glm::cross(camera.forward, camera.up);

    camera.aim_fov = 90.0f;
    
    if (controller_state.w.is_down) {
        displacement = displacement + camera.forward;
        did_move = true;
        if (controller_state.shift.is_down) {
            camera.aim_fov = 100.0f;
        }
    }
    if (controller_state.a.is_down) {
        displacement = displacement -= camera_right;
        did_move = true;
    }
    if (controller_state.s.is_down) {
        displacement = displacement - camera.forward;
        did_move = true;
    }
    if (controller_state.d.is_down) {
        displacement = displacement + camera_right;
        did_move = true;
    }

    real32 speed = 5.0f;

    if (controller_state.shift.is_down) {
        speed = 20.0f;
    }
    
    if (glm::length(displacement) > 0.00001f) {
        camera.position += glm::normalize(displacement) * speed * dt;
    }
}

void set_key_state(GLFWwindow *window, Key_State *key_state, int glfw_key) {
    int state = glfwGetKey(window, glfw_key);
    if (state == GLFW_PRESS) {
        key_state->was_down = key_state->is_down;
        key_state->is_down = true;
    } else if (state == GLFW_RELEASE) {
        key_state->was_down = key_state->is_down;
        key_state->is_down = false;
    }
}

void update_keys(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    
    set_key_state(window, &controller_state.w, GLFW_KEY_W);
    set_key_state(window, &controller_state.a, GLFW_KEY_A);
    set_key_state(window, &controller_state.s, GLFW_KEY_S);
    set_key_state(window, &controller_state.d, GLFW_KEY_D);
    set_key_state(window, &controller_state.t, GLFW_KEY_T);
    set_key_state(window, &controller_state.z, GLFW_KEY_Z);
    set_key_state(window, &controller_state.shift, GLFW_KEY_LEFT_SHIFT);
}

void update_camera(GLFWwindow *window) {
    real64 cursor_x_pos, cursor_y_pos;
    glfwGetCursorPos(window, &cursor_x_pos, &cursor_y_pos);
    cursor_x_pos = cursor_x_pos / camera.window_width;
    cursor_y_pos = -cursor_y_pos / camera.window_height;
    // NOTE: delta_x is positive from left edge to right, delta_y is positive from bottom to top
    real32 delta_x = (real32) cursor_x_pos - controller_state.last_cursor_x;
    real32 delta_y = (real32) cursor_y_pos - controller_state.last_cursor_y;

    controller_state.last_cursor_x = (real32) cursor_x_pos;
    controller_state.last_cursor_y = (real32) cursor_y_pos;

    camera.pitch += delta_y * 1.0f;
    camera.heading += -delta_x * 1.0f;

    real32 min_pitch_radians = glm::radians(-80.0f);
    real32 max_pitch_radians = glm::radians(80.0f);
    if (camera.pitch < min_pitch_radians) {
        camera.pitch = min_pitch_radians;
    } else if (camera.pitch > max_pitch_radians) {
        camera.pitch = max_pitch_radians;
    }

    real32 fov_speed = 112.5f;
    real32 dt = (real32) (glfwGetTime() - last_frame_time_seconds);
    if (camera.fov_y_degrees < camera.aim_fov) {
        camera.fov_y_degrees += fov_speed*dt;
        if (camera.fov_y_degrees > camera.aim_fov) {
            camera.fov_y_degrees = camera.aim_fov;
        }
    } else if (camera.fov_y_degrees > camera.aim_fov) {
        camera.fov_y_degrees -= fov_speed*dt;
        if (camera.fov_y_degrees < camera.aim_fov) {
            camera.fov_y_degrees = camera.aim_fov;
        }
    }
    
    
    glm::mat4 camera_transform = glm::mat4(1);
    camera_transform = (glm::rotate(camera_transform, camera.heading, glm::vec3(0.0f, 1.0f, 0.0f)) *
                        glm::rotate(camera_transform, camera.pitch, glm::vec3(1.0f, 0.0f, 0.0f)));
    camera.up = glm::normalize(glm::vec3(camera_transform * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)));
    camera.forward = glm::normalize(glm::vec3(camera_transform * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f)));
}

void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void reset_controller_state_was_down() {
    controller_state.w.was_down = false;
    controller_state.a.was_down = false;
    controller_state.s.was_down = false;
    controller_state.d.was_down = false;
    controller_state.t.was_down = false;
    controller_state.z.was_down = false;
    controller_state.shift.was_down = false;
}

int main(int argc, char **argv) {
    GLFWwindow *window;
    
    // start by setting error callback in case something goes wrong
    glfwSetErrorCallback(error_callback);

    // initialize glfw
    if (!glfwInit()) {
        fprintf(stderr, "can't initialize GLFW\n");
    }

    camera.window_width = 1280;
    camera.window_height = 720;
    
    // create the window used by our application
    window = glfwCreateWindow(camera.window_width, camera.window_height,
                              "Assignment #1", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // establish framebuffer size change and input callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // initialize glew
    glfwMakeContextCurrent(window);
    GLenum error = glewInit();
    if (error != GLEW_OK) {
        printf("Error starting GLEW: %s\n", glewGetErrorString(error));
        exit(0);
    }

    // NOTE: this implementation doesn't handle non-square grids or grids and resolutions that are not of size (2^n)+1,
    //       where n is a natural number.
    Terrain terrain = {};
    terrain.vertical_scale_factor = 1.0f;
    terrain.world_x_size = 100.0f;
    terrain.world_y_size = 100.0f;
    
    // NOTE: higher h = smoother terrain
    real32 h = 0.5f;
    real32 max_random_height = 1.0f;
    init_terrain(&terrain, "../data/initial_terrain1.txt", h, max_random_height);

    #if 1
    camera.position = glm::vec3(terrain.world_x_size / 2.0f,
                                terrain.vertical_scale_factor*terrain.max_height,
                                0.0f);
    #else
    camera.position = glm::vec3(terrain.world_x_size / 2.0f,
                                19.0f,
                                10.0f);
    camera.pitch = -0.17f;
    #endif
    camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.forward = glm::vec3(0.0f, 0.0f, -1.0f);
    camera.fov_y_degrees = 90.0f;
    camera.aim_fov = 90.0f;
    camera.near_y = 0.01f;
    camera.far_y = 1000.0f;

    gl_init();
    gl_init_terrain(&terrain, "../data/shaders/terrain.vs", "../data/shaders/terrain.fs");
    
    glfwSwapInterval(1);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    real64 cursor_x_pos, cursor_y_pos;
    glfwGetCursorPos(window, &cursor_x_pos, &cursor_y_pos);
    cursor_y_pos = -cursor_y_pos;
    controller_state.last_cursor_x = (real32) cursor_x_pos / camera.window_width;
    controller_state.last_cursor_y = (real32) cursor_y_pos / camera.window_height;

    Render_State render_state = {};
    
    last_frame_time_seconds = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwSwapBuffers(window);
        glfwPollEvents();
        update_keys(window);
        update_camera(window);
        do_movement();
        gl_draw_terrain(&render_state, terrain, (real32) glfwGetTime());
        reset_controller_state_was_down();
        last_frame_time_seconds = glfwGetTime();
    }

    glfwTerminate();
}
