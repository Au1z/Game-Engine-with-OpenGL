#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 3.0f, 6.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 cubePosition(0.0f, 0.5f, 5.0f);
float cubeSpeed = 2.5f;

float cubeYaw = 0.0f;
float mouseSensitivity = 0.1f;

float cameraPitch = 10.0f;

float cubeVelocityY = 0.0f;
float gravity = -9.81f;
bool isOnGround = true;

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

bool checkCollisionAABB(const AABB& box1, const AABB& box2) {
    return (box1.min.x <= box2.max.x && box1.max.x >= box2.min.x) &&
        (box1.min.y <= box2.max.y && box1.max.y >= box2.min.y) &&
        (box1.min.z <= box2.max.z && box1.max.z >= box2.min.z);
}

AABB rockBox;

struct Pillar {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 color;
    AABB box;
};

Pillar pillars[4];

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader(
        "../../src/3.model_loading/1.model_loading/1.model_loading.vs",
        "../../src/3.model_loading/1.model_loading/1.model_loading.fs"
    );

    // load models
    // -----------
    Model ourModel(FileSystem::getPath("resources/objects/rock/rock.obj"));

    glm::vec3 rockPos(0.0f, 0.0f, 0.0f);
    glm::vec3 rockScale(1.0f);

    glm::vec3 rockMin(FLT_MAX);
    glm::vec3 rockMax(-FLT_MAX);

    // loop ผ่านทุก mesh ใน model
    for (unsigned int i = 0; i < ourModel.meshes.size(); i++)
    {
        Mesh mesh = ourModel.meshes[i];

        for (unsigned int v = 0; v < mesh.vertices.size(); v++)
        {
            glm::vec3 pos = mesh.vertices[v].Position * rockScale + rockPos;

            rockMin = glm::min(rockMin, pos);
            rockMax = glm::max(rockMax, pos);
        }
    }

    rockBox.min = rockMin;
    rockBox.max = rockMax;

    
    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    float planeVertices[] = {
        // positions          // normals       // texcoords
         20.0f, 0.0f,  20.0f,  0.0f, 1.0f, 0.0f,  2.0f, 0.0f,
        -20.0f, 0.0f,  20.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
        -20.0f, 0.0f, -20.0f,  0.0f, 1.0f, 0.0f,  0.0f, 2.0f,

         20.0f, 0.0f,  20.0f,  0.0f, 1.0f, 0.0f,  2.0f, 0.0f,
        -20.0f, 0.0f, -20.0f,  0.0f, 1.0f, 0.0f,  0.0f, 2.0f,
         20.0f, 0.0f, -20.0f,  0.0f, 1.0f, 0.0f,  2.0f, 2.0f
    };
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // texcoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    unsigned int planeTexture;
    glGenTextures(1, &planeTexture);
    glBindTexture(GL_TEXTURE_2D, planeTexture);

    // set the texture wrapping/filtering options (on currently bound texture)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // load and generate the texture
    int width, height, nrChannels;
    unsigned char* data = stbi_load(FileSystem::getPath("resources/textures/marble.jpg").c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    float cubeVertices[] = {
        // positions          // normals           // texcoords
        // back face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,

        // front face
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,

        // left face
        -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,

        // right face
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,

         // bottom face
         -0.5f, -0.5f, -0.5f,  0.0f,-1.0f, 0.0f,   0.0f, 1.0f,
          0.5f, -0.5f, -0.5f,  0.0f,-1.0f, 0.0f,   1.0f, 1.0f,
          0.5f, -0.5f,  0.5f,  0.0f,-1.0f, 0.0f,   1.0f, 0.0f,
          0.5f, -0.5f,  0.5f,  0.0f,-1.0f, 0.0f,   1.0f, 0.0f,
         -0.5f, -0.5f,  0.5f,  0.0f,-1.0f, 0.0f,   0.0f, 0.0f,
         -0.5f, -0.5f, -0.5f,  0.0f,-1.0f, 0.0f,   0.0f, 1.0f,

         // top face
         -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
          0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
          0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
          0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
         -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
         -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f
    };

    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // texcoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);

    unsigned int cubeTexture;
    glGenTextures(1, &cubeTexture);
    glBindTexture(GL_TEXTURE_2D, cubeTexture);

    // ตั้งค่าการ wrap / filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    data = stbi_load(FileSystem::getPath("resources/textures/container2.png").c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load cube texture" << std::endl;
    }
    stbi_image_free(data);

    ourShader.use();
    ourShader.setInt("texture_diffuse1", 0);

    pillars[0].position = glm::vec3(7.0f, 3.0f, 0.0f);
    pillars[0].scale = glm::vec3(0.5f, 2.0f, 0.5f);
    pillars[0].color = glm::vec3(1.0f, 0.0f, 0.0f);
    pillars[0].box.min = pillars[0].position - pillars[0].scale * 0.5f;
    pillars[0].box.max = pillars[0].position + pillars[0].scale * 0.5f;

    pillars[1].position = glm::vec3(-3.0f, 0.0f, 7.0f);
    pillars[1].scale = glm::vec3(0.5f, 2.0f, 0.5f);
    pillars[1].color = glm::vec3(1.0f, 0.0f, 0.0f);
    pillars[1].box.min = pillars[1].position - pillars[1].scale * 0.5f;
    pillars[1].box.max = pillars[1].position + pillars[1].scale * 0.5f;

    pillars[2].position = glm::vec3(-1.0f, 4.0f, 7.0f);
    pillars[2].scale = glm::vec3(0.5f, 2.0f, 0.5f);
    pillars[2].color = glm::vec3(1.0f, 0.0f, 0.0f);
    pillars[2].box.min = pillars[2].position - pillars[2].scale * 0.5f;
    pillars[2].box.max = pillars[2].position + pillars[2].scale * 0.5f;

    pillars[3].position = glm::vec3(9.0f, 2.0f, -7.0f);
    pillars[3].scale = glm::vec3(0.5f, 2.0f, 0.5f);
    pillars[3].color = glm::vec3(1.0f, 0.0f, 0.0f);
    pillars[3].box.min = pillars[3].position - pillars[3].scale * 0.5f;
    pillars[3].box.max = pillars[3].position + pillars[3].scale * 0.5f;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        cubeVelocityY += gravity * deltaTime;
        cubePosition.y += cubeVelocityY * deltaTime;

        float cubeHalfHeight = 0.5f;
        float groundY = 0.0f;

        if (cubePosition.y <= groundY + cubeHalfHeight)
        {
            cubePosition.y = groundY + cubeHalfHeight;
            cubeVelocityY = 0.0f;
            isOnGround = true;
        }
        else
        {
            isOnGround = false;
        }

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        float distanceBehind = 3.0f;
        float heightOffset = 0.5f;

        float yawRad = glm::radians(cubeYaw);
        float pitchRad = glm::radians(cameraPitch);

        glm::vec3 offset;
        offset.x = sin(yawRad) * cos(pitchRad) * distanceBehind;
        offset.y = sin(pitchRad) * distanceBehind + heightOffset;
        offset.z = cos(yawRad) * cos(pitchRad) * distanceBehind;

        camera.Position = cubePosition + offset;

        camera.Front = glm::normalize(cubePosition - camera.Position);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render plane with texture
        glm::mat4 modelPlane = glm::mat4(1.0f);
        ourShader.setMat4("model", modelPlane);
        ourShader.setBool("useTexture", true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, planeTexture);
        ourShader.setInt("texture1", 0);
        glBindVertexArray(planeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));
        ourShader.setMat4("model", model);
        ourShader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setBool("useTexture", true);
        ourModel.Draw(ourShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);

        glm::mat4 modelCube = glm::mat4(1.0f);
        modelCube = glm::translate(modelCube, cubePosition);
        modelCube = glm::rotate(modelCube, glm::radians(cubeYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        modelCube = glm::scale(modelCube, glm::vec3(1.0f));
        ourShader.setMat4("model", modelCube);

        ourShader.setBool("useTexture", true);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        for (int i = 0; i < 4; i++)
        {
            glm::mat4 modelPillar = glm::mat4(1.0f);
            modelPillar = glm::translate(modelPillar, pillars[i].position);
            modelPillar = glm::scale(modelPillar, pillars[i].scale);
            ourShader.setMat4("model", modelPillar);

            ourShader.setBool("useTexture", false);
            ourShader.setVec3("objectColor", pillars[i].color);

            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float velocity = cubeSpeed * deltaTime;
    float yawRad = glm::radians(cubeYaw);

    glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));
    glm::vec3 right(cos(yawRad), 0.0f, -sin(yawRad));

    glm::vec3 newPos = cubePosition;
    float cubeHalfSize = 0.5f;

    cubeVelocityY += gravity * deltaTime;
    glm::vec3 tempPosY = newPos + glm::vec3(0.0f, cubeVelocityY * deltaTime, 0.0f);
    AABB cubeBoxY{ tempPosY - glm::vec3(cubeHalfSize), tempPosY + glm::vec3(cubeHalfSize) };

    for (int i = 0; i < 4; i++)
    {
        if (checkCollisionAABB(cubeBoxY, pillars[i].box))
        {
            pillars[i].color = glm::vec3(0.0f, 1.0f, 0.0f);

            if (cubeVelocityY < 0.0f)
            {
                tempPosY.y = pillars[i].box.max.y + cubeHalfSize;
                cubeVelocityY = 0.0f;
                isOnGround = true;
            }
            else if (cubeVelocityY > 0.0f)
            {
                tempPosY.y = pillars[i].box.min.y - cubeHalfSize;
                cubeVelocityY = 0.0f;
            }
        }
    }

    if (checkCollisionAABB(cubeBoxY, rockBox))
    {
        if (cubeVelocityY < 0.0f)
        {
            tempPosY.y = rockBox.max.y + cubeHalfSize;
            cubeVelocityY = 0.0f;
            isOnGround = true;
        }
        else if (cubeVelocityY > 0.0f)
        {
            tempPosY.y = rockBox.min.y - cubeHalfSize;
            cubeVelocityY = 0.0f;
        }
    }

    newPos.y = tempPosY.y;

    glm::vec3 move(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += forward * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= forward * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += right * velocity;

    glm::vec3 tempPosX = newPos + glm::vec3(move.x, 0.0f, 0.0f);
    AABB cubeBoxX{ tempPosX - glm::vec3(cubeHalfSize), tempPosX + glm::vec3(cubeHalfSize) };
    bool collisionX = checkCollisionAABB(cubeBoxX, rockBox);
    for (int i = 0; i < 4; i++)
        if (checkCollisionAABB(cubeBoxX, pillars[i].box))
        {
            if (tempPosY.y - cubeHalfSize < pillars[i].box.max.y - 0.01f)
            {
                pillars[i].color = glm::vec3(0.0f, 1.0f, 0.0f);
                collisionX = true;
            }
        }
    if (!collisionX) newPos.x += move.x;

    glm::vec3 tempPosZ = newPos + glm::vec3(0.0f, 0.0f, move.z);
    AABB cubeBoxZ{ tempPosZ - glm::vec3(cubeHalfSize), tempPosZ + glm::vec3(cubeHalfSize) };
    bool collisionZ = checkCollisionAABB(cubeBoxZ, rockBox);
    for (int i = 0; i < 4; i++)
        if (checkCollisionAABB(cubeBoxZ, pillars[i].box))
        {
            if (tempPosY.y - cubeHalfSize < pillars[i].box.max.y - 0.01f)
            {
                pillars[i].color = glm::vec3(0.0f, 1.0f, 0.0f);
                collisionZ = true;
            }
        }
    if (!collisionZ) newPos.z += move.z;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isOnGround)
    {
        cubeVelocityY = 5.0f;
        isOnGround = false;
    }

    cubePosition = newPos;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    static float lastX = SCR_WIDTH / 2.0f;
    static float lastY = SCR_HEIGHT / 2.0f;
    static bool firstMouse = true;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    cubeYaw -= xoffset;

    cameraPitch += yoffset;

    if (cameraPitch > 89.0f)
        cameraPitch = 89.0f;
    if (cameraPitch < -10.0f)
        cameraPitch = -10.0f;
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
