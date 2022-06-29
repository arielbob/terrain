#ifndef TERRAIN_H

struct Terrain {
    // NOTE: z is up in terrain's coordinate space
    // NOTE: points per side for low resolution terrain, in which all the points are specified by the user
    int32 max_x;
    int32 max_y;

    // NOTE: actual points per side for final terrain
    int32 x_resolution;
    int32 y_resolution;

    real32 *low_res_height_data;
    real32 *height_data;
    real32 *vertices;
    real32 *normals;
    real32 *uvs;
    uint32 *indices;

    real32 max_height;
    
    int32 num_vertices;
    int32 num_indices;
    int32 num_normals;
    int32 num_uvs;
    
    uint32 vao;
    uint32 shader_id;
    uint32 grass_texture_id;
    uint32 stone_texture_id;
    uint32 snow_texture_id;
    uint32 transition_texture_id;
    uint32 clouds_texture_id;

    // NOTE: for displaying the low-res wireframe
    uint32 low_res_wireframe_vao;
    uint32 low_res_wireframe_shader_id;
    int32 num_low_res_vertices;
    int32 num_low_res_indices;
    real32 *low_res_vertices;
    uint32 *low_res_indices;
    
    // NOTE: for rendering
    real32 vertical_scale_factor;
    real32 world_x_size;
    real32 world_y_size;
};

#define TERRAIN_H
#endif
