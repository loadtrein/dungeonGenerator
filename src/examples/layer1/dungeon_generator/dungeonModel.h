namespace octet{

  class Wall{

  private:

    vec4 wallPoints[4];

    mesh wallMesh;

    bool isRendered;

  public:

    Wall(vec4 p1, vec4 p2, vec4 p3, vec4 p4){
      wallPoints[0]=p1;
      wallPoints[1]=p2;
      wallPoints[2]=p3;
      wallPoints[3]=p4;

      this->isRendered = true;

      wallMesh.make_plane(p1,p2,p3,p4);
    }

    Wall(const Wall & rhs){
      wallPoints[0]=rhs.wallPoints[0];
      wallPoints[1]=rhs.wallPoints[1];
      wallPoints[2]=rhs.wallPoints[2];
      wallPoints[3]=rhs.wallPoints[3];

      wallMesh = rhs.wallMesh;

      isRendered = rhs.isRendered;
    }

    bool getRendered(){
      return this->isRendered;
    }

    void setRendered(bool value){
      this->isRendered = value;
    }

    vec4 getWallPoint(int index){
      return this->wallPoints[index];
    }

    void render(){
      wallMesh.render();
    }

  };

  class Room{

    std::vector<Wall> walls;

    vec4 midPoint;

    float width;
    float length;
    float area;

    int texture;

    
    //It calculates the midpoint from the bottom left point of the floor wall
    void generateMidPoint(vec4 p1){
      midPoint = vec4(p1.x()+width/2,p1.y(),p1.z()+length/2,p1.w());
    }

    void createWalls(){

      Wall floor = walls[0];

      for(int i=0; i!=4;++i){

        Wall wall(vec4(floor.getWallPoint(i).x(),floor.getWallPoint(i).y(),floor.getWallPoint(i).z(),floor.getWallPoint(i).w()),
          vec4(floor.getWallPoint(i).x(),floor.getWallPoint(i).y() - 2.0f,floor.getWallPoint(i).z(),floor.getWallPoint(i).w()),
          vec4(floor.getWallPoint((i==3)? 0 : i+1).x(),floor.getWallPoint((i==3)? 0 : i+1).y() - 2.0f,floor.getWallPoint((i==3)? 0 : i+1).z(),floor.getWallPoint((i==3)? 0 : i+1).w()),
          vec4(floor.getWallPoint((i==3)? 0 : i+1).x(),floor.getWallPoint((i==3)? 0 : i+1).y(),floor.getWallPoint((i==3)? 0 : i+1).z(),floor.getWallPoint((i==3)? 0 : i+1).w()));

        walls.push_back(wall);
      
      }

    }

  public:

    /*Room(const Room & rhs){

      grounPoints[0]=rhs.grounPoints[0];
      grounPoints[1]=rhs.grounPoints[1];
      grounPoints[2]=rhs.grounPoints[2];
      grounPoints[3]=rhs.grounPoints[3];

      width = rhs.width;
      length = rhs.length;
      area =  rhs.area;
      midPoint = rhs.midPoint;
      texture = rhs.texture;

    }*/

    Room(vec4 p1, float width, float length){

      //We create the floor of the room
      Wall roomFloor(p1, vec4(p1.x(),p1.y(),p1.z()+length,p1.w()), vec4(p1.x()+width,p1.y(),p1.z()+length,p1.w()),vec4(p1.x()+width,p1.y(),p1.z(),p1.w()));
      walls.push_back(roomFloor);

      this->width = width;
      this->length = length;

      area = width * length;

      texture = rand() / (RAND_MAX/3);

      generateMidPoint(p1);

      //We create the walls of the room
      createWalls();

    } 

    //Constructor just for the bounding rooms for the L-Shape Corridors
    Room(vec4 p1, vec4 p2, vec4 p3, vec4 p4){

      //We create the floor of the room
      Wall roomFloor(p1, p2, p3, p4);
      walls.push_back(roomFloor);

      this->width = abs(p1.x()) - abs(p4.x());
      this->length = abs(p1.z()) - abs(p4.z());

      area = width * length;

      texture = 3;

      generateMidPoint(p1);

      //WE NEED TO DETERMINE IF WE CREATE THE WALLS FOR THE CORRIDORS IN ORDER TO DETECT THE INTERSECTIONS ----- TO DOOOOOO!!!!

    }

    vec4 getMidPoint(){
      return this->midPoint;
    }

    vec4 getFloorPoint(int index){
      return this->walls[0].getWallPoint(index);
    }

    float getWidth(){
      return this->width;
    }

    float getLength(){
      return this->length;
    }

    float getArea(){
      return this->area;
    }

    int getTexture(){
      return this->texture;
    }

    void setTexture(int t){
      this->texture = t;
    }

    //TO DO
    bool getRendered(){
      return this->walls[0].getRendered();
    }

    //TO DO
    void setRendered(bool v){
     this->walls[0].setRendered(v);
    }

    void separate(vec4 separationVector){

      float xMovFloat = separationVector.x();

      float zMovFloat = separationVector.z();

      if(abs(xMovFloat) > abs(zMovFloat)){
        xMovFloat*=2;
      }else{
        zMovFloat*=2;
      }

      int xMovement = static_cast<int>(xMovFloat);
      int zMovement = static_cast<int>(zMovFloat);


      //We store the new values for the floor
      vec4 p1 (walls[0].getWallPoint(0).x()+xMovement,walls[0].getWallPoint(0).y(),walls[0].getWallPoint(0).z()+zMovement,walls[0].getWallPoint(0).w());
      vec4 p2 (walls[0].getWallPoint(1).x()+xMovement,walls[0].getWallPoint(1).y(),walls[0].getWallPoint(1).z()+zMovement,walls[0].getWallPoint(1).w());
      vec4 p3 (walls[0].getWallPoint(2).x()+xMovement,walls[0].getWallPoint(2).y(),walls[0].getWallPoint(2).z()+zMovement,walls[0].getWallPoint(2).w());
      vec4 p4 (walls[0].getWallPoint(3).x()+xMovement,walls[0].getWallPoint(3).y(),walls[0].getWallPoint(3).z()+zMovement,walls[0].getWallPoint(3).w());

      //We delete the old walls
      walls.clear();

      //We store the new ones
      Wall roomFloor(p1, p2, p3,p4);
      walls.push_back(roomFloor);

      generateMidPoint(p1);

      createWalls();

    }

    void render(){
      for(int i=0; i!=walls.size();++i){
        walls[i].render();
      }
    }

  };

  class PriorityQueueNode{
    Room * roomOrigin;
    Room * roomDestination;
    float distance;

  public:

    PriorityQueueNode(Room* r1, Room* r2, float d):roomOrigin(r1),roomDestination(r2),distance(d){}

    float getDistance(){
      return this->distance;
    }

    Room* getRoomOrigin(){
      return this->roomOrigin;
    }

    Room* getRoomDestination(){
      return this->roomDestination;
    }
  };


  class ComparePQNode {
  public:
    bool operator()(PriorityQueueNode& pqn1, PriorityQueueNode& pqn2)
    {
      if (pqn2.getDistance() < pqn1.getDistance()){
        return true;
      }else{
        return false;
      }
    }
  };

  class Graph{

    int numElements;
    std::vector<float> values;
    std::vector<Room*> rooms;


    int getIndexForRoom(vec4 point){
      for(int i=0; i!=rooms.size();++i){
        if(cmpf(rooms[i]->getMidPoint().x(),point.x()) && cmpf(rooms[i]->getMidPoint().z(),point.z())){
          return i;
        }
      }
    }

    int getIndexForRoom(Room* r){
      for(int i=0; i!=rooms.size();++i){
        if(cmpf(rooms[i]->getMidPoint().x(),r->getMidPoint().x()) && cmpf(rooms[i]->getMidPoint().z(),r->getMidPoint().z())){
          return i;
        }
      }
    }

    bool cmpf(float A, float B, float epsilon = 0.005f)
    { 
      return (fabs(A - B) < epsilon);
    }


  public:
   Graph(){

   }
   
   void addRoom(Room* r){
     rooms.push_back(r);
   }


   void initialiseGraph(int numElements){
     
     this->numElements = numElements;
     this->values = std::vector<float>(numElements*numElements);
     this->rooms.clear();

     for(int i=0;i!=numElements*numElements;++i){
       this->values[i] = 0.0f;
     }
   }

   float getValueAt(int i, int j){
     return this->values[i*numElements+j];
   }

   float getValueAt(Room* r1, Room* r2){
     int i = getIndexForRoom(r1);
     int j = getIndexForRoom(r2);

     return (this->values[i*numElements+j] != 0.0f)? this->values[i*numElements+j] : std::numeric_limits<float>::max();

   }

   void setValueAt(int i, int j, float v){
     this->values[i*numElements+j]= v;
   }

   void setValueAt(Room* rOrigin, Room* rDestination, float v){
     
     int row = getIndexForRoom(rOrigin);
     int column = getIndexForRoom(rDestination);

     this->values[row*numElements+column]= v;
   }

   Room* getClosestNode(Room* r, float &distance){

     float minimun= std::numeric_limits<float>::infinity();
     int node = 0;

     int row = getIndexForRoom(r);
    
     for(int j=0;j!=numElements;++j){
        if(values[row*numElements+j] != 0.0f && values[row*numElements+j] < minimun){
          minimun = values[row*numElements+j];
          node = j;
        }
     }

     distance = values[row*numElements+node];
     //This is made to always get the Closest Node in the next call (disadvante we destroy some of the values of the original graph)
     values[row*numElements+node] = 0.0;

     return rooms[node];
   
   }

   void setConnectionBetween(vec4 p1, vec4 p2){
     int posP1 = getIndexForRoom(p1);
     int posP2 = getIndexForRoom(p2);
     
     float dist = sqrt( (p1.x() - p2.x()) * (p1.x() - p2.x()) + (p1.z() - p2.z()) * (p1.z() - p2.z()) );

     setValueAt(posP1,posP2,dist);
     setValueAt(posP2,posP1,dist);
   }


   void printGraph(){
     for(int i=0;i!=numElements;++i){
       for(int j=0;j!=numElements;++j){
         cout<<values[i*numElements+j]<<" ";
       }
       cout<<endl;
     }
   }


  };


}