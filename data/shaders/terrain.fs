#version 330 core

uniform vec3 camera_position;
uniform sampler2D grass_texture;
uniform sampler2D stone_texture;
uniform sampler2D snow_texture;
uniform sampler2D transition_texture;
uniform sampler2D clouds_texture;
uniform float t;
uniform bool hide_textures;

in vec3 frag_pos;
in vec3 normal;
in vec2 uv;
in float scaled_max_height;

out vec4 FragColor;

void main() {
    float gamma = 2.2;
    
    vec3 fragment_to_sun = normalize(vec3(-1.0, 1.0, 0.0));
    vec3 fragment_to_camera = normalize(camera_position - frag_pos);
    vec3 halfway = normalize(fragment_to_sun + fragment_to_camera);
    vec3 sky_colour = vec3(0.8, 0.867, 0.890);
    vec3 sun_colour = vec3(1.0, 1.0, 1.0);
    vec3 grass_colour = pow(vec3(texture2D(grass_texture, uv)), vec3(gamma));
    vec3 stone_colour = pow(vec3(texture2D(stone_texture, uv)), vec3(gamma));
    vec3 snow_colour = pow(vec3(texture2D(snow_texture, uv)), vec3(gamma));
    float transition_mask_value = texture2D(transition_texture, uv).x;
    
    vec3 n = normalize(normal);
    
    float grass_end_height = 0.095;
    float stone_end_height = 0.1;
    float height_percent = frag_pos.y / scaled_max_height;
    float grass_to_stone_mix_factor = clamp(height_percent / grass_end_height, 0.0, 1.0);

    float transition_middle = 0.1;
    float transition_width = 0.08;
    float transition_mask_t = max(-abs(height_percent - transition_middle) / transition_width + 1, 0);
    grass_to_stone_mix_factor += (transition_mask_value * transition_mask_t);
    grass_to_stone_mix_factor = min(grass_to_stone_mix_factor, 1.0);
    
    vec3 grass_stone = mix(grass_colour, stone_colour, grass_to_stone_mix_factor);

    float snow_end_height = 1.0;
    float snow_area_height = snow_end_height - stone_end_height;
    float percentage_into_snow_area = clamp((height_percent - stone_end_height) / snow_area_height, 0, 1);

    vec3 grass_stone_snow = mix(grass_stone, snow_colour, percentage_into_snow_area);

    if (height_percent > 0.4) {
        grass_stone_snow = mix(grass_stone_snow, snow_colour, 1.0f);
    }

    if (height_percent > 0.1 && height_percent <= 0.4) {
        float width = 0.4 - 0.1;
        float t = -abs(height_percent - 0.4) / width + 1;
        grass_stone_snow = mix(grass_stone_snow, snow_colour, t);
    }
    
    vec2 cloud_uv = vec2((uv.x + t / 50.0) / 3.0, uv.y / 3.0);
    float cloud_value = min(1.0 * pow(texture2D(clouds_texture, cloud_uv).r, gamma), 1.0);

    vec3 diffuse;
    if (hide_textures) {
        gamma = 1.0;
        vec3 diffuse_colour = vec3(0.5, 0.5, 0.5);
        diffuse = max(dot(n, fragment_to_sun), 0) * diffuse_colour;
    } else {
        diffuse = max(dot(n, fragment_to_sun), 0.0) * cloud_value * grass_stone_snow;
    }

    vec3 ambient_colour = mix(sky_colour, grass_stone_snow, 0.9);
    vec3 ambient = 0.3 * ambient_colour;

    float specular_strength = 0.01 * cloud_value;
    // NOTE: higher exponent = sharper spec = shinier
    float shininess = 10.0;
    vec3 specular = specular_strength * sun_colour * pow(max(dot(n, halfway), 0.0), shininess);
    
    vec3 final_colour = pow(ambient + diffuse + specular, vec3(1.0 / gamma));
    
    FragColor = vec4(final_colour, 1.0);
}
