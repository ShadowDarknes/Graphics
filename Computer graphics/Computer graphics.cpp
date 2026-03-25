// Подключение заголовочных файлов
#include <GL/glew.h>   // Библиотека для управления расширениями OpenGL
#include <GLFW/glfw3.h> // Библиотека для создания окна и контекста OpenGL

#include <iostream>    // Для вывода сообщений в консоль
#include <cmath>       // Для математических функций (sin, cos)
#include <vector>      // Для работы с векторами
#include <string>      // Для работы со строками


#include <glm/glm.hpp> // Библиотека для работы с векторами и матрицами
#include <glm/gtc/matrix_transform.hpp> // Функции для создания матриц
#include <glm/gtc/type_ptr.hpp> // Для передачи матриц в шейдер

#include <assimp/Importer.hpp>      // Импортер моделей Assimp
#include <assimp/scene.h>           // Структура сцены Assimp
#include <assimp/postprocess.h>     // Постобработка импортированных данных


// Отключаем предупреждения о преобразовании double в float для Assimp
#pragma warning(disable: 4244)

// Структура вершины для OpenGL
struct Vertex {
    glm::vec3 Position;   // Позиция вершины
    glm::vec3 Normal;     // Нормаль вершины

    Vertex() : Position(glm::vec3(0.0f)), Normal(glm::vec3(0.0f)) {}
    Vertex(glm::vec3 pos, glm::vec3 norm) : Position(pos), Normal(norm) {}
};

// Класс Mesh для хранения и рендеринга одной сетки
class Mesh {
public:
    std::vector<Vertex> vertices;      // Вершины
    std::vector<unsigned int> indices; // Индексы
    unsigned int VAO, VBO, EBO;        // OpenGL буферы

    // Конструктор
    Mesh(std::vector<Vertex> verts, std::vector<unsigned int> inds) {
        vertices = verts;
        indices = inds;
        setupMesh();
    }

    // Настройка буферов OpenGL
    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // Загрузка данных вершин в VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // Загрузка индексов в EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Настройка атрибута позиции (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
        glEnableVertexAttribArray(0);

        // Настройка атрибута нормали (location = 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    // Отрисовка сетки
    void Draw() {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // Деструктор для очистки памяти
    ~Mesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};

// Класс Model для загрузки и управления всей моделью
class Model {
public:
    std::vector<Mesh> meshes;  // Вектор всех сеток модели
    std::string directory;     // Директория модели
    bool gammaCorrection;      // Коррекция гаммы

    // Конструктор
    Model(const std::string& path, bool gamma = false) : gammaCorrection(gamma) {
        loadModel(path);
    }

    // Отрисовка всей модели
    void Draw() {
        for (unsigned int i = 0; i < meshes.size(); i++) {
            meshes[i].Draw();
        }
    }

private:
    // Загрузка модели через Assimp
    void loadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |           // Преобразование всех полигонов в треугольники
            aiProcess_FlipUVs |                // Переворот UV координат (для OpenGL)
            aiProcess_CalcTangentSpace |       // Вычисление касательных и бикасательных
            aiProcess_GenNormals |             // Генерация нормалей, если их нет
            aiProcess_JoinIdenticalVertices |  // Объединение идентичных вершин
            aiProcess_OptimizeMeshes);         // Оптимизация мешей

        // Проверка на ошибки загрузки
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "Ошибка загрузки модели: " << importer.GetErrorString() << std::endl;
            return;
        }

        // Получение директории модели
        size_t lastSlash = path.find_last_of('/');
        if (lastSlash == std::string::npos) {
            lastSlash = path.find_last_of('\\');
        }
        if (lastSlash != std::string::npos) {
            directory = path.substr(0, lastSlash);
        }
        else {
            directory = "";
        }

        // Обработка узлов сцены
        processNode(scene->mRootNode, scene);

        std::cout << "Модель успешно загружена. Количество мешей: " << meshes.size() << std::endl;
        if (meshes.size() > 0) {
            std::cout << "Количество вершин в первом меше: " << meshes[0].vertices.size() << std::endl;
            std::cout << "Количество индексов в первом меше: " << meshes[0].indices.size() << std::endl;
        }
    }

    // Рекурсивная обработка узлов сцены
    void processNode(aiNode* node, const aiScene* scene) {
        // Обработка всех мешей текущего узла
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        // Рекурсивная обработка дочерних узлов
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    // Обработка отдельного меша
    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Резервируем память для оптимизации
        vertices.reserve(mesh->mNumVertices);

        // Обработка вершин с явным преобразованием double в float
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;

            // Позиция вершины - явное преобразование в float
            vertex.Position.x = static_cast<float>(mesh->mVertices[i].x);
            vertex.Position.y = static_cast<float>(mesh->mVertices[i].y);
            vertex.Position.z = static_cast<float>(mesh->mVertices[i].z);

            // Нормаль вершины - явное преобразование в float
            if (mesh->HasNormals()) {
                vertex.Normal.x = static_cast<float>(mesh->mNormals[i].x);
                vertex.Normal.y = static_cast<float>(mesh->mNormals[i].y);
                vertex.Normal.z = static_cast<float>(mesh->mNormals[i].z);
            }
            else {
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f); // Если нет нормалей, используем (0,1,0)
            }

            vertices.push_back(vertex);
        }

        // Резервируем память для индексов
        indices.reserve(mesh->mNumFaces * 3);

        // Обработка индексов (треугольников)
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            // Проверка, что грань содержит 3 индекса (треугольник)
            if (face.mNumIndices == 3) {
                for (unsigned int j = 0; j < face.mNumIndices; j++) {
                    indices.push_back(static_cast<unsigned int>(face.mIndices[j]));
                }
            }
            else {
                std::cerr << "Предупреждение: обнаружена грань с " << face.mNumIndices << " индексами" << std::endl;
            }
        }

        return Mesh(vertices, indices);
    }
};

// Глобальные переменные для управления камерой
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 8.0f);   // Позиция камеры (увеличил для робота)
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // Направление камеры
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);     // Вектор "вверх"

// Углы Эйлера для управления направлением камеры
float yaw = -90.0f;   // Рыскание (поворот вокруг оси Y)
float pitch = 0.0f;   // Тангаж (поворот вокруг оси X)
float lastX = 400.0f; // Последняя позиция мыши по X
float lastY = 300.0f; // Последняя позиция мыши по Y
bool firstMouse = true; // Флаг первого движения мыши

// Скорость движения камеры
float cameraSpeed = 0.1f;   // Увеличил скорость для удобства
float mouseSensitivity = 0.1f; // Чувствительность мыши

// Функция для обработки ошибок GLFW
static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "Ошибка GLFW: " << error << " - " << description << std::endl;
}

// Функция для обработки движения мыши
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos); // Инвертируем
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// Вершинный шейдер с поддержкой матриц
const char* vertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightDir;

out vec3 Normal;
out vec3 LightDir;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    
    // Передаем нормаль и направление света во фрагментный шейдер
    Normal = normalize(mat3(transpose(inverse(model))) * aNormal);
    LightDir = normalize(lightDir);
}
)";

// Фрагментный шейдер с простым освещением
const char* fragmentShaderSource = R"(
#version 460 core
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;

in vec3 Normal;
in vec3 LightDir;

void main() {
    // Простое освещение по Ламберту
    float diff = max(dot(Normal, LightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Ambient освещение
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 result = (ambient + diffuse) * objectColor;
    FragColor = vec4(result, 1.0);
}
)";

// Функция для компиляции шейдера
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Ошибка компиляции шейдера: " << infoLog << std::endl;
        return 0;
    }

    return shader;
}

// Функция для создания шейдерной программы
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (vertexShader == 0 || fragmentShader == 0) {
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Ошибка линковки программы: " << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Функция для обработки ввода с клавиатуры
void processInput(GLFWwindow* window) {
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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Дополнительные клавиши для увеличения/уменьшения скорости
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
        cameraSpeed += 0.01f;
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
        cameraSpeed -= 0.01f;
}

int main() {
    // 1. Инициализация GLFW
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Не удалось инициализировать GLFW" << std::endl;
        return -1;
    }

    // 2. Настройка параметров окна для OpenGL 4.6 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 3. Создание окна и контекста
    const int windowWidth = 800;
    const int windowHeight = 600;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "KUKA KR 120 R3200 PA - Robot Arm", NULL, NULL);

    if (!window) {
        std::cerr << "Не удалось создать окно GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Настройка обработчиков ввода
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);

    // 4. Инициализация GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cerr << "Не удалось инициализировать GLEW: " << glewGetErrorString(glewError) << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Выводим информацию об OpenGL
    std::cout << "Рендерер: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Версия OpenGL: " << glGetString(GL_VERSION) << std::endl;

    // 5. Загрузка 3D модели робота KUKA
    std::string modelPath = "17 KUKA KR 120 R3200 PA.obj";
    std::cout << "Загрузка модели: " << modelPath << std::endl;
    Model ourModel(modelPath);

    // 6. Создание шейдерной программы
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (shaderProgram == 0) {
        std::cerr << "Не удалось создать шейдерную программу" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // 7. Получение расположения uniform переменных
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    int lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    int lightDirLoc = glGetUniformLocation(shaderProgram, "lightDir");

    // 8. Создание матриц
    glm::mat4 model = glm::mat4(1.0f);

    // Масштабируем модель, если нужно (робот может быть слишком большим или маленьким)
    float scale = 0.5f; // Подберите масштаб под вашу модель
    model = glm::scale(model, glm::vec3(scale, scale, scale));

    // Матрица проекции (перспективная)
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(windowWidth) / static_cast<float>(windowHeight),
        0.1f,
        100.0f
    );

    // Параметры освещения
    glm::vec3 objectColor = glm::vec3(0.6f, 0.7f, 0.8f);  // Металлический серо-голубой цвет для робота
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);   // Белый свет
    glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 2.0f, 1.0f)); // Направление света сверху-сбоку

    // 9. Настройка OpenGL
    glEnable(GL_DEPTH_TEST); // Включение теста глубины для корректной отрисовки 3D объектов
    glEnable(GL_CULL_FACE);  // Включение отсечения задних граней для оптимизации
    glCullFace(GL_BACK);     // Отсекаем задние грани

    // 10. Основной цикл рендеринга
    glClearColor(0.1f, 0.15f, 0.2f, 1.0f); // Темно-синий фон, похожий на промышленный

    std::cout << "Начало рендеринга. Управление:" << std::endl;
    std::cout << "WASD - перемещение камеры" << std::endl;
    std::cout << "Space/Shift - вверх/вниз" << std::endl;
    std::cout << "Мышь - поворот камеры" << std::endl;
    std::cout << "Num+ / Num- - изменение скорости камеры" << std::endl;
    std::cout << "ESC - выход" << std::endl;

    while (!glfwWindowShouldClose(window)) {
        // Обработка ввода с клавиатуры
        processInput(window);

        // Очистка экрана
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Создаем матрицу вида
        glm::mat4 view = glm::lookAt(
            cameraPos,
            cameraPos + cameraFront,
            cameraUp
        );

        // Используем шейдерную программу
        glUseProgram(shaderProgram);

        // Передаем матрицы в шейдер
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Передаем параметры освещения в шейдер
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(objectColor));
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

        // Отрисовка загруженной модели
        ourModel.Draw();

        // Смена кадров и обработка событий
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 11. Завершение работы
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}