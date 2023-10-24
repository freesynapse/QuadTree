#type VERTEX_SHADER
#version 330 core


layout(location = 0) in vec3 a_position;
layout(location = 4) in vec2 a_uv;
layout(location = 5) in vec4 a_color;

uniform mat4 u_view_projection_matrix = mat4(1.0f);
uniform mat4 u_modelMatrix = mat4(1.0f);

out vec2 v_uv;
out vec4 v_color;


void main()
{
	v_uv = a_uv;
	v_color = a_color;
	gl_Position = u_view_projection_matrix * u_modelMatrix * vec4(a_position, 1.0f);
}


#type FRAGMENT_SHADER
#version 330 core


layout(location = 0) out vec4 out_color;

uniform sampler2D u_texture_sampler;

in vec2 v_uv;
in vec4 v_color;


void main()
{
	out_color = texture(u_texture_sampler, v_uv) * 1 + (0 * v_color);
	//out_color = vec4(v_uv.xy, 0.0f, 1.0f);
}


