////////////////////////////////////////////////////////////////////////////////
//
// (C) M. Dimitri Alvarez 2013
//
// Labyrithn game
//

#include <stdlib.h>

namespace octet {
	class element: public box{

		//Velocity of the element in the x-axis
		float velocity_x;
		//Velocity of the element in the y-axis
		float velocity_y;
		//If it is going to be rendered
		bool enabled;
	
	public:
		element(){
			enabled = true;
		}

		//Returns if the element collides with a collection
		bool collides_with_any(const element rhs[], int size) const {
			for(int i=0;i!=size;++i){
				if (this->collides_with(rhs[i])){
					return true;
				}
			}
			return false;
		}

		//Returns the element of the collection that collides with
		element* get_element_that_collides(element rhs[], int size) const {
			for(int i=0;i!=size;++i){
				if (this->collides_with(rhs[i])){
					return &rhs[i];
				}
			}
			return &rhs[0];
		}

		float getVelocityX(){
			return this->velocity_x;
		}

		float getVelocityY(){
			return this->velocity_y;
		}

		bool &is_enabled() {
			return enabled;
		}

		void setVelocityX(float velocity){
			this->velocity_x = velocity;
		}

		void setVelocityY(float velocity){
			this->velocity_y = velocity;
		}

	};

	class labyrithn_app : public octet::app {
		// Matrix to transform points in our camera space to the world.
		// This lets us move our camera
		mat4t cameraToWorld;

		// shader to draw a solid color
		color_shader color_shader_;

		// what state is the game in?
		enum state_t {
			start,
			playing,
			game_over
		};
		state_t state;

		// score counter
		int score;

		// GAME OBJECTS

		// walls
		element walls[84];

		// enemies
		element enemies[15];

		//coins
		element coins[162];

		//bullets
		element bullets[3];
		int numCaughtBullets;

		// the character
		element character;
		
		//the goal of the game
		element goal;

		bool gameStarted;

		//Number of lifes of the character
		int numLifes;

		// random number generator
		class random randomizer;

		// sounds
		ALuint collisionSound;
		ALuint goalSound;
		ALuint coinSound;
		ALuint shootSound;
		ALuint deadSound;
		unsigned cur_source;
		ALuint sounds[5];


		// move the objects before drawing
		void simulate() {
		
			if(gameStarted && state!=game_over){
			// Character movement
				if (is_key_down(key_up)) {
					character.translate(0, 0.05f);
					
					character.setVelocityY(0.05f);
					character.setVelocityX(0.0);

					if(character.collides_with_any(walls,sizeof(walls)/sizeof(walls[0]))){
						character.translate(0, -0.05f);
					}
				} else if (is_key_down(key_down)) {
					character.translate(0, -0.05f);

					character.setVelocityY(-0.05f);
					character.setVelocityX(0.0);

					if(character.collides_with_any(walls,sizeof(walls)/sizeof(walls[0]))){
						character.translate(0, +0.05f);
					}
				} else if(is_key_down(key_right)){
					character.translate(0.05f,0);

					character.setVelocityX(0.05f);
					character.setVelocityY(0.0);

					if(character.collides_with_any(walls,sizeof(walls)/sizeof(walls[0]))){
						character.translate(-0.05f,0);
					}
				}else if(is_key_down(key_left)){
					character.translate(-0.05f,0);

					character.setVelocityX(-0.05f);
					character.setVelocityY(0.0);

					if(character.collides_with_any(walls,sizeof(walls)/sizeof(walls[0]))){
						character.translate(+0.05f,0);
					}
				}
			}
		
			if (state == start) {

				//Pressing Space starts the game

				//We start the movements of the enemies when we start each round
				if (is_key_down('R')) {
					setLevelBeginning();
					for(int i = 0; i< sizeof(enemies)/sizeof(enemies[0]);i++){
						enemies[i].setVelocityX(0.0f);
						enemies[i].setVelocityY(0.07f);
						gameStarted=true;
					}

					for(int i = 0; i< sizeof(bullets)/sizeof(bullets[0]);i++){
						bullets[i].setVelocityX(0.0f);
						bullets[i].setVelocityY(0.0f);
					} 
					state = playing;

				}

			} else if (state == game_over) {

				//Pressing R we start the game again

				if(is_key_down('R')){
					SetGame();
				}

				
			} else if (state == playing) {

				//We move the enemies through the labyrinth

				for(int i = 0; i< sizeof(bullets)/sizeof(bullets[0]);i++){
					
					if(!bullets[i].collides_with_any(walls,sizeof(walls)/sizeof(walls[0])) && !bullets[i].collides_with(goal)){

						bullets[i].translate(bullets[i].getVelocityX(),bullets[i].getVelocityY());

					}else{

						bullets[i].setVelocityX(0.0);
						bullets[i].setVelocityY(0.0);
						bullets[i].is_enabled() = false;
					}

					if(bullets[i].collides_with_any(enemies,sizeof(enemies)/sizeof(enemies[0]))){
						
						element *enemie = bullets[i].get_element_that_collides(enemies,sizeof(enemies)/sizeof(enemies[0]));

						if(bullets[i].is_enabled() && enemie->is_enabled() && (bullets[i].getVelocityX() != 0 || bullets[i].getVelocityY() != 0)){

							enemie->is_enabled() = false;

							enemie->setVelocityX(0.0);
							enemie->setVelocityY(0.0);
							
							bullets[i].is_enabled() = false;

							ALuint source = get_sound_source();
							alSourcei(source, AL_BUFFER,deadSound);
							alSourcePlay(source);
							
						}
					}
				} 

				for(int i = 0; i< sizeof(enemies)/sizeof(enemies[0]);i++){
					enemies[i].translate(enemies[i].getVelocityX(),enemies[i].getVelocityY());

					if(enemies[i].collides_with_any(walls,sizeof(walls)/sizeof(walls[0]))){


						int randomNumber = randomizer.get(0, 10);

						if(enemies[i].getVelocityX()>0){
							enemies[i].setVelocityX(0);
							enemies[i].translate(-0.1f,0);
							if(randomNumber % 2 ==0){
								enemies[i].setVelocityY(-0.07f);
							}else{
								enemies[i].setVelocityY(0.07f);
							}
						}else if(enemies[i].getVelocityX()<0){
							enemies[i].setVelocityX(0);
							enemies[i].translate(+0.1f,0);
							if(randomNumber % 2 ==0){
								enemies[i].setVelocityY(-0.07f);
							}else{
								enemies[i].setVelocityY(0.07f);
							}
						}else if(enemies[i].getVelocityY()>0){
							enemies[i].setVelocityY(0);
							enemies[i].translate(0,-0.1f);
							if(randomNumber % 2 ==0){
								enemies[i].setVelocityX(-0.07f);
							}else{
								enemies[i].setVelocityX(0.07f);
							}
						}else if(enemies[i].getVelocityY()<0){
							enemies[i].setVelocityY(0);
							enemies[i].translate(0,+0.1f);
							if(randomNumber % 2 ==0){
								enemies[i].setVelocityX(-0.07f);
							}else{
								enemies[i].setVelocityX(0.07f);
							}
						}
					}
				}

				//Enemies collide with our character
				if(character.collides_with_any(enemies,sizeof(enemies)/sizeof(enemies[0]))){


					element *enemie = character.get_element_that_collides(enemies,sizeof(enemies)/sizeof(enemies[0]));

					if(enemie->is_enabled()){

						ALuint source = get_sound_source();
						alSourcei(source, AL_BUFFER, collisionSound);
						alSourcePlay(source);

						//We lose a life a we go back to the start point
						if(numLifes>0){
							numLifes--;
							state = start;
							gameStarted=false;
						}else{
							state = game_over;
						}
					}
					
				}

				// When the character collides with coins
				if (character.collides_with_any(coins,sizeof(coins)/sizeof(coins[0]))){

					element *coin = character.get_element_that_collides(coins,sizeof(coins)/sizeof(coins[0]));

					if(coin->is_enabled()){

						coin->is_enabled() = false;

						ALuint source = get_sound_source();
						alSourcei(source, AL_BUFFER,coinSound);
						alSourcePlay(source);
						score++;
					}
				}

				//When the character takes the bullets
				if (character.collides_with_any(bullets,sizeof(bullets)/sizeof(bullets[0]))){

					element *bullet = character.get_element_that_collides(bullets,sizeof(bullets)/sizeof(bullets[0]));

					if(bullet->is_enabled()){

						bullet->is_enabled() = false;
						numCaughtBullets ++;
						
						printf("Bullets:%d\n",numCaughtBullets);
						
						ALuint source = get_sound_source();
						alSourcei(source, AL_BUFFER,coinSound);
						alSourcePlay(source);
						
					}
				} 

				if (is_key_down(key_space) && numCaughtBullets>0) {
					
					for (int i = 0; i != sizeof(bullets)/sizeof(bullets[0]); ++i) {
						
						if(!bullets[i].is_enabled()){
							
							bullets[i].is_enabled() = true;
							numCaughtBullets--;
							
							printf("Bullets:%d\n",numCaughtBullets);

							ALuint source = get_sound_source();
							alSourcei(source, AL_BUFFER,shootSound);
							alSourcePlay(source);

							
							if(character.getVelocityX() > 0){
								bullets[i].set_relative(character, 0.2f, 0.0f);
								bullets[i].setVelocityX(0.09f);
								bullets[i].setVelocityY(0);
							}
							
							if(character.getVelocityX() <0){
								bullets[i].set_relative(character, -0.2f, 0.0f);
								bullets[i].setVelocityX(-0.09f);
								bullets[i].setVelocityY(0);
							}

							if(character.getVelocityY() > 0){
								bullets[i].set_relative(character, 0.0f, 0.2f);
								bullets[i].setVelocityY(0.09f);
								bullets[i].setVelocityX(0);
							}
							

							if(character.getVelocityY() < 0){
								bullets[i].set_relative(character, 0.0f, -0.2f);
								bullets[i].setVelocityY(-0.09f);
								bullets[i].setVelocityX(0);
							}
						}

						//When we shoot a bullet we exit the for, in order to get one press of the key, one shoot
						return;
					}
				}
			
				//When character reachs the goal
				if(character.collides_with(goal)){
					ALuint source = get_sound_source();
					alSourcei(source, AL_BUFFER,goalSound);
					alSourcePlay(source);
					state = game_over;
				}
			}
		}

		ALuint get_sound_source() { return sounds[cur_source++ % 2]; }


	public:

		// this is called when we construct the class
		labyrithn_app(int argc, char **argv) : app(argc, argv){
		}

		// this is called once OpenGL is initialized
		void app_init() {
			color_shader_.init();
			cameraToWorld.loadIdentity();
			cameraToWorld.translate(0, 0, 5);
			
			// sounds
			collisionSound = resources::get_sound_handle(AL_FORMAT_MONO16, "assets/labyrithn/collision.wav");
			goalSound = resources::get_sound_handle(AL_FORMAT_MONO16, "assets/labyrithn/success.wav"); 
			coinSound = resources::get_sound_handle(AL_FORMAT_MONO16, "assets/labyrithn/coin.wav");
			shootSound = resources::get_sound_handle(AL_FORMAT_MONO16, "assets/labyrithn/shoot.wav");
			deadSound = resources::get_sound_handle(AL_FORMAT_MONO16, "assets/labyrithn/death.wav");

			cur_source = 0;
			alGenSources(5, sounds);

			SetGame();
		} 

		void SetGame(){
			numLifes = 2;
			numCaughtBullets = 0;
			score = 0;

			setLevelBeginning();

			enableElementsGameReset();

		}

		void enableElementsGameReset()
		{
			for(int i = 0; i< sizeof(coins)/sizeof(coins[0]);i++){
				coins[i].is_enabled() = true;
			}

			for(int i = 0; i< sizeof(bullets)/sizeof(bullets[0]);i++){
				bullets[i].is_enabled() = true;
			}

			for(int i = 0; i< sizeof(enemies)/sizeof(enemies[0]);i++){
				enemies[i].is_enabled() = true;
			}
		}

		//We reset the round
		void setLevelBeginning()
		{
			gameStarted = false; 
			
			character.init(vec4(0.39f, 0.58f, 0.93f, 1), -4.7f, -4.7f, 0.25f, 0.25f);

			bullets[0].init(vec4(0, 0, 1, 1), 1.9f, -1.0f, 0.1f, 0.1f);
			bullets[1].init(vec4(0, 0, 1, 1), 2.8f, -1.0f, 0.1f, 0.1f);
			bullets[2].init(vec4(0, 0, 1, 1),-4.3f, 4.7f, 0.1f, 0.1f);


			//Coins

			int coinsIndex = 0;

			//ROW 1
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.7, -4.1, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.7, -3.5, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.7, -2.9, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.7, -2.1, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.7, -1.5, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.7, -0.9, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.7, -0.1, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.7, 0.5, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.7, 1.1, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.7, 1.7, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.7, 2.5, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.7, 4.0, 0.1f, 0.1f);

			//ROW 2
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.0, -4.7, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.0, -2.9, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.0, -2.1, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.0, -0.9, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.0, -0.1, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.0, 1.7, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.0, 2.5, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-3.8, 4.0, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-4.3, 3.2, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-2.9, 3.2, 0.1f, 0.1f);

			//ROW 3
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-3.3, -4.7, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-3.3, -4.1, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-3.3, -3.5, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-3.3, -2.9, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-3.3, -2.1, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-3.5, -1.3, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-3.1, -0.9, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-3.3, -0.1, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-3.3, 1.7, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-3.3, 2.5, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-2.8, 4.0, 0.1f, 0.1f);
      coins[coinsIndex++].init(vec4(1.0f, 0, 0, 1),-3.3, 4.7, 0.1f, 0.1f);
			

			//ROW 4
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-2.6, -4.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-2.6, -4.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-2.6, -3.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-2.6, -2.9, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-2.6, -2.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-2.6, -0.9, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-2.6, -0.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-2.6, 1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-2.6, 2.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-2.3, 4.7, 0.1f, 0.1f);


			//ROW 5
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.9, -4.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.9, -4.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.9, -3.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.9, -2.9, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.9, -2.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.9, -1.4, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.9, -0.9, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.9, -0.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.9, 0.6, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.9, 1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.9, 2.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.8, 4.0, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, 4.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, 3.2, 0.1f, 0.1f);

			//ROW 6
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, -4.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, -4.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, -3.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, -2.9, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, -2.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, -1.4, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, -0.9, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, -0.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, 0.6, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, 1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-1.3, 2.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.9, 4.0, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.3, 4.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),0.0, 3.2, 0.1f, 0.1f);
			

			//ROW 7
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.7, -3.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.7, -0.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.7, -0.9, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.7, -1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.7, 0.6, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.7, 1.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.7, 1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.7, 2.5, 0.1f, 0.1f);

			//ROW 8
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.1, -3.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.1, -0.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.1, -1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.1, 0.6, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.1, 1.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),-0.1, 1.7, 0.1f, 0.1f);

			//ROW 9
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),0.6, -4.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),0.6, -4.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),0.6, -3.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),0.6, -2.9, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),0.6, -2.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),0.6, -1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),0.6, -0.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),0.6, 0.6, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),0.6, 1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),0.6, 2.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),0.6, 4.0, 0.1f, 0.1f);

			//ROW 10
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.1, -4.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.1, -2.9, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.1, -1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.1, -0.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.5, 0.6, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.5, 1.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.5, 1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.5, 2.5, 0.1f, 0.1f);

			//ROW 11
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.6, -4.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.6, -4.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.6, -3.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.6, -2.9, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.6, -2.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.6, -1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.6, -1.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.6, -0.6, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.6, -0.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),1.5, 3.3, 0.1f, 0.1f);

			//ROW 12
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),2.1, -3.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),2.6, -3.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),2.1, -1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),2.3, 2.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),2.6, 4.0, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),2.0, 4.0, 0.1f, 0.1f);
			
			//ROW 13

			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, -4.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, -4.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, -3.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, -2.9, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, -2.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, -1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.2, -1.0, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, -0.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, 0.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, 1.0, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, 1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, 2.6, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, 3.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, 4.0, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),3.1, 4.7, 0.1f, 0.1f);

			//ROW 14
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, -4.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, -4.1, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, -3.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, -2.9, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, -2.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, -1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, -1.0, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, -0.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, 0.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, 1.0, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, 1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, 2.6, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, 3.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.0, 4.7, 0.1f, 0.1f);
			
			//ROW 15
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.6, -3.5, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.6, -1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.6, -1.0, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.6, -0.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.6, 0.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.6, 1.0, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.6, 1.7, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.6, 2.6, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.6, 3.3, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.6, 4.0, 0.1f, 0.1f);
			coins[coinsIndex++].init(vec4(1, 0, 0, 1),4.6, 4.7, 0.1f, 0.1f);

			//Enemies
			enemies[0].init(vec4(0, 0, 0, 1), -4.6f, -2.0f, 0.25f, 0.25f);
			enemies[1].init(vec4(0, 0, 0, 1), 4.6f, -3.4f, 0.25f, 0.25f);
			enemies[2].init(vec4(0, 0, 0, 1), -4.6f, -0.0f, 0.25f, 0.25f);
			enemies[3].init(vec4(0, 0, 0, 1), -1.5f, -4.7f, 0.25f, 0.25f);
			enemies[4].init(vec4(0, 0, 0, 1), 4.0f, 2.5f, 0.25f, 0.25f);
			enemies[5].init(vec4(0, 0, 0, 1), 3.0f, 2.5f, 0.25f, 0.25f);
			enemies[6].init(vec4(0, 0, 0, 1), -4.6f, 2.5f, 0.25f, 0.25f);
			enemies[7].init(vec4(0, 0, 0, 1), -3.8f, 3.5f, 0.25f, 0.25f);
			enemies[8].init(vec4(0, 0, 0, 1), -1.8f, 3.5f, 0.25f, 0.25f);
			enemies[9].init(vec4(0, 0, 0, 1), 4.6f, -1.65f, 0.25f, 0.25f);
			enemies[10].init(vec4(0, 0, 0, 1), 1.9f, -1.00f, 0.25f, 0.25f);
			enemies[11].init(vec4(0, 0, 0, 1), 1.8f, 3.5f, 0.25f, 0.25f);
			enemies[12].init(vec4(0, 0, 0, 1), 0.0f, 4.7f, 0.25f, 0.25f);
			//Goal enemies
			enemies[13].init(vec4(0, 0, 0, 1), 4.6f, 3.4f, 0.25f, 0.25f);
			enemies[14].init(vec4(0, 0, 0, 1), 3.3f, 4.7f, 0.25f, 0.25f);

			//Frame
			walls[0].init(vec4(0.18, 0.31, 0.18, 1), 0, -4.95f, 10.0f, 0.1f);
			walls[1].init(vec4(0.18, 0.31, 0.18, 1), 0,  4.95f, 10.0f, 0.1f);
			walls[2].init(vec4(0.18, 0.31, 0.18, 1), -4.95f, 0, 0.1f, 10);
			walls[3].init(vec4(0.18, 0.31, 0.18, 1), 4.95f,  0, 0.1f, 10);

			//Different buildings

			//City 1
			walls[4].init(vec4(0.18, 0.31, 0.18, 1), -4.0f, -3.8f, 1.0f, 1.4f);
			walls[5].init(vec4(0.18, 0.31, 0.18, 1), -4.0f, -2.5f, 2.0f, 0.5f);
			walls[6].init(vec4(0.18, 0.31, 0.18, 1), -2.4f, -2.5f, 0.5f, 0.5f);
			walls[7].init(vec4(0.18, 0.31, 0.18, 1), -1.4f, -2.5f, 0.8f, 0.5f);
			walls[8].init(vec4(0.18, 0.31, 0.18, 1), -2.4f, -3.5f, 1.5f, 0.7f);
			walls[9].init(vec4(0.18, 0.31, 0.18, 1), -2.4f, -4.4f, 1.5f, 0.3f);

			//City 2
			walls[11].init(vec4(0.18, 0.31, 0.18, 1), -2.9f, -1.7f, 2.5f, 0.3f);
			walls[12].init(vec4(0.18, 0.31, 0.18, 1), -4.2f, -1.5f, 0.3f, 0.7f);
			walls[13].init(vec4(0.18, 0.31, 0.18, 1), -3.0f, -0.5f, 4.0f, 0.3f);
			walls[14].init(vec4(0.18, 0.31, 0.18, 1), -3.5f, -0.8f, 0.3f, 0.5f);
			walls[15].init(vec4(0.18, 0.31, 0.18, 1), -2.8f, -1.3f, 0.3f, 0.5f);

			//City 3
			walls[16].init(vec4(0.18, 0.31, 0.18, 1), -0.4f, -2.5f, 1.5f, 1.3f);
			walls[17].init(vec4(0.18, 0.31, 0.18, 1), -0.4f, -4.3f, 1.5f, 1.3f);
			walls[18].init(vec4(0.18, 0.31, 0.18, 1), 2.3f, -2.5f, 1.0f, 1.3f);
			walls[19].init(vec4(0.18, 0.31, 0.18, 1), 2.3f, -4.3f, 1.0f, 1.3f);
			walls[20].init(vec4(0.18, 0.31, 0.18, 1), 1.1f, -4.7f, 0.5f, 0.8f);
			walls[21].init(vec4(0.18, 0.31, 0.18, 1), 1.1f, -3.5f, 0.5f, 0.8f);
			walls[22].init(vec4(0.18, 0.31, 0.18, 1), 1.1f, -2.3f, 0.5f, 0.8f);
			walls[23].init(vec4(0.18, 0.31, 0.18, 1), 3.5f, -3.3f, 0.3f, 2.2f);
			walls[24].init(vec4(0.18, 0.31, 0.18, 1), 4.8f, -2.5f, 1.0f, 1.3f);
			walls[25].init(vec4(0.18, 0.31, 0.18, 1), 4.8f, -4.3f, 1.0f, 1.3f);
			walls[26].init(vec4(0.18, 0.31, 0.18, 1), 2.5f, -1.3f, 1.4f, 0.3f);
			walls[27].init(vec4(0.18, 0.31, 0.18, 1), 4.5f, -1.3f, 1.4f, 0.3f);
			walls[28].init(vec4(0.18, 0.31, 0.18, 1), 0.1f, -1.3f, 2.5f, 0.3f);
			walls[29].init(vec4(0.18, 0.31, 0.18, 1), 0.3f, -0.8f, 1.6f, 1.0f);
			walls[30].init(vec4(0.18, 0.31, 0.18, 1), 2.3f, 0.7f, 1.0f, 3.0f);

			//City4
			walls[31].init(vec4(0.18, 0.31, 0.18, 1), 3.5f, -0.65f, 1.8f, 0.3f);
			walls[32].init(vec4(0.18, 0.31, 0.18, 1), 4.5f, 0.0f, 1.8f, 0.3f);
			walls[33].init(vec4(0.18, 0.31, 0.18, 1), 3.5f, 0.65f, 1.8f, 0.3f);
			walls[34].init(vec4(0.18, 0.31, 0.18, 1), 4.5f, 1.3f, 1.8f, 0.3f);
			walls[35].init(vec4(0.18, 0.31, 0.18, 1), 3.5f, 2.05f, 1.8f, 0.3f);
			walls[36].init(vec4(0.18, 0.31, 0.18, 1), 3.5f, 2.5f, 0.2f, 0.9f);

			//City 5
			walls[37].init(vec4(0.18, 0.31, 0.18, 1), 0.0f, 0.3f, 3.7f, 0.3f);
			walls[38].init(vec4(0.18, 0.31, 0.18, 1), -3.4f, 0.8f, 2.0f, 1.3f);
			walls[39].init(vec4(0.18, 0.31, 0.18, 1), -1.5f, 1.2f, 0.7f, 0.5f);
			walls[40].init(vec4(0.18, 0.31, 0.18, 1), 0.7f, 1.2f, 0.7f, 0.5f);
			walls[41].init(vec4(0.18, 0.31, 0.18, 1), -0.5f, 1.0f, 0.2f, 1.2f);
			walls[42].init(vec4(0.18, 0.31, 0.18, 1), 1.0f, 2.1f, 2.8f, 0.2f);
			walls[43].init(vec4(0.18, 0.31, 0.18, 1), -3.0f, 2.1f, 4.0f, 0.2f);

			//City 6
			walls[44].init(vec4(0.18, 0.31, 0.18, 1), 1.5f, 4.6f, 2.5f, 0.6f);
			walls[45].init(vec4(0.18, 0.31, 0.18, 1), 1.45f,3.9f, 0.4f, 0.8f);
			walls[46].init(vec4(0.18, 0.31, 0.18, 1), 2.5f,3.35f, 0.4f, 1.0f);
			walls[47].init(vec4(0.18, 0.31, 0.18, 1), 1.5f,3.0f, 2.0f, 0.3f);
			walls[48].init(vec4(0.18, 0.31, 0.18, 1), 0.6f,3.35f, 0.3f, 1.0f);
			walls[49].init(vec4(0.18, 0.31, 0.18, 1), -0.25f,2.5f, 0.3f, 1.0f);
			walls[50].init(vec4(0.18, 0.31, 0.18, 1), -1.35f, 2.9f, 2.5f, 0.3f);
			walls[51].init(vec4(0.18, 0.31, 0.18, 1), -4.5f, 2.9f, 2.5f, 0.3f);
			walls[60].init(vec4(0.18, 0.31, 0.18, 1), -0.5f, 4.0f, 0.5f, 0.5f);
			walls[61].init(vec4(0.18, 0.31, 0.18, 1), -1.3f, 4.0f, 0.5f, 0.5f);
			walls[62].init(vec4(0.18, 0.31, 0.18, 1), -2.3f, 4.0f, 0.5f, 0.5f);
			walls[63].init(vec4(0.18, 0.31, 0.18, 1), -3.3f, 4.0f, 0.5f, 0.5f);
			walls[64].init(vec4(0.18, 0.31, 0.18, 1), -4.3f, 4.0f, 0.5f, 0.5f);
			walls[65].init(vec4(0.18, 0.31, 0.18, 1), -0.8f, 3.1f, 0.5f, 0.5f);
			walls[66].init(vec4(0.18, 0.31, 0.18, 1), -1.8f, 3.1f, 0.5f, 0.5f);
			walls[67].init(vec4(0.18, 0.31, 0.18, 1), -3.8f, 3.1f, 0.5f, 0.5f);
			walls[68].init(vec4(0.18, 0.31, 0.18, 1), -4.8f, 3.1f, 0.5f, 0.5f);
			walls[69].init(vec4(0.18, 0.31, 0.18, 1), -0.8f, 4.9f, 0.5f, 0.5f);
			walls[70].init(vec4(0.18, 0.31, 0.18, 1), -1.8f, 4.9f, 0.5f, 0.5f);
			walls[71].init(vec4(0.18, 0.31, 0.18, 1), -2.8f, 4.9f, 0.5f, 0.5f);
			walls[72].init(vec4(0.18, 0.31, 0.18, 1), -3.8f, 4.9f, 0.5f, 0.5f);
			walls[73].init(vec4(0.18, 0.31, 0.18, 1), -4.8f, 4.9f, 0.5f, 0.5f);
			walls[74].init(vec4(0.18, 0.31, 0.18, 1), 2.3f, -1.0f, 0.5f, 0.5f);
			walls[75].init(vec4(0.18, 0.31, 0.18, 1), 2.55f, -1.7f, 0.5f, 0.5f);
			walls[76].init(vec4(0.18, 0.31, 0.18, 1), 1.3f, 3.8f, 0.3f, 0.1f);

			//Plaza  
			walls[81].init(vec4(0.18, 0.31, 0.18, 1), 4.0f, 3.0f, 2.6f, 0.2f);
			walls[82].init(vec4(0.18, 0.31, 0.18, 1), 2.8f, 3.4f, 0.2f, 0.9f);
			walls[83].init(vec4(0.18, 0.31, 0.18, 1), 2.8f, 4.7f, 0.2f, 0.8f);
			
			goal.init(vec4(0.39, 0.58, 0.93, 1), 3.9f, 4.0f, 1.0f, 1.0f);

			state = start;
		}

		// this is called to draw the world++
	
		void draw_world(int x, int y, int w, int h) {
			simulate();

			// set a viewport - includes whole window area
			glViewport(x, y, w, h);

			// background color
			
			glClearColor(1.0, 0.96, 0.56, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// allow Z buffer depth testing (closer objects are always drawn in front of far ones)
			glEnable(GL_DEPTH_TEST);

			// allow alpha blend (transparency when alpha channel is 0)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// draw the character, goal and bulls
			character.render(color_shader_, cameraToWorld);
			goal.render(color_shader_,cameraToWorld);

			for(int i = 0; i< sizeof(enemies)/sizeof(enemies[0]);i++){
				if(enemies[i].is_enabled()){
					enemies[i].render(color_shader_, cameraToWorld);
				}
			}

			// draw the labyrinth
			for (int i = 0; i != sizeof(walls)/sizeof(walls[0]); ++i) {
				walls[i].render(color_shader_, cameraToWorld);
			} 

			for (int i = 0; i != sizeof(bullets)/sizeof(bullets[0]); ++i) {
				if(bullets[i].is_enabled()){
					bullets[i].render(color_shader_, cameraToWorld);
				}
			}

			for(int i = 0; i< sizeof(coins)/sizeof(coins[0]);i++){
				if(coins[i].is_enabled()){
					coins[i].render(color_shader_, cameraToWorld);
				}
			}
		}
	};
}

