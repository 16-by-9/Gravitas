#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <iomanip>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos;
uniform vec3 viewPos;
out vec3 Normal;
out vec3 FragPos;
out vec3 LightPos;
out vec3 ViewPos;
void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aPos;
    gl_Position = projection * view * vec4(FragPos, 1.0);
    LightPos = lightPos;
    ViewPos = viewPos;
})glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
in vec3 Normal;
in vec3 FragPos;
in vec3 LightPos;
in vec3 ViewPos;
out vec4 FragColor;
uniform vec4 objectColor;
uniform bool isGrid;
uniform bool GLOW;
uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform float shininess;
void main() {
    if (isGrid) {
        FragColor = objectColor;
    } else if(GLOW){
        FragColor = vec4(objectColor.rgb * 100000, objectColor.a);
    }else {
        // Ambient
        vec3 ambient = ambientColor * vec3(objectColor);

        // Diffuse
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(LightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diffuseColor * diff * vec3(objectColor);

        // Specular
        vec3 viewDir = normalize(ViewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        vec3 specular = specularColor * spec * vec3(objectColor);

        FragColor = vec4(ambient + diffuse + specular, objectColor.a);
    }})glsl";

bool running = true;
bool pause = true;
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  1.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
float lastX = 400.0, lastY = 300.0;
float yaw = -90;
float pitch =0.0;
float deltaTime = 0.0;
float lastFrame = 0.0;
bool firstMouse = true;
bool leftMouseButtonPressed = false; // New global variable to track left mouse button state

const double G = 6.6743e-11; // m^3 kg^-1 s^-2
const float c = 299792458.0;
float initMass = float(pow(10, 22));
float sizeRatio = 30000.0f;

GLFWwindow* StartGLU();
GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource);
void CreateVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount);
void UpdateCam(GLuint shaderProgram, glm::vec3 cameraPos);
void processInput(GLFWwindow* window);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
glm::vec3 sphericalToCartesian(float r, float theta, float phi);
void DrawGrid(GLuint shaderProgram, GLuint gridVAO, size_t vertexCount);

class Object {
    public:
        GLuint VAO, VBO;
        glm::vec3 position = glm::vec3(400, 300, 0);
        glm::vec3 velocity = glm::vec3(0, 0, 0);
        size_t vertexCount;
        glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

        bool Initalizing = false;
        bool Launched = false;
        bool target = false;

        float mass;
        float density;  // kg / m^3  HYDROGEN
        float radius;

        glm::vec3 LastPos = position;
        bool glow;

        Object(glm::vec3 initPosition, glm::vec3 initVelocity, float mass, float density = 3344, glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), bool Glow = false) {   
            this->position = initPosition;
            this->velocity = initVelocity;
            this->mass = mass;
            this->density = density;
            this->radius = pow(((3 * this->mass/this->density)/(4 * 3.14159265359)), (1.0f/3.0f)) / sizeRatio;
            this->color = color;
            this->glow = Glow;
            

            // generate vertices (centered at origin)
            std::vector<float> vertices = Draw();
            vertexCount = vertices.size();

            CreateVBOVAO(VAO, VBO, vertices.data(), vertexCount);
        }

        std::vector<float> Draw() {
            std::vector<float> vertices;
            int stacks = 10;
            int sectors = 10;

            // generate circumference points using integer steps
            for(float i = 0.0f; i <= stacks; ++i){
                float theta1 = (i / stacks) * glm::pi<float>();
                float theta2 = (i+1) / stacks * glm::pi<float>();
                for (float j = 0.0f; j < sectors; ++j){
                    float phi1 = j / sectors * 2 * glm::pi<float>();
                    float phi2 = (j+1) / sectors * 2 * glm::pi<float>();
                    glm::vec3 v1 = sphericalToCartesian(this->radius, theta1, phi1);
                    glm::vec3 v2 = sphericalToCartesian(this->radius, theta1, phi2);
                    glm::vec3 v3 = sphericalToCartesian(this->radius, theta2, phi1);
                    glm::vec3 v4 = sphericalToCartesian(this->radius, theta2, phi2);

                    // Triangle 1: v1-v2-v3
                    vertices.insert(vertices.end(), {v1.x, v1.y, v1.z}); //      /|
                    vertices.insert(vertices.end(), {v2.x, v2.y, v2.z}); //     / |
                    vertices.insert(vertices.end(), {v3.x, v3.y, v3.z}); //    /__|
                    
                    // Triangle 2: v2-v4-v3
                    vertices.insert(vertices.end(), {v2.x, v2.y, v2.z});
                    vertices.insert(vertices.end(), {v4.x, v4.y, v4.z});
                    vertices.insert(vertices.end(), {v3.x, v3.y, v3.z});
                }   
            }
            return vertices;
        }
        
        void UpdatePos(){
            this->position[0] += this->velocity[0] / 94;
            this->position[1] += this->velocity[1] / 94;
            this->position[2] += this->velocity[2] / 94;
            this->radius = pow(((3 * this->mass/this->density)/(4 * 3.14159265359)), (1.0f/3.0f)) / sizeRatio;
        }
        void UpdateVertices() {
            // generate new vertices with current radius
            std::vector<float> vertices = Draw();
            
            // update VBO with new vertex data
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        }
        glm::vec3 GetPos() const {
            return this->position;
        }
        void accelerate(float x, float y, float z){
            this->velocity[0] += x / 96;
            this->velocity[1] += y / 96;
            this->velocity[2] += z / 96;
        }
        float CheckCollision(const Object& other) {
            float dx = other.position[0] - this->position[0];
            float dy = other.position[1] - this->position[1];
            float dz = other.position[2] - this->position[2];
            float distance = std::pow(dx*dx + dy*dy + dz*dz, (1.0f/2.0f));
            if (other.radius + this->radius > distance){
                return -0.2f;
            }
            return 1.0f;
        }
};
std::vector<Object> objs = {};

std::vector<float> CreateGridVertices(float size, int divisions, const std::vector<Object>& objs);
std::vector<float> UpdateGridVertices(std::vector<float> vertices, const std::vector<Object>& objs);

GLuint gridVAO, gridVBO;


int main() {
    GLFWwindow* window = StartGLU();
    GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    GLint ambientColorLoc = glGetUniformLocation(shaderProgram, "ambientColor");
    GLint diffuseColorLoc = glGetUniformLocation(shaderProgram, "diffuseColor");
    GLint specularColorLoc = glGetUniformLocation(shaderProgram, "specularColor");
    GLint shininessLoc = glGetUniformLocation(shaderProgram, "shininess");

    glUseProgram(shaderProgram);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback); // Set the mouse button callback
    glfwSetKeyCallback(window, keyCallback); // Set the key callback

    //projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 750000.0f);
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    cameraPos = glm::vec3(0.0f, 1000.0f, 5000.0f);

    
    objs = {
        // Sun
		Object(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), 1.989 * pow(10, 25), 1414, glm::vec4(1.0f, 0.929f, 0.176f, 1.0f), true),
		
	    // Mars
		Object(glm::vec3(-3000, 650, 0), glm::vec3(0, 0, 500), 5.97219 * pow(10, 23), 5515, glm::vec4(1.0f, 0.25f, 0.56f, 1.0f)),
		
		// Earth
		Object(glm::vec3(5000, 650, 0), glm::vec3(0, 0, -500), 5.97219 * pow(10, 23), 5515, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f)),
		
		// Moon
		Object(glm::vec3(5250, 650, 0), glm::vec3(0, 0, -50), 5.97219 * pow(10, 21), 5515, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)),
		
		// Jupiter
		Object(glm::vec3(0, 500, 9000), glm::vec3(-500, 50, 0), 5.97219 * pow(10, 23.5), 5515, glm::vec4(1.0f, 0.5f, 0.15f, 1.0f)),
		Object(glm::vec3(0, 550, 9500), glm::vec3(0, 0, -50), 5.97219 * pow(10, 21), 5515, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)),
		Object(glm::vec3(0, 450, 8500), glm::vec3(0, 0, -50), 5.97219 * pow(10, 21), 5515, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)),
		Object(glm::vec3(100, 500, 9000), glm::vec3(50, 0, 0), 5.97219 * pow(10, 21), 5515, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)),
		
		// Neptune
		Object(glm::vec3(0, -500, -10500), glm::vec3(-350, 50, 0), 5.97219 * pow(10, 23.5), 5515, glm::vec4(0.35f, 0.85f, 0.99f, 1.0f)),
		Object(glm::vec3(350, -450, -10500), glm::vec3(0, 0, -550), 5.97219 * pow(10, 21), 5515, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)),
		Object(glm::vec3(-350, -450, -10500), glm::vec3(0, 0, -550), 5.97219 * pow(10, 21), 5515, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)),
		Object(glm::vec3(0, -450, -11050), glm::vec3(-550, 0, 0), 5.97219 * pow(10, 21), 5515, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)),

    };
	
    std::vector<float> gridVertices = CreateGridVertices(20000.0f, 25, objs);
    CreateVBOVAO(gridVAO, gridVBO, gridVertices.data(), gridVertices.size());

    // Define light properties
    glm::vec3 lightPos(0.0f, 0.0f, 0.0f); // Sun's position
    glm::vec3 ambientColor(0.1f, 0.1f, 0.1f);
    glm::vec3 diffuseColor(0.8f, 0.8f, 0.8f);
    glm::vec3 specularColor(1.0f, 1.0f, 1.0f);
    float shininess = 32.0f;

    while (!glfwWindowShouldClose(window) && running == true) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui Window for Camera Data and Simulation Controls
        ImGui::Begin("Gravitas Controls");
        ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", cameraPos.x, cameraPos.y, cameraPos.z);
        ImGui::Text("Camera Front: (%.2f, %.2f, %.2f)", cameraFront.x, cameraFront.y, cameraFront.z);
        ImGui::Text("Camera Yaw: %.2f", yaw);
        ImGui::Text("Camera Pitch: %.2f", pitch);
        ImGui::Separator();

        if (ImGui::Button(pause ? "Resume Simulation" : "Pause Simulation")) {
            pause = !pause;
        }
        ImGui::SameLine();
        if (ImGui::Button("Exit")) {
            running = false;
        }
        ImGui::Separator();

        ImGui::Text("Planetary Data:");
        for (size_t i = 0; i < objs.size(); ++i) {
            ImGui::PushID(i);
            ImGui::Text("Object %zu:", i);
            ImGui::Text("  Position: (%.2f, %.2f, %.2f)", objs[i].position.x, objs[i].position.y, objs[i].position.z);
            ImGui::Text("  Velocity: (%.2f, %.2f, %.2f)", objs[i].velocity.x, objs[i].velocity.y, objs[i].velocity.z);
            ImGui::Text("  Mass: %.2e kg", objs[i].mass);
            ImGui::Text("  Radius: %.2f units", objs[i].radius);
            ImGui::PopID();
        }

        ImGui::End();

        processInput(window); // Call the new input processing function
        UpdateCam(shaderProgram, cameraPos);

        // Set light uniforms
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
        glUniform3fv(ambientColorLoc, 1, glm::value_ptr(ambientColor));
        glUniform3fv(diffuseColorLoc, 1, glm::value_ptr(diffuseColor));
        glUniform3fv(specularColorLoc, 1, glm::value_ptr(specularColor));
        glUniform1f(shininessLoc, shininess);

        // Draw the grid
        glUseProgram(shaderProgram);
        glUniform4f(objectColorLoc, 1.0f, 1.0f, 1.0f, 0.25f);
        glUniform1i(glGetUniformLocation(shaderProgram, "isGrid"), 1);
        glUniform1i(glGetUniformLocation(shaderProgram, "GLOW"), 0);
        gridVertices = UpdateGridVertices(gridVertices, objs);
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_DYNAMIC_DRAW);
        DrawGrid(shaderProgram, gridVAO, gridVertices.size());

        // Draw the triangles / sphere
        for(auto& obj : objs) {
            glUniform4f(objectColorLoc, obj.color.r, obj.color.g, obj.color.b, obj.color.a);

            for(auto& obj2 : objs){
                if(&obj2 != &obj && !obj.Initalizing && !obj2.Initalizing){
                    float dx = obj2.GetPos()[0] - obj.GetPos()[0];
                    float dy = obj2.GetPos()[1] - obj.GetPos()[1];
                    float dz = obj2.GetPos()[2] - obj.GetPos()[2];
                    float distance = sqrt(dx * dx + dy * dy + dz * dz);

                    if (distance > 0) {
                        std::vector<float> direction = {dx / distance, dy / distance, dz / distance};
                        distance *= 1000;
                        double Gforce = (G * obj.mass * obj2.mass) / (distance * distance);
                        

                        float acc1 = Gforce / obj.mass;
                        std::vector<float> acc = {direction[0] * acc1, direction[1]*acc1, direction[2]*acc1};
                        if(!pause){
                            obj.accelerate(acc[0], acc[1], acc[2]);
                        }

                        //collision
                        obj.velocity *= obj.CheckCollision(obj2);
                        // std::cout<<"radius: "<<obj.radius<<std::endl;
                    }
                }
            }
            if(obj.Initalizing){
                obj.radius = pow(((3 * obj.mass/obj.density)/(4 * 3.14159265359)), (1.0f/3.0f)) / sizeRatio;
                obj.UpdateVertices();
            }

            //update positions
            if(!pause){
                obj.UpdatePos();
            }
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position); // apply position
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(glGetUniformLocation(shaderProgram, "isGrid"), 0);
            if(obj.glow){
                glUniform1i(glGetUniformLocation(shaderProgram, "GLOW"), 1);
            } else {
                glUniform1i(glGetUniformLocation(shaderProgram, "GLOW"), 0);
            }
            
            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount / 3);
        }
        
        // ImGui rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    for (auto& obj : objs) {
        glDeleteVertexArrays(1, &obj.VAO);
        glDeleteBuffers(1, &obj.VBO);
    }

    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);

    glDeleteProgram(shaderProgram);
    glfwTerminate();

    glfwTerminate();
    return 0;
}

GLFWwindow* StartGLU() {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW, panic" << std::endl;
        return nullptr;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Gravitas Simulation™", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}

GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource) {
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }

    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }

    // Shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void CreateVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(float), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void UpdateCam(GLuint shaderProgram, glm::vec3 cameraPos) {
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void processInput(GLFWwindow* window) {
    if (ImGui::GetIO().WantCaptureKeyboard) {
        return; // ImGui is handling the keyboard input
    }

    float cameraSpeed =  2500.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraUp;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Pass key events to ImGui
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    // Only process if ImGui is not capturing keyboard and key is pressed (not held down)
    if (!ImGui::GetIO().WantCaptureKeyboard && action == GLFW_PRESS) {
        if (key == GLFW_KEY_P) {
            pause = !pause;
        }
        if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) {
            running = false;
        }
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // Pass mouse events to ImGui
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    if (ImGui::GetIO().WantCaptureMouse) {
        return; // ImGui is handling the mouse input
    }

    if (leftMouseButtonPressed) { // Only process mouse movement if left button is pressed
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top
        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if(pitch > 89.0f) pitch = 89.0f;
        if(pitch < -89.0f) pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
    // Pass mouse events to ImGui
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) {
        return; // ImGui is handling the mouse input
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            leftMouseButtonPressed = true;
            firstMouse = true; // Reset firstMouse when button is pressed to avoid jump
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Disable cursor for camera control
        } else if (action == GLFW_RELEASE) {
            leftMouseButtonPressed = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Enable cursor when button is released
        }
    }
    // Right mouse button functionality is removed as requested
    // Original left click functionality (spawn object) is also removed to prioritize camera dragging
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    // Pass scroll events to ImGui
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (ImGui::GetIO().WantCaptureMouse) {
        return; // ImGui is handling the scroll input
    }

    float cameraSpeed =  2500.0f * deltaTime;
    if(yoffset>0){
        cameraPos += cameraSpeed * cameraFront;
    } else if(yoffset<0){
        cameraPos -= cameraSpeed * cameraFront;
    }
}

glm::vec3 sphericalToCartesian(float r, float theta, float phi){
    float x = r * sin(theta) * cos(phi);
    float y = r * cos(theta);
    float z = r * sin(theta) * sin(phi);
    return glm::vec3(x, y, z);
};
void DrawGrid(GLuint shaderProgram, GLuint gridVAO, size_t vertexCount) {
    glUseProgram(shaderProgram);
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix for the grid
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(gridVAO);
    glPointSize(5.0f);
    glDrawArrays(GL_LINES, 0, vertexCount / 3);
    glBindVertexArray(0);
}
std::vector<float> CreateGridVertices(float size, int divisions, const std::vector<Object>& objs) {
    std::vector<float> vertices;
    float step = size / divisions;
    float halfSize = size / 2.0f;

    // x axis
    for (int yStep = 0; yStep <= divisions; ++yStep) { // Changed yStep loop to cover full range
        float y = -halfSize + yStep * step;
        for (int zStep = 0; zStep <= divisions; ++zStep) {
            float z = -halfSize + zStep * step;
            for (int xStep = 0; xStep < divisions; ++xStep) {
                float xStart = -halfSize + xStep * step;
                float xEnd = xStart + step;
                vertices.push_back(xStart); vertices.push_back(y); vertices.push_back(z);
                vertices.push_back(xEnd);   vertices.push_back(y); vertices.push_back(z);
            }
        }
    }
    // zaxis
    for (int xStep = 0; xStep <= divisions; ++xStep) {
        float x = -halfSize + xStep * step;
        for (int yStep = 0; yStep <= divisions; ++yStep) { // Changed yStep loop to cover full range
            float y = -halfSize + yStep * step;
            for (int zStep = 0; zStep < divisions; ++zStep) {
                float zStart = -halfSize + zStep * step;
                float zEnd = zStart + step;
                vertices.push_back(x); vertices.push_back(y); vertices.push_back(zStart);
                vertices.push_back(x); vertices.push_back(y); vertices.push_back(zEnd);
            }
        }
    }

    return vertices;
}

std::vector<float> UpdateGridVertices(std::vector<float> vertices, const std::vector<Object>& objs){
    
    // centre of mass calc
    float totalMass = 0.0f;
    float comY = 0.0f;
    for (const auto& obj : objs) {
        if (obj.Initalizing) continue;
        comY += obj.mass * obj.position.y;
        totalMass += obj.mass;
    }
    if (totalMass > 0) comY /= totalMass;
    
    float originalMaxY = -std::numeric_limits<float>::infinity();
    for (int i = 0; i < vertices.size(); i += 3) {
        originalMaxY = std::max(originalMaxY, vertices[i+1]);
    }

    float verticalShift = comY - originalMaxY;
    std::cout<<"vertical shift: "<<verticalShift<<" |         comY: "<<comY<<"|            originalmaxy: "<<originalMaxY<<std::endl;


    for (int i = 0; i < vertices.size(); i += 3) {

        // mass bending space
        glm::vec3 vertexPos(vertices[i], vertices[i+1], vertices[i+2]);
        glm::vec3 totalDisplacement(0.0f);
        for (const auto& obj : objs) {
            //f (obj.Initalizing) continue;

            glm::vec3 toObject = obj.GetPos() - vertexPos;
            float distance = glm::length(toObject);
            float distance_m = distance * 1000.0f;
            float rs = (2*G*obj.mass)/(c*c);

            float dz = 2 * sqrt(rs * (distance_m - rs));
            totalDisplacement.y += dz * 2.0f;
        }
        vertices[i+1] = totalDisplacement.y + -abs(verticalShift);
    }

    return vertices;
}