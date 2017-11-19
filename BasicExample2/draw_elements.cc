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

using namespace glm;

#define BUFFER_OFFSET(offset) ((GLvoid *) offset)
#define M_PI 3.14159265358979323846f

#define LPERF 10
#define NDIVS 40
#define MAX_VERTEXS LPERF*NDIVS
#define MAX_CARES (MAX_VERTEXS-NDIVS)*2

enum { Vertices, Indices, NumBuffers };

typedef struct {
	float x, y, z;
} Vertex3d;
typedef struct {
	int vertexs[3];
} Triangle;
typedef struct {
	Vertex3d vertexs[MAX_VERTEXS];
	Triangle tris[MAX_CARES];
	int nvertexs = 0, ntris = 0;
} Objecte;

struct perfil {
	Vertex3d p[LPERF];
	int np;
};

GLuint buffers[NumBuffers];
GLuint vPos;
GLuint program;
GLuint mvpMatrixID;

mat4 projMatrix;
mat4 viewMatrix;
mat4 modelMatrix;

int pitchAngle = 45;
int yawAngle = 5;
vec3 eyePosition;
bool drawWireframe;
float zoomLevel = 0.8f;

int rotationStep = 5;
float moveStep = 0.1;

int yMouse = 0;
int xMouse = 0;
bool estatM1 = false;

bool tipus = false;

Objecte malla;
void AfegirVertexs(Objecte &m, const perfil& p) {
	int i;
	for (i = 0; i < LPERF; i++)
		m.vertexs[m.nvertexs++] = p.p[i];
}
void GiraPerfil(const perfil& from, perfil& to, float angle) {
	int i;
	float r;
	for (i = 0; i < LPERF; i++) {
		r = from.p[i].x;
		to.p[i].x = r*cos(angle);
		to.p[i].y = from.p[i].y;
		to.p[i].z = r*sin(angle);
	}
}

void DefTri(Objecte &m, int ini, int fi, int numv) {
	ini = ini*numv;
	fi = fi*numv;
	//for (int i = 0; i < (numv ‐ 1); i++) {
	for (int i = 0; i < numv - 1; i++) {
		m.tris[m.ntris].vertexs[0] = ini + i + 1;
		m.tris[m.ntris].vertexs[1] = ini + i;
		m.tris[m.ntris].vertexs[2] = fi + i;
		m.ntris++;
		m.tris[m.ntris].vertexs[0] = ini + i + 1;
		m.tris[m.ntris].vertexs[1] = fi + i;
		m.tris[m.ntris].vertexs[2] = fi + i + 1;
		m.ntris++;
	}
}

void initObj(Objecte &m) {
	m = Objecte();
	float angle, x;
	perfil act, ini;
	// Definim el perfil (y=x^2)
	for (int i = 0; i < LPERF; i++) {
		x = 2.0 * (i + 1) / LPERF;
		ini.p[i].x = x;
		ini.p[i].y = (tipus?(sin(x*4)/2): (x*x));
		ini.p[i].z = 0;
	}
	// Generem el solid de revolucio
	m.nvertexs = m.ntris = 0;
	AfegirVertexs(m, ini);
	for (int i = 1; i < NDIVS; i++) {
		angle = (2.0 * M_PI * i) / NDIVS;
		GiraPerfil(ini, act, angle);
		AfegirVertexs(m, act);
		DefTri(m, i - 1, i, LPERF);
	}
	DefTri(m, NDIVS - 1, 0, LPERF);
}

//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐




void init() {
	initObj(malla);

	//
	// ‐‐‐ Load vertex data ‐‐‐
	//

	glGenBuffers(NumBuffers, buffers);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[Vertices]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(malla.vertexs),
		malla.vertexs, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[Indices]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(malla.tris),
		malla.tris, GL_STATIC_DRAW);

	//
	// ‐‐‐ Load shaders ‐‐‐
	//

	const char* vShader = {
		"#version 330\n"
		"in vec3 vPos;"
		"uniform mat4 mvp_matrix;"
		"void main() {"
		" gl_Position = mvp_matrix * vec4( vPos, 1 );"
		"}"
	};

	const char* fShader = {
		"#version 330\n"
		"out vec4 fragColor;"
		""
		"void main() {"
		"   fragColor = vec4(0.8, 0, 0, 1);"
		"}"
	};

	// Declare shader IDs
	GLuint vShaderID, fShaderID;

	// Create empty shader objects
	vShaderID = glCreateShader(GL_VERTEX_SHADER);
	fShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Attach shader source code the shader objects
	glShaderSource(vShaderID, 1, &vShader, NULL);
	glShaderSource(fShaderID, 1, &fShader, NULL);

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

	vPos = glGetAttribLocation(program, "vPos");
	mvpMatrixID = glGetUniformLocation(program, "mvp_matrix");

	glClearColor(0.0, 0.0, 1.0, 1.0);
}

//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐

void display() {
	glClear(GL_COLOR_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, (drawWireframe ? GL_LINE : GL_FILL));
	glUseProgram(program);
	mat4 scaleMatrix = scale(mat4(1.0f), vec3(zoomLevel));
	mat4 translateMatrix = translate(mat4(1.0f), eyePosition);
	mat4 rotationMatrixX = rotate(mat4(1.0f), radians((float)yawAngle), vec3(1.0f, 0.0f, 0.0f));
	mat4 rotationMatrixY = rotate(mat4(1.0f), radians((float)pitchAngle), vec3(0.0f, 1.0f, 0.0f));

	modelMatrix = translateMatrix * rotationMatrixY * rotationMatrixX * scaleMatrix;
	mat4 mvpMatrix = projMatrix * viewMatrix * modelMatrix;

	glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, value_ptr(mvpMatrix));

	glBindBuffer(GL_ARRAY_BUFFER, buffers[Vertices]);
	glVertexAttribPointer(vPos, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(vPos);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[Indices]);
	glDrawElements(GL_TRIANGLES, malla.ntris * 4 - 6, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glutSwapBuffers();
}

//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐

void reshape(int width, int height) {
	glViewport(0, 0, width, height);
	projMatrix = perspective(radians(60.0f), (float)width / (float)height, 0.1f, 5000.0f);
	viewMatrix = lookAt(vec3(0.0f, 0.0f, 2.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
}

//‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐‐

void keyboard(unsigned char key, int x, int y) {

	// Use u and d keys to control the camera pitch angle. 
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
		case 'w':
			eyePosition.z += moveStep;
			break;
		case 's':
			eyePosition.z -= moveStep;
			break;
		case 'k':
			pitchAngle += rotationStep;
			pitchAngle %= 360;
			break;

		case 'h':
			pitchAngle -= rotationStep;
			pitchAngle %= 360;
			break;
		case 'j':
			yawAngle += rotationStep;
			yawAngle %= 360;
			break;

		case 'u':
			yawAngle -= rotationStep;
			yawAngle %= 360;
			break;
		case 'x':
			drawWireframe = !drawWireframe;
			break;
		case 'z':
			tipus = !tipus;
			initObj(malla);
			glBindBuffer(GL_ARRAY_BUFFER, buffers[Vertices]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(malla.vertexs),
				malla.vertexs, GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[Indices]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(malla.tris),
				malla.tris, GL_STATIC_DRAW);
			break;
		default:
			break;
	}

	// Refresh display by calling glutPostRedisplay() to generate a display event. 
	// Don't call display() function directly. 
	glutPostRedisplay();
}

void mouseMovement(int x, int y) {
	xMouse = x;
	yMouse = y;
}
void mouseMotion(int x, int y) {

	pitchAngle -= x - xMouse;
	pitchAngle %= 360;
	yawAngle += y - yMouse;
	yawAngle %= 360;
	glutPostRedisplay();
	xMouse = x;
	yMouse = y;

}
void mouse(int button, int state, int x, int y) {
	switch(button) {
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

int main(int argc, char* argv[]) {
	std::cout << "Usa 'x' per activar/desactivar el wireframe" << std::endl
		<< "'WASD' + 'EQ' per moure la posicio de la camera, i 'UHJK' per rotar la malla" << std::endl
		<< "Tambe pots arrastrar el mouse dins la finestra per rotar la malla" << std::endl
		<< "Mou la roda del mouse per apropar-te o allunyar-te" << std::endl
		<< "Prem 'z' per canviar el tipus de malla" << std::endl;
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	glutCreateWindow(argv[0]);

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