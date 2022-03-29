#define _CRT_SECURE_NO_DEPRECATE
#define STB_IMAGE_IMPLEMENTATION
#define VEL 10.0f
#define ROT 0.5f
#define ANG 0.1f
#define TIME 0.5f
#define SMOOTH 0.000000000125f
#define PLANETS 5
#define SEPARATION 100

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream> 
#include <string> 
#include <stdlib.h> 
#include <time.h>   

#include "structures.h"
#include "stb_image.h"
#include <vector>

extern GLuint setShaders(const char* mVertx, const char* nFrag);

//Global var
GLuint shaderProgram;

unsigned int transformLoc;
unsigned int proyectionLoc;
unsigned int objectLoc;
unsigned int viewLoc;
unsigned int ourTextureLoc;

unsigned int ambientLightColorLoc;

unsigned int diffuseLightPosLoc[1];
unsigned int diffuseLightColorLoc[1];

unsigned int viewPosLoc;

float preFOV;
float preVel;

bool velPress;

float vel = 10;
int w = 1000, h = 1000;
glm::vec4 actPosCam;

double oldTime;
double nowTime;
double deltaTime;

//Definicion funciones.
void openGLinit();
void objectInit(object * obj);
bool objLoader (const char * path, 
	std::vector<glm::vec3> & out_vertices,
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals);
static void GLClearError();
void generarPlanetas(planeta* planetas, int n, object* obj);
void modelDraw(object obj, glm::mat4 * model);
void planeDraw(plane plane, glm::mat4 * model);
void planetDraw(planeta* planet, unsigned int i);
void modelMovement(plane * plane, glm::mat4 * model);
void processInput(GLFWwindow* win, plane * obj);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mousePos(GLFWwindow* win, plane* obj);
glm::vec3 randColor();
glm::vec3 randPos(int range);
glm::vec3 randSize(int max, int min);
float lerp(float x, float y, float f);
bool load_cube_map_side(GLuint texture, GLenum side_target, const char* file_name);
void create_cube_map(
	const char* front,
	const char* back,
	const char* top,
	const char* bottom,
	const char* left,
	const char* right,
	GLuint* tex_cube);
void drawSkybox(GLuint vao, GLuint tex, glm::mat4* view, glm::mat4* transform, glm::mat4* projection, plane raptor);
void iniSkybox(GLuint* vao, GLuint* tex);
void computeTangentBasis(
	std::vector<glm::vec3>& vertices,
	std::vector<glm::vec2>& uvs,
	std::vector<glm::vec3>& normals,

	std::vector<glm::vec3>& tangents,
	std::vector<glm::vec3>& bitangents
);

	//Matrix Func
	void projectionMatrix(glm::mat4 * projection);
	void viewMatrix(glm::mat4 * view, plane * plane);
	void modelMatrix(glm::mat4 * model);

//Main
int main() {

	//Iniciacion GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	srand(time(NULL));

	//Create window
	GLFWwindow* window = glfwCreateWindow(1000, 1000, "PROYECTO", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create window." << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	//Load pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initializa GLAD" << std::endl;
		return -1;
	}


	//init
	openGLinit();
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glfwSetCursorPos(window, w/2, h/2);

	//Shaders
	shaderProgram = setShaders("shader.vert", "shader.frag");

	//VARIABLES LOCALES

	object raptor_model = { /*VAO INDEX*/ 0, /*BUFFER INDEX*/ 0, 0, 0, 0, 0, /*VECTORS*/ std::vector<glm::vec3>(), std::vector<glm::vec2>(), std::vector<glm::vec3>(), /*OBJ*/ "raptor.obj", GL_TRIANGLES , /*Textures*/ 0, "texture.png", std::vector<glm::vec3>(), std::vector<glm::vec3>(), 0, "NormalMap.png" };
	object mars_model   = { /*VAO INDEX*/ 0, /*BUFFER INDEX*/ 0, 0, 0, 0, 0, /*VECTORS*/ std::vector<glm::vec3>(), std::vector<glm::vec2>(), std::vector<glm::vec3>(), /*OBJ*/ "planet.obj", GL_TRIANGLES , /*Textures*/ 0, "texture.png", std::vector<glm::vec3>(), std::vector<glm::vec3>(), 0, "NormalMapPlanet.png" };

	objectInit(&raptor_model);
	objectInit(&mars_model);
	plane raptor = { &raptor_model, &glm::mat4() , &preVel, glm::mat4(), /*POS*/ 0.0f, 100.0f, -300.0f, /*SCALE*/ 1.0f,  1.0f,  1.0f };

	planeta planetas[PLANETS];
	generarPlanetas(planetas, PLANETS, &mars_model);

	actPosCam = glm::vec4(raptor.px, raptor.py, raptor.pz -10.0f, 1.0f);

	//Matrices
	glm::mat4 transform     = glm::mat4();
	glm::mat4 view          = glm::mat4();
	glm::mat4 proyection    = glm::mat4();


	//skybox
	GLuint vao;
	GLuint skyTexture;

	iniSkybox(&vao, &skyTexture);

	glUseProgram(shaderProgram);

	//Main Loop
	while (!glfwWindowShouldClose(window))
	{

		//Time Correction
		nowTime = glfwGetTime();
		deltaTime = nowTime - oldTime;
		oldTime = nowTime;

		//Movement corrections
		velPress = FALSE;
		processInput(window, &raptor);
		preVel = lerp(preVel, vel, (float)(SMOOTH * deltaTime));

		//Clear screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		

			//Matrices
			viewMatrix(&view, &raptor);
			projectionMatrix(&proyection);
			modelMatrix(&transform);
	
			//Mouse
			mousePos(window, &raptor);
	
			//Dibujo avion
			transform = glm::mat4();
			modelMovement(&raptor, &transform);
			planeDraw(raptor, &transform);
	
			//Dibujo planetas
			transform = glm::mat4();
			for (int i = 0; i < PLANETS; i++)
				planetDraw(&planetas[i], i);
	
			//Dibujo firmamento
			drawSkybox(vao, skyTexture, &view, &transform, &proyection, raptor);


		//GLFW
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//GLFW
	glfwTerminate();
	return 0;

}




//OPENGL

void openGLinit() {

	glClearDepth(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);


}

static void GLClearError() {
	GLenum i;
	while (i = glGetError() != GL_NO_ERROR) {
		std::cout << "OpenGl Error: " << i << std::endl;
	}
}



//OBJECT INITIALIZATION

void objectInit(object * obj) {

	//Load Model
	objLoader(obj->objFile, obj->vertices, obj->uvs, obj->normals);
	computeTangentBasis(obj->vertices, obj->uvs, obj->normals, obj->tangents, obj->bitangents);

	//Indices
	glGenVertexArrays(1, &obj->VAO);
	glGenBuffers(1, &obj->vertexbuffer);
	glGenBuffers(1, &obj->uvbuffer);
	glGenBuffers(1, &obj->normalbuffer);
	glGenBuffers(1, &obj->tangentbuffer);
	glGenBuffers(1, &obj->bitangentbuffer);

	//Bind VAO
	glBindVertexArray(obj->VAO);

	//Vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, obj->vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, obj->vertices.size() * sizeof(glm::vec3), &obj->vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//UVs
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, obj->uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, obj->uvs.size() * sizeof(glm::vec2), &obj->uvs[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//Normals
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, obj->normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, obj->normals.size() * sizeof(glm::vec3), &obj->normals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//Tangents
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, obj->tangentbuffer);
	glBufferData(GL_ARRAY_BUFFER, obj->tangents.size() * sizeof(glm::vec3), &obj->tangents[0], GL_STATIC_DRAW);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//Bitangents
	glEnableVertexAttribArray(4);
	glBindBuffer(GL_ARRAY_BUFFER, obj->bitangentbuffer);
	glBufferData(GL_ARRAY_BUFFER, obj->bitangents.size() * sizeof(glm::vec3), &obj->bitangents[0], GL_STATIC_DRAW);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//Unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glDeleteBuffers(1, &obj->vertexbuffer);
	glDeleteBuffers(1, &obj->uvbuffer);
	glDeleteBuffers(1, &obj->normalbuffer);
	glDeleteBuffers(1, &obj->tangentbuffer);
	glDeleteBuffers(1, &obj->bitangentbuffer);

	//Textures
	glGenTextures(1, &obj->tex);
	glBindTexture(GL_TEXTURE_2D, obj->tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char * data = stbi_load(obj->texFile, &width, &height, &nrChannels, 0);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else
		printf("Error al cargar la textura '%s'.\n", obj->texFile);

	stbi_image_free(data);

	//Normal textures
	glGenTextures(1, &obj->normalTex);
	glBindTexture(GL_TEXTURE_2D, obj->normalTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int nwidth, nheight, nnrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* ndata = stbi_load(obj->normalTexFile, &nwidth, &nheight, &nnrChannels, 0);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nwidth, nheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, ndata);
	}
	else
		printf("Error al cargar la textura '%s'.\n", obj->normalTexFile);

	stbi_image_free(ndata);

}

bool objLoader (const char * path,
	std::vector<glm::vec3>& out_vertices,
	std::vector<glm::vec2>& out_uvs,
	std::vector<glm::vec3>& out_normals) {

	//Local var
	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	//Open file
	FILE* file = fopen(path, "r");
	if (file == NULL) {
		printf("File '%s' cant be opened.\n", path);
		return -1;
	}

	//Read file
	while (1) {

		//Read line
		char line[128];
		int res = fscanf(file, "%s", line);

		//EOF	
		if (res == EOF)
			break;

		//Vertices
		if (strcmp(line, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}

		//UVs
		else if (strcmp(line, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}

		//Normals
		else if (strcmp(line, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}

		//Faces
		else if (strcmp(line, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File cant be parsed.\n");
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}
	}

	//Processing vertices
	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		out_vertices.push_back(vertex);
	}

	//Processing UVs
	for (unsigned int i = 0; i < uvIndices.size(); i++) {
		unsigned int uvIndex = uvIndices[i];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		out_uvs.push_back(uv);
	}

	//Processing Normals
	for (unsigned int i = 0; i < normalIndices.size(); i++) {
		unsigned int normalIndex = normalIndices[i];
		glm::vec3 normal = temp_normals[normalIndex - 1];
		out_normals.push_back(normal);
	}

	return 0;

}

void generarPlanetas(planeta * planetas, int n, object * obj) {


	for (int i = 0; i < n; i++)
			{

			glm::vec3 color = randColor();
			glm::vec3 pos   = randPos(300);
			glm::vec3 size = randSize(10, 30);

			planetas[i] = { obj,
			&glm::mat4(),						//Transformation
			&glm::mat4(),						//Rotation
			pos,								//Position
			size,								//Size
			color,								//Color
			glm::vec3(0.01f, 0.01f, 0.01f),		//Ambient  Light Color
			color,								//Diffuse  Light Color
			color,								//Specular Light Color
			0.03f,								//Attenuation Constant
			0.00001f,							//Attenuation Linear
			0.0001f								//Attenuation Quadratic
			};

			}
}

void computeTangentBasis(
	std::vector<glm::vec3>& vertices,
	std::vector<glm::vec2>& uvs,
	std::vector<glm::vec3>& normals,

	std::vector<glm::vec3>& tangents,
	std::vector<glm::vec3>& bitangents
)
{
	for (int i = 0; i < vertices.size(); i += 3) {

		// Shortcuts for vertices
		glm::vec3& v0 = vertices[i + 0];
		glm::vec3& v1 = vertices[i + 1];
		glm::vec3& v2 = vertices[i + 2];

		// Shortcuts for UVs
		glm::vec2& uv0 = uvs[i + 0];
		glm::vec2& uv1 = uvs[i + 1];
		glm::vec2& uv2 = uvs[i + 2];

		// Edges of the triangle : position delta
		glm::vec3 deltaPos1 = v1 - v0;
		glm::vec3 deltaPos2 = v2 - v0;

		// UV delta
		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		// Tangent and bitangent
		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

		tangents.push_back(tangent);
		tangents.push_back(tangent);
		tangents.push_back(tangent);

		bitangents.push_back(bitangent);
		bitangents.push_back(bitangent);
		bitangents.push_back(bitangent);
	}
}


//DRAW FUNCTIONS

void modelDraw(object obj, glm::mat4 * model) {

	//Dibujo
	glBindVertexArray(obj.VAO);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, obj.tex);

	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(*model));

	ourTextureLoc = glGetUniformLocation(shaderProgram, "ourTexture");
	glUniform1i(ourTextureLoc, 0);

	unsigned int intLoc = glGetUniformLocation(shaderProgram, "useTex");
	glUniform1i(intLoc, TRUE);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture( GL_TEXTURE_2D, obj.normalTex );

	unsigned int normalLoc = glGetUniformLocation(shaderProgram, "normalTexture" );
	glUniform1i(normalLoc, 2);

	glDrawArrays(obj.mode, 0, obj.vertices.size());

}

void modelColDraw(object obj, glm::mat4* model, glm::vec3 color) {

	//Dibujo
	glBindVertexArray(obj.VAO);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, obj.tex);

	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(*model));

	ourTextureLoc = glGetUniformLocation(shaderProgram, "ourTexture");
	glUniform1i(ourTextureLoc, 0);

	unsigned int colLoc = glGetUniformLocation(shaderProgram, "Color");
	glUniform3f(colLoc,color.x, color.y, color.z);

	unsigned int intLoc = glGetUniformLocation(shaderProgram, "useTex");
	glUniform1i(intLoc, FALSE);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, obj.normalTex);

	unsigned int normalLoc = glGetUniformLocation(shaderProgram, "normalTexture");
	glUniform1i(normalLoc, 2);


	glDrawArrays(obj.mode, 0, obj.vertices.size());

}

void planeDraw(plane plane, glm::mat4* model) {

	//Transformacion
	*model = glm::mat4();
	*model = glm::translate(*model, glm::vec3(plane.px, plane.py, plane.pz));

	*model = *model * plane.rot;

	*plane.transform = *model;

	*model = glm::scale(*model, glm::vec3(plane.sx, plane.sy, plane.sz));

	modelDraw(*plane.obj, model);
}

void planetDraw(planeta * planet, unsigned int i) {

	//Transformamos
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, planet->pos);

	*planet->transform = model;

	model = glm::scale(model, planet->size);

	//Luz Puntual 1
	std::string base = "pointLights["; base += std::to_string(i); base += "].";
	std::string temp;

	temp = base + "position";
	unsigned int pLight_posLoc = glGetUniformLocation(shaderProgram, temp.c_str());
	glUniform3f(pLight_posLoc, planet->pos.x, planet->pos.y, planet->pos.z);

	temp = base + "ambient";
	unsigned int pLight_AmbLoc = glGetUniformLocation(shaderProgram, temp.c_str());
	glUniform3f(pLight_AmbLoc, planet->ambCol.x, planet->ambCol.y, planet->ambCol.z);

	temp = base + "diffuse";
	unsigned int pLight_DifLoc = glGetUniformLocation(shaderProgram, temp.c_str());
	glUniform3f(pLight_DifLoc, planet->difCol.x, planet->difCol.y, planet->difCol.z);

	temp = base + "specular";
	unsigned int pLight_SpcLoc = glGetUniformLocation(shaderProgram, temp.c_str());
	glUniform3f(pLight_SpcLoc, planet->spcCol.x, planet->spcCol.y, planet->spcCol.z);

	temp = base + "constant";
	unsigned int pLight_aConsLoc = glGetUniformLocation(shaderProgram, temp.c_str());
	glUniform1f(pLight_aConsLoc, planet->attConst);

	temp = base + "linear";
	unsigned int pLight_aLineLoc = glGetUniformLocation(shaderProgram, temp.c_str());
	glUniform1f(pLight_aLineLoc, planet->attLinear);

	temp = base + "quadratic";
	unsigned int pLight_aQuadLoc = glGetUniformLocation(shaderProgram, temp.c_str());
	glUniform1f(pLight_aQuadLoc, planet->attQuad);

	modelColDraw(*planet->obj, &model, planet->color);
}



//MOVEMENT FUNCITONS

void modelMovement(plane * plane, glm::mat4 * model) {

	float v = preVel;

	glm::vec4 dir = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

	dir = plane->rot * dir;

	plane->px += dir.x * v * deltaTime;
	plane->py += dir.y * v * deltaTime;
	plane->pz += dir.z * v * deltaTime;

	if (v > 1.0f && !velPress)
		vel -= vel / 100;

}



//MATH FUNCTIONS

glm::vec4 lerp(glm::vec4 x, glm::vec4 y, float f) {
	return x * f + y * (1 - f);
}

float lerp ( float x, float y, float f ) {
	return x * f + y * (1 - f);
}

glm::vec3 randColor()
{
	return glm::vec3(rand() % 255/200.0f, rand() % 255/200.0f, rand() % 255/200.0f);
}

glm::vec3 randPos(int range) 
{
	return glm::vec3( (rand() % range) - range/2 , (rand() % range) - range / 2, (rand() % range) - range / 2);
}

glm::vec3 randSize(int max, int min)
{
	float f = rand() % (max - min + 1) + min;
	return glm::vec3(f, f, f);
}


//SHADER MATRICES

void projectionMatrix(glm::mat4 * proyection) {

	preFOV = lerp(preFOV, 70.0f + preVel*2.0, (float)(SMOOTH * deltaTime));

	*proyection = glm::perspective(glm::radians(preFOV), (float)w/h, 0.1f, 3000.0f);
	proyectionLoc = glGetUniformLocation(shaderProgram, "projection");
	glUniformMatrix4fv(proyectionLoc, 1, GL_FALSE, glm::value_ptr(*proyection));
}

void viewMatrix(glm::mat4 * view, plane * obj) {

	glm::vec4 lookPos = glm::vec4(0.0f, 0.0f, 10.0f,   1.0f);
	glm::vec4 camePos = glm::vec4(0.0f, 1.0f, -7.0f, 1.0f);
	glm::vec4 up      = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
	glm::mat4 temp    = *obj->transform;

	up = obj->rot * up;

	lookPos = *obj->transform * lookPos;
	camePos = *obj->transform * camePos;

	actPosCam = lerp(actPosCam, camePos, (float)(SMOOTH * deltaTime));

	//*view = glm::lookAt(glm::vec3(camePos.x, camePos.y, camePos.z), glm::vec3(lookPos.x, lookPos.y, lookPos.z), glm::vec3(up.x, up.y, up.z));
	*view = glm::lookAt(glm::vec3(actPosCam.x, actPosCam.y, actPosCam.z), glm::vec3(lookPos.x, lookPos.y, lookPos.z), glm::vec3(up.x, up.y, up.z));

	viewLoc = glGetUniformLocation(shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(*view));

	//Luces
	//-----

	//Luz Direccional
	unsigned int dirLight_DirLoc = glGetUniformLocation(shaderProgram, "dirLight.direction");
	glUniform3f(dirLight_DirLoc, 0.0f, -1.0f, 0.0f);

	unsigned int dirLight_AmbLoc = glGetUniformLocation(shaderProgram, "dirLight.ambient");
	glUniform3f(dirLight_AmbLoc, 0.01f, 0.01f, 0.01f);

	unsigned int dirLight_DifLoc = glGetUniformLocation(shaderProgram, "dirLight.diffuse");
	glUniform3f(dirLight_DifLoc, 0.1f, 0.1f, 0.1f);

	unsigned int dirLight_SpcLoc = glGetUniformLocation(shaderProgram, "dirLight.specular");
	glUniform3f(dirLight_SpcLoc, 0.1f, 0.1f, 0.1f);

}

void modelMatrix(glm::mat4 * model) {
	*model = glm::mat4();
	transformLoc = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(*model));
}



//INPUT - CONTROL

void mousePos(GLFWwindow* win, plane * obj) {

	double xpos, ypos;
	glfwGetCursorPos(win, &xpos, &ypos);

	glm::mat4 temp = glm::mat4();

	//yaw
	if (xpos > (w / 2) + w / 10) {
		temp = glm::rotate(temp, (float)(ANG * deltaTime * (1 / preVel) * ((w / 2) - xpos) / 10), glm::vec3(0.0f, 1.0f, 0.0f));
		temp = glm::rotate(temp, (float)(-ANG * deltaTime * (1 / preVel) * ((w / 2) - xpos) / 20), glm::vec3(0.0f, 0.0f, 1.0f));
	}

	if (xpos < (w / 2) - w / 10) {
		temp = glm::rotate(temp, (float)(ANG * deltaTime * (1 / preVel) * ((w / 2) - xpos) / 10), glm::vec3(0.0f, 1.0f, 0.0f));
		temp = glm::rotate(temp, (float)(-ANG * deltaTime * (1 / preVel) * ((w / 2) - xpos) / 20), glm::vec3(0.0f, 0.0f, 1.0f));
	}


	//pitch
	if (ypos > (h / 2) + h / 10){}
		temp = glm::rotate(temp, (float)(-ANG * deltaTime * (1/preVel) * ((h/2) - ypos)/10), glm::vec3(1.0f, 0.0f, 0.0f));

	if (ypos < (h / 2) - h / 10)
		temp = glm::rotate(temp, (float)(-ANG * deltaTime * (1/preVel) * ((h/2) - ypos)/10), glm::vec3(1.0f, 0.0f, 0.0f));

	//rotation
	obj->rot = obj->rot * temp;

}

void processInput(GLFWwindow* win, plane* obj) {

	if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(win, true);

	//speed
	if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS && vel < 20) {
		vel += -log(vel) / log(1.5f) + 7.4f;
	}

	if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
		vel -= VEL * deltaTime * 10;

	//For rotation
	glm::mat4 temp = glm::mat4();

	//roll
	if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS)
		temp = glm::rotate(temp, (float)(-ANG * deltaTime * 1/vel * 100), glm::vec3(0.0f, 0.0f, 1.0f));

	if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS)
		temp = glm::rotate(temp, (float)( ANG * deltaTime * 1/vel * 100), glm::vec3(0.0f, 0.0f, 1.0f));

	//rotation
	obj->rot = obj->rot * temp;

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	w = width;
	h = height;
}


//SKYBOX

bool load_cube_map_side(
	GLuint texture, GLenum side_target, const char* file_name) {
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	int x, y, n;
	int force_channels = 4;
	unsigned char* image_data = stbi_load(
		file_name, &x, &y, &n, force_channels);
	if (!image_data) {
		fprintf(stderr, "ERROR: could not load %s\n", file_name);
		return false;
	}
	// non-power-of-2 dimensions check
	if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
		fprintf(stderr,
			"WARNING: image %s is not power-of-2 dimensions\n",
			file_name);
	}

	// copy image data into 'target' side of cube map
	glTexImage2D(
		side_target,
		0,
		GL_RGBA,
		x,
		y,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		image_data);
	free(image_data);
	return true;
}

void create_cube_map(
	const char* front,
	const char* back,
	const char* top,
	const char* bottom,
	const char* left,
	const char* right,
	GLuint* tex_cube) {
	// generate a cube-map texture to hold all the sides
	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, tex_cube);

	// load each image and copy into a side of the cube-map texture
	load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, front);
	load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, back);
	load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, top);
	load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, bottom);
	load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, left);
	load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_X, right);
	// format cube map texture
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void drawSkybox(GLuint vao, GLuint tex, glm::mat4 * view, glm::mat4 * transform, glm::mat4 * projection, plane raptor) 
	{

	*view = glm::mat4(glm::mat3(*view));

	*transform = glm::mat4();
	*transform = glm::translate(*transform, glm::vec3(raptor.pz, raptor.py, raptor.pz));

	*transform = glm::scale(*transform, glm::vec3(100.0f, 100.0f, 100.0f));

	transformLoc = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(*transform));

	unsigned int intLoc = glGetUniformLocation(shaderProgram, "useSky");
	glUniform1i(intLoc, TRUE);

	unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(*view));

	unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(*projection));

	unsigned int texLoc = glGetUniformLocation(shaderProgram, "skybox");
	glUniform1i(texLoc, 1);

	glDepthMask(GL_FALSE);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);

	intLoc = glGetUniformLocation(shaderProgram, "useSky");
	glUniform1i(intLoc, FALSE);

}

void iniSkybox(GLuint * vao, GLuint * tex)
{

	//skybox
	float points[] = {
  -10.0f,  10.0f, -10.0f,
  -10.0f, -10.0f, -10.0f,
   10.0f, -10.0f, -10.0f,
   10.0f, -10.0f, -10.0f,
   10.0f,  10.0f, -10.0f,
  -10.0f,  10.0f, -10.0f,

  -10.0f, -10.0f,  10.0f,
  -10.0f, -10.0f, -10.0f,
  -10.0f,  10.0f, -10.0f,
  -10.0f,  10.0f, -10.0f,
  -10.0f,  10.0f,  10.0f,
  -10.0f, -10.0f,  10.0f,

   10.0f, -10.0f, -10.0f,
   10.0f, -10.0f,  10.0f,
   10.0f,  10.0f,  10.0f,
   10.0f,  10.0f,  10.0f,
   10.0f,  10.0f, -10.0f,
   10.0f, -10.0f, -10.0f,

  -10.0f, -10.0f,  10.0f,
  -10.0f,  10.0f,  10.0f,
   10.0f,  10.0f,  10.0f,
   10.0f,  10.0f,  10.0f,
   10.0f, -10.0f,  10.0f,
  -10.0f, -10.0f,  10.0f,

  -10.0f,  10.0f, -10.0f,
   10.0f,  10.0f, -10.0f,
   10.0f,  10.0f,  10.0f,
   10.0f,  10.0f,  10.0f,
  -10.0f,  10.0f,  10.0f,
  -10.0f,  10.0f, -10.0f,

  -10.0f, -10.0f, -10.0f,
  -10.0f, -10.0f,  10.0f,
   10.0f, -10.0f, -10.0f,
   10.0f, -10.0f, -10.0f,
  -10.0f, -10.0f,  10.0f,
   10.0f, -10.0f,  10.0f
	};

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * 36 * sizeof(float), &points, GL_STATIC_DRAW);

	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	create_cube_map("front.png", "back.png", "bottom.png", "top.png", "left.png", "right.png", tex);

}