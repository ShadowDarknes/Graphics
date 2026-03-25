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
#include <map>

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
};

// Структура для временного хранения данных при загрузке
struct TempVertex {
    glm::vec3 position;
    glm::vec3 normal;
};

// Функция для загрузки OBJ файла с поддержкой разных форматов
std::vector<Vertex> loadOBJ(const std::string& path) {
    std::vector<Vertex> vertices;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<std::vector<int>> faceVertices;

    std::ifstream file(path);

    if (!file.is_open()) {
        std::cout << "Не удалось открыть файл: " << path << std::endl;
        return vertices;
    }

    std::string line;
    int faceCount = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        if (line.substr(0, 2) == "v ") {
            float x, y, z;
            sscanf_s(line.c_str(), "v %f %f %f", &x, &y, &z);
            positions.push_back(glm::vec3(x, y, z));
        }
        else if (line.substr(0, 2) == "vn") {
            float nx, ny, nz;
            sscanf_s(line.c_str(), "vn %f %f %f", &nx, &ny, &nz);
            normals.push_back(glm::normalize(glm::vec3(nx, ny, nz)));
        }
        else if (line.substr(0, 2) == "f ") {
            faceCount++;
            std::string faceStr = line.substr(2);

            // Разбираем грани (поддерживаем разные форматы)
            std::vector<int> vIndices, nIndices;
            size_t pos = 0;
            std::string token;

            while ((pos = faceStr.find(' ')) != std::string::npos) {
                token = faceStr.substr(0, pos);
                if (!token.empty()) {
                    int v, vt, vn;
                    if (sscanf_s(token.c_str(), "%d/%d/%d", &v, &vt, &vn) == 3) {
                        vIndices.push_back(v - 1);
                        nIndices.push_back(vn - 1);
                    }
                    else if (sscanf_s(token.c_str(), "%d//%d", &v, &vn) == 2) {
                        vIndices.push_back(v - 1);
                        nIndices.push_back(vn - 1);
                    }
                    else if (sscanf_s(token.c_str(), "%d/%d", &v, &vt) == 2) {
                        vIndices.push_back(v - 1);
                        // Если нет нормалей, используем (0,1,0)
                        nIndices.push_back(-1);
                    }
                    else if (sscanf_s(token.c_str(), "%d", &v) == 1) {
                        vIndices.push_back(v - 1);
                        nIndices.push_back(-1);
                    }
                }
                faceStr.erase(0, pos + 1);
            }

            // Последний токен
            if (!faceStr.empty()) {
                int v, vt, vn;
                if (sscanf_s(faceStr.c_str(), "%d/%d/%d", &v, &vt, &vn) == 3) {
                    vIndices.push_back(v - 1);
                    nIndices.push_back(vn - 1);
                }
                else if (sscanf_s(faceStr.c_str(), "%d//%d", &v, &vn) == 2) {
                    vIndices.push_back(v - 1);
                    nIndices.push_back(vn - 1);
                }
                else if (sscanf_s(faceStr.c_str(), "%d/%d", &v, &vt) == 2) {
                    vIndices.push_back(v - 1);
                    nIndices.push_back(-1);
                }
                else if (sscanf_s(faceStr.c_str(), "%d", &v) == 1) {
                    vIndices.push_back(v - 1);
                    nIndices.push_back(-1);
                }
            }

            // Триангуляция (разбиваем на треугольники)
            for (size_t i = 1; i < vIndices.size() - 1; i++) {
                // Первая вершина
                if (vIndices[0] < positions.size()) {
                    glm::vec3 pos1 = positions[vIndices[0]];
                    glm::vec3 norm1 = (nIndices[0] >= 0 && nIndices[0] < normals.size()) ?
                        normals[nIndices[0]] : glm::vec3(0.0f, 1.0f, 0.0f);

                    // Вторая вершина
                    if (vIndices[i] < positions.size()) {
                        glm::vec3 pos2 = positions[vIndices[i]];
                        glm::vec3 norm2 = (nIndices[i] >= 0 && nIndices[i] < normals.size()) ?
                            normals[nIndices[i]] : glm::vec3(0.0f, 1.0f, 0.0f);

                        // Третья вершина
                        if (vIndices[i + 1] < positions.size()) {
                            glm::vec3 pos3 = positions[vIndices[i + 1]];
                            glm::vec3 norm3 = (nIndices[i + 1] >= 0 && nIndices[i + 1] < normals.size()) ?
                                normals[nIndices[i + 1]] : glm::vec3(0.0f, 1.0f, 0.0f);

                            vertices.push_back({ pos1.x, pos1.y, pos1.z, norm1.x, norm1.y, norm1.z });
                            vertices.push_back({ pos2.x, pos2.y, pos2.z, norm2.x, norm2.y, norm2.z });
                            vertices.push_back({ pos3.x, pos3.y, pos3.z, norm3.x, norm3.y, norm3.z });
                        }
                    }
                }
            }
        }
    }

    file.close();
    std::cout << "Загружено вершин из OBJ: " << vertices.size() << std::endl;
    std::cout << "Загружено позиций: " << positions.size() << std::endl;
    std::cout << "Загружено нормалей: " << normals.size() << std::endl;
    std::cout << "Обработано граней: " << faceCount << std::endl;

    return vertices;
}

// Функция для вычисления нормалей, если их нет в файле
void computeNormals(std::vector<Vertex>& vertices) {
    if (vertices.empty()) return;

    std::cout << "Вычисляем нормали для " << vertices.size() << " вершин..." << std::endl;

    // Временный массив для хранения нормалей
    std::vector<glm::vec3> tempNormals(vertices.size(), glm::vec3(0.0f));

    // Вычисляем нормали для каждого треугольника
    for (size_t i = 0; i < vertices.size(); i += 3) {
        if (i + 2 >= vertices.size()) break;

        glm::vec3 v1(vertices[i].x, vertices[i].y, vertices[i].z);
        glm::vec3 v2(vertices[i + 1].x, vertices[i + 1].y, vertices[i + 1].z);
        glm::vec3 v3(vertices[i + 2].x, vertices[i + 2].y, vertices[i + 2].z);

        glm::vec3 edge1 = v2 - v1;
        glm::vec3 edge2 = v3 - v1;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        tempNormals[i] += normal;
        tempNormals[i + 1] += normal;
        tempNormals[i + 2] += normal;
    }

    // Нормализуем полученные нормали
    for (size_t i = 0; i < vertices.size(); i++) {
        if (glm::length(tempNormals[i]) > 0.01f) {
            glm::vec3 norm = glm::normalize(tempNormals[i]);
            vertices[i].nx = norm.x;
            vertices[i].ny = norm.y;
            vertices[i].nz = norm.z;
        }
        else {
            vertices[i].nx = 0.0f;
            vertices[i].ny = 1.0f;
            vertices[i].nz = 0.0f;
        }
    }

    std::cout << "Нормали вычислены!" << std::endl;
}

std::string getCurrentDirectory() {
    char buffer[1024];
    if (_getcwd(buffer, sizeof(buffer)) != NULL) {
        return std::string(buffer);
    }
    return "unknown";
}

// ------------------- CAMERA -------------------
glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, 4.0f);
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

    GLFWwindow* window = glfwCreateWindow(800, 600, "KUKA KR 120 R3200 PA - Phong Lighting", NULL, NULL);
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

    // Включаем отсечение задних граней для правильного отображения
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

    // ========== ЗАГРУЗКА МОДЕЛИ ==========
    std::string modelPath = "17 KUKA KR 120 R3200 PA.obj";
    std::cout << "\n=== ПОИСК МОДЕЛИ ===" << std::endl;
    std::cout << "Текущая директория: " << getCurrentDirectory() << std::endl;
    std::cout << "Путь к модели: " << modelPath << std::endl;

    std::vector<Vertex> modelVertices = loadOBJ(modelPath);

    // Если нормали не загрузились, вычисляем их
    bool hasNormals = false;
    for (const auto& v : modelVertices) {
        if (v.nx != 0.0f || v.ny != 0.0f || v.nz != 0.0f) {
            hasNormals = true;
            break;
        }
    }

    if (!hasNormals && !modelVertices.empty()) {
        std::cout << "Нормали не найдены в файле, вычисляем автоматически..." << std::endl;
        computeNormals(modelVertices);
    }

    GLuint modelVAO = 0, modelVBO = 0;
    bool modelLoaded = false;

    if (!modelVertices.empty()) {
        glGenVertexArrays(1, &modelVAO);
        glGenBuffers(1, &modelVBO);

        glBindVertexArray(modelVAO);
        glBindBuffer(GL_ARRAY_BUFFER, modelVBO);
        glBufferData(GL_ARRAY_BUFFER, modelVertices.size() * sizeof(Vertex), modelVertices.data(), GL_STATIC_DRAW);

        // Позиция
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);

        // Нормаль
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        modelLoaded = true;
        std::cout << "Модель загружена в GPU! Вершин: " << modelVertices.size() << std::endl;
    }

    // ========== ШЕЙДЕРЫ С УЛУЧШЕННЫМ ОСВЕЩЕНИЕМ ==========
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    out vec3 FragPos;
    out vec3 Normal;
    
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        gl_Position = projection * view * vec4(FragPos, 1.0);
        Normal = normalize(mat3(transpose(inverse(model))) * aNormal);
    }
    )";

    const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 FragPos;
    in vec3 Normal;
    
    struct Material {
        vec3 ambient;
        vec3 diffuse;
        vec3 specular;
        float shininess;
    };
    
    struct Light {
        vec3 position;
        vec3 ambient;
        vec3 diffuse;
        vec3 specular;
    };
    
    uniform Material material;
    uniform Light light;
    uniform vec3 viewPos;
    
    void main() {
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(light.position - FragPos);
        
        // Ambient
        vec3 ambient = light.ambient * material.ambient;
        
        // Diffuse
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = light.diffuse * (diff * material.diffuse);
        
        // Specular
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specular = light.specular * (spec * material.specular);
        
        vec3 result = ambient + diffuse + specular;
        
        // Добавляем небольшой оттенок для лучшей визуализации
        FragColor = vec4(result, 1.0);
    }
    )";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "Ошибка вершинного шейдера: " << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Ошибка фрагментного шейдера: " << infoLog << std::endl;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Ошибка линковки: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Получение uniform locations
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");

    int materialAmbientLoc = glGetUniformLocation(shaderProgram, "material.ambient");
    int materialDiffuseLoc = glGetUniformLocation(shaderProgram, "material.diffuse");
    int materialSpecularLoc = glGetUniformLocation(shaderProgram, "material.specular");
    int materialShininessLoc = glGetUniformLocation(shaderProgram, "material.shininess");

    int lightPosLoc = glGetUniformLocation(shaderProgram, "light.position");
    int lightAmbientLoc = glGetUniformLocation(shaderProgram, "light.ambient");
    int lightDiffuseLoc = glGetUniformLocation(shaderProgram, "light.diffuse");
    int lightSpecularLoc = glGetUniformLocation(shaderProgram, "light.specular");

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    // ========== НАСТРОЙКА МОДЕЛИ ==========
    glm::mat4 modelMat = glm::mat4(1.0f);

    float scale = 0.8f;
    modelMat = glm::scale(modelMat, glm::vec3(scale));
    modelMat = glm::rotate(modelMat, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMat = glm::rotate(modelMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::translate(modelMat, glm::vec3(-0.66f, -1.04f, 0.0f));

    // ========== МАТЕРИАЛ (металлический с легким оттенком) ==========
    glm::vec3 materialAmbient = glm::vec3(0.3f, 0.3f, 0.35f);
    glm::vec3 materialDiffuse = glm::vec3(0.7f, 0.7f, 0.75f);
    glm::vec3 materialSpecular = glm::vec3(0.9f, 0.9f, 1.0f);
    float materialShininess = 64.0f;

    // ========== ИСТОЧНИК СВЕТА ==========
    glm::vec3 lightPos = glm::vec3(2.5f, 4.0f, 2.5f);
    glm::vec3 lightAmbient = glm::vec3(0.25f, 0.25f, 0.25f);
    glm::vec3 lightDiffuse = glm::vec3(0.9f, 0.9f, 0.9f);
    glm::vec3 lightSpecular = glm::vec3(1.0f, 1.0f, 1.0f);

    std::cout << "\n=== НАСТРОЙКИ ===" << std::endl;
    std::cout << "Материал: Металлический" << std::endl;
    std::cout << "Shininess: " << materialShininess << std::endl;
    std::cout << "Позиция света: (" << lightPos.x << ", " << lightPos.y << ", " << lightPos.z << ")" << std::endl;
    std::cout << "Отсечение граней: ВКЛЮЧЕНО (GL_BACK)" << std::endl;
    std::cout << "Порядок обхода: CCW" << std::endl;
    std::cout << "==================\n" << std::endl;

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

        glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        glUseProgram(shaderProgram);

        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

        glUniform3fv(materialAmbientLoc, 1, glm::value_ptr(materialAmbient));
        glUniform3fv(materialDiffuseLoc, 1, glm::value_ptr(materialDiffuse));
        glUniform3fv(materialSpecularLoc, 1, glm::value_ptr(materialSpecular));
        glUniform1f(materialShininessLoc, materialShininess);

        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(lightAmbientLoc, 1, glm::value_ptr(lightAmbient));
        glUniform3fv(lightDiffuseLoc, 1, glm::value_ptr(lightDiffuse));
        glUniform3fv(lightSpecularLoc, 1, glm::value_ptr(lightSpecular));

        if (modelLoaded) {
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