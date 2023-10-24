#type VERTEX_SHADER
#version 330 core

layout(location = 0) in vec3 a_position;

uniform mat4 u_view_projection_matrix = mat4(1.0f);
uniform mat4 u_modelMatrix = mat4(1.0f);

void main()
{
	gl_Position = u_view_projection_matrix * u_modelMatrix * vec4(a_position, 1.0f);
}


#type FRAGMENT_SHADER
#version 330 core

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
}


