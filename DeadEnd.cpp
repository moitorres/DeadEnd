/*
    Client program for the game "Dead End"

    Moises Uriel Torres A01021323
    Daniel Atilano A01020270
*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <time.h>
// Signals library
#include <errno.h>
#include <signal.h>
//SFML libraries
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
// Sockets libraries
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/poll.h>
// Custom libraries
#include "sockets.h"
#include "fatal_error.h"

using namespace std;

#define BUFFER_SIZE 1024

///// FUNCTION DECLARATIONS
void usage(char * program);
void onInterrupt(int signal);
void setupHandlers();
void startGame(int connection_fd);
void startScreen(sf::RenderWindow &window);
void deathScreen(sf::RenderWindow &window);
void victoryScreen(sf::RenderWindow &window);
bool collides(sf::Sprite sprite, sf::CircleShape circle);
sf::Vector2f generateRandomPosition(sf::RenderWindow &window);
bool playerMoves();
/*void createMaze(Node nodeList[], sf::RenderWindow &window);
void mergeGroup(Node nodeList[], int group1, int group2);*/

int interrupt_exit=0;

///// MAIN FUNCTION
int main(int argc, char * argv[])
{
    int connection_fd;

    printf("\n=== Dead End ===\n");

    // Check the correct arguments
    if (argc != 3)
    {
        usage(argv[0]);
    }

    //Setup the signal handlers
    setupHandlers();
    // Start the server
    connection_fd = connectSocket(argv[1], argv[2]);
	// Start the game
    startGame(connection_fd);
    // Close the socket
    close(connection_fd);

    return 0;
}

///// FUNCTION DEFINITIONS

/*
    Explanation to the user of the parameters required to run the program
*/
void usage(char * program)
{
    printf("Usage:\n");
    printf("\t%s {server_address} {port_number}\n", program);
    exit(EXIT_FAILURE);
}

// Handler for SIGINT
void onInterrupt(int signal)
{
    interrupt_exit = 1;
}

// Define signal handlers
void setupHandlers()
{
    //Create list for blocked signals
    sigset_t new_set;
    //The set is filled with all possible signals
    sigfillset(&new_set);
    //All save signal 2; i.e. Ctrl-C
    sigdelset(&new_set,2);
    //The new set blocks all other signals
    sigprocmask(SIG_SETMASK, &new_set, NULL);

    struct sigaction new_action;
    new_action.sa_handler = onInterrupt;
    sigaction(SIGINT, &new_action, NULL);
}

/*
    Function that runs the game 
*/
void startGame(int connection_fd)
{
    char buffer[BUFFER_SIZE];
    //Status for the messages from the server
    int status;
    //Counter and limit for the sound queues
    int counter, limit=3;
    int poll_response;
    int timeStart;
    double timeElapsed=0;
    int random;
    bool soundPlaying = false;

    //Create a structure to hold the file descriptors to poll
    struct pollfd test_fds[1];
    test_fds[0].fd = connection_fd;
    test_fds[0].events = POLLIN;

    /*
        Load all the sounds
    */

    //A sound is created
    sf::Sound sound;

    //A sound buffer is created for the breathing sound
    sf::SoundBuffer breathing;
    //A file is loaded into the buffer
    if(!breathing.loadFromFile("./sounds/breathing.wav"))
    {
        printf("Error: loading sound\n");
    }

    //A sound buffer is created for the running sound
    sf::SoundBuffer running;
    //A file is loaded into the buffer
    if(!running.loadFromFile("./sounds/running.wav"))
    {
        printf("Error: loading sound\n");
    }

    //A sound buffer is created for the door sound
    sf::SoundBuffer door;
    //A file is loaded into the buffer
    if(!door.loadFromFile("./sounds/door.wav"))
    {
        printf("Error: loading sound\n");
    }

    /*
        Create the window
    */

    //The window for the game is created
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Dead End");

    /*
        Create all the sprites and objects
    */

    //The player is created
    sf::Texture texturePlayer;
    //A texture for the player is loaded
    if (!texturePlayer.loadFromFile("./images/player_sprite.png"))
    {
        printf("Error: loading texture\n");
    }
    texturePlayer.setSmooth(true);
    sf::Sprite player;
    player.setTexture(texturePlayer);
    player.setPosition(sf::Vector2f(window.getSize().x / 2.f -70, window.getSize().y /2.f - 90));
    player.setScale(sf::Vector2f(0.3f, 0.3f));

    //The light that sorrounds the player is created
    sf::CircleShape light(150.0f);
    light.setPosition(player.getPosition().x - 90.0f, player.getPosition().y - 80.0f);
    light.setFillColor(sf::Color(255,221,0,40));

    //The item to win the game is generated
    //First, the texture is loaded
    sf::Texture textureKey;
    if (!textureKey.loadFromFile("./images/key.png"))
    {
        printf("Error: loading texture\n");
    }
    textureKey.setSmooth(true);
    //Then the sprite is created and the texture is applied to it
    sf::Sprite key;
    key.setTexture(textureKey);
    //And it is scaled down
    key.setScale(sf::Vector2f(0.3f, 0.3f));
    //A random position for it is generated
    key.setPosition(generateRandomPosition(window));

    /*
        Start the game
    */

    //The start screen is printed
    startScreen(window);

    //Send message to the server
    sprintf(buffer,"The game is about to begin\n");
    sendString(connection_fd, buffer, BUFFER_SIZE);
 
    bzero(buffer, BUFFER_SIZE);

    //While the window is opened
    while(window.isOpen() && !interrupt_exit)
    {
        //Poll event for the window
        sf::Event event;
        while(window.pollEvent(event))
        {
            //If the user clicks the close button, the windows closes
            if (event.type == sf::Event::Closed)
                window.close();
        }

        //Check if the server send a message
        poll_response = poll(test_fds,1,0);
        if (poll_response == 1)
        {
            //Receive the message
            recvString(connection_fd,buffer,BUFFER_SIZE);
            
            //IF the code is 1, the sound is played
            if (strncmp(buffer,"1", BUFFER_SIZE)==0)
            {
                //A timer starts
                timeStart = clock();

                //The sound starts playing
                soundPlaying = true;

                //A random between 1 and 3 is created
                random = rand() % 3 + 1;

                //If the random is 1, the running sound effect is played
                if(random == 1)
                    sound.setBuffer(running);
                //If the random is 2, the door sound effect is played
                else if(random == 2)
                    sound.setBuffer(door);
                //If the random is 3, the breathing sound effect is played
                else
                    sound.setBuffer(breathing);

                sound.play();
            }

            //IF the code is -1, it means the server is about to disconnect
            if (strncmp(buffer,"-1", BUFFER_SIZE)==0)
            {
                printf("\nServer disconnected\n");
                break;
            } 
        }

        bzero(buffer, BUFFER_SIZE);

        //If the sound has stopped playing
        if(soundPlaying && sound.getStatus() == sf::SoundSource::Status::Stopped)
        {
            //The boolean becomes false
            soundPlaying = false;
        }
        //If the sound is still playing
        else if(soundPlaying)
        {
            //The program checks for how much time the sound has been playing
            timeElapsed = (clock()-timeStart)/(double)(CLOCKS_PER_SEC);
        }

        //If the player moves while the sound is playing, they die
        if(soundPlaying && timeElapsed>0.5){
            if(playerMoves())
            {
                //The program tells the server the user died
                sprintf(buffer,"0");
                sendString(connection_fd, buffer, BUFFER_SIZE);
                //The death screen is shown
                deathScreen(window);
                //The window closes
                window.close();
                //The cycle breaks
                break;
            }
        }
        //If the sound isn't playing, they can move freely
        else{
            //If the player moves left
            if( (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) && player.getPosition().x>0)
            {
                player.move(-0.5f, 0.0f);
                light.move(-0.5f, 0.0f);
            }
            //If the player moves right
            if((sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) && (player.getPosition().x + player.getGlobalBounds().width)<window.getSize().x)
            {
                player.move(0.5f, 0.0f);
                light.move(0.5f, 0.0f);
            }
            //If the player moves up
            if((sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) && player.getPosition().y>0)
            {
                player.move(0.0f, -0.5f);
                light.move(0.0f, -0.5f);
            }
            //If the player moves down
            if((sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) && (player.getPosition().y + player.getGlobalBounds().height)<window.getSize().y)
            {
                player.move(0.0f, 0.5f);
                light.move(0.0f, 0.5f);

                
            }
        }

        //The window clears itself with a black background
        window.clear(sf::Color::Black);

        //If the player touches the key, they win
        if(player.getGlobalBounds().contains(key.getPosition()) || player.getGlobalBounds().intersects(key.getGlobalBounds())){

            bzero(buffer, BUFFER_SIZE);

            //The program tells the server the user won
            sprintf(buffer,"1");
            sendString(connection_fd, buffer, BUFFER_SIZE);
            //The victory screen is loaded
            victoryScreen(window);
            //The window closes
            window.close();
            break;
        }

        //If the light touches the key, the key is drawn
        if(light.getGlobalBounds().intersects(key.getGlobalBounds()) || collides(key,light)){
            window.draw(key);
        }

        //Draw the player
        window.draw(player);

        //Draw the light
        window.draw(light);

        // Display the new buffer
        window.display();
    }

    bzero(buffer, BUFFER_SIZE);

    //The program tells the server the client disconnected
    sprintf(buffer,"-1");
    sendString(connection_fd, buffer, BUFFER_SIZE);
    
}

//Function for printing the start screen
void startScreen(sf::RenderWindow &window)
{
    //Poll event for the window
    sf::Event event;
    while(window.pollEvent(event))
    {
        //If the user clicks the close button, the windows closes
        if (event.type == sf::Event::Closed)
            window.close();
    }

    //The font for the deathscreen is loaded
    sf::Font font;
    if (!font.loadFromFile("./fonts/8bitOperatorPlus8-Regular.ttf"))
    {
        printf("Error: loading font\n");
    }

    //A text for the tilte is created and the font is applied to it
    sf::Text title;
    title.setFont(font);
    //the string "Dead End" is added to the text
    title.setString("Dead End");
    //The size of the text is set
    title.setCharacterSize(200);
    // set the color
    title.setFillColor(sf::Color::White);

    //A text for the instructions is created and the font is applied to it
    sf::Text instructions;
    instructions.setFont(font);
    instructions.setString("If you hear any sound, don't move.\nFind the key to escape.");
    //The size of the text is set
    instructions.setCharacterSize(100);
    // set the color
    instructions.setFillColor(sf::Color::White);
    instructions.setPosition(title.getPosition().x, title.getPosition().y + 250);

    //The text is printed for five seconds and then, the game starts
    window.clear(sf::Color::Black);
    window.draw(title);
    window.draw(instructions);
    window.display();

    sleep(4.5);
    
}

//Function for printing the death screen
void deathScreen(sf::RenderWindow &window)
{
    //Poll event for the window
    sf::Event event;
    while(window.pollEvent(event))
    {
        //If the user clicks the close button, the windows closes
        if (event.type == sf::Event::Closed)
            window.close();
    }

    //The font for the deathscreen is loaded
    sf::Font font;
    if (!font.loadFromFile("./fonts/8bitOperatorPlus8-Regular.ttf"))
    {
        printf("Error: loading font\n");
    }

    //A text is created and the font is applied to it
    sf::Text text;
    text.setFont(font);

    //the string "game over" is added to the text
    text.setString("Game Over");

    //The size of the text is set
    text.setCharacterSize(200);

    // set the color
    text.setFillColor(sf::Color::Red);

    //The text is printed for five seconds and then, the game finishes
    window.clear(sf::Color::Black);
    window.draw(text);
    window.display();

    sleep(4.5);
    
}

//Function for printing the victory screen
void victoryScreen(sf::RenderWindow &window)
{

    //Poll event for the window
    sf::Event event;
    while(window.pollEvent(event))
    {
        //If the user clicks the close button, the windows closes
        if (event.type == sf::Event::Closed)
            window.close();
    }

    //The font for the deathscreen is loaded
    sf::Font font;
    if (!font.loadFromFile("./fonts/8bitOperatorPlus8-Regular.ttf"))
    {
        printf("Error: loading font\n");
    }

    //A text is created and the font is applied to it
    sf::Text text;
    text.setFont(font);

    //the string "You Won!" is added to the text
    text.setString("You Won!");

    //The size of the text is set
    text.setCharacterSize(200);

    // set the color
    text.setFillColor(sf::Color::Green);

    //The text is printed for five seconds and then, the game finishes
    window.clear(sf::Color::Black);
    window.draw(text);
    window.display();

    sleep(4.5);
    
}

//Function that checks if a circle shape collides with a sprite
bool collides(sf::Sprite sprite, sf::CircleShape circle)
{
    int x = circle.getPosition().x;
    int y = circle.getPosition().y;
    int finalX = x + circle.getGlobalBounds().width;
    int finalY = y + circle.getGlobalBounds().height;

    //Check if the upper left side of the circle collide with any part of the sprite
    if(x>sprite.getPosition().x && x<sprite.getGlobalBounds().width && y>sprite.getPosition().y && y<sprite.getGlobalBounds().height)
        return true;
    //Check if the down right side of the circle collide with any part of the sprite
    else if(finalX>sprite.getPosition().x && finalX<sprite.getGlobalBounds().width && finalY>sprite.getPosition().y && finalY<sprite.getGlobalBounds().height)
        return true;
    else
        return false;
}

//Function for generating a random position to hide the key for winning the game
sf::Vector2f generateRandomPosition(sf::RenderWindow &window)
{
    sf::Vector2f vector;

    //The random is seeded
    srand(time(NULL));
    
    //Random values for x and y are generated
    vector.x = rand() % (window.getSize().x - 50) + 50;
    vector.y = rand() % (window.getSize().y - 50) + 50;

    return vector;
}

//Function that returns true if the player is moving
bool playerMoves(){

    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)
    || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)
    || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)
    || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
    {
        return true;
    }
    else
    {
        return false;
    }
    
}

/*
    The following two functions weren't used in the final version of the project
    They were used to randomly generate a maze.
*/

/*Function that creates a maze using the kruskall algorithm 
void createMaze(Node nodeList[], sf::RenderWindow &window)
{
    //The random is seeded
    srand(time(NULL));
    
    //The groups of the nodes are initialized
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; ++i)
        nodeList[i].group = i;

    //A vector for the walls is created
    std::vector<Wall> wallVec;
    //Cycles over the width and height of the maze
    for (int col = 0; col < GRID_WIDTH; ++col)
        for (int row = 0; row < GRID_HEIGHT; ++row)
        {
            //A new node is created, according to the current row and column
            Node* node1 = &(nodeList[col + row * GRID_WIDTH]);
            int check_side[2] = {SIDE_RIGHT, SIDE_DOWN};

            //The next rows and columns of the node are saved
            for (int i = 0; i < 2; ++i)
            {
                //The next column and row are checked
                int next_col = nextCol(col, check_side[i]);
                int next_row = nextRow(row, check_side[i]);

                //If the next row and column are within the maze area
                if (indexIsValid(next_col, next_row))
                {
                    //A wall is created
                    Wall wall;
                    //The current node and next node are saved in the wall
                    wall.node1 = node1;
                    wall.node2 = &(nodeList[next_col + next_row * GRID_WIDTH]);
                    //The wall is added to the vector of walls
                    wallVec.push_back(wall);
                }
            }
        }
    
    //If the vector of walls isn't empty, create that wall and then erase it from the vector
    while (!wallVec.empty())
    {
        int rndWall = rand() % wallVec.size();
        Wall* wall = &(wallVec.at(rndWall));
        Node* node1 = wall->node1;
        Node* node2 = wall->node2;
        if (node1->group != node2->group)
        {
            joinNodes(nodeList, node1, node2);
            mergeGroup(nodeList, node1->group, node2->group);
        }

        wallVec.erase(wallVec.begin() + rndWall);
    }
}

//Function for merging two groups of nodes
void mergeGroup(Node nodeList[], int group1, int group2)
{
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; ++i)
    {
        if (nodeList[i].group == group2)
            nodeList[i].group = group1;
    }
}*/