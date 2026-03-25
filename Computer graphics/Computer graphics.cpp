#define _CRT_SECURE_NO_WARNINGS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <direct.h>

struct Vertex {
    float x, y, z;
    float r, g, b;
};

std::vector<Vertex> loadOBJ(const std::string& path) {
    std::vector<Vertex> vertices;
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cout << "Не удалось открыть файл: " << path << std::endl;
        return vertices;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 2) == "v ") {
            float x, y, z;
            sscanf_s(line.c_str(), "v %f %f %f", &x, &y, &z);
            // Металлический цвет для робота
            vertices.push_back({ x, y, z, 0.7f, 0.7f, 0.8f });
        }
    }

    file.close();
    std::cout << "Загружено вершин из OBJ: " << vertices.size() << std::endl;
    return vertices;
}

std::string getCurrentDirectory() {
    char buffer[1024];
    if (_getcwd(buffer, sizeof(buffer)) != NULL) {
        return std::string(buffer);
    }
    return "unknown";
}

// ------------------- CAMERA -------------------
glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, 4.0f);  // Подняли камеру для лучшего обзора
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void processInput(GLFWwindow* window) {
    float speed = 5.0f * deltaTime;
    if (speed > 0.5f) speed = 0.5f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += speed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= speed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    if (!glfwInit()) {
        std::cout << "GLFW init failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "KUKA KR 120 R3200 PA", NULL, NULL);
    if (!window) {
        std::cout << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW init failed\n";
        return -1;
    }

    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);

    // ВАЖНО: Отключаем отсечение задних граней, чтобы видеть все плоскости
    glDisable(GL_CULL_FACE);
    // Или можно включить двухстороннее освещение:
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

    // ========== ЗАГРУЗКА МОДЕЛИ ==========
    std::string modelPath = "17 KUKA KR 120 R3200 PA.obj";
    std::cout << "\n=== ПОИСК МОДЕЛИ ===" << std::endl;
    std::cout << "Текущая директория: " << getCurrentDirectory() << std::endl;
    std::cout << "Путь к модели: " << modelPath << std::endl;

    std::vector<Vertex> modelVertices = loadOBJ(modelPath);

    GLuint modelVAO = 0, modelVBO = 0;
    bool modelLoaded = false;

    if (!modelVertices.empty()) {
        float minX = modelVertices[0].x, maxX = modelVertices[0].x;
        float minY = modelVertices[0].y, maxY = modelVertices[0].y;
        float minZ = modelVertices[0].z, maxZ = modelVertices[0].z;

        for (const auto& v : modelVertices) {
            minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);
            minY = std::min(minY, v.y); maxY = std::max(maxY, v.y);
            minZ = std::min(minZ, v.z); maxZ = std::max(maxZ, v.z);
        }

        float centerX = (minX + maxX) / 2.0f;
        float centerY = (minY + maxY) / 2.0f;
        float centerZ = (minZ + maxZ) / 2.0f;

        std::cout << "\n=== ИНФОРМАЦИЯ О МОДЕЛИ ===" << std::endl;
        std::cout << "Вершин: " << modelVertices.size() << std::endl;
        std::cout << "Размеры: " << maxX - minX << " x " << maxY - minY << " x " << maxZ - minZ << std::endl;
        std::cout << "Центр модели: (" << centerX << ", " << centerY << ", " << centerZ << ")" << std::endl;

        glGenVertexArrays(1, &modelVAO);
        glGenBuffers(1, &modelVBO);

        glBindVertexArray(modelVAO);
        glBindBuffer(GL_ARRAY_BUFFER, modelVBO);
        glBufferData(GL_ARRAY_BUFFER, modelVertices.size() * sizeof(Vertex), modelVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        modelLoaded = true;
        std::cout << "Модель загружена в GPU!" << std::endl;
        std::cout << "==========================\n" << std::endl;
    }

    // ========== ШЕЙДЕРЫ ==========
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    out vec3 vertexColor;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        vertexColor = aColor;
    }
    )";

    const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in vec3 vertexColor;
    void main() {
        FragColor = vec4(vertexColor, 1.0);
    }
    )";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projLoc = glGetUniformLocation(shaderProgram, "projection");

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    // ========== НАСТРОЙКА МОДЕЛИ ДЛЯ ПРАВИЛЬНОГО ОТОБРАЖЕНИЯ ==========
    glm::mat4 modelMat = glm::mat4(1.0f);

    // 1. МАСШТАБ - подбираем под размер экрана
    float scale = 0.8f;  // Увеличиваем для лучшей видимости
    modelMat = glm::scale(modelMat, glm::vec3(scale));

    // 2. ПОВОРОТ - чтобы робот стоял вертикально и смотрел вперед
    // Поворачиваем вокруг X, чтобы робот стоял на ногах (было 90, стало 0)
    modelMat = glm::rotate(modelMat, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // Поворачиваем вокруг Y, чтобы робот смотрел на камеру
    modelMat = glm::rotate(modelMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // 3. ЦЕНТРИРОВАНИЕ - смещаем модель в центр
    // Из ваших данных центр примерно (0.66, 1.04, 0)
    modelMat = glm::translate(modelMat, glm::vec3(-0.66f, -1.04f, 0.0f));

    std::cout << "\n=== НАСТРОЙКИ ОТОБРАЖЕНИЯ ===" << std::endl;
    std::cout << "Масштаб модели: " << scale << std::endl;
    std::cout << "Поворот: 0° вокруг X, 180° вокруг Y" << std::endl;
    std::cout << "Смещение для центрирования: (-0.66, -1.04, 0)" << std::endl;
    std::cout << "Позиция камеры: (0, 1.5, 4)" << std::endl;
    std::cout << "Режим отображения: показаны все плоскости (CULL_FACE OFF)" << std::endl;
    std::cout << "============================\n" << std::endl;

    std::cout << "\n=== УПРАВЛЕНИЕ ===" << std::endl;
    std::cout << "WASD - движение камеры" << std::endl;
    std::cout << "Мышь - поворот камеры" << std::endl;
    std::cout << "Space/Shift - вверх/вниз" << std::endl;
    std::cout << "ESC - выход" << std::endl;
    std::cout << "================\n" << std::endl;

    // ========== ОСНОВНОЙ ЦИКЛ ==========
    int frameCount = 0;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        lastFrame = currentFrame;

        glfwPollEvents();
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        if (modelLoaded) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
            glBindVertexArray(modelVAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)modelVertices.size());

            if (frameCount % 60 == 0) {
                std::cout << "Рисуем модель, вершин: " << modelVertices.size() << std::endl;
            }
        }

        frameCount++;
        glfwSwapBuffers(window);
    }

    // Очистка
    if (modelLoaded) {
        glDeleteVertexArrays(1, &modelVAO);
        glDeleteBuffers(1, &modelVBO);
    }
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}