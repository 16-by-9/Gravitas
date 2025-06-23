// Gravitas
// Original framework by kavan010, heavily modified and extended
// Features: ImGui interface, collision system, preset scenarios, trail rendering
// Author: 16-by-9 - 2025

#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <vector>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <deque>
#include <fstream>
#include <sstream>
#include "Mesh.h"

// Physics Constants
namespace Physics {
    constexpr double G = 6.6743e-11;           // Gravitational constant (m^3 kg^-1 s^-2)
    constexpr float LIGHT_SPEED = 299792458.0f; // Speed of light (m/s)
    constexpr float TIME_SCALE = 94.0f;         // Time scaling factor
    constexpr float ACCELERATION_DAMPING = 96.0f;
    constexpr float SIZE_RATIO = 30000.0f;      // Size scaling for visual representation
}

// Rendering Constants
namespace Rendering {
    constexpr int WINDOW_WIDTH    = 1200;
    constexpr int WINDOW_HEIGHT   = 800;
    constexpr float FOV           = 45.0f;
    constexpr float NEAR_PLANE    = 0.1f;
    constexpr float FAR_PLANE     = 750000.0f;
    constexpr int SPHERE_STACKS   = 12;
    constexpr int SPHERE_SECTORS  = 12;

    // Grid constants
    constexpr int   GRID_SIZE     = 25;
    constexpr float GRID_SPACING  = 20000.0f / GRID_SIZE;
}

class CelestialBody;

// Shader Sources
namespace Shaders {
    const char* VERTEX_SHADER = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out float lightIntensity;
out vec3 worldPos;

void main() {
    vec4 worldPosition = model * vec4(aPos, 1.0);
    worldPos = worldPosition.xyz;
    gl_Position = projection * view * worldPosition;
    
    vec3 normal = normalize(aPos);
    vec3 dirToCenter = normalize(-worldPos);
    lightIntensity = max(dot(normal, dirToCenter), 0.15);
}
)glsl";

    const char* FRAGMENT_SHADER = R"glsl(
#version 330 core
in float lightIntensity;
in vec3 worldPos;
out vec4 FragColor;

uniform vec4 objectColor;
uniform bool isGrid;
uniform bool isGlowing;
uniform bool hasTrail;
uniform float glowIntensity;

void main() {
    if (isGrid) {
        FragColor = objectColor;
    } else if (isGlowing) {
        float glow = glowIntensity * 2.0;
        FragColor = vec4(objectColor.rgb * glow, objectColor.a);
    } else {
        float fade = smoothstep(0.0, 1.0, lightIntensity);
        vec3 finalColor = objectColor.rgb * (0.3 + 0.7 * fade);
        FragColor = vec4(finalColor, objectColor.a);
    }
}
)glsl";

    const char* TRAIL_VERTEX_SHADER = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in float aAge;
uniform mat4 view;
uniform mat4 projection;
out float trailAge;

void main() {
    gl_Position = projection * view * vec4(aPos, 1.0);
    trailAge = aAge;
}
)glsl";

    const char* TRAIL_FRAGMENT_SHADER = R"glsl(
#version 330 core
in float trailAge;
out vec4 FragColor;
uniform vec4 trailColor;

void main() {
    float alpha = 1.0 - trailAge;
    FragColor = vec4(trailColor.rgb, trailColor.a * alpha * alpha);
}
)glsl";
}

// Collision result enum
enum class CollisionType {
    NONE,
    ELASTIC,
    INELASTIC,
    MERGE
};

// Simulation presets
enum class SimulationPreset {
    EMPTY,
    SOLAR_SYSTEM,
    BINARY_STARS,
    GALAXY_COLLISION,
    CUSTOM
};

// Trail point structure
struct TrailPoint {
    glm::vec3 position;
    float age;
    
    TrailPoint(const glm::vec3& pos, float a = 0.0f) : position(pos), age(a) {}
};

// Enhanced celestial body class
class CelestialBody {
public:
    // Physical properties
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec3 acceleration{0.0f};
    float mass = 1e22f;
    float density = 3344.0f;
    float radius = 1.0f;
    
    // Visual properties
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    bool isGlowing = false;
    float glowIntensity = 1.0f;
    bool showTrail = true;
    
    // State flags
    bool isBeingCreated = true;
    bool isFixed = false;  // For fixed objects like central stars
    bool isDestroyed = false;
    bool isPaused = false;
    bool showGrid = true;
    bool enableCollisions = true;
    bool enableRelativisticEffects = false;
    float timeScale = 1.0f;
    float gravitationalConstant = Physics::G;
    
    // Trail system
    std::deque<TrailPoint> trail;
    static constexpr size_t MAX_TRAIL_POINTS = 1000;
    static constexpr float TRAIL_UPDATE_INTERVAL = 0.1f;
    float trailTimer = 0.0f;
    
    // Identification
    std::string name = "Unnamed Body";
    size_t id;
    
    CelestialBody(const glm::vec3& pos, const glm::vec3& vel, float m, 
                  float d = 3344.0f, const glm::vec4& c = glm::vec4(1.0f), 
                  const std::string& n = "Body");
    
    ~CelestialBody();
    
    // Physics methods
    void updatePhysics(float deltaTime);
    void applyForce(const glm::vec3& force);
    CollisionType checkCollision(const CelestialBody& other) const;
    void handleCollision(CelestialBody& other, CollisionType type);
    
    // Rendering methods
    void generateMesh();
    void updateTrail(float deltaTime);
    void render(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection);
    void renderTrail(GLuint trailShader, const glm::mat4& view, const glm::mat4& projection);
	GLuint VAO = 0, VBO = 0;
    GLuint trailVAO = 0, trailVBO = 0;
    size_t vertexCount = 0;
	
	void computeRadiusFromMassAndDensity();

    // Utility methods
    float getSchwarzschildRadius() const;
    glm::vec3 getGravitationalField(const glm::vec3& point) const;
    void setPresetProperties(const std::string& preset);
	
	// Mesh rendering
	Mesh mesh;
	
	// Rendering assets
    static size_t nextId;
    std::vector<float> generateSphereVertices() const;
    void updateRadius();
};

// Simulation engine class
class SimulationEngine {
public:
    void render(GLuint shaderProgram, GLuint trailShader, const glm::mat4& view, const glm::mat4& projection);
    CelestialBody* getBodyById(size_t id);
    void saveState(const std::string& filename);
    void loadState(const std::string& filename);
    void loadBodyFromFile(std::ifstream& file);
    std::vector<CelestialBody*> getBodiesInRadius(const glm::vec3& center, float radius);
    glm::vec3 getCenterOfMass() const;
    float getTotalEnergy() const;
    glm::vec3 randomPointOnSphere(float radius);
    std::string formatScientific(float value, int precision = 3);

    SimulationEngine();
    ~SimulationEngine();
    
    void update(float deltaTime);
    void removeBody(size_t id);
    void clearBodies();
    void loadPreset(SimulationPreset preset);
    
    // Physics calculations
    void calculateGravitationalForces();
    void updateGridDeformation();
    glm::vec3 calculateCenterOfMass() const;
    
    // Grid management and rendering
    void initializeGrid();
    void renderGrid(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection);
	void generateMesh();
	void addBody(std::unique_ptr<CelestialBody> body);
	
public:
    bool showGrid;
    bool isPaused;
    bool enableCollisions;
    bool enableRelativisticEffects;
    float timeScale;
    float gravitationalConstant = Physics::G;
    GLuint gridVAO = 0, gridVBO = 0;
    std::vector<std::unique_ptr<CelestialBody>> bodies;
    const std::vector<std::unique_ptr<CelestialBody>>& getBodies() const;
	std::vector<float> gridVertices;
	std::vector<float> createGridVertices() const;
    
private:
    void applySpacetimeDeformation();
};

// Camera system
class Camera {
public:
    glm::vec3 position{0.0f, 1000.0f, 5000.0f};
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 right{1.0f, 0.0f, 0.0f};
    
    float yaw = -90.0f;
    float pitch = 0.0f;
    float speed = 10000.0f;
    float sensitivity = 0.1f;
    float zoom = 45.0f;
    
    bool firstMouse = true;
    float lastX = Rendering::WINDOW_WIDTH / 2.0f;
    float lastY = Rendering::WINDOW_HEIGHT / 2.0f;
    
    void processKeyboard(int direction, float deltaTime);
    void processMouseMovement(float xOffset, float yOffset);
    void processMouseScroll(float yOffset);
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    
    // Camera modes
    void followBody(const CelestialBody* body);
    void orbitBody(const CelestialBody* body, float distance);
    
private:
    void updateCameraVectors();
    const CelestialBody* followTarget = nullptr;
};

// UI Manager for ImGui interface
class UIManager {
public:
    bool showDemoWindow = false;
    bool showSimulationControls = true;
    bool showBodyCreator = false;
    bool showSystemInfo = true;
    bool showPresets = false;
    
    // Body creation parameters
    struct BodyCreationParams {
        float mass = 1e24f;
        float density = 3344.0f;
        glm::vec3 position{0.0f};
        glm::vec3 velocity{0.0f};
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
        bool isGlowing = false;
        std::string name = "New Body";
    } creationParams;
    
    UIManager();
    ~UIManager();
    
    void initialize(GLFWwindow* window);
    void render(SimulationEngine& engine, Camera& camera);
    void cleanup();
    
private:
    bool initialized = false;
    
    void renderMainMenuBar(SimulationEngine& engine);
    void renderSimulationControls(SimulationEngine& engine);
    void renderBodyCreator(SimulationEngine& engine);
    void renderSystemInfo(const SimulationEngine& engine, const Camera& camera);
    void renderPresetSelector(SimulationEngine& engine);
    void renderBodyList(SimulationEngine& engine);
};

// Preset manager for common scenarios
class PresetManager {
public:
    static void loadSolarSystem(SimulationEngine& engine);
    static void loadBinaryStars(SimulationEngine& engine);
    static void loadGalaxyCollision(SimulationEngine& engine);
    static void loadCustomPreset(SimulationEngine& engine, const std::string& filename);
    
private:
    static std::unique_ptr<CelestialBody> createSun();
    static std::unique_ptr<CelestialBody> createEarth();
    static std::unique_ptr<CelestialBody> createMoon();
    static std::unique_ptr<CelestialBody> createMars();
    static std::unique_ptr<CelestialBody> createJupiter();
};

// Main application class
class GravitySimulator {
public:
    GravitySimulator();
    ~GravitySimulator();
    
    bool initialize();
    void run();
    void cleanup();
    
private:
    GLFWwindow* window = nullptr;
    std::unique_ptr<SimulationEngine> engine;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<UIManager> ui;
    
    GLuint shaderProgram = 0;
    GLuint trailShaderProgram = 0;
    
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    
    bool initializeOpenGL();
    bool initializeShaders();
    void setupCallbacks();
    void processInput();
    void render();
    
    // Callback functions
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};

// Utility functions
namespace Utils {
    GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource);
    GLuint compileShader(GLenum type, const char* source);
    glm::vec3 sphericalToCartesian(float r, float theta, float phi);
    std::string formatMass(float mass);
    std::string formatDistance(float distance);
    glm::vec4 getColorFromTemperature(float temperature);
}