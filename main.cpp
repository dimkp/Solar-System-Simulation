#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

const int WIDTH = 1000;
const int HEIGHT = 1000;

// Scaling
const float AU_TO_UNITS = 2.0f;           // 1 AU == 2 OpenGL units (compress distances)
const float SIM_YEARS_TO_SECONDS = 20.0f; // 1 real year == 20 seconds

// Planet 
struct Planet {
    const char* name;
    float a_au;       // semi-major axis in AU
    float e;          // eccentricity
    float period_days;// orbital period in days
    float size;       // visual radius
    float color[3];
};


std::vector<Planet> planets = {
    {"Mercury",  0.387f, 0.2056f,   88.0f,    0.12f, {0.7f, 0.7f, 0.7f}},
    {"Venus",    0.723f, 0.0068f,  224.7f,   0.16f, {0.95f,0.8f, 0.3f}},
    {"Earth",    1.000f, 0.0167f,  365.25f,  0.17f, {0.2f, 0.5f, 1.0f}},
    {"Mars",     1.524f, 0.0934f,  687.0f,   0.14f, {1.0f, 0.4f, 0.2f}},
    {"Jupiter",  5.203f, 0.0484f, 4331.0f,   0.40f, {1.0f, 0.8f, 0.6f}},
    {"Saturn",   9.537f, 0.0542f,10747.0f,   0.35f, {0.95f,0.9f, 0.6f}},
    {"Uranus",  19.191f, 0.0472f,30589.0f,   0.28f, {0.5f, 0.85f,0.9f}},
    {"Neptune", 30.068f, 0.0086f,60190.0f,   0.28f, {0.35f,0.45f,1.0f}}
};


void drawSphere(float radius, int slices, int stacks) {
    for (int i = 0; i <= stacks; ++i) {
        float V = i / (float)stacks;
        float phi = V * (float)M_PI;

        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float U = j / (float)slices;
            float theta = U * (float)(2.0f * M_PI);

            float x = cosf(theta) * sinf(phi);
            float y = cosf(phi);
            float z = sinf(theta) * sinf(phi);
            glVertex3f(x * radius, y * radius, z * radius);

            float phi2 = phi + (float)M_PI / stacks;
            float x2 = cosf(theta) * sinf(phi2);
            float y2 = cosf(phi2);
            float z2 = sinf(theta) * sinf(phi2);
            glVertex3f(x2 * radius, y2 * radius, z2 * radius);
        }
        glEnd();
    }
}

void drawOrbitEllipse(float a, float e, int segments = 180) {
    float b = a * sqrtf(1.0f - e * e);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float theta = (2.0f * (float)M_PI * i) / segments;
        float x = a * cosf(theta);
        float z = b * sinf(theta);
        glVertex3f(x, 0.0f, z);
    }
    glEnd();
}


void perspectiveGL(float fovY_deg, float aspect, float zNear, float zFar) {
    float fovY_rad = fovY_deg * (float)M_PI / 180.0f;
    float fH = tanf(fovY_rad / 2.0f) * zNear;
    float fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}


void lookAt(float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ) {

    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;
    float flen = sqrtf(fx * fx + fy * fy + fz * fz);
    if (flen == 0.0f) return;
    fx /= flen; fy /= flen; fz /= flen;

    float ux = upX, uy = upY, uz = upZ;
    float ulen = sqrtf(ux * ux + uy * uy + uz * uz);
    if (ulen == 0.0f) return;
    ux /= ulen; uy /= ulen; uz /= ulen;

    float sx = fy * uz - fz * uy;
    float sy = fz * ux - fx * uz;
    float sz = fx * uy - fy * ux;
    float slen = sqrtf(sx * sx + sy * sy + sz * sz);
    if (slen == 0.0f) return;
    sx /= slen; sy /= slen; sz /= slen;

    float ux2 = sy * fz - sz * fy;
    float uy2 = sz * fx - sx * fz;
    float uz2 = sx * fy - sy * fx;

    float mat[16] = {
        sx, ux2, -fx, 0.0f,
        sy, uy2, -fy, 0.0f,
        sz, uz2, -fz, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    glMultMatrixf(mat);
    glTranslatef(-eyeX, -eyeY, -eyeZ);
}


void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    lookAt(0.0f, 60.0f, 0.01f,   // eye 
        0.0f, 0.0f, 0.0f,     // center
        0.0f, 0.0f, -1.0f);   // up

    // Sun at origin
    glPushMatrix();
    glColor3f(1.0f, 1.0f, 0.0f);
    drawSphere(1.2f, 30, 30);
    glPopMatrix();

    // time in seconds
    float t = (float)glfwGetTime();

    // Draw orbits and planets
    for (size_t i = 0; i < planets.size(); ++i) {
        const Planet& p = planets[i];

        float a = p.a_au * AU_TO_UNITS;
        float e = p.e;
        float b = a * sqrtf(1.0f - e * e);

        // Orbit
        glColor3f(0.45f, 0.45f, 0.45f);
        drawOrbitEllipse(a, e, 240);

        // compute orbital angle from scaled period:
        // yearsPerOrbit = period_days / 365
        // angular speed (rad per simulation second) = (2*pi) / (yearsPerOrbit * SIM_YEARS_TO_SECONDS)
        float yearsPerOrbit = p.period_days / 365.0f;
        float angVel = (2.0f * (float)M_PI) / (yearsPerOrbit * SIM_YEARS_TO_SECONDS);
        float angle = t * angVel; // current angle along ellipse parameterization

        // parametric ellipse position (we param by angle)
        float x = a * cosf(angle);
        float z = b * sinf(angle);

        // draw planet at (x,0,z)
        glPushMatrix();
        glTranslatef(x, 0.0f, z);
        glColor3fv(p.color);
        drawSphere(p.size, 18, 18);
        glPopMatrix();
    }
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (height == 0) 
        height = 1;
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)width / (float)height;
    perspectiveGL(45.0f, aspect, 0.1f, 500.0f);
    glMatrixMode(GL_MODELVIEW);
}

int main() {
    std::cout << "Solar System Simulation\n" << std::endl;

    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return 1;
    }

    // Request an OpenGL 2.1 compatibility context so immediate-mode works
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#if defined(__APPLE__)
	// Programm not compatible with Apple OpenGL
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Solar System - Top Down (GLFW)", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW init error: " << glewGetErrorString(err) << "\n";
        glfwTerminate();
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    
    int bufferWidth, bufferHeight;
    glfwGetFramebufferSize(window, &bufferWidth, &bufferHeight);
    framebuffer_size_callback(window, bufferWidth, bufferHeight);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    while (!glfwWindowShouldClose(window)) {
        renderScene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
	std::cout << "Solar System Simulation finished!" << std::endl;
    return 0;
}
