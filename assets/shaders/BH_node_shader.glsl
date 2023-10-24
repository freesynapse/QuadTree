#type VERTEX_SHADER
#version 450 core

layout(location = 0) in vec3 a_position;

uniform mat4 u_view_projection_matrix = mat4(1.0);
uniform float u_point_scale;
uniform float u_zoom_level;

//
void main()
{
	gl_Position = u_view_projection_matrix * vec4(a_position.xy, 0.0, 1.0);
	gl_PointSize = max(pow((a_position.z / u_zoom_level), 0.5),
					   u_point_scale / u_zoom_level);
}


#type FRAGMENT_SHADER
#version 450 core

layout(location = 0) out vec4 out_color;

uniform vec4 u_color = vec4(1.0);

//
void main()
{
	out_color = u_color;
}


