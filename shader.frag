#version 330 core


//Estructuras
struct DirLight{
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight{
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

//Function prototypes
vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 objColor);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objColor);

//INPUT

#define N_POINTS 5


//FUNCIONES
uniform DirLight   dirLight;
uniform PointLight pointLights[N_POINTS];
float LinearizeDepth(float depth);

out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;
in vec3 TexCoords;

in mat3 TBN;

uniform vec3 Color;

uniform sampler2D ourTexture;
uniform samplerCube skybox;
uniform sampler2D normalTexture;

uniform int useTex;
uniform int useSky;

uniform vec3 viewPos;

float near = 0.1;
float far  = 2000.0f;

//MAIN

void main()
{

	//VAR
	float depth  = LinearizeDepth( gl_FragCoord.z ) / far;

	//vec3 norm    = normalize( Normal );
	vec3 norm    = texture(normalTexture, TexCoord).rgb;
	norm = norm * 2.0 - 1.0f;
	norm = normalize( TBN * norm );

	vec3 viewDir = normalize( viewPos - FragPos );
	vec3 objColor;

		if(useTex != 0)
			objColor = texture( ourTexture, TexCoord ).xyz;
		else
			objColor = Color;

	vec3 I = normalize(FragPos - viewPos);
	vec3 R = reflect(I, norm);

	objColor += texture(skybox, R).rgb;

	//1.Directional Light
	vec3 result = calcDirLight(dirLight, norm, viewDir, objColor);

	//2.Point Lights
	for( int i = 0 ; i < N_POINTS ; i++ )
		result += calcPointLight(pointLights[i], norm, FragPos, viewDir, objColor);

	if (useSky == 0)
		FragColor = vec4(result, 1.0f);
	else
		FragColor = texture(skybox, TexCoords);

	FragColor -= vec4( vec3( depth ), 1.0f );

}




//FUNCIONES

/*Directional Light*/
vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 objColor)
{
	//LightDirection
	vec3 lightDir = normalize( -light.direction );

	//Diffuse
	float diff = max( dot( normal, lightDir ), 0.0f );

	//Specular
	vec3 halfDir = normalize( lightDir + viewDir );
	float spec = pow( max( dot( normal, halfDir ), 0.0f ), 128.0f );

	//Results
	vec3 ambient   = light.ambient  * objColor;
	vec3 diffuse   = light.diffuse  * diff * objColor;
	vec3 specular  = light.specular * spec * objColor;

	return (ambient + diffuse + specular);
}

/*Point Light*/
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objColor)
{
	//Light Direction
	vec3 lightDir = normalize( light.position - fragPos );

	//Diffuse
	float diff = max( dot( normal, lightDir ), 0.0f );

	//Specular
	vec3 halfDir = normalize( lightDir + viewDir );
	float spec   = pow( max( dot( normal, halfDir ), 0.0f ), 128.0f );

	//Atenuation
	float distance    = length( light.position - FragPos );
	float attenuation = 1.0f / ( light.constant + light.linear * distance + light.quadratic * ( distance * distance ) );

	//Results
	vec3 ambient  = light.ambient  * objColor;
	vec3 diffuse  = light.diffuse  * diff * objColor;
	vec3 specular = light.specular * spec * objColor;

	ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

	return (ambient + diffuse + specular);

}

float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0;
	return ( 2.0 * near * far ) / ( far + near - z * ( far - near ) );
}