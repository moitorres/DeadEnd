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
#include "mazeHelper.h"

using namespace std;

#define BUFFER_SIZE 1024

///// FUNCTION DECLARATIONS
void usage(char * program);
void startGame(int connection_fd);
void createMaze(Node nodeList[], sf::RenderWindow &window);
void deathScreen(sf::RenderWindow &window);
bool playerMoves();
void mergeGroup(Node nodeList[], int group1, int group2);

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
    bool soundPlaying = false;

    //A sound buffer is created
    sf::SoundBuffer footsteps;
    //A file is loaded into the buffer
    if(!footsteps.loadFromFile("./sounds/Airhorn.wav"))
    {
        printf("Error: loading sound\n");
    }
    //A sound is created
    sf::Sound sound;
    sound.setBuffer(footsteps);

    //Create a structure to hold the file descriptors to poll
    struct pollfd test_fds[1];
    test_fds[0].fd = connection_fd;
    test_fds[0].events = POLLIN;

    //A list for the nodes in the maze is created, according to the width and height defined
    Node nodeList[GRID_WIDTH * GRID_HEIGHT];

    //The window for the game is created
    sf::RenderWindow window(sf::VideoMode(GRID_WIDTH * NODE_SIZE, GRID_HEIGHT * NODE_SIZE), "Dead End");

    //The player is created
    sf::RectangleShape player(sf::Vector2f(15.0f, 15.0f));
    player.setPosition(10.0f,10.0f);
    player.setFillColor(sf::Color::Red);

    //The maze is created
    createMaze(nodeList, window);

    //Send message to the server
    sprintf(buffer,"The game is about to begin\n");
    sendString(connection_fd, buffer, BUFFER_SIZE);

    bzero(buffer, BUFFER_SIZE);

    //While the window is opened
    while(window.isOpen())
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
        //If the server send a message, it means the client must play a sound
        if (poll_response == 1)
        {
            printf("Message recived from the server\n");
            //Receive the sound to play
            recvString(connection_fd,buffer,BUFFER_SIZE);
            
            //IF the code is 1, the sound is played
            if (strncmp(buffer,"1", BUFFER_SIZE)==0)
            {
                //A timer starts
                timeStart = clock();
                //The sound starts playing
                soundPlaying = true;
                sound.play();
            }
            
        }
        //If there is a connection error
        else if(poll_response == -1)
        {
            printf("\nServer exit\n");
            break;
        }

        bzero(buffer, BUFFER_SIZE);

        //If the sound has stopped playing
        if(soundPlaying && sound.getStatus() == sf::SoundSource::Status::Stopped)
        {
            //The boolean becomes false
            soundPlaying = false;
            //The program tells the server the user survived
            sprintf(buffer,"1");
            sendString(connection_fd, buffer, BUFFER_SIZE);
        }

        //The program checks for how much time the sound has been playing
        timeElapsed = (clock()-timeStart)/(double)(CLOCKS_PER_SEC);

        //If the player moves while the sound is playing, they die
        if(soundPlaying && timeElapsed>1 && timeElapsed<5){
            if(playerMoves())
            {
                //The program tells the server the user died
                sprintf(buffer,"-1");
                sendString(connection_fd, buffer, BUFFER_SIZE);
                //The death screen is shown
                deathScreen(window);
                //The cycle breaks
                break;
            }
        }
        //If the sound isn't playing, they can move freely
        else{
            //If the player moves left
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            {
                player.move(-1.3f, 0.0f);
            }
            //If the player moves right
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            {
                player.move(1.3f, 0.0f);
            }
            //If the player moves up
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            {
                player.move(0.0f, -1.3f);
            }
            //If the player moves down
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            {
                player.move(0.0f, 1.3f);
            }
        }

        //The window clears itself with a black background
        window.clear(sf::Color::Black);

        //Draw the maze 
        drawMaze(window, nodeList);

        //Draw the player
        window.draw(player);

        // Display the new buffer
        window.display();
    }

}

//Function that creates a maze using the kruskall algorithm 
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

//Function for printing the death screen
void deathScreen(sf::RenderWindow &window)
{
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

    //The text is printed for ten seconds and then, the game finishes
    window.clear(sf::Color::Black);
    window.draw(text);
    window.display();

    sleep(10);
    
}

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

//Function for merging two groups of nodes
void mergeGroup(Node nodeList[], int group1, int group2)
{
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; ++i)
    {
        if (nodeList[i].group == group2)
            nodeList[i].group = group1;
    }
}