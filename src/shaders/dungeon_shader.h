#include <stdlib.h>

namespace octet {


  class dungeon_shader : public shader {

    // index for model space to projection space matrix
    GLuint modelToProjection_index;
    GLuint samplerIndex_;

  public:
    void init() {

      // vertex shader
      const char vertex_shader[] = SHADER_STR(
        // setting a variable to varying permits to share among vertex shader and fragment shader
        varying vec4 pos_;
      varying vec2 uv_;


      // attributes are passed with OPENGL glVertexAttribPointer
      attribute vec4 pos;    
      attribute vec2 uv;

      // from glUniform
      uniform mat4 modelToProjection;

      void main() {
        pos_ = pos;          
        uv_ = uv;

        gl_Position = modelToProjection * pos;
      }
      );

      // fragment shader
      const char fragment_shader[] = SHADER_STR(

      varying vec4 pos_;
      varying vec2 uv_;

      uniform sampler2D sampler;

      void main() {
        //srand (static_cast <unsigned> (time(0)));
        gl_FragColor = texture2D(sampler, uv_);
      }
      );

      init_uniforms(vertex_shader, fragment_shader);
    }

    void init_uniforms(const char *vertex_shader, const char *fragment_shader) {
      // use the common shader code to compile and link the shaders
      // the result is a shader program
      shader::init(vertex_shader, fragment_shader);

      // extract the indices of the uniforms to use later
      modelToProjection_index = glGetUniformLocation(program(), "modelToProjection");
      samplerIndex_ = glGetUniformLocation(program(), "sampler");

    }

    void render(const mat4t &modelToProjection, int sampler) {
      // tell openGL to use the program
      shader::render();

      // set the uniforms
      glUniformMatrix4fv(modelToProjection_index, 1, GL_FALSE, modelToProjection.get()); 

      glUniform1i(samplerIndex_, sampler);
    }


  };

}