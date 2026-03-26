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
#include <sstream>
#include <windows.h>

// Структура вершины: содержит позицию (x,y,z) и нормаль (nx,ny,nz)
struct Vertex {
    float x, y, z;      // Координаты вершины
    float nx, ny, nz;   // Нормаль для освещения
};

// Структура для хранения части модели (группы из OBJ файла)
struct ModelPart {
    std::vector<Vertex> vertices;   // Массив вершин части
    GLuint VAO, VBO;                // OpenGL объекты: массив вершин и буфер вершин
    std::string name;               // Имя части (из OBJ группы)
    glm::vec3 pivotPoint;           // Точка поворота части
    glm::vec3 originalPosition;     // Оригинальная позиция (для сброса)
    float rotationX, rotationY, rotationZ; // Углы поворота (не используются в иерархии)

    // Конструктор по умолчанию, инициализирует все поля нулями
    ModelPart() : VAO(0), VBO(0), pivotPoint(0.0f), originalPosition(0.0f),
        rotationX(0.0f), rotationY(0.0f), rotationZ(0.0f) {
    }
};

/**
 * Разбирает токен грани OBJ формата (например "1//2" или "1/2/3")
 * @param token - строка с индексом вершины
 * @param v - выходной индекс вершины
 * @param vt - выходной индекс текстурной координаты
 * @param vn - выходной индекс нормали
 */
void parseFaceToken(const std::string& token, int& v, int& vt, int& vn) {
    v = vt = vn = -1;
    // Проверяем формат "v//vn" (без текстурных координат)
    if (token.find("//") != std::string::npos) {
        sscanf_s(token.c_str(), "%d//%d", &v, &vn);
    }
    else {
        size_t firstSlash = token.find('/');
        if (firstSlash == std::string::npos) {
            // Только индекс вершины
            v = std::stoi(token);
        }
        else {
            size_t secondSlash = token.find('/', firstSlash + 1);
            if (secondSlash == std::string::npos) {
                // Формат "v/vt"
                sscanf_s(token.c_str(), "%d/%d", &v, &vt);
            }
            else {
                // Формат "v/vt/vn"
                sscanf_s(token.c_str(), "%d/%d/%d", &v, &vt, &vn);
            }
        }
    }
}

/**
 * Загружает OBJ файл с поддержкой групп (g или o)
 * @param path - путь к OBJ файлу
 * @return вектор частей модели с разделенными по группам вершинами
 */
std::vector<ModelPart> loadOBJWithGroups(const std::string& path) {
    std::vector<ModelPart> parts;
    std::map<std::string, std::vector<Vertex>> partVertices; // Вершины по группам

    std::vector<glm::vec3> positions;   // Все позиции вершин из OBJ
    std::vector<glm::vec3> normals;     // Все нормали из OBJ
    std::string currentGroup = "default"; // Текущая группа

    std::ifstream file(path);

    if (!file.is_open()) {
        std::cout << "Не удалось открыть файл: " << path << std::endl;
        return parts;
    }

    std::string line;

    // Построчное чтение OBJ файла
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // Обработка группы или объекта (g или o)
        if (line.substr(0, 2) == "g " || line.substr(0, 2) == "o ") {
            currentGroup = line.substr(2);
            // Удаляем пробелы и символы перевода строки
            size_t end = currentGroup.find_last_not_of(" \n\r\t");
            if (end != std::string::npos) {
                currentGroup.erase(end + 1);
            }
            if (currentGroup.empty()) currentGroup = "default";
            std::cout << "Найдена группа: '" << currentGroup << "'" << std::endl;
        }
        // Обработка вершины (v)
        else if (line.substr(0, 2) == "v ") {
            float x, y, z;
            if (sscanf_s(line.c_str(), "v %f %f %f", &x, &y, &z) == 3) {
                positions.push_back(glm::vec3(x, y, z));
            }
        }
        // Обработка нормали (vn)
        else if (line.substr(0, 2) == "vn") {
            float nx, ny, nz;
            if (sscanf_s(line.c_str(), "vn %f %f %f", &nx, &ny, &nz) == 3) {
                normals.push_back(glm::normalize(glm::vec3(nx, ny, nz)));
            }
        }
        // Обработка грани (f)
        else if (line.substr(0, 2) == "f ") {
            std::string faceStr = line.substr(2);
            std::vector<int> vIndices, nIndices;

            std::stringstream ss(faceStr);
            std::string token;
            // Разбираем каждый токен грани (вершина/текстура/нормаль)
            while (ss >> token) {
                int v, vt, vn;
                parseFaceToken(token, v, vt, vn);

                if (v > 0) {
                    vIndices.push_back(v - 1); // OBJ индексы с 1, переводим в 0
                }
                if (vn > 0) {
                    nIndices.push_back(vn - 1);
                }
                else {
                    nIndices.push_back(-1); // Нормаль отсутствует
                }
            }

            // Триангуляция полигона (разбиваем на треугольники)
            for (size_t i = 1; i < vIndices.size() - 1; i++) {
                if (vIndices[0] < (int)positions.size() &&
                    vIndices[i] < (int)positions.size() &&
                    vIndices[i + 1] < (int)positions.size()) {

                    // Получаем позиции вершин треугольника
                    glm::vec3 pos1 = positions[vIndices[0]];
                    glm::vec3 norm1 = (nIndices[0] >= 0 && nIndices[0] < (int)normals.size()) ?
                        normals[nIndices[0]] : glm::vec3(0.0f, 1.0f, 0.0f);

                    glm::vec3 pos2 = positions[vIndices[i]];
                    glm::vec3 norm2 = (nIndices[i] >= 0 && nIndices[i] < (int)normals.size()) ?
                        normals[nIndices[i]] : glm::vec3(0.0f, 1.0f, 0.0f);

                    glm::vec3 pos3 = positions[vIndices[i + 1]];
                    glm::vec3 norm3 = (nIndices[i + 1] >= 0 && nIndices[i + 1] < (int)normals.size()) ?
                        normals[nIndices[i + 1]] : glm::vec3(0.0f, 1.0f, 0.0f);

                    // Создаем вершины треугольника
                    Vertex v1 = { pos1.x, pos1.y, pos1.z, norm1.x, norm1.y, norm1.z };
                    Vertex v2 = { pos2.x, pos2.y, pos2.z, norm2.x, norm2.y, norm2.z };
                    Vertex v3 = { pos3.x, pos3.y, pos3.z, norm3.x, norm3.y, norm3.z };

                    // Добавляем треугольник в текущую группу
                    partVertices[currentGroup].push_back(v1);
                    partVertices[currentGroup].push_back(v2);
                    partVertices[currentGroup].push_back(v3);
                }
            }
        }
    }

    file.close();

    // Вывод информации о найденных группах
    std::cout << "\n=== НАЙДЕННЫЕ ГРУППЫ ===" << std::endl;
    for (const auto& [name, vertices] : partVertices) {
        std::cout << "Группа: '" << name << "', вершин: " << vertices.size() << std::endl;
    }
    std::cout << "========================\n" << std::endl;

    // Создаем OpenGL объекты для каждой группы
    for (auto& [name, vertices] : partVertices) {
        if (!vertices.empty()) {
            ModelPart part;
            part.name = name;
            part.vertices = vertices;
            part.rotationX = 0.0f;
            part.rotationY = 0.0f;
            part.rotationZ = 0.0f;

            // Вычисляем минимальную Y для pivot'а плеча
            float minY = vertices[0].y;
            float maxY = vertices[0].y;
            for (const auto& v : vertices) {
                if (v.y < minY) minY = v.y;
                if (v.y > maxY) maxY = v.y;
            }

            // Вычисляем центр масс части
            glm::vec3 center(0.0f);
            for (const auto& v : vertices) {
                center += glm::vec3(v.x, v.y, v.z);
            }
            center /= (float)vertices.size();

            // Устанавливаем точку поворота:
            // - для плеч используем нижнюю точку (вращение вокруг основания)
            // - для остальных частей используем центр
            if (name.find("plecho") != std::string::npos || name.find("Plecho") != std::string::npos) {
                part.pivotPoint = glm::vec3(center.x, minY, center.z);
                std::cout << "Для части '" << name << "' используем pivot в нижней точке: Y=" << minY << std::endl;
            }
            else {
                part.pivotPoint = center;
            }

            part.originalPosition = part.pivotPoint;

            std::cout << "Часть '" << name << "': центр (" << center.x << ", " << center.y << ", " << center.z
                << "), pivot (" << part.pivotPoint.x << ", " << part.pivotPoint.y << ", " << part.pivotPoint.z << ")" << std::endl;

            // Создаем OpenGL буферы
            glGenVertexArrays(1, &part.VAO);
            glGenBuffers(1, &part.VBO);

            glBindVertexArray(part.VAO);
            glBindBuffer(GL_ARRAY_BUFFER, part.VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

            // Атрибут 0: позиция (3 float)
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            glEnableVertexAttribArray(0);

            // Атрибут 1: нормаль (3 float)
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glBindVertexArray(0);

            parts.push_back(part);
        }
    }

    return parts;
}

/**
 * Получает текущую рабочую директорию
 * @return строка с путем к текущей директории
 */
std::string getCurrentDirectory() {
    char buffer[1024];
    if (_getcwd(buffer, sizeof(buffer)) != NULL) {
        return std::string(buffer);
    }
    return "unknown";
}

// ------------------- УПРАВЛЕНИЕ КАМЕРОЙ -------------------
glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, 4.0f);    // Позиция камеры
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // Направление камеры
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);     // Вектор "вверх" для камеры

float yaw = -90.0f;        // Угол поворота по горизонтали (в градусах)
float pitch = 0.0f;        // Угол поворота по вертикали
float lastX = 400.0f;      // Последняя X координата мыши
float lastY = 300.0f;      // Последняя Y координата мыши
bool firstMouse = true;    // Флаг первого движения мыши
float deltaTime = 0.0f;    // Время между кадрами
float lastFrame = 0.0f;    // Время предыдущего кадра

// ------------------- АНИМАЦИОННЫЕ ПАРАМЕТРЫ -------------------
float platformRotation = 0.0f;   // Угол поворота платформы (вся платформа)
float shoulder1Angle = 0.0f;     // Угол поворота первого плеча
float shoulder2Angle = 0.0f;     // Угол поворота второго плеча

// Ограничения углов
const float MAX_ANGLE_SHOULDER1 = 50.0f;   // Максимальный угол первого плеча
const float MIN_ANGLE_SHOULDER1 = -85.0f;  // Минимальный угол первого плеча
const float MAX_ANGLE_SHOULDER2 = 90.0f;  // Максимальный угол второго плеча
const float MIN_ANGLE_SHOULDER2 = -55.0f;  // Минимальный угол второго плеча
const float MAX_ROTATION = 185.0f;         // Максимальный поворот платформы
const float MIN_ROTATION = -185.0f;        // Минимальный поворот платформы
const float ANGLE_SPEED = 60.0f;           // Скорость вращения (градусов в секунду)

const float SHOULDER1_DIRECTION = 1.0f;    // Направление вращения плеча 1
const float SHOULDER2_DIRECTION = 1.0f;    // Направление вращения плеча 2

/**
 * Обработчик движения мыши для управления камерой
 * @param window - указатель на окно GLFW
 * @param xpos - новая X координата мыши
 * @param ypos - новая Y координата мыши
 */
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    // Вычисляем смещение мыши
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos; // Инвертируем Y
    lastX = (float)xpos;
    lastY = (float)ypos;

    // Чувствительность мыши
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Обновляем углы камеры
    yaw += xoffset;
    pitch += yoffset;

    // Ограничиваем pitch, чтобы избежать переворота камеры
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Вычисляем новое направление камеры
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

/**
 * Обработка ввода с клавиатуры
 * @param window - указатель на окно GLFW
 */
void processInput(GLFWwindow* window) {
    float speed = 5.0f * deltaTime;

    glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));

    // Движение камеры (WASD)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= right * speed;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += right * speed;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += speed * cameraUp;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= speed * cameraUp;

    float d = ANGLE_SPEED * deltaTime; // Изменение углов за кадр

    // --- Управление поворотом платформы (Q/E) ---
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        platformRotation -= d;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        platformRotation += d;

    // --- Управление первым плечом (R/F) ---
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        shoulder1Angle += d;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        shoulder1Angle -= d;

    // --- Управление вторым плечом (T/G) ---
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
        shoulder2Angle += d;
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
        shoulder2Angle -= d;

    // === Применяем ограничения ===
    platformRotation = std::clamp(platformRotation,
        MIN_ROTATION, MAX_ROTATION);

    shoulder1Angle = std::clamp(shoulder1Angle,
        MIN_ANGLE_SHOULDER1, MAX_ANGLE_SHOULDER1);

    shoulder2Angle = std::clamp(shoulder2Angle,
        MIN_ANGLE_SHOULDER2, MAX_ANGLE_SHOULDER2);

    // --- Сброс всех углов (K) ---
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        platformRotation = 0;
        shoulder1Angle = 0;
        shoulder2Angle = 0;
    }

    // --- Выход из программы (ESC) ---
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// Глобальные переменные для хранения точек иерархии
glm::vec3 g_platformPivot;      // Точка поворота платформы
glm::vec3 g_shoulder1Pivot;     // Точка поворота первого плеча
glm::vec3 g_shoulder1Joint;     // Точка соединения - ось вращения для второго плеча
glm::vec3 g_shoulder2LocalPivot; // Локальный pivot второго плеча

bool pivotsFound = false;       // Флаг, указывающий, что pivot'ы найдены

/**
 * Применяет иерархические трансформации к части модели
 * @param part - часть модели для трансформации
 * @param baseTransform - базовая трансформация (масштабирование и позиционирование)
 * @return матрица модели для отрисовки части
 */
glm::mat4 applyHierarchicalTransform(const ModelPart& part, const glm::mat4& baseTransform) {
    const std::string& name = part.name;

    // === БАЗОВАЯ ТРАНСФОРМАЦИЯ ===
    glm::mat4 model = baseTransform;

    // === ТРАНСФОРМАЦИЯ ПЛАТФОРМЫ ===
    glm::mat4 platformTransform = model;
    if (name == "Platform" || name == "platform" ||
        name.find("1_plecho") != std::string::npos ||
        name.find("2_plecho") != std::string::npos) {

        // Поворот всей платформы вокруг её pivot'а
        platformTransform = glm::translate(platformTransform, g_platformPivot);
        platformTransform = glm::rotate(platformTransform, glm::radians(platformRotation), glm::vec3(0, 1, 0));
        platformTransform = glm::translate(platformTransform, -g_platformPivot);
    }

    // === ТРАНСФОРМАЦИЯ ПЕРВОГО ПЛЕЧА ===
    glm::mat4 shoulder1Transform = platformTransform;
    if (name.find("1_plecho") != std::string::npos ||
        name.find("2_plecho") != std::string::npos) {

        // Вращение первого плеча вокруг его pivot'а (нижняя точка)
        shoulder1Transform = glm::translate(shoulder1Transform, g_shoulder1Pivot);
        shoulder1Transform = glm::rotate(shoulder1Transform, glm::radians(shoulder1Angle), glm::vec3(0, 0, 1));
        shoulder1Transform = glm::translate(shoulder1Transform, -g_shoulder1Pivot);
    }

    // === ВЫБОР ТРАНСФОРМАЦИИ В ЗАВИСИМОСТИ ОТ ЧАСТИ ===

    // Основание (Statina) - только базовая трансформация
    if (name == "Statina" || name == "statina") {
        return model;
    }

    // Платформа - сохраняем её pivot и возвращаем трансформацию платформы
    if (name == "Platform" || name == "platform") {
        g_platformPivot = part.pivotPoint;
        return platformTransform;
    }

    // Первое плечо - сохраняем его pivot и точку соединения
    if (name.find("1_plecho") != std::string::npos || name.find("1_Plecho") != std::string::npos) {
        g_shoulder1Pivot = part.pivotPoint;

        // Точка соединения для второго плеча (ось вращения)
        // Y координата берется из данных модели (1.5144 - константа для конкретной модели)
        g_shoulder1Joint = glm::vec3(g_shoulder1Pivot.x, 1.5144f, g_shoulder1Pivot.z);

        pivotsFound = true;
        return shoulder1Transform;
    }

    // Второе плечо - вращается вокруг точки соединения с первым плечом
    if (name.find("2_plecho") != std::string::npos || name.find("2_Plecho") != std::string::npos) {
        if (!pivotsFound) return model;

        glm::mat4 shoulder2Transform = shoulder1Transform;

        // Вращение второго плеча вокруг точки соединения
        shoulder2Transform = glm::translate(shoulder2Transform, g_shoulder1Joint);
        shoulder2Transform = glm::rotate(shoulder2Transform, glm::radians(shoulder2Angle), glm::vec3(0, 0, 1));
        shoulder2Transform = glm::translate(shoulder2Transform, -g_shoulder1Joint);

        return shoulder2Transform;
    }

    // Для всех остальных частей возвращаем базовую трансформацию
    return model;
}

int main() {
    // Настройка кодировки консоли для корректного отображения русских символов
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    // Инициализация GLFW
    if (!glfwInit()) {
        std::cout << "GLFW init failed\n";
        return -1;
    }

    // Настройка параметров окна OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Создание окна
    GLFWwindow* window = glfwCreateWindow(800, 600, "KUKA KR 120 R3200 PA - Correct Hierarchy", NULL, NULL);
    if (!window) {
        std::cout << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Скрываем курсор

    // Инициализация GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW init failed\n";
        return -1;
    }

    // Настройка OpenGL
    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);     // Включение Z-буфера
    glEnable(GL_CULL_FACE);      // Включение отсечения задних граней
    glCullFace(GL_BACK);         // Отсекаем задние грани
    glFrontFace(GL_CCW);         // Порядок вершин - против часовой стрелки

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

    // Загрузка модели
    std::string modelPath = "17 KUKA KR 120 R3200 PA.obj";
    std::cout << "\n=== LOADING MODEL ===" << std::endl;
    std::cout << "Path: " << modelPath << std::endl;

    std::vector<ModelPart> modelParts = loadOBJWithGroups(modelPath);

    if (modelParts.empty()) {
        std::cout << "Model not loaded or contains no groups!" << std::endl;
        return -1;
    }

    std::cout << "\nLoaded parts: " << modelParts.size() << std::endl;

    // Вывод информации о всех загруженных частях
    std::cout << "\n=== ИМЕНА ВСЕХ ЧАСТЕЙ ===" << std::endl;
    for (const auto& part : modelParts) {
        std::cout << "Часть: '" << part.name << "', pivot: ("
            << part.pivotPoint.x << ", " << part.pivotPoint.y << ", " << part.pivotPoint.z << ")" << std::endl;
    }
    std::cout << "===========================\n" << std::endl;

    // --- Вершинный шейдер ---
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

    // --- Фрагментный шейдер (модель освещения Фонга) ---
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
        
        // Ambient (фоновое освещение)
        vec3 ambient = light.ambient * material.ambient;
        
        // Diffuse (рассеянное освещение)
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = light.diffuse * (diff * material.diffuse);
        
        // Specular (зеркальное освещение)
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specular = light.specular * (spec * material.specular);
        
        vec3 result = ambient + diffuse + specular;
        FragColor = vec4(result, 1.0);
    }
    )";

    // Компиляция вершинного шейдера
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "Vertex shader error: " << infoLog << std::endl;
    }

    // Компиляция фрагментного шейдера
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Fragment shader error: " << infoLog << std::endl;
    }

    // Линковка шейдерной программы
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Linking error: " << infoLog << std::endl;
    }

    // Удаляем шейдеры, они уже слинкованы в программу
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Получаем расположения uniform-переменных в шейдере
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

    // Матрица проекции (перспективная)
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    // Параметры материала модели
    glm::vec3 materialAmbient = glm::vec3(0.3f, 0.3f, 0.35f);
    glm::vec3 materialDiffuse = glm::vec3(0.7f, 0.7f, 0.75f);
    glm::vec3 materialSpecular = glm::vec3(0.9f, 0.9f, 1.0f);
    float materialShininess = 64.0f;

    // Параметры источника света
    glm::vec3 lightPos = glm::vec3(2.5f, 4.0f, 2.5f);
    glm::vec3 lightAmbient = glm::vec3(0.25f, 0.25f, 0.25f);
    glm::vec3 lightDiffuse = glm::vec3(0.9f, 0.9f, 0.9f);
    glm::vec3 lightSpecular = glm::vec3(1.0f, 1.0f, 1.0f);

    // Базовая трансформация модели (масштабирование и центрирование)
    glm::mat4 baseTransform = glm::mat4(1.0f);
    float scale = 0.8f;
    baseTransform = glm::scale(baseTransform, glm::vec3(scale));
    baseTransform = glm::translate(baseTransform, glm::vec3(-0.66f, -1.04f, 0.0f));

    // Вывод справки по управлению
    std::cout << "\n=== HIERARCHICAL CONTROLS ===" << std::endl;
    std::cout << "Camera: WASD + Mouse, Space/Shift - up/down" << std::endl;
    std::cout << "Q/E - Platform rotation (entire robot rotates)" << std::endl;
    std::cout << "R/F - First shoulder rotation (around bottom point)" << std::endl;
    std::cout << "T/G - Second shoulder rotation (around connection point with first shoulder)" << std::endl;
    std::cout << "K - Reset all angles" << std::endl;
    std::cout << "ESC - Exit" << std::endl;
    std::cout << "========================================\n" << std::endl;
    std::cout << "\nNOTE: Second shoulder now rotates around the connection point with first shoulder!\n" << std::endl;

    int frameCount = 0;

    // Основной цикл
    while (!glfwWindowShouldClose(window)) {
        // Вычисление времени между кадрами
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        if (deltaTime > 0.1f) deltaTime = 0.1f; // Ограничение максимального deltaTime
        lastFrame = currentFrame;

        glfwPollEvents();
        processInput(window);

        // Очистка буферов
        glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Матрица вида (камера)
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        // Установка uniform-переменных в шейдере
        glUseProgram(shaderProgram);

        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

        glUniform3fv(materialAmbientLoc, 1, glm::value_ptr(materialAmbient));
        glUniform3fv(materialDiffuseLoc, 1, glm::value_ptr(materialDiffuse));
        glUniform3fv(materialSpecularLoc, 1, glm::value_ptr(materialSpecular));
        glUniform1f(materialShininessLoc, materialShininess);

        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(lightAmbientLoc, 1, glm::value_ptr(lightAmbient));
        glUniform3fv(lightDiffuseLoc, 1, glm::value_ptr(lightDiffuse));
        glUniform3fv(lightSpecularLoc, 1, glm::value_ptr(lightSpecular));

        // Отрисовка каждой части модели с иерархическими трансформациями
        for (const auto& part : modelParts) {
            glm::mat4 modelMat = applyHierarchicalTransform(part, baseTransform);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
            glBindVertexArray(part.VAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)part.vertices.size());
        }

        // Вывод текущих углов в консоль (раз в 60 кадров)
        if (frameCount % 60 == 0) {
            std::cout << "\rPlatform=" << platformRotation
                << " Shoulder1=" << shoulder1Angle
                << " Shoulder2=" << shoulder2Angle << "   " << std::flush;
        }

        frameCount++;
        glfwSwapBuffers(window); // Обмен буферов
    }

    // Очистка ресурсов
    for (auto& part : modelParts) {
        glDeleteVertexArrays(1, &part.VAO);
        glDeleteBuffers(1, &part.VBO);
    }
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}