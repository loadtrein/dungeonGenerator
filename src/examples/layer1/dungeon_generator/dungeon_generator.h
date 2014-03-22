#include <random>
#include "ofxDelaunay.h"

using namespace std;

namespace octet{


  class dungeon_generator : public app {

    mat4t modelToWorld;
    mat4t cameraToWorld;

    enum {   
      //ROOMS = 50,
      ROOMS = 30,
      RADIUS = 10
      //RADIUS = 30,
    };    

    mesh grid;
    color_shader color_shader_;
    dungeon_shader dungeon_shader_;
    

    std::vector<Room> rooms;

    GLuint textures[6];

    ofxDelaunay triangulator;

    Graph roomsGraph;

    Graph minimunSpanningTree;

  public:

    dungeon_generator(int argc, char **argv) : app(argc, argv){}

    void app_init() {

      modelToWorld.loadIdentity();
      cameraToWorld.loadIdentity();

      cameraToWorld.rotateX(90.0);
      cameraToWorld.translate(0.0,0.0,10.0);

      srand (static_cast <unsigned> (time(0)));
      
      color_shader_.init();
      dungeon_shader_.init();

      textures[0] = resources::get_texture_handle(GL_RGBA, "assets/dungeon_generator/wall.gif");
      textures[1] = resources::get_texture_handle(GL_RGBA, "assets/dungeon_generator/yellowWall.gif");
      textures[2] = resources::get_texture_handle(GL_RGBA, "assets/dungeon_generator/greenWall.gif");
      textures[3] = resources::get_texture_handle(GL_RGBA, "assets/dungeon_generator/blueWall.gif");
      textures[4] = resources::get_texture_handle(GL_RGBA, "assets/dungeon_generator/lightblueWall.gif");
      textures[5] = resources::get_texture_handle(GL_RGBA, "assets/dungeon_generator/redWall.gif");

      srand (static_cast <unsigned> (time(0)));

      triangulator.init(ROOMS);

      createGrid();

      generateRooms();

      //printRooms();

      while(roomsOverlap()){
        separateTiles();
      }


      cout<<"ROOMS OVERLAPPING:"<<roomsOverlap()<<endl;

      //printRooms();

      cout<<"CREATING 1X1 TILES"<<endl;

      createSmallTiles();

      //printRooms();

      selectRoomsAndCreateGraph();

    }

    void createGrid(){

      mesh_builder b;
      b.init();

      for (int i=-30;i<30;i++){
        for (int j=-30;j<30;j++){
          b.add_plane(vec4(i,0,j,0),vec4(i,0,j+1,0),vec4(i+1,0,j+1,0),vec4(i+1,0,j,0));
        }
      }

      b.get_mesh(grid);
    }

    void generateRooms(){

      std::default_random_engine generator;
      std::normal_distribution<double> distribution(5.0,2.0);


      for(int i=0; i!=ROOMS;++i){

        float angle = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/(2*3.14159)));
        float randomRadius = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/RADIUS));

        int xPos = static_cast<int>(randomRadius*cos(angle));
        int zPos = static_cast<int>(randomRadius*sin(angle));

        float ratio=0.0f;
        int width = 0, length = 0;

        do{

          width = static_cast<int>(distribution(generator));
          length = static_cast<int>(distribution(generator));

          if(width>=length){
            ratio = static_cast<float>(width)/static_cast<float>(length);
          }else{
            ratio = static_cast<float>(length)/static_cast<float>(width);
          }

        }while(ratio < 1.0 || ratio >=2.0);


        Room room(vec4(static_cast<float>(xPos),0.0,static_cast<float>(zPos),0.0),static_cast<float>(width),static_cast<float>(length));
        rooms.push_back(room);

      }
    }

    void separateTiles(){

       int numCloseRooms = 0;

       vec4 sumVector(0.0f,0.0f,0.0f,0.0f);


       for(int i=0;i!=rooms.size();++i){
         for(int j=0;j!=rooms.size();++j){

           if (i!=j){
              if ( roomsOverlap(rooms[i],rooms[j])) {

                vec4 destination = rooms[j].getMidPoint();

                vec4 origin = rooms[i].getMidPoint();

                vec4 oppositeVector = origin - destination;

                if(all(oppositeVector == vec4(0.0f,0.0f,0.0f,0.0f))){
                  
                }else{
                  oppositeVector = oppositeVector.normalize();
                }


                sumVector+=oppositeVector;

                numCloseRooms++;

              }
           }
  
         }

         if(numCloseRooms > 0){
           
           if(all(sumVector == vec4(0.0f,0.0f,0.0f,0.0f))){
             sumVector = vec4(0.5f,0.0f,0.0f,0.0f);
           }else{
             sumVector = sumVector.normalize();
           }

           rooms[i].separate(sumVector);
         }

         sumVector = vec4(0.0f,0.0f,0.0f,0.0f);
         numCloseRooms = 0;

       }
    }

    bool roomsOverlap(){

      for(int i=0;i!=rooms.size();++i){
        for(int j=0;j!=rooms.size();++j){
          if(i!=j){
            
            if( rooms[i].getGroundPoint(0).x() < rooms[j].getGroundPoint(2).x() &&
                rooms[i].getGroundPoint(2).x() > rooms[j].getGroundPoint(0).x() &&
                rooms[i].getGroundPoint(0).z() < rooms[j].getGroundPoint(2).z() &&
                rooms[i].getGroundPoint(2).z() > rooms[j].getGroundPoint(0).z()){

                  return true;
            }
          }
        }
      }

      return false;
    }

    bool roomsOverlap(Room r1, Room r2){
      
      if( r1.getGroundPoint(0).x() < r2.getGroundPoint(2).x() &&
          r1.getGroundPoint(2).x() > r2.getGroundPoint(0).x() &&
          r1.getGroundPoint(0).z() < r2.getGroundPoint(2).z() &&
          r1.getGroundPoint(2).z() > r2.getGroundPoint(0).z()){

          return true;
      }
    
      return false;
    }

    void createSmallTiles() {

      //We calculate the boundaries of the dungeon map
      float maxX=0, minX=0,maxZ=0, minZ=0;

      for(int i=0;i!=rooms.size();++i){
        for(int j=0;j!=4;++j){

          if(rooms[i].getGroundPoint(j).x() > maxX){
            maxX = rooms[i].getGroundPoint(j).x();
          }

          if(rooms[i].getGroundPoint(j).x() < minX){
            minX = rooms[i].getGroundPoint(j).x();
          }

          if(rooms[i].getGroundPoint(j).z() > maxZ){
            maxZ = rooms[i].getGroundPoint(j).z();
          }

          if(rooms[i].getGroundPoint(j).z() < minZ){
            minZ = rooms[i].getGroundPoint(j).z();
          }

        }
      }

      fillWithSmallTiles(minX,maxX,minZ,maxZ);

    }

    void fillWithSmallTiles(float minX, float maxX, float minZ, float maxZ){

      for(int i=minX;i!=maxX;++i){
        for(int j=minZ;j!=maxZ;++j){
         
          //We pass the midpoint of the 1x1 tile to determine if it is inside any room
          if(!isPointContainedInAnyRoom(static_cast<float>(i)+0.5f,static_cast<float>(j)+0.5f)){
            Room room(vec4(static_cast<float>(i),0.0f,static_cast<float>(j),0.0f),static_cast<float>(1),static_cast<float>(1));
            room.setTexture(4);
            rooms.push_back(room);

          }
        }
      }

    }

    bool isPointContainedInAnyRoom(float x, float z){
      for(int k=0;k!=rooms.size();++k){

        if(x >= rooms[k].getGroundPoint(0).x() && x <= rooms[k].getGroundPoint(3).x() &&
          z >= rooms[k].getGroundPoint(0).z() && z <= rooms[k].getGroundPoint(1).z()){
          return true;
        }
      }

      return false;
    }

    void selectRoomsAndCreateGraph(){
      
      int numRooms=0;

      for(int i=0;i!=rooms.size();++i){
        if(rooms[i].getArea() >30 ){
          rooms[i].setTexture(5);
          triangulator.addPoint(rooms[i].getMidPoint().x(), rooms[i].getMidPoint().z());
          roomsGraph.addRoom(&rooms[i]);
          cout<<"Add point: "<<rooms[i].getMidPoint().x()<<" "<<rooms[i].getMidPoint().z()<<" "<<endl;
          numRooms++;
        }
      }
              
      triangulator.triangulate();

      roomsGraph.initialiseGraph(numRooms);

      int numTris = triangulator.getNumTriangles();
      ITRIANGLE *tris = triangulator.getTriangles();
      XYZ *points = triangulator.getPoints();

      for (int i=0; i<numTris; i++)
      {

        XYZ p1 = points[tris[i].p1];
        XYZ p2 = points[tris[i].p2];
        XYZ p3 = points[tris[i].p3];

        roomsGraph.setConnectionBetween(vec4(p1.x,0.0f,p1.y,0.0),vec4(p2.x,0.0f,p2.y,0.0));
        roomsGraph.setConnectionBetween(vec4(p2.x,0.0f,p2.y,0.0),vec4(p3.x,0.0f,p3.y,0.0));
        roomsGraph.setConnectionBetween(vec4(p3.x,0.0f,p3.y,0.0),vec4(p1.x,0.0f,p1.y,0.0));

      }

      roomsGraph.printGraph();
    }


    void printRooms(){
      for(int i=0;i!=rooms.size();++i){

        float ratio=0;

        if(rooms[i].getWidth()>=rooms[i].getLength()){
          ratio = rooms[i].getWidth()/rooms[i].getLength();
        }else{
          ratio = rooms[i].getLength()/rooms[i].getWidth();
        }


        cout<<i+1<<". x: "<<rooms[i].getGroundPoint(0).x()<<" z:"<<rooms[i].getGroundPoint(0).z()<<endl;
        cout<<"width: "<<rooms[i].getWidth()<<" length: "<<rooms[i].getLength()<<" ratio: "<<ratio<<" area: "<<rooms[i].getArea()<<endl<<endl;
      }
    }

    void keyboard(){
      if(is_key_down('D')){
        cameraToWorld.translate(-0.1f,0.0f,0.0f);
      }

      if(is_key_down('A')){
        cameraToWorld.translate(0.1f,0.0f,0.0f);
      }

      if(is_key_down('W')){
        cameraToWorld.translate(0.0f,-0.1f,0.0f);
      }

      if(is_key_down('S')){
        cameraToWorld.translate(0.0f,0.1f,0.0f);
      }

      if(is_key_down('Q')){
        cameraToWorld.translate(0.0f,0.0f,-0.1f);
      }

      if(is_key_down('E')){
        cameraToWorld.translate(0.0f,0.0f,0.1f);
      }

      if (is_key_down(key_left)) {
        cameraToWorld.rotateY(-1.0f);
      }

      if (is_key_down(key_right)) {
        cameraToWorld.rotateY(1.0f);
      }

      if(is_key_down(key_up)){
        cameraToWorld.rotateX(1.0f);
      }

      if(is_key_down(key_down)){
        cameraToWorld.rotateX(-1.0f);
      }

      if(is_key_down(key_space)){
        cameraToWorld.loadIdentity();
        cameraToWorld.rotateX(90.0f);
        cameraToWorld.translate(0.0f,0.0f,5.0f);
      }

      if(is_key_down('R')){
       printRooms();
      }
    }

    void draw_world(int x, int y, int w, int h) {

      keyboard();

      // set a viewport - includes whole window area
      glViewport(x, y, w, h);

      // background color
      glClearColor(0.53f, 0.81f, 0.98f, 1);
      //glClearColor(0, 0, 0, 1);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // allow Z buffer depth testing (closer objects are always drawn in front of far ones)
      glEnable(GL_DEPTH_TEST);

      

      // allow alpha blend (transparency when alpha channel is 0)
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


      //DUNGEON MESH RENDERING

      mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);


      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, textures[0]);

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, textures[1]);

      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, textures[2]);

      glActiveTexture(GL_TEXTURE3);
      glBindTexture(GL_TEXTURE_2D, textures[3]);

      glActiveTexture(GL_TEXTURE4);
      glBindTexture(GL_TEXTURE_2D, textures[4]);

      glActiveTexture(GL_TEXTURE5);
      glBindTexture(GL_TEXTURE_2D, textures[5]);


      renderCoordinatesOrigin(modelToProjection);


      int numTris = triangulator.getNumTriangles();
      ITRIANGLE *tris = triangulator.getTriangles();
      XYZ *points = triangulator.getPoints();

      for (int i=0; i<numTris; i++)
      {

        XYZ p1 = points[tris[i].p1];
	      XYZ p2 = points[tris[i].p2];
        XYZ p3 = points[tris[i].p3];

        renderTriangulatedRooms(modelToProjection,p1,p2,p3);
      }
      
      

      grid.set_mode(GL_LINE_STRIP);
      dungeon_shader_.render(modelToProjection,4);
      grid.render();
      
      for(int i=0;i!=rooms.size();++i){

        dungeon_shader_.render(modelToProjection,static_cast<int>(rooms[i].getTexture()));
        if(rooms[i].getRendered())
          rooms[i].render();
      } 
      

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);

           
    }

    void renderCoordinatesOrigin( mat4t modelToProjection ) 
    {
      color_shader_.render(modelToProjection,vec4(1.0f,0.0f,0.0f,1.0f));

      float vertices[] = {
        -0.15f, 0.0f, -0.15f,
        0.15f, 0.0f, -0.15f, 
        0.15f, 0.0f,  0.15f, 
        -0.15f, 0.0f,  0.15f,
      };

      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)vertices );
      glEnableVertexAttribArray(attribute_pos);

      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    }

    void renderTriangulatedRooms(mat4t modelToProjection, XYZ p1, XYZ p2, XYZ p3){

      color_shader_.render(modelToProjection,vec4(0.0f,1.0f,0.0f,1.0f));

     float vertices[] = {
        p1.x, 0, p1.y,
        p2.x, 0, p2.y,
        p3.x, 0, p3.y,
      };

      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)vertices );
      glEnableVertexAttribArray(attribute_pos);

      glDrawArrays(GL_LINE_LOOP, 0, 3);

    }

  };   

}    