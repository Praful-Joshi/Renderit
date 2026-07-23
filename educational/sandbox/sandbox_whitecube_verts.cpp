#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static const char* VERTEX_SHADER_PROGRAM = R"glsl(
#version 330 core

layout(location = 0) in vec3 a_position;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec3 v_fragPos;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    gl_Position   = u_projection * u_view * worldPos;
    v_fragPos = worldPos.xyz;
}
)glsl";

static const char* FRAGMENT_SHADER_PROGRAM = R"glsl(
#version 330 core

in vec3 v_fragPos;
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)glsl";

// This is how we define 3D objects in CG. In this case it's a cube.
static const float CUBE_VERTS[] = {
    // FRONT
    -0.5f,-0.5f, 0.5f,
     0.5f,-0.5f, 0.5f,
     0.5f, 0.5f, 0.5f,

     0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f, 0.5f,

    // BACK
    -0.5f,-0.5f,-0.5f,
    -0.5f, 0.5f,-0.5f,
     0.5f, 0.5f,-0.5f,

     0.5f, 0.5f,-0.5f,
     0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f,

    // LEFT
    -0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f,

    -0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,

    // RIGHT
     0.5f, 0.5f, 0.5f,
     0.5f,-0.5f,-0.5f,
     0.5f, 0.5f,-0.5f,

     0.5f,-0.5f,-0.5f,
     0.5f, 0.5f, 0.5f,
     0.5f,-0.5f, 0.5f,

    // TOP
    -0.5f, 0.5f,-0.5f,
    -0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f,

     0.5f, 0.5f, 0.5f,
     0.5f, 0.5f,-0.5f,
    -0.5f, 0.5f,-0.5f,

    // BOTTOM
    -0.5f,-0.5f,-0.5f,
     0.5f,-0.5f,-0.5f,
     0.5f,-0.5f, 0.5f,

     0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,
};

// This function updates the renderable area also known as 
// viewport's size and sets it to w and h.
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
    // --- Creating a window ---

    // Init GLFW - our windowing library
    glfwInit();

    // Some GLFW Settings

    // Tell GLFW that you are using opengl version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // Tell GLFW that you are using core profile of the opengl
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Tell GLFW that the opengl context we will create next needs
    // to be forward compatible
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Some GLFW Settings

    // Create the actual window
    GLFWwindow* window = glfwCreateWindow(900, 700, "OpenGLTest", nullptr, nullptr);

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

    // Enable depth testing in OpenGL, allowing the renderer to use a 
    // depth buffer (z-buffer) to determine which fragments are in front of others.
    // Also our first opengl function call!!!
    glEnable(GL_DEPTH_TEST);

    // Some logs to test if opengl function definitions loaded correctly.
    std::cout << "OpenGL " << glGetString(GL_VERSION)
              << " | " << glGetString(GL_RENDERER) << "\n";

    // --- Transferring model data from CPU to GPU ---
    // Currently our model's data lives in the CPU RAM in an array (CUBE_VERTS)
    // We need to transfer it to the GPU VRAM. For this we need to tell opengl
    // to create an array on the GPU to store our data and return the ID of that array

    // Create an usigned int to store GPU array's ID
    GLuint GPUArrayID;

    // Tell opengl to create the GPU array and return its ID into GPUArrayID
    glGenBuffers(1, &GPUArrayID);
    
    // Before we transfer our data into this newly created array, we need to start
    // recording this process. GPU uses this recording to see exactly where the model's
    // data lies. For this we tell opengl to create an array that records and return its ID.

    GLuint GPURecorderID;
    glGenVertexArrays(1, &GPURecorderID);

    // Start recording because in the next step we are gonna transfer data
    glBindVertexArray(GPURecorderID);

    // Tell opengl to activate the GPU array that will store the data
    glBindBuffer(GL_ARRAY_BUFFER, GPUArrayID);

    // Transfer data to currently activated GPU array
    // p1 - type of data this GPU array will store. In this case, vertex data
    // p2 - size of the vertex data
    // p3 - the actual vertex data
    // p4 - tell GPU that this data wont change at all and you will need to draw it often
    // in our case, every frame,
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTS), CUBE_VERTS, GL_STATIC_DRAW);

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
                        3*sizeof(float),
                        (void*)0);

    // Tell opengl to make the 0 position active
    glEnableVertexAttribArray(0);
    // Data transfer complete, stop our recorder
    glBindVertexArray(0);

    // ── Camera ────────────────────────────────────────────────────────────────
    glm::vec3 cameraPos  = glm::vec3(4.0f, 2.5f, 3.0f);
    glm::mat4 view       = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0,1,0));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                            (float)900/700, 0.1f, 100.0f);
    
    // Compile both the vertex and shader code and return the ID of the compiled program                                        
    GLuint shaderProgram   = buildShaderProgram(VERTEX_SHADER_PROGRAM,   FRAGMENT_SHADER_PROGRAM);

    // Get access to constants defined in the vertex shader so we can pass data to it
    GLint shaderModelMatrix  = glGetUniformLocation(shaderProgram, "u_model");
    GLint shaderViewMatrix   = glGetUniformLocation(shaderProgram, "u_view");
    GLint shaderProjectionMatrix   = glGetUniformLocation(shaderProgram, "u_projection");

    // run this loop till the GLFW window is closed
    while (!glfwWindowShouldClose(window))
    {
        // Close the window if esc key is pressed 
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        // Tell opengl which bg color to use for the renderable area
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        // Tell opengl to clear the renderable area and apply the color we set above.
        // Also tell opengl to clear the z-buffer data
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Tell opengl to use shaderProgram for following instructions
        glUseProgram(shaderProgram);

        // Send data to vertex constants
        glUniformMatrix4fv(shaderModelMatrix, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniformMatrix4fv(shaderViewMatrix,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(shaderProjectionMatrix,  1, GL_FALSE, glm::value_ptr(projection));

        // Tell opengl to watch the recording that we did previously and draw accordingly
        glBindVertexArray(GPURecorderID);

        // Draw 36 recorder vertices as triangles.
        // p1 - type of primitive to draw, in this case triangles
        // p2 - index of where our data starts, in our case 0th index as we have only 1 object
        // p3 - number of vertices to draw
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Drawing complete. Stop watching recording and forget the shader
        glBindVertexArray(0);
        glUseProgram(0);

        // Clear the current buffer and bring the back buffer forward
        glfwSwapBuffers(window);

        // Tell opengl to listen for input
        glfwPollEvents();
    }
    // Close the glfw window
    glfwTerminate();
    return 0;
}