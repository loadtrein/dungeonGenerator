#include <random>
#include <queue>
#include "ofxDelaunay.h"

using namespace std;

namespace octet{

  /************************************************************************/
  /* Class that contains the main procedures to generate random 2D dungeons                                                                     */
  /************************************************************************/
  class dungeon_generator : public app {

    mat4t modelToWorld;
    mat4t cameraToWorld;

    enum {   
      ROOMS = 20,
      RADIUS = 10
    };    

    mesh grid;
    color_shader color_shader_;
    dungeon_shader dungeon_shader_;
    
    std::vector<Room> rooms;

    std::vector<Room*> minimumTreeRooms;

    GLuint textures[6];

    ofxDelaunay triangulator;

    Graph roomsGraph;

    Graph minimunSpanningTree;

    bool showDelaunayTriangulation;

    bool showMinimumSpanningTree;

    bool showGrid;

    bool showCorridors;

    std::vector<vec4> corridorPoints;

    std::vector<Room> corridors;

    int step;

  public:

    dungeon_generator(int argc, char **argv) : app(argc, argv){}


    /************************************************************************/
    /* Function that is called to initialise all the elements of the class                                                                     */
    /************************************************************************/
    void app_init() {

      modelToWorld.loadIdentity();
      cameraToWorld.loadIdentity();

      cameraToWorld.rotateX(90.0);
      

      srand (static_cast <unsigned> (time(0)));
      
      color_shader_.init();
      dungeon_shader_.init();

      textures[0] = resources::get_texture_handle(GL_RGBA, "assets/dungeon_generator/wall.gif");
      textures[1] = resources::get_texture_handle(GL_RGBA, "assets/dungeon_generator/blueWall.gif");
      textures[2] = resources::get_texture_handle(GL_RGBA, "assets/dungeon_generator/greenWall.gif");
      textures[3] = resources::get_texture_handle(GL_RGBA, "assets/dungeon_generator/yellowWall.gif");
      textures[4] = resources::get_texture_handle(GL_RGBA, "assets/dungeon_generator/lightblueWall.gif");
      textures[5] = resources::get_texture_handle(GL_RGBA, "assets/dungeon_generator/redWall.gif");

      srand (static_cast <unsigned> (time(0)));

      showDelaunayTriangulation = false;

      showCorridors = false;

      showMinimumSpanningTree = false;

      showGrid = false;

      step = 1;

      triangulator.init(ROOMS);

      createGrid();

      dungeonAlgorithm();


    }


    /************************************************************************/
    /* Algorithm that step by step procedurally generates the dungeon                                                                     */
    /************************************************************************/
    void dungeonAlgorithm() 
    {

      cout<<"Generating..."<<endl;

      generateRooms();

      //printRooms();

      while(roomsOverlap()){
        separateTiles();
      }


      //printRooms();

      createSmallTiles();

      //printRooms();

      selectRoomsAndCreateCompleteGraph();

      //roomsGraph.printGraph();

      createMinimumSpanningTree();

      //minimunSpanningTree.printGraph();

      generateRandomEdges();

      //minimunSpanningTree.printGraph();

      generateLShapesForCorridors();

      generateFinalDungeon();

      cout<<"...Generated"<<endl;
    }


    /************************************************************************/
    /* Creates a grid in the screen for debugging purposes                                                                     */
    /************************************************************************/
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


    /************************************************************************/
    /* Function that generates rooms in random positions within a given radius. They follow a certain aspect ratio      */
    /************************************************************************/
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

    /************************************************************************/
    /* Function that separates the rooms using separation behaviours               */
    /************************************************************************/
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

    /************************************************************************/
    /* Function that determines if there are any overlappings between rooms in the world    */
    /************************************************************************/
    bool roomsOverlap(){

      for(int i=0;i!=rooms.size();++i){
        for(int j=0;j!=rooms.size();++j){
          if(i!=j){
            
            if( rooms[i].getFloorPoint(0).x() < rooms[j].getFloorPoint(2).x() &&
                rooms[i].getFloorPoint(2).x() > rooms[j].getFloorPoint(0).x() &&
                rooms[i].getFloorPoint(0).z() < rooms[j].getFloorPoint(2).z() &&
                rooms[i].getFloorPoint(2).z() > rooms[j].getFloorPoint(0).z()){

                  return true;
            }
          }
        }
      }

      return false;
    }


    /************************************************************************/
    /* Function that determines if two rooms overlap                    */
    /************************************************************************/
    bool roomsOverlap(Room r1, Room r2){
      
      if( r1.getFloorPoint(0).x() < r2.getFloorPoint(2).x() &&
          r1.getFloorPoint(2).x() > r2.getFloorPoint(0).x() &&
          r1.getFloorPoint(0).z() < r2.getFloorPoint(2).z() &&
          r1.getFloorPoint(2).z() > r2.getFloorPoint(0).z()){

          return true;
      }
    
      return false;
    }


    /************************************************************************/
    /* Function that fills with small tiles gaps between the rooms         */
    /************************************************************************/
    void createSmallTiles() {

      //We calculate the boundaries of the dungeon map
      float maxX=0, minX=0,maxZ=0, minZ=0;

      for(int i=0;i!=rooms.size();++i){
        for(int j=0;j!=4;++j){

          if(rooms[i].getFloorPoint(j).x() > maxX){
            maxX = rooms[i].getFloorPoint(j).x();
          }

          if(rooms[i].getFloorPoint(j).x() < minX){
            minX = rooms[i].getFloorPoint(j).x();
          }

          if(rooms[i].getFloorPoint(j).z() > maxZ){
            maxZ = rooms[i].getFloorPoint(j).z();
          }

          if(rooms[i].getFloorPoint(j).z() < minZ){
            minZ = rooms[i].getFloorPoint(j).z();
          }

        }
      }
      
      if(maxX > maxZ){
        cameraToWorld.loadIdentity();
        cameraToWorld.rotateX(90.0);
        cameraToWorld.translate(0.0,0.0,maxX);
      }else{
        cameraToWorld.loadIdentity();
        cameraToWorld.rotateX(90.0);
        cameraToWorld.translate(0.0,0.0,maxZ);
      }


      fillWithSmallTiles(minX,maxX,minZ,maxZ);

    }


    /************************************************************************/
    /* Function that fills with small tiles gaps between the rooms         */
    /************************************************************************/
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


    /************************************************************************/
    /* Function that determines if a given point lies inside a room         */
    /************************************************************************/
    bool isPointContainedInAnyRoom(float x, float z){
      for(int k=0;k!=rooms.size();++k){

        if(x >= rooms[k].getFloorPoint(0).x() && x <= rooms[k].getFloorPoint(3).x() &&
          z >= rooms[k].getFloorPoint(0).z() && z <= rooms[k].getFloorPoint(1).z()){
          return true;
        }
      }

      return false;
    }


    /************************************************************************/
    /* Function that selects the biggest rooms and creates a completely connected graph */
    /************************************************************************/
    void selectRoomsAndCreateCompleteGraph(){
      
      int numRooms=0;

      for(int i=0;i!=rooms.size();++i){

        if(rooms[i].getArea() >30 ){

          rooms[i].setTexture(5);

          triangulator.addPoint(rooms[i].getMidPoint().x(), rooms[i].getMidPoint().z());
          minimumTreeRooms.push_back(&rooms[i]);

          //cout<<"Add point: "<<rooms[i].getMidPoint().x()<<" "<<rooms[i].getMidPoint().z()<<" "<<endl;
          numRooms++;
        }
      }
              
      triangulator.triangulate();

      //Create graphs and add rooms
      roomsGraph.initialiseGraph(numRooms);
      minimunSpanningTree.initialiseGraph(numRooms);

      for(int i=0; i!=minimumTreeRooms.size();++i){
        roomsGraph.addRoom(minimumTreeRooms[i]);
        minimunSpanningTree.addRoom(minimumTreeRooms[i]);
      }

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

      //roomsGraph.printGraph();
    }


    /************************************************************************/
    /* Function that creates a minimum spanning tree from the main graph                                                                     */
    /************************************************************************/
    void createMinimumSpanningTree(){

      //We add the first room
      std::vector<Room*> addedNodes; 
      addedNodes.push_back(minimumTreeRooms[0]);

      //We add all the rooms except for the first
      std::vector<Room*> remainingNodes;
      for(int i=1;i!=minimumTreeRooms.size();++i){
        remainingNodes.push_back(minimumTreeRooms[i]);
      }

      while(remainingNodes.size() != 0){

        //We create it everytime inside the loop because there's no function to clear the priority queue
        std::priority_queue <PriorityQueueNode, vector<PriorityQueueNode>, ComparePQNode> minimunEdges;
          

        //We fill the priority queue with the closest nodes of the existing tree nodes
        for(int j=0; j!=addedNodes.size();++j){
          for(int k=0; k!=remainingNodes.size();++k){
              
              float distance=roomsGraph.getValueAt(addedNodes[j],remainingNodes[k]);
              minimunEdges.push(PriorityQueueNode(addedNodes[j],remainingNodes[k],distance));

          }
        }

        //We get the closest node to the existing tree
        PriorityQueueNode pqn = minimunEdges.top();
        Room *rOrigin =  pqn.getRoomOrigin();
        Room *rDestination =  pqn.getRoomDestination();

        //We add the room to our minimum spanning tree and delete it from the remaining nodes
        addedNodes.push_back(rDestination);
        remainingNodes.erase(remainingNodes.begin() + getIndexToBeRemoved(rDestination,remainingNodes));

        //We set the connection between the rooms in the graph
        minimunSpanningTree.setValueAt(rOrigin,rDestination,1.0);

      }

      //minimunSpanningTree.printGraph();
      
    }

    int getIndexToBeRemoved(Room* r, std::vector<Room*> nodes){
      int index = 0;

      for(int i=0;i!=nodes.size();++i){
        if(r->getMidPoint().x() == nodes[i]->getMidPoint().x() && 
          r->getMidPoint().z() == nodes[i]->getMidPoint().z() ){
            index = i;
        }
      }

      return index;
    }


    /************************************************************************/
    /* Function that adds ramdom edges from the main graph to the minimun spanning tree to create loops within the dungeon */
    /************************************************************************/
    void generateRandomEdges(){

      //Euler formula to get the number of edges
      int numberOfEdges = triangulator.getNumTriangles() + minimumTreeRooms.size() - 1;

      int numAdditionalEdges = (numberOfEdges * 15) / 100;

      while(numAdditionalEdges != 0){
      
        int row = rand() / (RAND_MAX/minimumTreeRooms.size());
        int column = rand() / (RAND_MAX/minimumTreeRooms.size());

        //cout<<"Random row: "<<row<<" Random column: "<<column<<endl;

        //If the edge belongs to the delaunay triangulation and it is not already on the minimun spanning tree
        if(roomsGraph.getValueAt(row,column) != 0.0f && minimunSpanningTree.getValueAt(row,column) != 1.0f){
          minimunSpanningTree.setValueAt(row,column,1.0f);
          numAdditionalEdges--;
        }
      }

    }
    


    /************************************************************************/
    /* Function that generates L-shaped corridors following the connections within the minimum spanning tree */
    /************************************************************************/
    void generateLShapesForCorridors(){

      for(int i=0; i!=minimumTreeRooms.size();++i){
        for(int j=0; j!=minimumTreeRooms.size();++j){

          //If two rooms are connected
          if(minimunSpanningTree.getValueAt(i,j) == 1.0f){
            
            vec4 vectorBetweenRooms = minimumTreeRooms[i]->getMidPoint() - minimumTreeRooms[j]->getMidPoint();

            int randomGeneration = rand() / (RAND_MAX/2);

            //Determines if the corridor appears on the right or on the left of each pair of rooms
            vec4 randomPoint;
            
            if(randomGeneration == 0){
                randomPoint = vec4(minimumTreeRooms[i]->getMidPoint().x(),0.0f,minimumTreeRooms[j]->getMidPoint().z(),0.0f);
                
            }else {
                randomPoint = vec4(minimumTreeRooms[j]->getMidPoint().x(),0.0f,minimumTreeRooms[i]->getMidPoint().z(),0.0f);
            }


            //Points for the L-shape blue lines---> JUST FOR DEBUGGING PURPOSES
            corridorPoints.push_back(minimumTreeRooms[i]->getMidPoint());
            corridorPoints.push_back(randomPoint);
            corridorPoints.push_back(minimumTreeRooms[j]->getMidPoint());

            createCorridors(minimumTreeRooms[i]->getMidPoint(),randomPoint);
            createCorridors(randomPoint,minimumTreeRooms[j]->getMidPoint());

          }
        }
      }

    }


    /************************************************************************/
    /* Function that generates L-shaped corridors following the connections within the minimum spanning tree */
    /************************************************************************/
    void createCorridors(vec4 point1, vec4 point2){

      vec4 p1,p2,p3,p4;


      //Corridor lies on the z-axis
      if((point1.x() - point2.x()) == 0.0f){

        if(point1.z() > point2.z()){
          vec4 temp = point1;
          point1 = point2;
          point2 = temp;
        }

        p1=vec4(point1.x()-0.4f,point1.y(),point1.z()-0.4f,point1.w());
        p2=vec4(point2.x()-0.4f,point2.y(),point2.z()+0.4f,point2.w());
        p3=vec4(point2.x()+0.4f,point2.y(),point2.z()+0.4f,point2.w());
        p4=vec4(point1.x()+0.4f,point1.y(),point1.z()-0.4f,point1.w());

      //Corridor lies on the x-axis
      }else if((point1.z() - point2.z()) == 0.0f){

        if(point1.x() > point2.x()){
          vec4 temp = point1;
          point1 = point2;
          point2 = temp;
        }

        p1=vec4(point1.x()-0.4f,point1.y(),point1.z()-0.4f,point1.w());
        p2=vec4(point1.x()-0.4f,point1.y(),point1.z()+0.4f,point1.w());
        p3=vec4(point2.x()+0.4f,point2.y(),point2.z()+0.4f,point2.w());
        p4=vec4(point2.x()+0.4f,point2.y(),point2.z()-0.4f,point2.w());

      }

      Room room(p1,p2,p3,p4);
      corridors.push_back(room);
    }


    /************************************************************************/
    /* Function that renders the final dungeon                                                                     */
    /************************************************************************/
    void generateFinalDungeon(){

      for(int i=0;i!=rooms.size();++i){
        rooms[i].setRendered(false);
      }

      //If a room overlaps with any corridor we know it belongs to the dungeon, so we render it to obtain the final dungeon
      for(int i=0;i!=rooms.size();++i){
        for(int j=0;j!=corridors.size();++j){
          if(roomsOverlap(rooms[i],corridors[j])){
            rooms[i].setRendered(true);
          }
        }
      }
    }


    /************************************************************************/
    /* Prints rooms info - Debugging purposes                               */
    /************************************************************************/
    void printRooms(){
      for(int i=0;i!=rooms.size();++i){

        float ratio=0;

        if(rooms[i].getWidth()>=rooms[i].getLength()){
          ratio = rooms[i].getWidth()/rooms[i].getLength();
        }else{
          ratio = rooms[i].getLength()/rooms[i].getWidth();
        }


        cout<<i+1<<". x: "<<rooms[i].getFloorPoint(0).x()<<" z:"<<rooms[i].getFloorPoint(0).z()<<endl;
        cout<<"width: "<<rooms[i].getWidth()<<" length: "<<rooms[i].getLength()<<" ratio: "<<ratio<<" area: "<<rooms[i].getArea()<<endl<<endl;
      }
    }


    /************************************************************************/
    /* Function that gets the keyboard events                              */
    /************************************************************************/
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


      //Step by step execution
      if(is_key_down('I')){

        if(step ==1 ){
          cout<<"Generating rooms"<<endl;
          step++;
          rooms.clear();
          minimumTreeRooms.clear();
          roomsGraph;
          minimunSpanningTree;
          corridorPoints.clear();
          corridors.clear();
          triangulator.reset();
          generateRooms();

        }else if(step == 2){
          cout<<"Separating rooms"<<endl;
          step++;
          while(roomsOverlap()){
            separateTiles();
          }

        }else if(step == 3){
          cout<<"Create small tiles"<<endl;
          step++;
          createSmallTiles();

        }else if(step == 4){
          cout<<"Delaunay Triangulation"<<endl;
          step++;
          selectRoomsAndCreateCompleteGraph();
          this->showDelaunayTriangulation = true;

        }else if(step == 5){
          cout<<"Minimum Spanning Tree"<<endl;
          step++;
          createMinimumSpanningTree();
          generateRandomEdges();
          this->showDelaunayTriangulation = false;
          this->showMinimumSpanningTree=true;

        }else if(step == 6){
          cout<<"Generate Corridors"<<endl;
          step++;
          generateLShapesForCorridors();
          this->showMinimumSpanningTree=false;
          this->showCorridors = true;

        }else if(step == 7){
          cout<<"Final dungeon"<<endl;
          step = 1;
          this->showCorridors = false;
          generateFinalDungeon();
        }
      }

      if(is_key_down('T')){
       if(showDelaunayTriangulation){
         showDelaunayTriangulation = false;
       }else{
         showDelaunayTriangulation = true;
       }
      }

      if(is_key_down('C')){
        if(showCorridors){
          showCorridors = false;
        }else{
          showCorridors = true;
        }
      }

      if(is_key_down('M')){
        if(showMinimumSpanningTree){
          showMinimumSpanningTree = false;
        }else{
          showMinimumSpanningTree = true;
        }
      }

      if(is_key_down('G')){
        if(showGrid){
          showGrid = false;
        }else{
          showGrid = true;
        }
      }

      if(is_key_down('R')){
        rooms.clear();
        minimumTreeRooms.clear();
        roomsGraph;
        minimunSpanningTree;
        corridorPoints.clear();
        corridors.clear();
        triangulator.reset();

        dungeonAlgorithm();
      }
    }


    /************************************************************************/
    /* Game loop                                                                     */
    /************************************************************************/
    void draw_world(int x, int y, int w, int h) {

      keyboard();

      // set a viewport - includes whole window area
      glViewport(x, y, w, h);

      // background color
      //glClearColor(0.53f, 0.81f, 0.98f, 1);
      glClearColor(0, 0, 0, 1);

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



      //Delaunay triangulation
      if(showDelaunayTriangulation){

        int numTris = triangulator.getNumTriangles();
        ITRIANGLE *tris = triangulator.getTriangles();
        XYZ *points = triangulator.getPoints();

        for (int i=0; i<numTris; i++)
        {

          XYZ p1 = points[tris[i].p1];
	        XYZ p2 = points[tris[i].p2];
          XYZ p3 = points[tris[i].p3];

          renderDelaunayTriangulation(modelToProjection,p1,p2,p3);
        }
      }

      //Minimun spanning tree
      if(showMinimumSpanningTree){
        for(int i=0; i!=minimumTreeRooms.size();++i){
          for(int j=0; j!=minimumTreeRooms.size();++j){
            if(i!=j){
              if(minimunSpanningTree.getValueAt(i,j) == 1.0f){
                renderLines(modelToProjection, minimumTreeRooms[i]->getMidPoint(),minimumTreeRooms[j]->getMidPoint(),vec4(0.0f,1.0f,0.0f,1.0f));
              }
            }
          }
        }
      }
      
      

      grid.set_mode(GL_LINE_STRIP);
      dungeon_shader_.render(modelToProjection,4);
      
      //Shows the debugging grid
      if(showGrid){
        renderCoordinatesOrigin(modelToProjection);
        grid.render();
      }
      
     if(showCorridors){

       //L-Shape blue line corridors
       for(int p=0; p!=corridorPoints.size();p+=3){
         renderCorridors(modelToProjection, corridorPoints[p], corridorPoints[p+1],corridorPoints[p+2],vec4(0.0f,0.0f,1.0f,1.0f));
       }

       //Yellow corridors
       for(int i=0;i!=corridors.size();++i){
          dungeon_shader_.render(modelToProjection,static_cast<int>(corridors[i].getTexture()));
          if(corridors[i].getRendered())
            corridors[i].render();
        }
     }
      
      
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

    void renderDelaunayTriangulation(mat4t modelToProjection, XYZ p1, XYZ p2, XYZ p3){

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

    void renderCorridors(mat4t modelToProjection, vec4 p1, vec4 p2, vec4 p3, vec4 color){
      color_shader_.render(modelToProjection,color);

      float vertices[] = {
        p1.x(), 0, p1.z(),
        p2.x(), 0, p2.z(),
        p3.x(), 0, p3.z(),
      };

      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)vertices );
      glEnableVertexAttribArray(attribute_pos);

      glDrawArrays(GL_LINE_STRIP, 0, 3);
    }

    void renderLines(mat4t modelToProjection, vec4 p1, vec4 p2, vec4 color){

      color_shader_.render(modelToProjection,color);

      float vertices[] = {
        p1.x(), 0, p1.z(),
        p2.x(), 0, p2.z(),
      };

      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)vertices );
      glEnableVertexAttribArray(attribute_pos);

      glDrawArrays(GL_LINES, 0, 2);

    }

  };   

}    