#include "main.h"
#include "terrain.h"
#include <random>

int32 get_array_index(int32 row_index, int32 column_index, int32 max_x, int32 max_y) {
    if (row_index < 0) {
        row_index = max_y + row_index;
    } else if (row_index >= max_y) {
        row_index %= max_y;
    }
    
    if (column_index < 0) {
        column_index = max_x + column_index;
    } else if (column_index >= max_x) {
        column_index %= max_x;
    }

    int32 final_index = row_index*max_x + column_index;
    assert(final_index < max_x*max_y);
    
    return final_index;
}

bool32 is_whitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

bool32 is_end(char c) {
    return (c == '\0');
}

// NOTE: this modifies buffer
char *get_next_word(char **buffer) {
    while (!is_end(**buffer) && is_whitespace(**buffer)) {
        (*buffer)++;
    }

    char *begin = *buffer;
    while (!is_end(**buffer) && !is_whitespace(**buffer)) {
        (*buffer)++;
    }

    if (!is_end(**buffer)) {
        **buffer = '\0';
        (*buffer)++;
    }
    return begin;
}

void init_terrain(Terrain *terrain, char *initial_heights_file, real32 h, real32 max_random_height) {
    real64 terrain_start_time = get_seconds();
    std::default_random_engine generator;
    std::normal_distribution<real32> distribution(0.0, max_random_height);

    real64 start_time = get_seconds();
    // NOTE: get the data points for the low-res grid
    char *initial_heights_buffer = read_file(initial_heights_file);

    // NOTE: get low res grid exponent
    char *current_word = get_next_word(&initial_heights_buffer);
    assert(current_word);
    int32 low_res_grid_size_exponent = atoi(current_word);
    assert(low_res_grid_size_exponent >= 0);

    // NOTE: get resolution exponent (to calculate amount of grid points on one side of full grid)
    current_word = get_next_word(&initial_heights_buffer);
    assert(current_word);
    int32 resolution_exponent = atoi(current_word);
    assert(resolution_exponent >= low_res_grid_size_exponent);

    int32 grid_size = (int32) (powf(2.0f, (real32) low_res_grid_size_exponent)) + 1;
    int32 resolution = (int32) (powf(2.0f, (real32) resolution_exponent)) + 1;
    
    terrain->max_x = grid_size;
    terrain->max_y = grid_size;
    terrain->x_resolution = resolution;
    terrain->y_resolution = resolution;

    terrain->low_res_height_data = (real32 *) malloc(terrain->max_x * terrain->max_y * sizeof(real32));
    terrain->height_data = (real32 *) malloc(terrain->x_resolution * terrain->y_resolution * sizeof(real32));
        
    int32 low_res_height_data_length = terrain->max_x*terrain->max_y;
    int32 current_index = 0;
    while (*current_word && current_index < low_res_height_data_length) {
        current_word = get_next_word(&initial_heights_buffer);
        if (*current_word) {
            real32 height = (real32) atof(current_word);
            terrain->low_res_height_data[current_index] = height;
            current_index++;
        } else {
            printf("Not enough initial values. Expected %d numbers, got %d.\n", low_res_height_data_length, current_index);
            assert(false);
        }
    }
    assert(current_index == low_res_height_data_length);
    printf("Completed reading and parsing initial heights file in %f seconds.\n", get_seconds() - start_time);
    
    // NOTE: yeah, having separate dx and dy is pointless since they're always the same.
    //       i tried handling non-square grids, but ran into issues with this implementation..
    int32 dx = (terrain->x_resolution - 1) / (terrain->max_x - 1);
    int32 dy = (terrain->y_resolution - 1) / (terrain->max_y - 1);

    // NOTE: overlay the low-res data points onto the high-res grid
    start_time = get_seconds();
    for (int32 row_index = 0; row_index < terrain->y_resolution; row_index += dy) {
        for (int32 column_index = 0; column_index < terrain->x_resolution; column_index += dx) {
            int32 low_res_height_index = get_array_index(row_index / dy, column_index / dx, terrain->max_x, terrain->max_y);
            real32 low_res_height = terrain->low_res_height_data[low_res_height_index];
            int32 height_index = get_array_index(row_index, column_index, terrain->x_resolution, terrain->y_resolution);
            terrain->height_data[height_index] = low_res_height;
        }
    }
    printf("Completed setting initial points in %f seconds.\n", get_seconds() - start_time);
    
    terrain->max_height = FLT_MIN;
    if (terrain->max_x != terrain->x_resolution ||
        terrain->max_y != terrain->y_resolution) {
        // NOTE: do diamond-square algorithm
        real32 smoothing_factor = powf(2.0f, -h);
        real32 s = smoothing_factor;

        start_time = get_seconds();
        
        while (dx > 1) {
            // NOTE: square
            for (int32 row_index = dy / 2; row_index < terrain->y_resolution; row_index += dy) {
                for (int32 column_index = dx / 2; column_index < terrain->x_resolution; column_index += dx) {
                    int32 x_offset = dx / 2;
                    int32 y_offset = dy / 2;
                    real32 top_left = terrain->height_data[get_array_index(row_index - y_offset,
                                                                          column_index - x_offset,
                                                                          terrain->x_resolution, terrain->y_resolution)];
                    real32 top_right = terrain->height_data[get_array_index(row_index - y_offset,
                                                                           column_index + x_offset,
                                                                           terrain->x_resolution, terrain->y_resolution)];
                    real32 bottom_right = terrain->height_data[get_array_index(row_index + y_offset,
                                                                              column_index + x_offset,
                                                                              terrain->x_resolution, terrain->y_resolution)];
                    real32 bottom_left = terrain->height_data[get_array_index(row_index + y_offset,
                                                                             column_index - x_offset,
                                                                             terrain->x_resolution, terrain->y_resolution)];

                    real32 random_number = s*distribution(generator);
                    real32 generated_height = (top_left + top_right + bottom_right + bottom_left) / 4 + random_number;
                    int32 array_index = get_array_index(row_index, column_index, terrain->x_resolution, terrain->y_resolution);
                    terrain->height_data[array_index] = generated_height;
                    terrain->max_height = fmaxf(terrain->max_height, generated_height);
                }
            }

            // NOTE: diamond
            int32 row_increment = dy / 2;
            for (int32 row_index = 0; row_index < terrain->y_resolution; row_index += row_increment) {
                int32 start_column_index = ((row_index/row_increment + 1) % 2) * (dx / 2);
                for (int32 column_index = start_column_index; column_index < terrain->x_resolution; column_index += dx) {
                    int32 x_offset = dx / 2;
                    int32 y_offset = dy / 2;

                    real32 sum = 0;
                    int32 count = 0;
                    
                    // top
                    if (row_index > 0) {
                        sum += terrain->height_data[get_array_index(row_index - y_offset,
                                                                    column_index,
                                                                    terrain->x_resolution, terrain->y_resolution)];
                        count++;
                    }

                    // right
                    if (column_index < (terrain->x_resolution - 1)) {
                        sum += terrain->height_data[get_array_index(row_index,
                                                                    column_index + x_offset,
                                                                    terrain->x_resolution, terrain->y_resolution)];
                        count++;
                    }

                    // bottom
                    if (row_index < (terrain->y_resolution - 1)) {
                        sum += terrain->height_data[get_array_index(row_index + y_offset,
                                                                    column_index,
                                                                    terrain->x_resolution, terrain->y_resolution)];
                        count++;
                    }

                    if (column_index > 0) {
                        sum += terrain->height_data[get_array_index(row_index,
                                                                    column_index - x_offset,
                                                                    terrain->x_resolution, terrain->y_resolution)];
                        count++;
                    }

                    real32 random_number = s*distribution(generator);
                    real32 generated_height = (sum / count) + random_number;
                    int32 array_index = get_array_index(row_index, column_index, terrain->x_resolution, terrain->y_resolution);
                    terrain->height_data[array_index] = generated_height;
                    terrain->max_height = fmaxf(terrain->max_height, generated_height);
                }
            }

            dx /= 2;
            dy /= 2;
            s *= smoothing_factor;
        }

        printf("Completed diamond-square in %f seconds.\n", get_seconds() - start_time);
    }
        
    // NOTE: create vertices
    start_time = get_seconds();
    terrain->num_vertices = terrain->x_resolution * terrain->y_resolution;
    terrain->vertices = (real32 *) malloc(terrain->num_vertices * 3 * sizeof(real32));
    for (int32 row_index = 0; row_index < terrain->y_resolution; row_index++) {
        for (int32 column_index = 0; column_index < terrain->x_resolution; column_index++) {
            int32 square_index = get_array_index(row_index, column_index,
                                                 terrain->x_resolution, terrain->y_resolution);
            // NOTE: we do it in this order for the default GLM coordinate space, which is
            //       +x = right, +y = up, +z = out of the screen
            terrain->vertices[3*square_index]     = (real32) column_index;
            terrain->vertices[3*square_index + 1] = terrain->height_data[square_index];
            terrain->vertices[3*square_index + 2] = (real32) -terrain->y_resolution + row_index + 1;
        }
    }
    printf("Generated vertices in %f seconds.\n", get_seconds() - start_time);

    // NOTE: create indices
    start_time = get_seconds();
    terrain->num_indices = (terrain->x_resolution - 1)*(terrain->y_resolution - 1)*6;
    terrain->indices = (uint32 *) malloc(terrain->num_indices * sizeof(uint32));
    for (int32 row_index = 0; row_index < terrain->y_resolution - 1; row_index++) {
        for (int32 column_index = 0; column_index < terrain->x_resolution - 1; column_index++) {
            // triangle 1
            uint32 p1, p2, p3;
            p1 = get_array_index(row_index+1, column_index+1, terrain->x_resolution, terrain->y_resolution);
            p2 = get_array_index(row_index,   column_index+1, terrain->x_resolution, terrain->y_resolution);
            p3 = get_array_index(row_index,   column_index,   terrain->x_resolution, terrain->y_resolution);
                
            // triangle 2
            uint32 p4, p5, p6;
            p4 = p1;
            p5 = p3;
            p6 = get_array_index(row_index+1, column_index,   terrain->x_resolution, terrain->y_resolution);

            int32 triangle_index = 6*(row_index*(terrain->y_resolution - 1) + column_index);
            terrain->indices[triangle_index]     = p1;
            terrain->indices[triangle_index + 1] = p2;
            terrain->indices[triangle_index + 2] = p3;
            terrain->indices[triangle_index + 3] = p4;
            terrain->indices[triangle_index + 4] = p5;
            terrain->indices[triangle_index + 5] = p6;
        }
    }
    printf("Generated indices in %f seconds.\n", get_seconds() - start_time);

    // NOTE: generate face normals
    start_time = get_seconds();
    int32 num_faces_x = terrain->x_resolution - 1;
    int32 num_faces_y = terrain->y_resolution - 1;
    real32 *face_normals = (real32 *) malloc(num_faces_x * num_faces_y * 3 * sizeof(real32));
    for (int32 row_index = 0; row_index < num_faces_y; row_index++) {
        for (int32 column_index = 0; column_index < num_faces_x; column_index++) {
            // NOTE: get vertex start index for the 3 points of the triangle
            int32 p1_index = 3*get_array_index(row_index+1, column_index+1, terrain->x_resolution, terrain->y_resolution);
            int32 p2_index = 3*get_array_index(row_index,   column_index+1, terrain->x_resolution, terrain->y_resolution);                
            int32 p3_index = 3*get_array_index(row_index,   column_index,   terrain->x_resolution, terrain->y_resolution);
                
            glm::vec3 p1 = glm::vec3(terrain->vertices[p1_index], terrain->vertices[p1_index+1], terrain->vertices[p1_index+2]);
            glm::vec3 p2 = glm::vec3(terrain->vertices[p2_index], terrain->vertices[p2_index+1], terrain->vertices[p2_index+2]);
            glm::vec3 p3 = glm::vec3(terrain->vertices[p3_index], terrain->vertices[p3_index+1], terrain->vertices[p3_index+2]);

            glm::vec3 a = p2 - p1;
            glm::vec3 b = p3 - p2;
            glm::vec3 face_normal = glm::cross(a, b);
            int32 face_normal_index = 3*get_array_index(row_index, column_index, num_faces_x, num_faces_y);

            face_normals[face_normal_index]     = face_normal.x;
            face_normals[face_normal_index + 1] = face_normal.y;
            face_normals[face_normal_index + 2] = face_normal.z;
        }
    }
    printf("Generated face normals in %f seconds.\n", get_seconds() - start_time);

    // NOTE: generate vertex normals from face normals
    start_time = get_seconds();
    terrain->num_normals = terrain->num_vertices;
    terrain->normals = (real32 *) malloc(terrain->num_normals * 3 * sizeof(real32));
    for (int32 row_index = 0; row_index < terrain->y_resolution; row_index++) {
        for (int32 column_index = 0; column_index < terrain->x_resolution; column_index++) {
            int32 top_left_face_index     = 3*get_array_index(row_index - 1, column_index - 1, num_faces_x, num_faces_y);
            int32 top_right_face_index    = 3*get_array_index(row_index - 1, column_index,     num_faces_x, num_faces_y);
            int32 bottom_left_face_index  = 3*get_array_index(row_index,     column_index - 1, num_faces_x, num_faces_y);
            int32 bottom_right_face_index = 3*get_array_index(row_index,     column_index,     num_faces_x, num_faces_y);

            glm::vec3 n1 = glm::vec3(face_normals[top_left_face_index],     face_normals[top_left_face_index+1],     face_normals[top_left_face_index+2]);
            glm::vec3 n2 = glm::vec3(face_normals[top_right_face_index],    face_normals[top_right_face_index+1],    face_normals[top_right_face_index+2]);
            glm::vec3 n3 = glm::vec3(face_normals[bottom_left_face_index],  face_normals[bottom_left_face_index+1],  face_normals[bottom_left_face_index+2]);
            glm::vec3 n4 = glm::vec3(face_normals[bottom_right_face_index], face_normals[bottom_right_face_index+1], face_normals[bottom_right_face_index+2]);

            glm::vec3 vertex_normal = (n1 + n2 + n3 + n4) / 4.0f;
            int32 normal_index = 3*get_array_index(row_index, column_index, terrain->x_resolution, terrain->y_resolution);
            terrain->normals[normal_index] =   vertex_normal.x;
            terrain->normals[normal_index+1] = vertex_normal.y;
            terrain->normals[normal_index+2] = vertex_normal.z;
        }
    }
    printf("Generated vertex normals in %f seconds.\n", get_seconds() - start_time);

    // NOTE: generate UVs
    start_time = get_seconds();
    terrain->num_uvs = terrain->num_vertices;
    terrain->uvs = (real32 *) malloc(terrain->num_uvs * 2 * sizeof(real32));
    for (int32 row_index = 0; row_index < terrain->y_resolution; row_index++) {
        for (int32 column_index = 0; column_index < terrain->x_resolution; column_index++) {
            real32 u = (real32) column_index / (terrain->x_resolution - 1);
            real32 v = (real32) ((terrain->y_resolution - 1) - row_index) / (terrain->y_resolution - 1);

            int32 uv_index = 2*get_array_index(row_index, column_index, terrain->x_resolution, terrain->y_resolution);
            terrain->uvs[uv_index]     = u;
            terrain->uvs[uv_index + 1] = v;
        }
    }
    printf("Generated UVs in %f seconds.\n", get_seconds() - start_time);

    // NOTE: generate low-res vertices
    start_time = get_seconds();
    terrain->num_low_res_vertices = terrain->max_x * terrain->max_y;
    terrain->low_res_vertices = (real32 *) malloc(terrain->num_low_res_vertices * 3 * sizeof(real32));
    for (int32 row_index = 0; row_index < terrain->max_y; row_index++) {
        for (int32 column_index = 0; column_index < terrain->max_x; column_index++) {
            int32 square_index = get_array_index(row_index, column_index,
                                                 terrain->max_x, terrain->max_y);
            // NOTE: we do it in this order for the default GLM coordinate space, which is
            //       +x = right, +y = up, +z = out of the screen
            terrain->low_res_vertices[3*square_index]     = (real32) column_index;
            terrain->low_res_vertices[3*square_index + 1] = terrain->low_res_height_data[square_index];
            terrain->low_res_vertices[3*square_index + 2] = (real32) -terrain->max_y + row_index + 1;
        }
    }
    printf("Generated low-res vertices in %f seconds.\n", get_seconds() - start_time);

    // NOTE: generate low-res indices
    start_time = get_seconds();
    terrain->num_low_res_indices = (terrain->max_x - 1)*(terrain->max_y - 1)*6;
    terrain->low_res_indices = (uint32 *) malloc(terrain->num_low_res_indices * sizeof(uint32));
    for (int32 row_index = 0; row_index < terrain->max_y - 1; row_index++) {
        for (int32 column_index = 0; column_index < terrain->max_x - 1; column_index++) {
            // triangle 1
            uint32 p1, p2, p3;
            p1 = get_array_index(row_index+1, column_index+1, terrain->max_x, terrain->max_y);
            p2 = get_array_index(row_index,   column_index+1, terrain->max_x, terrain->max_y);
            p3 = get_array_index(row_index,   column_index,   terrain->max_x, terrain->max_y);
                
            // triangle 2
            uint32 p4, p5, p6;
            p4 = p1;
            p5 = p3;
            p6 = get_array_index(row_index+1, column_index,   terrain->max_x, terrain->max_y);

            int32 triangle_index = 6*(row_index*(terrain->max_y - 1) + column_index);
            terrain->low_res_indices[triangle_index]     = p1;
            terrain->low_res_indices[triangle_index + 1] = p2;
            terrain->low_res_indices[triangle_index + 2] = p3;
            terrain->low_res_indices[triangle_index + 3] = p4;
            terrain->low_res_indices[triangle_index + 4] = p5;
            terrain->low_res_indices[triangle_index + 5] = p6;
        }
    }
    printf("Generated low-res indices in %f seconds.\n", get_seconds() - start_time);
    printf("Terrain generation total time: %f seconds\n", get_seconds() - terrain_start_time);
    printf("\n");
}
