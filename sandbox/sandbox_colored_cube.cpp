#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define COLOR_RED       1.0f,0.0f,0.0f
#define COLOR_GREEN     0.0f,1.0f,0.0f
#define COLOR_BLUE      0.0f,0.0f,1.0f
#define COLOR_YELLOW    1.0f,1.0f,0.0f
#define COLOR_ORANGE    0.9f,0.3f,0.1f
#define COLOR_WHITE     0.9f,0.9f,0.9f

#define VERT0          -0.5f, -0.5f,  0.5f
#define VERT1           0.5f, -0.5f,  0.5f
#define VERT2           0.5f,  0.5f,  0.5f
#define VERT3          -0.5f,  0.5f,  0.5f
#define VERT4          -0.5f, -0.5f, -0.5f
#define VERT5           0.5f, -0.5f, -0.5f
#define VERT6           0.5f,  0.5f, -0.5f
#define VERT7          -0.5f,  0.5f, -0.5f

const int WINDOW_WIDTH = 900, WINDOW_HEIGHT = 700;

// This is how we define 3D objects in CG. In this case it's a cube.
GLfloat CUBE_VERTS[] = {
    // FRONT (red)
    VERT0, COLOR_RED,      //0
    VERT1, COLOR_RED,      //1
    VERT2, COLOR_RED,      //2
    VERT3, COLOR_RED,      //3

    // BACK (cyan)
    VERT4, COLOR_WHITE,    //4
    VERT5, COLOR_WHITE,    //5
    VERT6, COLOR_WHITE,    //6
    VERT7, COLOR_WHITE,    //7

    // LEFT (yellow)
    VERT4, COLOR_YELLOW,   //8
    VERT0, COLOR_YELLOW,   //9
    VERT3, COLOR_YELLOW,   //10
    VERT7, COLOR_YELLOW,   //11 

    // RIGHT (green)
    VERT5, COLOR_GREEN,    //12
    VERT1, COLOR_GREEN,    //13
    VERT2, COLOR_GREEN,    //14
    VERT6, COLOR_GREEN,    //15

    // TOP (blue)
    VERT3, COLOR_BLUE,     //16
    VERT2, COLOR_BLUE,     //17
    VERT6, COLOR_BLUE,     //18
    VERT7, COLOR_BLUE,     //19

    // BOTTOM (violet)
    VERT0, COLOR_ORANGE,   //20
    VERT1, COLOR_ORANGE,   //21
    VERT5, COLOR_ORANGE,   //22
    VERT4, COLOR_ORANGE,   //23
};

GLuint CUBE_INDICES[] = {
    0,1,2, 2,3,0,           //front face
    4,5,6, 6,7,4,           //back face
    8,9,10, 10,11,8,        //left face
    12,13,14, 14,15,12,     //right face
    16,17,18, 18,19,16,     //top face
    20,21,22, 22,23,20      //bottom face
};

// This function updates the renderable area also known as 
// viewport. Sets its size to w and h.
static void onResize(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "[Shader compile error]\n" << log << "\n";
    }
    return s;
}

static GLuint buildShaderProgram(const char* vert, const char* frag) {
    GLuint v = compileShader(GL_VERTEX_SHADER,   vert);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, frag);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(p, 512, nullptr, log);
        std::cerr << "[Program link error]\n" << log << "\n";
    }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

int main()
{
//NOTE --- GLFW & GLAD Init, Create a window ---

    // Init GLFW - our windowing library
    glfwInit();

    // Tell GLFW that you are using opengl version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // Tell GLFW that you are using core profile of the opengl
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Tell GLFW that the opengl context we will create next needs
    // to be forward compatible
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create the actual window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGLTest", nullptr, nullptr);

    // Set the window's context to current opengl context.
    glfwMakeContextCurrent(window);

    // Tell GLFW to call onResize function when the window
    // size changes. Passes the updated w,h to onResize(). 
    glfwSetFramebufferSizeCallback(window, onResize);

    // Tell GLFW to turn on vsync. That means the framerate of 
    // our program will match the display's frame rate.
    glfwSwapInterval(1);

    // Load opengl function definitions using GLAD
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Some logs to test if opengl function definitions loaded correctly.
    std::cout << "OpenGL " << glGetString(GL_VERSION)
              << " | " << glGetString(GL_RENDERER) << "\n";

    // Enable depth testing in OpenGL, allowing the renderer to use a 
    // depth buffer (z-buffer) to determine which fragments are in front of others.
    // Also our first opengl function call!!!
    glEnable(GL_DEPTH_TEST);

//NOTE --- GLFW & GLAD Init, Create a window ---


//NOTE --- Transferring model data from CPU to GPU, describing layout of the vertex data ---

    // Currently our model's data lives in the CPU RAM in an array (CUBE_VERTS)
    // We need to transfer it to the GPU VRAM. For this we need to tell opengl
    // to create an array on the GPU to store our data and return the ID of that array
    // Create an usigned int to store GPU array's ID
    GLuint GPUVertsArrayID;

    // Tell opengl to create the GPU array and return its ID into GPUArrayID
    glGenBuffers(1, &GPUVertsArrayID);
    
    // We can draw the same cube by defining only 8 vertices instead of 36 and using 
    // an indices array to tell the GPU how to use the same 8 verrtices to build 12 indices
    // of the 12 triangles we need to draw for 6 faces of the cube. 1 square face = 2 tris.

    // Create an indices array on the GPU and store its ID
    GLuint GPUIndsArrayID;
    glGenBuffers(1, &GPUIndsArrayID);

    // Before we transfer our data into this newly created array, we need to start
    // recording this process. GPU uses this recording to see exactly where the model's
    // data lies. For this we tell opengl to create an array that records and return its ID.

    GLuint GPURecorderID;
    glGenVertexArrays(1, &GPURecorderID);

    // Start recording because in the next step we are gonna transfer data
    glBindVertexArray(GPURecorderID);

    // Tell opengl to activate the GPU array that will store the data
    glBindBuffer(GL_ARRAY_BUFFER, GPUVertsArrayID);

    // Transfer data to currently activated GPU array
    // p1 - type of data this GPU array will store. In this case, vertex data
    // p2 - size of the vertex data
    // p3 - the actual vertex data
    // p4 - tell GPU that this data wont change at all and you will need to draw it often
    // in our case, every frame,
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTS), CUBE_VERTS, GL_STATIC_DRAW);

    // activate the indices buffer too
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GPUIndsArrayID);

    // send indices data to the buffer. GPU uses this data to index into the CUBE_VERTS and get 36 verts
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES, GL_STATIC_DRAW);

    // Our vertex shader will store the position, normal and UV coordinates from
    // the vertex data separately. Each one gets a variable. Each of these variables
    // gets an integer ID like 0, 1, 2 etc for ease of use
    // We need to tell opengl which vertex shader variable ID gets what data
    // Tell opengl about ID 0 which will store positions of the vertex
    // p1 - vertex shader variable ID
    // p2 - number of elements this ID will hold
    // p3 - type of each of those elements
    // p4 - after how many bites will this data appear again
    // p5 - does it start immediately or has some offset
    glVertexAttribPointer(0,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        6*sizeof(float),
                        (void*)0);
    
    glVertexAttribPointer(1,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        6*sizeof(float),
                        reinterpret_cast<void*>(3 * sizeof(float)));

    // Tell opengl to make the 0 position active
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Data transfer complete, stop our recorder
    glBindVertexArray(0);

//NOTE --- Transferring model data from CPU to GPU, describing layout of the vertex data ---


//NOTE --- Creating MVP matrices ---

    // Model Matrix. We are gonna put the cube at the origin of the world space so we can use
    // an identity matrix as a model matrix.
    glm::mat4 model = glm::mat4(1.0f);

    // View Matrix. To create a view matrix, we need to specify the location of our camera in the
    // world space, the point where our camera will look at and the 'up' direction to specify the 
    // rotation of the camera.
    glm::vec3 cameraPos         = glm::vec3(4.0f, 2.5f, -3.0f);
    glm::vec3 cameraLookAtPos   = glm::vec3(0.0f);
    glm::vec3 cameraUpDirection = glm::vec3(0, 1, 0);
    glm::mat4 view              = glm::lookAt(cameraPos, cameraLookAtPos, cameraUpDirection);


    // Projection matrix. To take a picture from the camera or to project the view on our screen,
    // we need to define the field of view of our camera, the aspect ratio we want the picture in and
    // the near and far distances our camera can capture.
    float fieldOfView           = glm::radians(45.0f);
    float aspectRatio           = (float)WINDOW_WIDTH/WINDOW_HEIGHT; // We are using the same aspect ratio as our window
    float cameraNearDistance    = 0.1f;
    float cameraFarDistance     = 100.0f;
    glm::mat4 projection        = glm::perspective(fieldOfView, aspectRatio, cameraNearDistance, cameraFarDistance);

//NOTE --- Creating MVP matrices ---

    
//NOTE --- Compiling shaders and fetching uniform locations ---    

    GLuint shaderProgram   = buildShaderProgram(VERTEX_SHADER_PROGRAM,   FRAGMENT_SHADER_PROGRAM);

    // Get access to constants defined in the vertex shader so we can pass data to it
    GLint shaderModelMatrix  = glGetUniformLocation(shaderProgram, "u_model");
    GLint shaderViewMatrix   = glGetUniformLocation(shaderProgram, "u_view");
    GLint shaderProjectionMatrix   = glGetUniformLocation(shaderProgram, "u_projection");

//NOTE --- Compiling shaders and fetching uniform locations ---  


//NOTE --- Render loop ---

    while (!glfwWindowShouldClose(window))
    {
        // Close the window if esc key is pressed 
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
    //NOTE --- Some window settings ---

        // Tell opengl which bg color to use for the renderable area
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        // Tell opengl to clear the renderable area and apply the color we set above.
        // Also tell opengl to clear the z-buffer data
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //NOTE --- Some window settings ---


    //NOTE --- Draw call ---

        // Tell opengl to use our compiled shader program for the next draw call
        glUseProgram(shaderProgram);

        // Pass MVC to vertex shader
        glUniformMatrix4fv(shaderModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(shaderViewMatrix,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(shaderProjectionMatrix,  1, GL_FALSE, glm::value_ptr(projection));

        // Tell opengl to watch the recording that we did previously and draw accordingly
        glBindVertexArray(GPURecorderID);

        // This draw call goes through each index in the index array, uses that index to get a vertex from the 
        // verts array. When it gets 3 vertices, it draws a triangle.
        // p1 - type of primitive to draw, in this case triangles
        // p2 - total number of indices
        // p3 - data type of each index
        // p4 - offset 
        // Draw using element array
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // Drawing complete. Stop watching recording and forget the shader
        glBindVertexArray(0);
        glUseProgram(0);

    //NOTE --- Draw call ---

        // Clear the current buffer and bring the back buffer forward
        glfwSwapBuffers(window);

        // Tell opengl to listen for input
        glfwPollEvents();
    }

//NOTE --- Render loop ---


//NOTE --- Program Termination ---

    // Close the glfw window
    glfwTerminate();
    return 0;

//NOTE --- Program Termination ---

}

static const char* VERTEX_SHADER_PROGRAM = R"glsl(
#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec3 v_fragPos;
out vec3 v_fragColor;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    gl_Position   = u_projection * u_view * worldPos;
    v_fragPos = worldPos.xyz;
    v_fragColor = a_color;
}
)glsl";

static const char* FRAGMENT_SHADER_PROGRAM = R"glsl(
#version 330 core

in vec3 v_fragPos;
in vec3 v_fragColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(v_fragColor, 1.0);
}
)glsl";