////////////////////////////////////////////////////////////////////////////////
//
// (C) M. Dimitri Alvarez 2013
//
// L-system
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

  //Every segment of the L-System
	class LSystemSegment{

    mat4t modelToWorld;

    //Width and Height of the segment
    float width;
    float height;      

    //Texture
    int texture;

    //It must be rendered or not
    bool rendered;

    //Segment type: trunk or leaf
    std::string type;

  public:

    void init(int _texture, mat4t &m, float w, float h, std::string t) {
      modelToWorld = m;
      width = w ;
      height = h;
      texture = _texture;
      rendered = true;
      type = t;
    }

    void setRendered(bool r){
      this->rendered=r;
    }

    bool isRendered(){
      return this->rendered;
    }

    std::string getType(){
      return this->type;
    }

    void setTexture(GLuint t){
      this->texture = t;
    }

    void render(texture_shader &shader, mat4t &cameraToWorld, float degree) {

      mat4t rotationMatrix (1.0f);
      rotationMatrix.rotateY(degree);

      // build a projection matrix: model -> world -> camera -> projection
      // the projection space is the cube -1 <= x/w, y/w, z/w <= 1
      mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld * rotationMatrix, cameraToWorld);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture);

      // set up the uniforms for the shader
      shader.render(modelToProjection, 0);

      // this is an array of the positions of the corners of the box in 3D
      // a straight "float" here means this array is being generated here at runtime.
      float vertices[] = {
        -(width*0.5), 0, 0,
        (width*0.5), 0, 0,
         (width*0.5),  height, 0,
        -(width*0.5),  height, 0,
      };

      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)vertices );
      glEnableVertexAttribArray(attribute_pos);

      // this is an array of the positions of the corners of the texture in 2D
      static const float uvs[] = {
        0,  0,
        1,  0,
        1,  1,
        0,  1,
      };

      // attribute_uv is position in the texture of each corner
      // each corner (vertex) has 2 floats (x, y)
      // there is no gap between the 2 floats and hence the stride is 2*sizeof(float)
      glVertexAttribPointer(attribute_uv, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)uvs );
      glEnableVertexAttribArray(attribute_uv);

      // finally, draw the sprite (4 vertices)
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    }
  };

  //Class used to store a L-System rule
	class LSystemRule{
		std::string ifPart;
		std::string thenPart;

	public:

		LSystemRule(std::string ip, std::string tp){
			this->ifPart = ip;
			this->thenPart = tp;
		}

		void setIfPart(std::string ifp){
			this->ifPart = ifp;
		}
		
		void setThenPart(std::string tp){
			this->thenPart = tp;
		}

		std::string getIfPart(){
			return this->ifPart;
		}

		std::string getThenPart(){
			return this->thenPart;
		}

	};
  
	class LSystemHandler : public octet::app {
		
    // Matrix to transform points in our camera space to the world.
		// This lets us move our camera
		mat4t cameraToWorld;

    mat4t modelToWorld;

    texture_shader_no_alpha texture_shader_no_alpha_;
    texture_shader_alpha texture_shader_alpha_;

		ifstream file;
    //Stores different files' names
		std::string lSystemsFiles [9];
    int numLSystem;
		
    
		//L-system parameters
		std::string axiom;
		vector<LSystemRule> rules;
		float currentAngle;
    float angle;
    int nIterations;

		//General elements
		int currentIteration;
		std::string lSystemCode;
    vector<LSystemSegment> lSystemBranches;
    float branchLength;
    float branchWidth;
    stack<mat4t> stackDrawingPointer;
    float LSystemHeight;
    float LSystemWidth;
    float LSystemWidthPos;
    float LSystemWidthNeg;
    float shapeRotationDegree;

    //Field of view
    float FOVx;
    float FOVy;
   
    //Bool to control key pressing
		bool iterationForward;
		bool iterationBackward;
    bool applySeasonChange;

    //Hud font
    GLuint font_texture;

    // information for our text
    bitmap_font font;
    
    //Textures
    GLuint trunk;
    GLuint leaf1;
    GLuint leaf2;
    GLuint leaf3;
    GLuint currentLeaf;
    GLuint currentSegment;

    //Determines the season
    int currentSeason;

    bool is3DLsystem;

		void simulate() {

      //Iteration forward
			 if(is_key_down(key_right) && !this->iterationForward){
				
         resetLSystemValues();

         this->iterationForward=true;
         this->currentIteration++;

         stepIterationForwardLSystem();

         processLSystemCode();

         //cout<<lSystemCode<<endl;

         //cout<<"Height: "<<this->LSystemHeight<<"Width: "<<this->LSystemWidth<<endl;

         setCameraPosition();
			 
			 }else{

				 this->iterationForward=false;
			 }

       //Iteration backward
			 if(is_key_down(key_left) && !this->iterationBackward){
				 
				 if(currentIteration>0){

          resetLSystemValues();

					this->iterationBackward=true;
					this->currentIteration--;

					if(currentIteration==0){
						setLSystemCodeAsAxiom();
					}else{
						stepIterationBackwardLSystem();
					}

          processLSystemCode();

          //cout<<lSystemCode<<endl;

          //cout<<"Height: "<<this->LSystemHeight<<"Width: "<<this->LSystemWidth<<endl;

          setCameraPosition();
         }


			 }else{

				 this->iterationBackward=false;

			 }


       //Season changes
       if(is_key_down('L') && !this->applySeasonChange){

         this->applySeasonChange = true;

         if(currentSeason == 4){
           currentSeason = 1;
         }else{
           currentSeason++;
         }

         for(int i = 0; i != lSystemBranches.size();++i){
           if(lSystemBranches[i].getType() == "leaf"){

             if(currentSeason == 1){
               lSystemBranches[i].setRendered(true);
               this->currentLeaf = leaf1;
             }else if(currentSeason == 2){
                this->currentLeaf = leaf2;
             }else if(currentSeason == 3){
               this->currentLeaf = leaf3;
             }else if(currentSeason == 4){
               lSystemBranches[i].setRendered(false);
             }
             
             lSystemBranches[i].setTexture(this->currentLeaf);

           }
         }

       }else{
         this->applySeasonChange = false;
       }

       //Angle increase
       if(is_key_down('Z')){
         this->currentAngle+=1.0f;
         resetLSystemValues();
         processLSystemCode();
       }

       //Angle decrease
       if(is_key_down('X')){
         this->currentAngle-=1.0f;
         resetLSystemValues();
         processLSystemCode();
       }

       //Branch increase
       if(is_key_down('C')){
         this->branchLength+=0.1f;
         resetLSystemValues();
         processLSystemCode();
       }

       //Branch decrease
       if(is_key_down('V')){
         this->branchLength-=0.1f;
         resetLSystemValues();
         processLSystemCode();
       }

       //Reset Angle, Branch, Position
       if(is_key_down(key_space)){
         this->branchLength=1.0f;
         this->currentAngle=this->angle;
         this->shapeRotationDegree = 0.0;
         resetLSystemValues();
         processLSystemCode();
         setCameraPosition();
       }
       

       //We display the different L-System

       if(is_key_down(key_f1)){
         lSystemBranches.clear();
         rules.clear();
         numLSystem = 1;
         this->is3DLsystem = false;
         setLSystem(lSystemsFiles[0]);
       }

       if(is_key_down(key_f2)){
         lSystemBranches.clear();
         rules.clear();
         numLSystem = 2;
         this->is3DLsystem = false;
         setLSystem(lSystemsFiles[1]);
       }

       if(is_key_down(key_f3)){
         lSystemBranches.clear();
         rules.clear();
         numLSystem = 3;
         this->is3DLsystem = false;
         setLSystem(lSystemsFiles[2]);
       }

       if(is_key_down(key_f4)){
         lSystemBranches.clear();
         rules.clear();
         numLSystem = 4;
         this->is3DLsystem = false;
         setLSystem(lSystemsFiles[3]);
       }

       if(is_key_down(key_f5)){
         lSystemBranches.clear();
         rules.clear();
         numLSystem = 5;
         this->is3DLsystem = false;
         setLSystem(lSystemsFiles[4]);
       }

       if(is_key_down(key_f6)){
         lSystemBranches.clear();
         rules.clear();
         numLSystem = 6;
         this->is3DLsystem = false;
         setLSystem(lSystemsFiles[5]);
       }

       if(is_key_down(key_f7)){
         lSystemBranches.clear();
         rules.clear();
         numLSystem = 7;
         this->is3DLsystem = false;
         setLSystem(lSystemsFiles[6]);
       }

       if(is_key_down(key_f8)){
         lSystemBranches.clear();
         rules.clear();
         numLSystem = 8;
         this->is3DLsystem = true;
         setLSystem(lSystemsFiles[7]);
       }

       if(is_key_down(key_f9)){
         lSystemBranches.clear();
         rules.clear();
         numLSystem = 9;
         this->is3DLsystem = false;
         setLSystem(lSystemsFiles[8]);
       }

       //Move camera up and down
       if(is_key_down('D')){
         cameraToWorld.translate(-1.0,0,0);
       }

       if(is_key_down('A')){
         cameraToWorld.translate(1.0,0,0);
       }

       if(is_key_down('W')){
         cameraToWorld.translate(0,-1.0,0);
       }

       if(is_key_down('S')){
         cameraToWorld.translate(0,1.0,0);
       }

       //Rotation of the l-system
       if(is_key_down('Q')){
         this->shapeRotationDegree-=1.5;
       }

       if(is_key_down('E')){
         this->shapeRotationDegree+=1.5;
       } 

       //Zoom in and out
       if(is_key_down('I')){
         cameraToWorld.translate(0,0,-1.0);
       }

       if(is_key_down('O')){
         cameraToWorld.translate(0,0,1.0);
       }

		}

    void resetLSystemValues() 
    {
      modelToWorld.loadIdentity();
      lSystemBranches.clear();

      this->LSystemHeight = 0.0f;
      this->LSystemWidth = 0.0f;
      this->LSystemWidthPos = 0.0f;
      this->LSystemWidthNeg = 0.0f;
    }

    //Draw text on the screen
    void draw_text(texture_shader_no_alpha &shader, float x, float y, float z, float scale, const char *text) {
      mat4t modelToWorld = cameraToWorld;
      modelToWorld.translate(x, y, z);
      modelToWorld.scale(scale, scale, 1);
      mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

      enum { max_quads = 80 };
      bitmap_font::vertex vertices[max_quads*4];
      uint32_t indices[max_quads*6];
      aabb bb(vec3(0, 0, 0), vec3(256, 256, 0));

      unsigned num_quads = font.build_mesh(bb, vertices, indices, max_quads, text, 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, font_texture);

      shader.render(modelToProjection, 0);

      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].x );
      glEnableVertexAttribArray(attribute_pos);
      glVertexAttribPointer(attribute_uv, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].u );
      glEnableVertexAttribArray(attribute_uv);

      glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, indices);
    } 

    //L-System initializer
    void setLSystem(std::string file) 
    {
      cameraToWorld.loadIdentity();
      cameraToWorld.translate(-15,45,50);
      modelToWorld.loadIdentity();
      this->iterationForward = false;
      this->iterationBackward = false;

      this->currentIteration = 0;
      this->branchLength = 1;
      this->branchWidth = 0.2;
      this->LSystemHeight = 0.0f;
      this->LSystemWidth = 0.0f;
      this->LSystemWidthPos = 0.0f;
      this->LSystemWidthNeg = 0.0f;
      this->shapeRotationDegree = 0.0f;

      ProcessFile(file);

      this->lSystemCode = this->axiom;

      processLSystemCode();

      //cout<<"Height: "<<this->LSystemHeight<<"Width: "<<this->LSystemWidth<<endl;

    }

    //Set and store the names of the L-System files
		void SetLSystemsFiles(){
			
			std::stringstream sstm;
			std::string string1 = "lsystems/lsystem";
			std::string string2 = ".txt";
			
			for(int i=0;i<sizeof(lSystemsFiles)/sizeof(lSystemsFiles[0]);i++){
				sstm <<  string1 << i+1 << string2;
				lSystemsFiles[i]=sstm.str();
				sstm.str("");
			}
		}

		//--------------------FILE READING AND PROCESSING------------------

		void ProcessFile(std::string fileToRead){

			std::string line;

			file.open(fileToRead);

			if(file.is_open()){

				while (getline(file,line) ){
					if(line.find("Axiom",0) != std::string::npos){
						processAxiomFileLine(line);
					}

					if(line.find("Rules",0) != std::string::npos){
						processRulesFileLine(line);
					}

					if(line.find("Angle",0)!= std::string::npos){
						processAngleFileLine(line);
					}

          if(line.find("N",0) != std::string::npos){
            processNIterationsFileLine(line);
          }
				}

			}else{
				cout<<"Unable to open "<<fileToRead<<endl;
			}

			file.close();
		}

		void processAxiomFileLine( std::string line ){
			
			std::istringstream is(line);
			std::string waste;
			
			//We store the axiom
			is>>waste>>this->axiom;
		}

		void processRulesFileLine(std::string line){
			std::istringstream is(line);
			std::string wholeRule;

			//we do not keep "rules:" string
			is>>wholeRule;

			//we start processing the rules from the file
			while(is>>wholeRule){
				
				//We search and store the If part of the rule
				std::size_t arrowBeginning =	wholeRule.find("->",0);
				std::string ifPart = wholeRule.substr(1,arrowBeginning-1);

				//We search and store the Then part of the rule
				std::size_t bracketClose =	wholeRule.find(")",0);
				std::string thenPart = wholeRule.substr(arrowBeginning+2,(bracketClose - (arrowBeginning+2)));
				
				//We insert the rules into rules vector
				LSystemRule rule(ifPart,thenPart);
				rules.push_back(rule);
			}
		}

		void processAngleFileLine(std::string line){
			std::istringstream is(line);
			std::string waste;
			std::string stringAngle;

			//We store the angle
			is>>waste>>stringAngle;

			//We convert the string angle into a float value
			this->currentAngle = atof(stringAngle.c_str());
      this->angle = atof(stringAngle.c_str());
		}

    void processNIterationsFileLine( std::string line ) 
    {
      std::istringstream is(line);
      std::string waste;
      std::string stringIterations;

      //We store the number of iterations
      is>>waste>>stringIterations;

      //We convert the string iterations into a int value
      this->nIterations = atoi(stringIterations.c_str());
    }

		//--------------------L SYSTEM OPERATIONS----------------------------


		void setLSystemCodeAsAxiom(){

			this->lSystemCode = this->axiom;
		}

		void stepIterationForwardLSystem(){
	
			std::string temp="";
			bool thereWasChanges;

			for(int i=0;i!=this->lSystemCode.size();++i){

				thereWasChanges = false;

				for(int j=0;j!=this->rules.size();++j){

					std::string ifPart = rules[j].getIfPart();
					std::string thenPart = rules[j].getThenPart();

					if(lSystemCode[i] == ifPart[0]){
						temp+=thenPart;
						thereWasChanges = true;
					}
				}

				if(!thereWasChanges){
					temp+=lSystemCode[i];
				}
			}

			this->lSystemCode = temp;
		}

		void stepIterationBackwardLSystem(){

			int currentIteration = 0;
			this->lSystemCode = this->axiom;


			while(currentIteration!=this->currentIteration){
				currentIteration++;

				std::string temp="";

				bool thereWasChanges;

				for(int i=0;i!=this->lSystemCode.size();++i){

					thereWasChanges = false;

					for(int j=0;j!=this->rules.size();++j){

						std::string ifPart = rules[j].getIfPart();
						std::string thenPart = rules[j].getThenPart();

						if(lSystemCode[i] == ifPart[0]){
							temp+=thenPart;
							thereWasChanges = true;
						}
					}
					if(!thereWasChanges){
						temp+=lSystemCode[i];
					}
				}

				this->lSystemCode = temp;
			}
		}

    //We read and process the L-System Code
		void processLSystemCode(){

			for(int i=0;i!=this->lSystemCode.size();++i){

				switch (lSystemCode[i])
				{
				case 'F':{
          //Draw Forward
          drawForward();
					break;
                 }
        case 'X':{
          //Draw Forward
          drawForward();
          break;
                 }

        case 'L':{
          //Draw Forward
          this->currentSegment = this->currentLeaf;
          break;
                 }
        case 'T':{
          //Draw Forward
          this->currentSegment = this->trunk;
          break;
                 }

				case '+':{
          //Turn right X degrees
          modelToWorld.rotateZ(this->currentAngle);
          break;
                 }

				case '-':{
          //Turn left X degrees
          modelToWorld.rotateZ(-(this->currentAngle));
          break;
                 }

        case '&':{
          //Turn right X degrees
          modelToWorld.rotateX(this->currentAngle);
          break;
                 }

        case '^':{
          //Turn left X degrees
          modelToWorld.rotateX(-(this->currentAngle));
          break;
                 }

        case '\\':{
          //Turn right X degrees
          modelToWorld.rotateY(this->currentAngle);
          break;
                 }

        case '/':{
          //Turn left X degrees
          modelToWorld.rotateY(-(this->currentAngle));
          break;
                 }
				case '[':{
          //Push position and angle
          mat4t matrix = modelToWorld;
          stackDrawingPointer.push(matrix);
					break;   
                 }

				case ']':{
          //Pop last Drawing pointer

          this->modelToWorld = stackDrawingPointer.top();

          stackDrawingPointer.pop();

					break;
                 }
				}

        calculateHeightWidthLSystem();
			
			}

		}

    //We draw forward and L-System Segment 
    void drawForward(){

      LSystemSegment lSElement;
      
      if(this->currentSegment == this->currentLeaf){
        lSElement.init(currentLeaf,modelToWorld, this->branchWidth+0.6, this->branchLength+1.2,"leaf");
      }else{
        lSElement.init(trunk,modelToWorld, this->branchWidth+0.1, this->branchLength,"trunk");
      }

      lSystemBranches.push_back(lSElement);

      modelToWorld.translate(0,this->branchLength,0);
      
      
    }

    //We calculate the height and width of the L-System
    void calculateHeightWidthLSystem(){
      
      vec4 translationRow = modelToWorld.row(3);
      float x = translationRow.x();
      float y = translationRow.y();

      if(y > this->LSystemHeight){
        this->LSystemHeight = translationRow.y();
      }

      if(x > 0 &&  x > this->LSystemWidthPos){
        this->LSystemWidthPos = translationRow.x();
      }

      if(x < 0 &&  x < this->LSystemWidthNeg){
        this->LSystemWidthNeg = translationRow.x();
      }

      this->LSystemWidth = this->LSystemWidthPos + (- this->LSystemWidthNeg);

    }

    //We adjust the distance of the camera to the rendered image using a bounding sphere
    void setCameraPosition(){

      float boudingSphereradius=0.0f;

      if(this->LSystemHeight > 90.0f || this->LSystemWidth > 70.0f){

        if(this->LSystemHeight  > this->LSystemWidth){
          boudingSphereradius = this->LSystemHeight/2;
        }else if (this->LSystemWidth  > this->LSystemHeight){
          boudingSphereradius = this->LSystemWidth/2;
        }

        cameraToWorld.loadIdentity();
        cameraToWorld.translate(boudingSphereradius*-0.32, boudingSphereradius, boudingSphereradius*1.07);

      }else{

        cameraToWorld.loadIdentity();
        cameraToWorld.translate(-15,45,50);

      }
    }

    //Yields the string representation of the rules to be displayed on the HUD
    std::string getRulesString() 
    {
      std::string chain="";

      for(int j=0;j!=this->rules.size();++j){

        std::string ifPart = rules[j].getIfPart();
        std::string thenPart = rules[j].getThenPart();
      
        chain+="*"+ifPart+"="+thenPart+"\n";
      }

      return chain;
    }


  public:

    // this is called when we construct the class
    LSystemHandler(int argc, char **argv) : app(argc, argv), font(512, 256, "assets/big.fnt"){
    }

    // this is called once OpenGL is initialized
    void app_init() {

      texture_shader_no_alpha_.init();
      texture_shader_alpha_.init();

      //Different textures
      trunk = resources::get_texture_handle(GL_RGBA, "assets/lsystems/trunk.gif");
      leaf1 = resources::get_texture_handle(GL_RGBA, "assets/lsystems/leaf1.gif");
      leaf2 = resources::get_texture_handle(GL_RGBA, "assets/lsystems/leaf2.gif");
      leaf3 = resources::get_texture_handle(GL_RGBA, "assets/lsystems/leaf3.gif");

      font_texture = resources::get_texture_handle(GL_RGBA, "assets/big_0_black.gif");

      //We start with the green leaves: Spring
      this->currentLeaf = leaf1;
      this->currentSeason = 1;
      this->is3DLsystem = false;

      SetLSystemsFiles();

      numLSystem = 1;

      setLSystem(lSystemsFiles[0]);

    }

		// this is called to draw the world
		void draw_world(int x, int y, int w, int h) {
			
			simulate();

      this->FOVx = w;
      this->FOVy = h;

			// set a viewport - includes whole window area
			glViewport(x, y, w, h);

			// background color

			glClearColor(0.93, 0.94, 0.85, 1);
      //glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// allow Z buffer depth testing (closer objects are always drawn in front of far ones)
      if(this->is3DLsystem){
			  glEnable(GL_DEPTH_TEST);
      }else{
        glDisable(GL_DEPTH_TEST);
      }

      // allow alpha blend (transparency when alpha channel is 0)
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


      //---------------------------------------------HUD DISPLAY---------------------------------------------------------------

      
      char lsystemName[15];

      sprintf(lsystemName, "L-System %d\n", numLSystem);
      
      draw_text(texture_shader_no_alpha_, - 1.8, + 1.8, - 3 ,1.0f/256, lsystemName);

      char info[100];

      sprintf(info, "N: %d\nIteration: %d\nAngle: %.1f\nBranch Length: %.1f",this->nIterations,this->currentIteration,this->currentAngle,this->branchLength);

      draw_text(texture_shader_no_alpha_, - 1.9, + 1.6, - 3 ,1.0f/310, info);


      char axiomRules[160];

      std::string rulesString = getRulesString();

      sprintf(axiomRules, "Axiom: %s\nRules:\n%s\n",this->axiom.c_str(),rulesString.c_str());

      draw_text(texture_shader_no_alpha_, - 1.9, + 0.7, - 3 ,1.0f/310, axiomRules);


			// draw the lsystem
      for(int i = 0; i != lSystemBranches.size();++i){
        if(lSystemBranches[i].isRendered()){
          if(this->is3DLsystem){
            lSystemBranches[i].render(texture_shader_alpha_,cameraToWorld,shapeRotationDegree);
          }else{
            lSystemBranches[i].render(texture_shader_no_alpha_,cameraToWorld,shapeRotationDegree);
          }
      }

      }
    }

  };
}
