// Подключение заголовочных файлов
// Важно: glew.h подключается до glfw.h
#include <GL/glew.h>   // Библиотека для управления расширениями OpenGL
#include <GLFW/glfw3.h> // Библиотека для создания окна и контекста OpenGL

#include <iostream>    // Для вывода сообщений в консоль
#include <cmath>       // Для математических функций (sin, cos)

// Функция для обработки ошибок GLFW (полезна для отладки)
static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "Ошибка GLFW: " << error << " - " << description << std::endl;
}

// Вершинный шейдер (GLSL)
const char* vertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)";

// Фрагментный шейдер (GLSL) с uniform переменной для цвета
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

    // Проверка на ошибки компиляции
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

    // Проверка на ошибки линковки
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Ошибка линковки программы: " << infoLog << std::endl;
        return 0;
    }

    // Шейдеры можно удалить после линковки
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
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
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Variant 19 - VBO VAO EBO", NULL, NULL);

    if (!window) {
        std::cerr << "Не удалось создать окно GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

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

    // 5. Данные вершин для треугольника (3 вершины)
    // Для создания двух треугольников (квадрата) нужно 4 вершины и 6 индексов
    // Но по заданию используем треугольник
    float vertices[] = {
        // Координаты вершин
         0.0f,  0.5f, 0.0f,  // Верхняя вершина
        -0.5f, -0.5f, 0.0f,  // Нижняя левая
         0.5f, -0.5f, 0.0f   // Нижняя правая
    };

    // Индексы для EBO (для треугольника достаточно 3 индекса)
    unsigned int indices[] = {
        0, 1, 2   // Треугольник из вершин 0, 1, 2
    };

    // 6. Создание VAO, VBO, EBO
    GLuint VAO, VBO, EBO;

    // Генерация объектов
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Настройка VAO
    glBindVertexArray(VAO);

    // Настройка VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Настройка EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Настройка атрибутов вершин
    // Атрибут позиции (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Отвязываем буферы (необязательно, но полезно для предотвращения случайных изменений)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // EBO остается привязанным к VAO

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

    // 8. Основной цикл рендеринга
    // Цвет фона по варианту 19: (1.0, 1.0, 1.0) - белый
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Получаем расположение uniform переменной в шейдере
    int colorLocation = glGetUniformLocation(shaderProgram, "ourColor");

    while (!glfwWindowShouldClose(window)) {
        // a) Очистка старого кадра
        glClear(GL_COLOR_BUFFER_BIT);

        // b) Анимация цвета через uniform переменную
        // Получаем текущее время
        float timeValue = glfwGetTime();

        // Рассчитываем цвет в зависимости от времени
        // Используем синусоидальные функции для плавного изменения цветов
        // Вариант 19: базовый цвет (0.3, 1.0, 1.0) - бирюзовый, но с анимацией
        float red = 0.3f + 0.7f * (sin(timeValue) * 0.5f + 0.5f);     // Изменяется от 0.3 до 1.0
        float green = 1.0f;                                           // Зеленый остается максимальным
        float blue = 1.0f;                                            // Синий остается максимальным

        // Альтернативный вариант с полным циклом изменения цветов:
        // float red = (sin(timeValue) + 1.0f) / 2.0f;
        // float green = (sin(timeValue + 2.0f) + 1.0f) / 2.0f;
        // float blue = (sin(timeValue + 4.0f) + 1.0f) / 2.0f;

        // Используем шейдерную программу
        glUseProgram(shaderProgram);

        // Передаем цвет во фрагментный шейдер через uniform переменную
        glUniform4f(colorLocation, red, green, blue, 1.0f);

        // c) Отрисовка треугольника с использованием VAO и EBO
        glBindVertexArray(VAO);
        // Используем glDrawElements для отрисовки по индексам
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
        // glBindVertexArray(0); // Необязательно, так как следующий кадр снова привяжет VAO

        // d) Смена кадров (двойная буферизация)
        glfwSwapBuffers(window);

        // e) Обработка событий
        glfwPollEvents();
    }

    // 9. Завершение работы: освобождение ресурсов
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}