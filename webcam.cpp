/**
 * Originally based off 
 * https://github.com/cirosantilli/cpp-cheat/blob/master/opengl/glfw_webcam_image_process.c
 *
 */

#include "common.h"
#include "v4l2/common_v4l2.h"

static const GLuint WIDTH = 640;
static const GLuint HEIGHT = 480;
static const GLfloat vertices[] = {
/*  xy            uv */
    -1.0,  1.0,   0.0, 1.0,
     0.0,  1.0,   0.0, 0.0,
     0.0, -1.0,   1.0, 0.0,
    -1.0, -1.0,   1.0, 1.0,
};
static const GLuint indices[] = {
    0, 1, 2,
    0, 2, 3,
};

static const GLchar *vertex_shader_source =
    "#version 330 core\n"
    "in vec2 coord2d;\n"
    "in vec2 vertexUv;\n"
    "out vec2 fragmentUv;\n"
    "void main() {\n"
    "    gl_Position = vec4(coord2d, 0, 1);\n"
    "    fragmentUv = vertexUv;\n"
    "}\n";
static const GLchar *fragment_shader_source =
    "#version 330 core\n"
    "in vec2 fragmentUv;\n"
    "out vec3 color;\n"
    "uniform sampler2D textureSampler;\n"
    "void main() {\n"
    "    color = texture(textureSampler, fragmentUv.yx).rgb;\n"
    "}\n";

static const GLchar *vertex_shader_source2 =
    "#version 330 core\n"
    "in vec2 coord2d;\n"
    "in vec2 vertexUv;\n"
    "out vec2 fragmentUv;\n"
    "void main() {\n"
    "    gl_Position = vec4(coord2d + vec2(1.0, 0.0), 0, 1);\n"
    "    fragmentUv = vertexUv;\n"
    "}\n";
static const GLchar *fragment_shader_source2 =
    "#version 330 core\n"
    "in vec2 fragmentUv;\n"
    "out vec3 color;\n"
    "uniform sampler2D textureSampler;\n"
    "/* pixel Delta. How large a pixel is in 0.0 to 1.0 that textures use. */\n"
    "uniform vec2 pixD;\n"
    "void main() {\n"

    /*"// Identity\n"*/
    "    vec2 uv = vec2(fragmentUv.y, pixD.x-fragmentUv.x);\n"
    "    uv.x += 0.1*sin(uv.y*20.0);\n"
    "    color = texture(textureSampler, uv ).rgb;\n"

    "}\n";

int main(int argc, char **argv) {
    CommonV4l2 common_v4l2;
    GLFWwindow *window;
    GLint
        coord2d_location,
        textureSampler_location,
        vertexUv_location,
        coord2d_location2,
        pixD_location2,
        textureSampler_location2,
        vertexUv_location2
    ;
    GLuint
        ebo,
        program,
        program2,
        texture,
        vbo,
        vao,
        vao2
    ;
    unsigned int
        cpu,
        width,
        height
    ;
    uint8_t *image;
    float *image2 = NULL;

    /* CLI arguments. */
    if (argc > 1) {
        width = strtol(argv[1], NULL, 10);
    } else {
        width = WIDTH;
    }
    if (argc > 2) {
        height = strtol(argv[2], NULL, 10);
    } else {
        height = HEIGHT;
    }
    if (argc > 3) {
        cpu = (argv[3][0] == '1');
    } else {
        cpu = 0;
    }

    /* Window system. */
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    window = glfwCreateWindow(2 * width, height, __FILE__, NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();
    CommonV4l2_init(&common_v4l2, (char*) COMMON_V4L2_DEVICE, width, height);

    /* Shader setup. */
    program = common_get_shader_program(vertex_shader_source, fragment_shader_source);
    coord2d_location = glGetAttribLocation(program, "coord2d");
    vertexUv_location = glGetAttribLocation(program, "vertexUv");
    textureSampler_location = glGetUniformLocation(program, "textureSampler");

    /* Shader setup 2. */
    const GLchar *fs;
    if (cpu) {
        fs = fragment_shader_source;
    } else {
        fs = fragment_shader_source2;
    }
    program2 = common_get_shader_program(vertex_shader_source2, fs);
    coord2d_location2 = glGetAttribLocation(program2, "coord2d");
    vertexUv_location2 = glGetAttribLocation(program2, "vertexUv");
    textureSampler_location2 = glGetUniformLocation(program2, "textureSampler");
    pixD_location2 = glGetUniformLocation(program2, "pixD");

    /* Create vbo. */
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* Create ebo. */
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    /* vao. */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(coord2d_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(vertices[0]), (GLvoid*)0);
    glEnableVertexAttribArray(coord2d_location);
    glVertexAttribPointer(vertexUv_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(vertices[0])));
    glEnableVertexAttribArray(vertexUv_location);
    glBindVertexArray(0);

    /* vao2. */
    glGenVertexArrays(1, &vao2);
    glBindVertexArray(vao2);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(coord2d_location2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(vertices[0]), (GLvoid*)0);
    glEnableVertexAttribArray(coord2d_location2);
    glVertexAttribPointer(vertexUv_location2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(vertices[0])));
    glEnableVertexAttribArray(vertexUv_location2);
    glBindVertexArray(0);

    /* Texture buffer. */
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    /* Constant state. */
    glViewport(0, 0, 2 * width, height);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glActiveTexture(GL_TEXTURE0);

    /* Main loop. */
    common_fps_init();
    do {
        /* Blocks until an image is available, thus capping FPS to that.
         * 30FPS is common in cheap webcams. */
        CommonV4l2_updateImage(&common_v4l2);
        image = (uint8_t*) CommonV4l2_getImage(&common_v4l2);
        glClear(GL_COLOR_BUFFER_BIT);

        /* Original. */
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB, width, height,
            0, GL_RGB, GL_UNSIGNED_BYTE, image
        );
        glUseProgram(program);
        glUniform1i(textureSampler_location, 0);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        /* Modified. */
        glUseProgram(program2);
        glUniform1i(textureSampler_location2, 0);
        glUniform2f(pixD_location2, 1.0 / width, 1.0 / height);
        glBindVertexArray(vao2);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
        common_fps_print();
    } while (!glfwWindowShouldClose(window));

    /* Cleanup. */
    CommonV4l2_deinit(&common_v4l2);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    glfwTerminate();
    return EXIT_SUCCESS;
}
