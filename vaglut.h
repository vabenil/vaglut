#ifndef VAGLUT_H
#define VAGLUT_H

#include <GL/glew.h>
#include <GL/glu.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


#ifdef VAGLUT_STATIC
#define VAGLUT_DEF static
#else
#define VAGLUT_DEF
#endif /* VAGLUT_STATIC */


VAGLUT_DEF
void vaglut_set_wireframe_mode(GLboolean wiref_mode)
{
    glPolygonMode(GL_FRONT_AND_BACK, wiref_mode ? GL_LINE : GL_FILL);
}

VAGLUT_DEF
int vaglut_load_file(char **output_buf, const char *file_name)
{
    if (file_name == NULL) return -1;

    if(access(file_name, F_OK) != -1) {
        int buf_len;
        long int buf_size;
        
        // Open file
        FILE *file = fopen(file_name, "re");
        {
            if(file) {
                fseek(file, 0, SEEK_END);
                buf_size = ftell(file);
                buf_len = buf_size / sizeof(char);
                rewind(file);

                *output_buf = (char *)malloc(buf_size + 2);

                GLulong read_size =
                    fread(
                        *output_buf,
                        sizeof(char),
                        buf_len, file);

                if (read_size == buf_size) {  
                    // fread doesn't set the \0 character
                    *(*(output_buf) + buf_len) = '\0';

                    fclose(file);
                    return buf_len;
                } 

                if(feof(file)) { 
                    printf("Error reading file %s: Unexpected end of file\n",
                            file_name);
                } else { 
                    printf("Error: Couldn't read file %s\n", file_name);
                }
            } 
        }
        fclose(file); /* Close file */
    } else {
        fprintf(stderr, "Couldn't access %s file\n", file_name);
    }

    return -1;
}


VAGLUT_DEF
void vaglut_clear_errors()
{
    while (glGetError() != GL_NO_ERROR);
} 


VAGLUT_DEF
GLboolean vaglut_check_errors()
{
    GLenum error;
    while ((error = glGetError())) {
        fprintf(stderr, "GL_ERROR: %d\n", error);
        return 1;
    }
    return 0;
} 


VAGLUT_DEF
GLuint vaglut_shader_check_error(
        GLuint shader, GLboolean is_program,
        GLenum flag, const char *error_message)
{
    int success = GL_FALSE;
    char error_log[1024] = {0};
    int log_size = 0;

    is_program
        ? glGetProgramiv(shader, flag, &success)
        : glGetShaderiv(shader, flag, &success);

    if (!success) {
        is_program
            ? glGetProgramInfoLog(
                    shader, sizeof(error_log), &log_size, error_log)
            : glGetShaderInfoLog(
                    shader, sizeof(error_log), &log_size, error_log);

        if (log_size) {
            fprintf( stderr, "%s\n", error_message);
            fprintf(stderr, "%s\n", error_log);
            /* saveErrorLog( error_log ); */
        }
    }
    return success;
}


VAGLUT_DEF
GLint vaglut_program_attach_shaders(
        GLuint program, GLuint vert_shader,
        GLuint geom_shader, GLuint frag_shader)
{
    GLint success;
    if (vert_shader)
    glAttachShader(program, vert_shader);
    
    if (geom_shader)
    glAttachShader(program, geom_shader);

    if (frag_shader)
    glAttachShader(program, frag_shader);

    glLinkProgram(program);
    success = vaglut_shader_check_error(
            program, GL_TRUE,
            GL_LINK_STATUS,
            "Error: Couldn't link program");

    if (!success) return success;

    glValidateProgram(program);

    success = vaglut_shader_check_error(
            program, GL_TRUE,
            GL_VALIDATE_STATUS,
            "Error: Program is invalid");

    return success;
} 


VAGLUT_DEF
GLint vaglut_shader_compile_src(GLuint shader, const char *src)
{
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint success = vaglut_shader_check_error(
            shader, GL_FALSE,
            GL_COMPILE_STATUS,
            "Error: couldn't compile shader");

    return success;
}


VAGLUT_DEF
GLint vaglut_shader_compile_f(GLuint shader, const char *file_name)
{
    char *src = NULL;
    int buf_len = vaglut_load_file(&src, file_name);
    if (buf_len == -1) {
        return 0;
    }

    if (src == NULL) {
        printf("Invalid batch\n");
        return 0;
    }

    GLint success = vaglut_shader_compile_src(shader, src);

    free(src);

    return success;
} 


VAGLUT_DEF
GLuint vaglut_shaders_compile_src(
        const char *vert_shader_src,
        const char *geom_shader_src,
        const char *frag_shader_src)
{
    GLuint program, v_shader, g_shader, f_shader;
    v_shader = vert_shader_src ? glCreateShader(GL_VERTEX_SHADER) : 0;
    g_shader = geom_shader_src ? glCreateShader(GL_GEOMETRY_SHADER) : 0;
    f_shader = frag_shader_src ? glCreateShader(GL_FRAGMENT_SHADER) : 0;
    program = glCreateProgram(); 

    if (vert_shader_src)
    if (!vaglut_shader_compile_src(v_shader, vert_shader_src)) {
        return GL_FALSE;
    }

    if (geom_shader_src)
    if (!vaglut_shader_compile_src(g_shader, geom_shader_src)) {
        return GL_FALSE;
    }

    if (frag_shader_src)
    if (!vaglut_shader_compile_src(f_shader, frag_shader_src)) {
        return GL_FALSE;
    }

    if (!vaglut_program_attach_shaders(
                program, v_shader, g_shader, f_shader)) {
        return GL_FALSE;
    }

    glDeleteShader(v_shader);
    glDeleteShader(g_shader);
    glDeleteShader(f_shader);

    return program;
}


VAGLUT_DEF
GLuint vaglut_shaders_compile_f(
        const char *vert_shader_f,
        const char *geom_shader_f,
        const char *frag_shader_f)
{
    char *v_shader_src = NULL;
    char *g_shader_src = NULL;
    char *f_shader_src = NULL;

    vaglut_load_file(&v_shader_src, vert_shader_f);
    vaglut_load_file(&g_shader_src, geom_shader_f);
    vaglut_load_file(&f_shader_src, frag_shader_f);

    GLuint program = 
        vaglut_shaders_compile_src(v_shader_src, g_shader_src, f_shader_src);

    return program;
} 


#endif /* VAGLUT_H */
