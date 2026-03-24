// Подключение заголовочных файлов
// Важно: glew.h подключается до glfw.h
#include <GL/glew.h>   // Библиотека для управления расширениями OpenGL
#include <GLFW/glfw3.h> // Библиотека для создания окна и контекста OpenGL

#include <iostream>    // Для вывода сообщений в консоль
#include <cstdlib>     // Для system("pause") при необходимости

// Функция для обработки ошибок GLFW (полезна для отладки)
static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "Ошибка GLFW: " << error << " - " << description << std::endl;
}

int main() {
    // 1. Инициализация GLFW
    glfwSetErrorCallback(glfwErrorCallback); // Устанавливаем обработчик ошибок
    if (!glfwInit()) {
        std::cerr << "Не удалось инициализировать GLFW" << std::endl;
        return -1;
    }

    // 2. Настройка параметров окна для OpenGL 1.0
    // Запрашиваем OpenGL версии 1.0 (без использования Core Profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // Для версии 1.0 НЕ используем GLFW_OPENGL_FORWARD_COMPAT и GLFW_OPENGL_PROFILE
    // Они закомментированы, как указано в инструкции.
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 3. Создание окна и контекста
    const int windowWidth = 800;
    const int windowHeight = 600;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Variant 19", NULL, NULL);

    if (!window) {
        std::cerr << "Не удалось создать окно GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Делаем созданный контекст текущим для этого потока
    glfwMakeContextCurrent(window);

    // Включаем функцию вертикальной синхронизации (vsync) для плавности, необязательно
    glfwSwapInterval(1);

    // 4. Инициализация GLEW
    // Важно: glewInit() должен вызываться после создания контекста OpenGL
    glewExperimental = GL_TRUE; // Разрешаем GLEW использовать новейшие техники
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cerr << "Не удалось инициализировать GLEW: " << glewGetErrorString(glewError) << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Выводим информацию об OpenGL в консоль (дополнительно)
    std::cout << "Рендерер: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Версия OpenGL: " << glGetString(GL_VERSION) << std::endl;

    // 5. Основной цикл рендеринга
    // Цвет фона по варианту 19: (1.0, 1.0, 1.0) - белый
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Цвет треугольника по варианту 19: (0.3, 1.0, 1.0) - бирюзовый
    // Устанавливаем цвет, который будет использоваться для всех вершин
    // в режиме immediate mode (glBegin/glEnd)
    glColor3f(0.3f, 1.0f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        // a) Очистка старого кадра
        glClear(GL_COLOR_BUFFER_BIT);

        // b) Отрисовка нового кадра (треугольника)
        // Начало отрисовки примитива "Треугольник"
        glBegin(GL_TRIANGLES);
        // Первая вершина (верхняя)
        glVertex2f(0.0f, 0.5f);
        // Вторая вершина (нижняя левая)
        glVertex2f(-0.5f, -0.5f);
        // Третья вершина (нижняя правая)
        glVertex2f(0.5f, -0.5f);
        glEnd(); // Конец отрисовки

        // c) Смена кадров (двойная буферизация)
        glfwSwapBuffers(window);

        // d) Обработка событий (клавиатура, мышь, закрытие окна)
        glfwPollEvents();
    }

    // 6. Завершение работы: освобождение ресурсов
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}