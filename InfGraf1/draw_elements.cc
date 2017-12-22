#include <GL/glew.h> // This must appear before freeglut.h

// Freeglut header
#include <GL/freeglut.h>

// GLM header files
#include <glm/glm.hpp> 

#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtx/transform2.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <vector>

// C++ header files
#include <iostream>

using namespace std;
using namespace glm;

#define BUFFER_OFFSET(offset) ((GLvoid *) offset)

// VBO buffer IDs
GLuint vertexArrayBufferID = 0;
GLuint normalArrayBufferID = 0;

GLuint program; // shader program ID

				// Shader variable IDs
GLint vPos; // vertex attribute: position
GLint normalID; // vertex attribute: normal

GLint mvpMatrixID; // uniform variable: model, view, projection matrix
GLint modelMatrixID; // uniform variable: model, view matrix
GLint normalMatrixID; // uniform variable: normal matrix for transforming normals
GLint lightSourcePositionID; // uniform variable: for lighting calculation
GLint diffuseLightProductID; // uniform variable: for lighting calculation
GLint ambientID;
GLint attenuationAID;
GLint attenuationBID;
GLint attenuationCID;

// Transformation matrices
mat4 projMatrix;
mat4 mvpMatrix;
mat4 modelMatrix;
mat4 viewMatrix;
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

// Mouse controlled rotation angles
float rotateX = 0;
float rotateY = 0;

int nVerts = 0;

bool loadOBJ(
	const char * path,
	std::vector<glm::vec3> & out_vertices,
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals
) {
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

//---------------------------------------------------------------
// Initialize vertex arrays and VBOs

void prepareVBOs() {
	// Define a 3D pyramid. 
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals; // Won't be used at the moment.
	bool res = loadOBJ("model.obj", vertices, uvs, normals);
	nVerts = vertices.size();
	// Get an unused buffer object name. Required after OpenGL 3.1. 
	glGenBuffers(1, &vertexArrayBufferID);

	// If it's the first time the buffer object name is used, create that buffer. 
	glBindBuffer(GL_ARRAY_BUFFER, vertexArrayBufferID);

	// Allocate memory for the active buffer object. 
	// 1. Allocate memory on the graphics card for the amount specified by the 2nd parameter.
	// 2. Copy the data referenced by the third parameter (a pointer) from the main memory to the 
	//    memory on the graphics card. 
	// 3. If you want to dynamically load the data, then set the third parameter to be NULL. 
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(glm::vec3), &vertices[0].x, GL_STATIC_DRAW);

	glGenBuffers(1, &normalArrayBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, normalArrayBufferID);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0].x, GL_STATIC_DRAW);
}

//---------------------------------------------------------------
// Print out the output of the shader compiler
void printLog(GLuint obj) {
	int infologLength = 0;
	char infoLog[1024];

	if (glIsShader(obj)) {
		glGetShaderInfoLog(obj, 1024, &infologLength, infoLog);
	}
	else {
		glGetProgramInfoLog(obj, 1024, &infologLength, infoLog);
	}

	if (infologLength > 0) {
		cout << infoLog;
	}
}

//-------------------------------------------------------------------
void prepareShaders() {
	// Vertex shader source code
	// A point light source is implemented. 
	// For simplicity, only the ambient and diffuse components are implemented. 
	// The lighting is calculated in world space, not in camera space. 
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

	// Fragment shader source code
	const char* fSource = {
		"#version 330\n"
		"in vec4 color;"
		"out vec4 fragColor;"
		"void main() {"
		"	fragColor = color;"
		"}"
	};

	// Declare shader IDs
	GLuint vShader, fShader;

	// Create empty shader objects
	vShader = glCreateShader(GL_VERTEX_SHADER);
	fShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Attach shader source code the shader objects
	glShaderSource(vShader, 1, &vSource, NULL);
	glShaderSource(fShader, 1, &fSource, NULL);

	// Compile shader objects
	glCompileShader(vShader);
	printLog(vShader);

	glCompileShader(fShader);
	printLog(fShader);

	// Create an empty shader program object
	program = glCreateProgram();

	// Attach vertex and fragment shaders to the shader program
	glAttachShader(program, vShader);
	glAttachShader(program, fShader);

	// Link the shader program
	glLinkProgram(program);
	printLog(program);
}

//---------------------------------------------------------------
// Retrieve the IDs of the shader variables. Later we will
// use these IDs to pass data to the shaders. 
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

//---------------------------------------------------------------
// Build the model matrix. This matrix will transform the 3D object to the proper place. 
mat4 buildModelMatrix() {

	mat4 rotationXMatrix = rotate(mat4(1.0f), radians(rotateX), vec3(1.0f, 0.0f, 0.0f));
	mat4 rotationYMatrix = rotate(mat4(1.0f), radians(rotateY), vec3(0.0f, 1.0f, 0.0f));

	mat4 matrix = rotationYMatrix * rotationXMatrix;

	return matrix;
}

//---------------------------------------------------------------
void buildMatrices() {
	modelMatrix = buildModelMatrix();

	mvpMatrix = projMatrix * viewMatrix * modelMatrix;

	normalMatrix = column(normalMatrix, 0, vec3(modelMatrix[0][0], modelMatrix[0][1], modelMatrix[0][2]));
	normalMatrix = column(normalMatrix, 1, vec3(modelMatrix[1][0], modelMatrix[1][1], modelMatrix[1][2]));
	normalMatrix = column(normalMatrix, 2, vec3(modelMatrix[2][0], modelMatrix[2][1], modelMatrix[2][2]));

	// Use glm::inverseTranspose() to create a normal matrix, which is used to transform normal vectors. 
	normalMatrix = inverseTranspose(normalMatrix);
}

//---------------------------------------------------------------
// Handles the display event
void display() {
	// Clear the window with the background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	buildMatrices();

	setShaderVariables();

	// Activate the shader program
	glUseProgram(program);


	// If the buffer object already exists, make that buffer the current active one. 
	// If the buffer object name is 0, disable buffer objects. 
	glBindBuffer(GL_ARRAY_BUFFER, vertexArrayBufferID);

	// Associate the vertex array in the buffer object with the vertex attribute: "position"
	glVertexAttribPointer(vPos, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	// Enable the vertex attribute: "position"
	glEnableVertexAttribArray(vPos);

	glBindBuffer(GL_ARRAY_BUFFER, normalArrayBufferID);
	glVertexAttribPointer(normalID, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(normalID);

	// Start the shader program. Draw the object. The third parameter is the number of triangles. 
	glDrawArrays(GL_TRIANGLES, 0, nVerts);

	// Refresh the window
	glutSwapBuffers();
}

//---------------------------------------------------------------
// Handles the reshape event
void reshape(int width, int height) {
	// Specify the width and height of the picture within the window
	glViewport(0, 0, width, height);

	projMatrix = perspective(fieldOfView, (float)width / (float)height, nearPlane, farPlane);

	viewMatrix = lookAt(eyePosition, lookAtCenter, upVector);
}

//---------------------------------------------------------------
// Read mouse motion data and convert them to rotation angles. 
void passiveMotion(int x, int y) {

	rotateY = (float)x * -0.8f;
	rotateX = (float)y * -0.8f;

	// Generate a dislay event to force refreshing the window. 
	glutPostRedisplay();
}

//-----------------------------------------------------------------
void init() {
	prepareVBOs();

	prepareShaders();

	getShaderVariableLocations(program);

	setLightingParam();

	// Specify the background color
	glClearColor(1, 1, 1, 1);

	glEnable(GL_DEPTH_TEST);
}

//---------------------------------------------------------------
void main(int argc, char *argv[]) {
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	glutCreateWindow("Lighting Demo");

	glutReshapeWindow(800, 800);

	glewInit();

	init();

	// Register the display callback function
	glutDisplayFunc(display);

	// Register the reshape callback function
	glutReshapeFunc(reshape);

	// Register the passive mouse motion call back function
	// This function is called when the mouse moves within the window
	// while no mouse buttons are pressed. 
	glutPassiveMotionFunc(passiveMotion);

	// Start the event loop
	glutMainLoop();
}