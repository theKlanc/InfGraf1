//////////////////////////////////////////////////////////////////////////////
//
// draw‐elements.cxx ‐ render a cube using glDrawElements() indexing into
// a set of vertex attributes stored in array‐of‐structures (AOS) storage.
//

#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // Transformation related functions
#include <glm/gtc/type_ptr.hpp> 
#include <iostream>
#include <vector>

#include <glm/gtx/transform2.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtc/matrix_inverse.hpp>

using namespace glm;
using namespace std;


#define BUFFER_OFFSET(offset) ((GLvoid *) offset)
#define M_PI 3.14159265358979323846f

#define LPERF 10
#define NDIVS 40
#define MAX_VERTEXS LPERF*NDIVS
#define MAX_CARES (MAX_VERTEXS-NDIVS)*2

enum { Vertices, Normals, NumBuffers };

GLuint buffers[NumBuffers];
GLuint vPos;
GLuint normalID;
GLuint program;
GLuint mvpMatrixID;
GLuint modelMatrixID;
GLint normalMatrixID; // uniform variable: normal matrix for transforming normals
GLint lightSourcePositionID; // uniform variable: for lighting calculation
GLint diffuseLightProductID; // uniform variable: for lighting calculation
GLint ambientID;
GLint attenuationAID;
GLint attenuationBID;
GLint attenuationCID;

mat4 projMatrix;
mat4 mvpMatrix;
mat4 viewMatrix;
mat4 modelMatrix;
mat3 normalMatrix;  // Normal matrix for transforming normals

					// Light parameters
vec4 lightSourcePosition = vec4(0.0f, 4.0f, 0.0f, 1.0f);
vec4 diffuseMaterial = vec4(0.5f, 0.5f, 1.0f, 1.0f);
vec4 diffuseLightIntensity = vec4(1.0f, 1.0f, 1.0f, 1.0f);
vec4 ambient = vec4(0.2f, 0.2f, 0.2f, 1.0f);
float attenuationA = 1.0f;
float attenuationB = 0.2f;
float attenuationC = 0.0f;

vec4 diffuseLightProduct;

// Camera parameters
vec3 eyePosition = vec3(0.0f, 0.0f, 4.0f);
vec3 lookAtCenter = vec3(0.0f, 0.0f, 0.0f);
vec3 upVector = vec3(0.0f, 1.0f, 0.0f);
float fieldOfView = 30.0f;
float nearPlane = 0.1f;
float farPlane = 1000.0f;

int pitchAngle = 45;
int yawAngle = 5;
bool drawWireframe;
float zoomLevel = 0.8f;

int rotationStep = 5;
float moveStep = 0.1;

int yMouse = 0;
int xMouse = 0;
bool estatM1 = false;

bool tipus = false;

//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐
typedef struct {
	std::vector<glm::vec3> vertexs;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	int nvertexs = 0;
} Objecte;
Objecte malla;

bool loadOBJ(const char * path,	std::vector<glm::vec3> & out_vertices,	std::vector<glm::vec2> & out_uvs, std::vector<glm::vec3> & out_normals) {
	printf("Loading OBJ file %s...\n", path);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;


	FILE * file = fopen(path, "r");
	if (file == NULL) {
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}

	while (1) {

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

				   // else : parse lineHeader

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				fclose(file);
				return false;
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
		else {
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}

	// For each vertex of each triangle
	for (unsigned int i = 0; i<vertexIndices.size(); i++) {

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		glm::vec3 normal = temp_normals[normalIndex - 1];

		// Put the attributes in buffers
		out_vertices.push_back(vertex);
		out_uvs.push_back(uv);
		out_normals.push_back(normal);

	}
	fclose(file);
	return true;
}

void initObj(Objecte &malla, string path) {
	loadOBJ(path.c_str(), malla.vertexs, malla.uvs, malla.normals);
	malla.nvertexs = malla.vertexs.size();
}

//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐
float rotateX = 0;
float rotateY = 0;

void getShaderVariableLocations(GLuint shaderProgram) {

	// Retrieve the ID of a vertex attribute, i.e. position
	vPos = glGetAttribLocation(shaderProgram, "vPos");
	normalID = glGetAttribLocation(shaderProgram, "normal");

	mvpMatrixID = glGetUniformLocation(shaderProgram, "mvpMatrix");

	modelMatrixID = glGetUniformLocation(shaderProgram, "modelMatrix");
	normalMatrixID = glGetUniformLocation(shaderProgram, "normalMatrix");

	lightSourcePositionID = glGetUniformLocation(shaderProgram, "lightSourcePosition");
	diffuseLightProductID = glGetUniformLocation(shaderProgram, "diffuseLightProduct");
	ambientID = glGetUniformLocation(shaderProgram, "ambient");

	attenuationAID = glGetUniformLocation(shaderProgram, "attenuationA");
	attenuationBID = glGetUniformLocation(shaderProgram, "attenuationB");
	attenuationCID = glGetUniformLocation(shaderProgram, "attenuationC");
}

//---------------------------------------------------------------
void setShaderVariables() {
	// value_ptr is a glm function
	glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, value_ptr(mvpMatrix));
	glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, value_ptr(modelMatrix));
	glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, value_ptr(normalMatrix));

	glUniform4fv(lightSourcePositionID, 1, value_ptr(lightSourcePosition));
	glUniform4fv(diffuseLightProductID, 1, value_ptr(diffuseLightProduct));
	glUniform4fv(ambientID, 1, value_ptr(ambient));
	glUniform1f(attenuationAID, attenuationA);
	glUniform1f(attenuationBID, attenuationB);
	glUniform1f(attenuationCID, attenuationC);
}

//---------------------------------------------------------------
// Set lighting related parameters
void setLightingParam() {
	diffuseLightProduct = diffuseMaterial * diffuseLightIntensity;
}


mat4 buildModelMatrix() {

	mat4 rotationXMatrix = rotate(mat4(1.0f), radians(rotateX), vec3(1.0f, 0.0f, 0.0f));
	mat4 rotationYMatrix = rotate(mat4(1.0f), radians(rotateY), vec3(0.0f, 1.0f, 0.0f));

	mat4 matrix = rotationYMatrix * rotationXMatrix;

	return matrix;
}
void buildMatrices() {
	modelMatrix = buildModelMatrix();
	mat4 scaleMatrix = scale(mat4(1.0f), vec3(zoomLevel));
	mvpMatrix = projMatrix * viewMatrix * modelMatrix*scaleMatrix;

	normalMatrix = column(normalMatrix, 0, vec3(modelMatrix[0][0], modelMatrix[0][1], modelMatrix[0][2]));
	normalMatrix = column(normalMatrix, 1, vec3(modelMatrix[1][0], modelMatrix[1][1], modelMatrix[1][2]));
	normalMatrix = column(normalMatrix, 2, vec3(modelMatrix[2][0], modelMatrix[2][1], modelMatrix[2][2]));

	// Use glm::inverseTranspose() to create a normal matrix, which is used to transform normal vectors. 
	normalMatrix = inverseTranspose(normalMatrix);
}


void display() {
	//Esborrem
	viewMatrix = lookAt(eyePosition, lookAtCenter, upVector);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	buildMatrices();

	setShaderVariables();
	//Definim mode de poligons
	glPolygonMode(GL_FRONT_AND_BACK, (drawWireframe ? GL_LINE : GL_FILL));
	//Definim grup de shaders a usar
	glUseProgram(program);
	//Declarem i definim matrius x mvp

	//glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, value_ptr(mvpMatrix));

	glBindBuffer(GL_ARRAY_BUFFER, buffers[Vertices]);
	glVertexAttribPointer(vPos, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(vPos);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[Normals]);
	glVertexAttribPointer(normalID, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(normalID);

	glDrawArrays(GL_TRIANGLES,0, malla.nvertexs);

	glutSwapBuffers();
}

//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐

void reshape(int width, int height) {
	glViewport(0, 0, width, height);
	projMatrix = perspective(fieldOfView, (float)width / (float)height, nearPlane, farPlane);
	viewMatrix = lookAt(eyePosition, lookAtCenter, upVector);
}

//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
		case 'a':
			eyePosition.x += moveStep;
			break;
		case 'd':
			eyePosition.x -= moveStep;
			break;
		case 'q':
			eyePosition.y += moveStep;
			break;
		case 'e':
			eyePosition.y -= moveStep;
			break;
		case 's':
			eyePosition.z += moveStep;
			break;
		case 'w':
			eyePosition.z -= moveStep;
			break;
		case 'x':
			drawWireframe = !drawWireframe;
			break;
		default:
			break;
	}
	glutPostRedisplay();
}

void mouseMovement(int x, int y) {
	xMouse = x;
	yMouse = y;
}
void mouseMotion(int x, int y) {

	rotateY-= x - xMouse;
	rotateX+= y - yMouse;
	glutPostRedisplay();
	xMouse = x;
	yMouse = y;

}
void mouse(int button, int state, int x, int y) {
	switch (button) {
		case 3:
			zoomLevel += 0.01f;
			glutPostRedisplay();
			break;
		case 4:
			zoomLevel -= 0.01f;
			glutPostRedisplay();
			break;
	}
	if (zoomLevel < 0)zoomLevel = 0;
}

void init() {
	glViewport(0, 0, 800, 800);
	projMatrix = perspective(fieldOfView, 800.0f / 800.0f, nearPlane, farPlane);
	viewMatrix = lookAt(eyePosition, lookAtCenter, upVector);

	string path = "model.obj";
	initObj(malla, path);

	glGenBuffers(NumBuffers, buffers);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[Vertices]);
	glBufferData(GL_ARRAY_BUFFER, malla.vertexs.size() * sizeof(glm::vec3),
		&malla.vertexs[0].x, GL_STATIC_DRAW);


	glBindBuffer(GL_ARRAY_BUFFER, buffers[Normals]);
	glBufferData(GL_ARRAY_BUFFER, malla.normals.size() * sizeof(glm::vec3), &malla.normals[0].x, GL_STATIC_DRAW);


	const char* vSource = {
		"#version 330\n"
		"in vec4 vPos;"
		"in vec4 normal;"

		"uniform mat4x4 mvpMatrix;"
		"uniform mat4x4 modelMatrix;"
		"uniform mat3x3 normalMatrix;"
		"uniform vec4 lightSourcePosition;"
		"uniform vec4 diffuseLightProduct;"
		"uniform vec4 ambient;"
		"uniform float attenuationA;"
		"uniform float attenuationB;"
		"uniform float attenuationC;"
		"out vec4 color;"

		"void main() {"
		"	gl_Position = mvpMatrix * vPos;"
		// Transform the vertex position to the world space. 
		"	vec4 transformedVertex = modelMatrix * vPos;"
		// Transform the normal vector to the world space. 
		"	vec3 transformedNormal = normalize(normalMatrix * normal.xyz);"
		// Light direction
		"	vec3 lightVector = normalize(transformedVertex.xyz - lightSourcePosition.xyz);"
		// Distance between the light source and vertex
		"   float dist = distance(lightSourcePosition.xyz, transformedVertex.xyz);"
		// Attenuation factor
		"   float attenuation = 1.0f / (attenuationA + (attenuationB * dist) + (attenuationC * dist * dist));"
		// Calculate the diffuse component of the lighting equation.
		"	vec4 diffuse = attenuation * (max(dot(transformedNormal, lightVector), 0.0) * diffuseLightProduct);"
		// Combine the ambient component and diffuse component. 
		"	color = ambient + diffuse;"
		"}"
	};
	const char* fSource = {
		"#version 330\n"
		"in vec4 color;"
		"out vec4 fragColor;"
		"void main() {"
		"	fragColor = color;"
		"}"
	};

	// Declare shader IDs
	GLuint vShaderID, fShaderID;

	// Create empty shader objects
	vShaderID = glCreateShader(GL_VERTEX_SHADER);
	fShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Attach shader source code the shader objects
	glShaderSource(vShaderID, 1, &vSource, NULL);
	glShaderSource(fShaderID, 1, &fSource, NULL);

	// Compile shader objects
	glCompileShader(vShaderID);
	glCompileShader(fShaderID);

	// Create an empty shader program object
	program = glCreateProgram();

	// Attach vertex and fragment shaders to the shader program
	glAttachShader(program, vShaderID);
	glAttachShader(program, fShaderID);

	// Link the shader program
	glLinkProgram(program);

	//Guardem localització de variables dels shadersç
	getShaderVariableLocations(program);

	setLightingParam();

	//Color de fons
	glClearColor(0.0, 0.0, 1.0, 1.0);
	glEnable(GL_DEPTH_TEST);
}

int main(int argc, char* argv[]) {
	std::cout << "Usa 'x' per activar/desactivar el wireframe" << std::endl
		<< "'WASD' + 'EQ' per moure la posicio de la camera" << std::endl
		<< "Tambe pots arrastrar el mouse dins la finestra per rotar la malla" << std::endl
		<< "Mou la roda del mouse per apropar-te o allunyar-te" << std::endl;
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	glutCreateWindow(argv[0]);

	glutReshapeWindow(800, 800);

	glewInit();

	init();

	glutDisplayFunc(display);

	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMotionFunc(mouseMotion);
	glutMouseFunc(mouse);
	glutPassiveMotionFunc(mouseMovement);
	glutMainLoop();
}