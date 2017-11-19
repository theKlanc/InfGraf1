#include <stdlib.h>
#include <GL/glew.h> // This must appear before freeglut.h
#include <GL/freeglut.h>
#include <cmath>

#define BUFFER_OFFSET(offset) ((GLvoid *) offset)
#define M_PI 3.14159265358979323846f

enum { Vertices, Indices, NumBuffers };

GLuint buffers[NumBuffers];
GLuint vPos;
GLuint program;
#define MAX_VERTEXS 200
#define MAX_CARES 200
typedef struct {
	float x, y, z;
} Vertex3d;
typedef struct {
	GLfloat vertexs[3];
} Triangle;
typedef struct {
	Vertex3d vertexs[MAX_VERTEXS]{
		{ -0.75, -0.5, 0.0 },
		{ 0.75, -0.5, 0.0},
		{ 0.0, 0.75, 0.0 }
	};
	Triangle tris[MAX_CARES]= {
		{ 0, 1,2 }
	};
	int nvertexs=3, ntris=1;
} Objecte;
#define LPERF 7
#define NDIVS 20
struct perfil {
	Vertex3d p[LPERF];
	int np;
};

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
		for(int i = 0; i<numv-1;i++){
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
	float angle, x;
	perfil act, ini;
	// Definim el perfil (y=x^2)
	for (int i = 0; i < LPERF; i++) {
		x = 2.0 * (i + 1) / LPERF;
		ini.p[i].x = x;
		ini.p[i].y = x*x;
		ini.p[i].z = 0;
	}
	// Generem el solid de revolucio
	m.nvertexs = m.ntris = 0;
	AfegirVertexs(m, ini);
	for (int i = 1; i < NDIVS; i++) {
		angle = (2.0 * M_PI * i) / NDIVS;
		GiraPerfil(ini, act, angle);
		AfegirVertexs(m, act);
		DefTri(m, i - 1, i,LPERF);//potser s ha d arreglar
	}
	DefTri(m, NDIVS - 1, 0,LPERF);//potser s ha d arreglar
}

void init() {
	//initObj(malla);

	glGenBuffers(NumBuffers, buffers);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[Vertices]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(malla.vertexs),
		malla.vertexs, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[Indices]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(malla.tris),
		malla.tris, GL_STATIC_DRAW);
	
	// OpenGL vertex shader source code
	const char* vSource = {
		"#version 330\n"
		"in vec4 vPos;"
		"void main() {"
		"	gl_Position = vPos * vec4(1.0f, 1.0f, 1.0f, 1.0f);"
		"}"
	};

	// OpenGL fragment shader source code
	const char* fSource = {
		"#version 330\n"
		"out vec4 fragColor;"
		"void main() {"
		"	fragColor = vec4(0.8, 0.8, 0, 1);"
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
	glCompileShader(fShader);

	// Create an empty shader program object
	program = glCreateProgram();

	// Attach vertex and fragment shaders to the shader program
	glAttachShader(program, vShader);
	glAttachShader(program, fShader);

	// Link the shader program
	glLinkProgram(program);

	// Retrieve the ID of a vertex attribute, i.e. position
	vPos = glGetAttribLocation(program, "vPos");

	// Specify the background color
	glClearColor(0, 0, 0, 1);
}

void reshape(int width, int height) {
	// Specify the width and height of the picture within the window
	glViewport(0, 0, width, height);
}

void display() {
	// Clear the window with the background color
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[Vertices]);
	glVertexAttribPointer(vPos, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(vPos);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[Indices]);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glutSwapBuffers();
}

void main(int argc, char *argv[]) {
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

	glutCreateWindow(argv[0]);

	glewInit();

	init();

	// Register the display callback function
	glutDisplayFunc(display);

	// Register the reshape callback function
	glutReshapeFunc(reshape);

	// Start the event loop
	glutMainLoop();
}