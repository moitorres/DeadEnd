/*
    Client program for the game "Dead End"

    Moises Uriel Torres A01021323
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <time.h>
//SFML libraries
#include <SFML/Graphics.hpp>
// Sockets libraries
#include <netdb.h>
#include <arpa/inet.h>
// Custom libraries
#include "sockets.h"
#include "fatal_error.h"
#include "mazeHelper.h"

#define BUFFER_SIZE 1024

///// FUNCTION DECLARATIONS
void usage(char * program);
void startGame(int connection_fd);
void createMaze(Node nodeList[], sf::RenderWindow &window);
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
    int status;

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

        //Movement of the player
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

        //The windows clears itself with a black background
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

//Function for merging two groups of nodes
void mergeGroup(Node nodeList[], int group1, int group2)
{
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; ++i)
    {
        if (nodeList[i].group == group2)
            nodeList[i].group = group1;
    }
}