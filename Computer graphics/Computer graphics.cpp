// Подключение заголовочных файлов
#include <GL/glew.h>   // Библиотека для управления расширениями OpenGL
#include <GLFW/glfw3.h> // Библиотека для создания окна и контекста OpenGL

#include <iostream>    // Для вывода сообщений в консоль
#include <cmath>       // Для математических функций (sin, cos)
#include <glm/glm.hpp> // Библиотека для работы с векторами и матрицами
#include <glm/gtc/matrix_transform.hpp> // Функции для создания матриц (perspective, lookAt)
#include <glm/gtc/type_ptr.hpp> // Для передачи матриц в шейдер (value_ptr)

// Глобальные переменные для управления камерой
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);   // Позиция камеры
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // Направление камеры
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);     // Вектор "вверх"

// Углы Эйлера для управления направлением камеры
float yaw = -90.0f;   // Рыскание (поворот вокруг оси Y)
float pitch = 0.0f;   // Тангаж (поворот вокруг оси X)
float lastX = 400.0f; // Последняя позиция мыши по X (ширина окна / 2)
float lastY = 300.0f; // Последняя позиция мыши по Y (высота окна / 2)
bool firstMouse = true; // Флаг первого движения мыши

// Скорость движения камеры
float cameraSpeed = 0.05f;
float mouseSensitivity = 0.1f; // Чувствительность мыши

// Функция для обработки ошибок GLFW
static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "Ошибка GLFW: " << error << " - " << description << std::endl;
}

// Функция для обработки движения мыши
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // Вычисляем смещение мыши
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Инвертируем, так как Y увеличивается снизу вверх
    lastX = xpos;
    lastY = ypos;

    // Применяем чувствительность
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    // Обновляем углы Эйлера
    yaw += xoffset;
    pitch += yoffset;

    // Ограничиваем угол тангажа, чтобы избежать переворота камеры
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // Вычисляем новый вектор направления камеры
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

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// Фрагментный шейдер с uniform переменной для цвета
const char* fragmentShaderSource = R"(
#version 460 core
out vec4 FragColor;
uniform vec4 ourColor;

void main() {
    FragColor = ourColor;
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
    // Движение вперед/назад
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;

    // Движение влево/вправо (используем векторное произведение для получения правого вектора)
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    // Движение вверх/вниз
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraUp;

    // Выход по клавише ESC
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Variant 19 - Camera Movement", NULL, NULL);

    if (!window) {
        std::cerr << "Не удалось создать окно GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Настройка обработчиков ввода
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Скрываем курсор и захватываем его
    glfwSetCursorPosCallback(window, mouseCallback); // Устанавливаем обработчик движения мыши

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

    // 5. Данные вершин для треугольника
    float vertices[] = {
        // Координаты вершин
         0.0f,  0.5f, 0.0f,  // Верхняя вершина
        -0.5f, -0.5f, 0.0f,  // Нижняя левая
         0.5f, -0.5f, 0.0f   // Нижняя правая
    };

    // Индексы для EBO
    unsigned int indices[] = {
        0, 1, 2   // Треугольник из вершин 0, 1, 2
    };

    // 6. Создание VAO, VBO, EBO
    GLuint VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 7. Создание шейдерной программы
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (shaderProgram == 0) {
        std::cerr << "Не удалось создать шейдерную программу" << std::endl;
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // 8. Получение расположения uniform переменных
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int colorLoc = glGetUniformLocation(shaderProgram, "ourColor");

    // 9. Создание матриц
    glm::mat4 model = glm::mat4(1.0f); // Модельная матрица (единичная, без трансформаций)

    // Матрица проекции (перспективная)
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),              // Угол обзора (field of view)
        (float)windowWidth / (float)windowHeight, // Соотношение сторон
        0.1f,                             // Ближняя плоскость отсечения
        100.0f                            // Дальняя плоскость отсечения
    );

    // 10. Основной цикл рендеринга
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // Белый фон

    while (!glfwWindowShouldClose(window)) {
        // Обработка ввода с клавиатуры
        processInput(window);

        // Очистка экрана
        glClear(GL_COLOR_BUFFER_BIT);

        // Анимация цвета через uniform переменную
        float timeValue = glfwGetTime();
        float red = 0.3f + 0.7f * (sin(timeValue) * 0.5f + 0.5f);
        float green = 1.0f;
        float blue = 1.0f;

        // Создаем матрицу вида (view matrix) с использованием LookAt
        glm::mat4 view = glm::lookAt(
            cameraPos,                    // Позиция камеры
            cameraPos + cameraFront,      // Точка, на которую смотрит камера
            cameraUp                      // Вектор "вверх"
        );

        // Используем шейдерную программу
        glUseProgram(shaderProgram);

        // Передаем матрицы в шейдер
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Передаем цвет во фрагментный шейдер
        glUniform4f(colorLoc, red, green, blue, 1.0f);

        // Отрисовка треугольника
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

        // Смена кадров и обработка событий
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 11. Завершение работы: освобождение ресурсов
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}