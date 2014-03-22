////////////////////////////////////////////////////////////////////////////////
//
// (C) M. Dimitri Alvarez 2013
//
// Fractal Landscape
//

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include <cmath>
#include <math.h> 

     
namespace octet {

#define LANDSCAPE_SIZE 65
  
  class Point{
 
    float x;
    float y;
    float z;
    bool justGenerated;

  public:

    Point(){
      this->x = 0.0;
      this->y = 0.0;
      this->z = 0.0;
      this->justGenerated = false;
    }

    Point(float x,float y,float z, bool b){
      this->x = x;
      this->y = y;
      this->z = z;
      this->justGenerated = b;
    }

    float getX(){
      return this->x;
    }

    void setX(float x){
      this->x = x; 
    }

    float getY(){
      return this->y;
    }

    void setY(float y){
      this->y = y; 
    }

    float getZ(){
      return this->z;
    }

    void setZ(float z){
      this->z = z; 
    }

    bool isJustGenerated(){
      return this->justGenerated;
    }

    void setJustGenerated(bool b){
      this->justGenerated = b;
    }
  };

  class Tile{
  public:
    Point points [4];
  };


	class Landscape : public octet::app {
		
    // Matrix to transform points in our camera space to the world.
		// This lets us move our camera
		mat4t cameraToWorld;

    mat4t modelToWorld;

    color_shader color_shader_;

    Point heightMap [LANDSCAPE_SIZE] [LANDSCAPE_SIZE];

    Tile wireFrameVertices [(LANDSCAPE_SIZE-1)*(LANDSCAPE_SIZE-1)];

    //Bool to control key pressing
		bool iterationForward;
		bool iterationBackward;
        
    int currentIteration;


  public:

    // this is called when we construct the class
    Landscape(int argc, char **argv) : app(argc, argv){
    }

    // this is called once OpenGL is initialized
    void app_init() {

      color_shader_.init();

      cameraToWorld.loadIdentity();
      cameraToWorld.translate(LANDSCAPE_SIZE/2,6,10);
      modelToWorld.loadIdentity();

      srand (static_cast <unsigned> (time(0)));

      setXZValues();

      setInitialCorners();

      diamondSquareAlgorithm();

      generateVerticesWireFrameModel();

    }

    void renderMap(Tile tile){
      mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);
      
      // set up the uniforms for the shader
      color_shader_.render(modelToProjection, vec4(0.0,0.0,0.0,1));

      // this is an array of the positions of the corners of the box in 3D
      // a straight "float" here means this array is being generated here at runtime.


      float vertices[] = {
        tile.points[0].getX(),  tile.points[0].getY(), tile.points[0].getZ(),
        tile.points[1].getX(),  tile.points[1].getY(), tile.points[1].getZ(),
        tile.points[2].getX(),  tile.points[2].getY(), tile.points[2].getZ(),
        tile.points[3].getX(),  tile.points[3].getY(), tile.points[3].getZ(),
      };

        // attribute_pos (=0) is position of each corner
        // each corner has 3 floats (x, y, z)
        // there is no gap between the 3 floats and hence the stride is 3*sizeof(float)
        glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)vertices);
        glEnableVertexAttribArray(attribute_pos);


        // finally, draw the box (4 vertices)
        glDrawArrays(GL_LINE_LOOP, 0, 4);
        //index=0;

      //}
    } 

    void generateVerticesWireFrameModel(){

      int index=0;

      for(int i=0; i!=LANDSCAPE_SIZE;++i){
        for(int j=0; j!=LANDSCAPE_SIZE;++j){

          if( i<(LANDSCAPE_SIZE-1) && j<(LANDSCAPE_SIZE-1)){

            wireFrameVertices[index].points[0] = heightMap[i][j];
            wireFrameVertices[index].points[1] = heightMap[i+1][j];
            wireFrameVertices[index].points[2] = heightMap[i+1][j+1];
            wireFrameVertices[index].points[3] = heightMap[i][j+1];

            index++;

          }

        }
      }
    }

    void printMatrix(){

      printf("\n");

      for(int i=0; i!=LANDSCAPE_SIZE;++i){
        for(int j=0; j!=LANDSCAPE_SIZE;++j){
          printf("%.2f ",heightMap[i][j].getY());
        }
        printf("\n");
      }

      printf("\n");
    }


    float getDiamondGeneratedValue(int row, int column, int distance, float randomLow, float randomHigh){

      float generatedValue =0.0;

      //We get the mean
      generatedValue = ( heightMap[std::abs(row-distance)][std::abs(column-distance)].getY() + 
                         heightMap[std::abs(row-distance)][std::abs(column+distance)].getY() + 
                         heightMap[std::abs(row+distance)][std::abs(column-distance)].getY() +
                         heightMap[std::abs(row+distance)][std::abs(column+distance)].getY() ) / 4;


      float r = randomLow + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(randomHigh-randomLow)));

      //printf("Random number Diamond: %.2f\n\n",r);

      //We generate a random value
      generatedValue += r;

      return generatedValue;
    }

    float getSquareGeneratedValue( int row, int column, int distance, float randomLow, float randomHigh ) 
    {
      float generatedValue =0.0;
      int counter = 0;

      if(column + distance < LANDSCAPE_SIZE){
        counter++;
        generatedValue+= heightMap[std::abs(row)][std::abs(column+distance)].getY();
      }

      if(row + distance < LANDSCAPE_SIZE){
        counter++;
        generatedValue+= heightMap[std::abs(row+distance)][std::abs(column)].getY();
      }

      if(column - distance >= 0){
        counter++;
        generatedValue+= heightMap[std::abs(row)][std::abs(column-distance)].getY();
      }

      if(row - distance >= 0){
        counter++;
        generatedValue+= heightMap[std::abs(row-distance)][std::abs(column)].getY();
      }

      //We get the mean
      generatedValue = generatedValue / counter;

      float r = randomLow + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(randomHigh-randomLow)));

     //printf("Number Square: %.2f\n\n",generatedValue);

      //We add a random value
      generatedValue += r;

      return generatedValue;
    }

    void diamondSquareAlgorithm(){

      int distance = LANDSCAPE_SIZE-1;
      float randomLow = -30.0;
      float randomHigh = 30.0;


      for(int iteration=0;iteration!=std::log10((double)LANDSCAPE_SIZE-1)/std::log10((double)2.0);++iteration){
        
          // Diamond step
          //printf("Diamond step %d \n",iteration+1);

          for(int i=0; i!=LANDSCAPE_SIZE;++i){
            for(int j=0; j!=LANDSCAPE_SIZE;++j){
          
              if(heightMap[i][j].getY() != 0.0f && !heightMap[i][j].isJustGenerated()){
                if( (i + distance < LANDSCAPE_SIZE) && (j + distance < LANDSCAPE_SIZE)){

                    int row = i + distance/2;
                    int column = j + distance/2;

                    heightMap[row][column].setY(getDiamondGeneratedValue(row,column,distance/2,randomLow,randomHigh));
                    heightMap[row][column].setJustGenerated(true);
                }
              }

            }
          }

          setPointsAsExistingValues();
          //printMatrix();

          // Square step
          //printf("Square step %d \n",iteration+1);

          for(int k=0; k!=LANDSCAPE_SIZE;++k){
            for(int l=0; l!=LANDSCAPE_SIZE;++l){

              if(heightMap[k][l].getY() != 0.0f && !heightMap[k][l].isJustGenerated()){
                if( (k + distance/2 < LANDSCAPE_SIZE) && (l + distance/2 < LANDSCAPE_SIZE)){

                    int row1 = k;
                    int column1 = l + distance/2;

                    if(heightMap[row1][column1].getY() == 0.0f || !heightMap[row1][column1].isJustGenerated()){

                      heightMap[row1][column1].setY(getSquareGeneratedValue(row1,column1,distance/2,randomLow,randomHigh));
                      heightMap[row1][column1].setJustGenerated(true);
                    }

                    int row2 = k + distance/2;
                    int column2 = l;

                    if(heightMap[row2][column2].getY() == 0.0f || !heightMap[row2][column2].isJustGenerated()){

                      heightMap[row2][column2].setY(getSquareGeneratedValue(row2,column2,distance/2,randomLow,randomHigh));
                      heightMap[row2][column2].setJustGenerated(true);
                    }
                
                }
              }
            }
          }

          setPointsAsExistingValues();
          //printMatrix();

          
          distance /= 2;

          randomHigh /= 2;
          randomLow /= 2;

      }
          
    }

    void setPointsAsExistingValues(){
      for(int i=0; i!=LANDSCAPE_SIZE;++i){
        for(int j=0; j!=LANDSCAPE_SIZE;++j){
          heightMap[i][j].setJustGenerated(false);
        }
      }
    }

    void setInitialCorners() 
    {

      heightMap[0][0].setY( static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/10.0f)) );
      heightMap[0][LANDSCAPE_SIZE-1].setY( static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/10.0f)) );
      heightMap[LANDSCAPE_SIZE-1][0].setY( static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/10.0f)) );
      heightMap[LANDSCAPE_SIZE-1][LANDSCAPE_SIZE-1].setY( static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/10.0f)) );

    }

    void setXZValues(){
      for(int i=0; i!=LANDSCAPE_SIZE;++i){
        for(int j=0; j!=LANDSCAPE_SIZE;++j){
          heightMap[i][j].setX((float)i);
          heightMap[i][j].setZ((float)j);
        }
      }
    }

    void simulate() {

      //Iteration forward
      if(is_key_down(key_right) && !this->iterationForward){



        this->iterationForward=true;
        this->currentIteration++;

      }else{

        this->iterationForward=false;
      }

      //Iteration backward
      if(is_key_down(key_left) && !this->iterationBackward){

        if(currentIteration>0){

          this->iterationBackward=true;
          this->currentIteration--;

        }


      }else{

        this->iterationBackward=false;

      }


      //Move camera up and down
      if(is_key_down('D')){
        cameraToWorld.translate(1.5,0,0);
      }

      if(is_key_down('A')){
        cameraToWorld.translate(-1.5,0,0);
      }

      if(is_key_down('W')){
        cameraToWorld.translate(0,+1.5,0);
      }

      if(is_key_down('S')){
        cameraToWorld.translate(0,-1.5,0);
      }

      //Rotation of the l-system
      if(is_key_down('Q')){
        cameraToWorld.rotateY(-1.5);
      }

      if(is_key_down('E')){
        cameraToWorld.rotateY(1.5);
      } 

      if(is_key_down(key_up)){
        cameraToWorld.rotateX(1.5);
      }

      if(is_key_down(key_down)){
        cameraToWorld.rotateX(-1.5);
      } 

      //Zoom in and out
      if(is_key_down('I')){
        cameraToWorld.translate(0,0,-1.5);
      }

      if(is_key_down('O')){
        cameraToWorld.translate(0,0,1.5);
      }

      if(is_key_down(key_space)){
        cameraToWorld.loadIdentity();
        cameraToWorld.translate(LANDSCAPE_SIZE/2,6,10);
      }

      if(is_key_down('G')){

        printf("Generando...\n");

        setXZValues();

        setInitialCorners();

        diamondSquareAlgorithm();

        generateVerticesWireFrameModel();

        printf("Generado...\n");
      }
    }

		// this is called to draw the world
		void draw_world(int x, int y, int w, int h) {
			
			simulate();

			// set a viewport - includes whole window area
			glViewport(x, y, w, h);

			// background color

			glClearColor(0.93f, 0.94f, 0.85f, 1);
      //glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// allow Z buffer depth testing (closer objects are always drawn in front of far ones)
      
			glEnable(GL_DEPTH_TEST);
      

      // allow alpha blend (transparency when alpha channel is 0)
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


			// draw the Landscape
      for(int i=0;i!=sizeof(wireFrameVertices)/sizeof(wireFrameVertices[0]);++i){
        renderMap(wireFrameVertices[i]);
      }

    }

  };
}  
