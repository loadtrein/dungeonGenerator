namespace octet{

  class Wall{

  public:

    vec4 wallPoints[4];

    Wall(vec4 p1, vec4 p2, vec4 p3, vec4 p4){
      wallPoints[0]=p1;
      wallPoints[1]=p2;
      wallPoints[2]=p3;
      wallPoints[3]=p4;
    }

    Wall(const Wall & rhs){
      wallPoints[0]=rhs.wallPoints[0];
      wallPoints[1]=rhs.wallPoints[1];
      wallPoints[2]=rhs.wallPoints[2];
      wallPoints[3]=rhs.wallPoints[3];
    }


  };

  class Room{

    vec4 grounPoints[4];

    vec4 midPoint;

    float width;
    float length;
    float area;

    int texture;

    bool isRendered;

    mesh meshRoom;

    void generateMidPoint(){

      midPoint = vec4(grounPoints[0].x()+width/2,grounPoints[0].y(),
        grounPoints[0].z()+length/2,grounPoints[0].w());
    }

  public:

    Room(const Room & rhs){
      meshRoom = rhs.meshRoom;

      grounPoints[0]=rhs.grounPoints[0];
      grounPoints[1]=rhs.grounPoints[1];
      grounPoints[2]=rhs.grounPoints[2];
      grounPoints[3]=rhs.grounPoints[3];

      width = rhs.width;
      length = rhs.length;
      area =  rhs.area;
      midPoint = rhs.midPoint;
      texture = rhs.texture;
      isRendered = rhs.isRendered;

    }

    Room(vec4 p1, float width, float length){

      grounPoints[0] = p1;
      grounPoints[1] = vec4(p1.x(),p1.y(),p1.z()+length,p1.w());
      grounPoints[2] = vec4(p1.x()+width,p1.y(),p1.z()+length,p1.w());
      grounPoints[3] = vec4(p1.x()+width,p1.y(),p1.z(),p1.w());

      this->width = width;
      this->length = length;

      area = width * length;

      isRendered = true;

      texture = rand() / (RAND_MAX/4);

      generateMidPoint();

      meshRoom.make_plane(grounPoints[0],grounPoints[1],grounPoints[2],grounPoints[3]);
    } 

    vec4 getMidPoint(){
      return this->midPoint;
    }

    vec4 getGroundPoint(int index){
      return this->grounPoints[index];
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

    bool getRendered(){
      return this->isRendered;
    }

    void setRendered(bool value){
      this->isRendered = value;
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

      meshRoom.init();

      grounPoints[0] = vec4(grounPoints[0].x()+xMovement,grounPoints[0].y(),grounPoints[0].z()+zMovement,grounPoints[0].w());
      grounPoints[1] = vec4(grounPoints[1].x()+xMovement,grounPoints[1].y(),grounPoints[1].z()+zMovement,grounPoints[1].w());
      grounPoints[2] = vec4(grounPoints[2].x()+xMovement,grounPoints[2].y(),grounPoints[2].z()+zMovement,grounPoints[2].w());
      grounPoints[3] = vec4(grounPoints[3].x()+xMovement,grounPoints[3].y(),grounPoints[3].z()+zMovement,grounPoints[3].w());

      generateMidPoint();

      meshRoom.make_plane(grounPoints[0],grounPoints[1],grounPoints[2],grounPoints[3]);

    }

    void render(){
      meshRoom.render();
    }

  };


  class Graph{

    int numElements;
    std::vector<float> values;
    std::vector<Room*> rooms;

  public:
   Graph(){

   }
   
   void addRoom(Room* r){
     rooms.push_back(r);
   }

   void initialiseGraph(int numElements){
     
     this->numElements = numElements;
     this->values = std::vector<float>(numElements*numElements);

     for(int i=0;i!=numElements*numElements;++i){
       this->values[i] = 0.0f;
     }
   }

   float getValueAt(int i, int j){
     return this->values[i*numElements+j];
   }

   void setValueAt(int i, int j, float v){
     this->values[i*numElements+j]= v;
   }

   int getMinimunAt(int row){

     float minimun= std::numeric_limits<float>::infinity();
    
     for(int j=0;j!=numElements;++j){
        if(values[row*numElements+j] != 0.0f && values[row*numElements+j] < minimun){
          minimun = values[row*numElements+j];
          minimun = j;
        }
     }

     return static_cast<int>(minimun);
   
   }

   void setConnectionBetween(vec4 p1, vec4 p2){
     int posP1 = getIndexForRoom(p1);
     int posP2 = getIndexForRoom(p2);
     
     float dist = sqrt( (p1.x() - p2.x()) * (p1.x() - p2.x()) + (p1.z() - p2.z()) * (p1.z() - p2.z()) );

     setValueAt(posP1,posP2,dist);
     setValueAt(posP2,posP1,dist);
   }

   int getIndexForRoom(vec4 point){
     for(int i=0; i!=rooms.size();++i){
       if(cmpf(rooms[i]->getMidPoint().x(),point.x()) && cmpf(rooms[i]->getMidPoint().z(),point.z())){
         return i;
       }
     }
   }

   bool cmpf(float A, float B, float epsilon = 0.005f)
   { 
     return (fabs(A - B) < epsilon);
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