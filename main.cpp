#include <iostream>
#include <pthread.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include <fstream>

struct Player {
    int hand;
};
std::queue<int> deck;
Player players[3];
bool roundOver = false;
int roundCounter = 0;
int activePlayer;
pthread_cond_t con[3];
pthread_cond_t dealerCon;
pthread_mutex_t dealerMutex;
pthread_mutex_t playerMutex;
int escapePlan = 0;
int sigCounter = 0;
int escapeCounter=0;
std::ofstream fout;
const int SEED = 20; // change this to test various seeds
/*
void checkDeck() {
    std::queue<int> deckChecker;
    deckChecker = deck;
    fout << "Deck:";
    while (deckChecker.size() > 0) {
        fout << " " << deckChecker.front() << " ";
        deckChecker.pop();
    }
    fout << std::endl;

}
*/
void *dealerActions(void *) {
    do {
        pthread_mutex_lock(&dealerMutex); // locks the mutex for the dealer
        fout << "DEALER: shuffle" << std::endl;
        roundOver = false;
        std::queue<int> empty;
        deck.swap(empty); // clears the deck
        int arr[52];
        int temp;
        int j = 0;
        // initializes an array with all the cards unsorted
        for (int i = 0; i < 13; i++) {
            arr[j] = i;
            j++;
            arr[j] = i;
            j++;
            arr[j] = i;
            j++;
            arr[j] = i;
            j++;
        }
        // shuffles card in array
        for (int i = 0; i < 52; i++) {
            // Random for remaining positions.
            int r = rand() % 52;
            temp = arr[i];
            arr[i] = arr[r];
            arr[r] = temp;
        }
        //puts all the cards in the deck
        for (int j = 0; j < 52; j++) {
            deck.push(arr[j]);
        }
        // deals to each player
        players[0].hand = deck.front();
        deck.pop();
        fout << "PLAYER 1: hand " << players[0].hand << std::endl;
        players[1].hand = deck.front();
        deck.pop();
        fout << "PLAYER 2: hand " << players[1].hand << std::endl;
        players[2].hand = deck.front();
        deck.pop();
        fout << "PLAYER 3: hand " << players[2].hand << std::endl;
        activePlayer = roundCounter;
        roundCounter++;
        sigCounter = 0;
        pthread_cond_signal(&con[activePlayer]);

        //pthread_mutex_unlock(&dealerMutex);
        pthread_cond_wait(&dealerCon, &dealerMutex);
        pthread_mutex_unlock(&dealerMutex);
        // used to escape from the loop so that the threads join in main
        if (escapePlan == 3) {
            break;
        }
    } while (roundCounter < 4);
    return NULL;

}



void *playerActions(void *currentPlayerIndex) {
    //pthread_mutex_lock(&playerMutex);
    while (roundCounter < 4) {
        pthread_mutex_lock(&playerMutex);
        pthread_cond_wait(&con[((long)currentPlayerIndex)], &playerMutex);
        //tells the player threads when to stop
        if(escapePlan == 3)
        {
            if(escapeCounter == 2)
            {
                // tells the dealer to stop
                pthread_mutex_unlock(&playerMutex);
                pthread_cond_signal(&dealerCon);
                break;
            }
            // tells the remaining players to stop
            escapeCounter++;
            fout << "PLAYER " << ((long)currentPlayerIndex) + 1 << ": round completed" << std::endl;
            pthread_cond_signal(&con[(((long)currentPlayerIndex) + 1) % 3]);
            pthread_mutex_unlock(&playerMutex);
            break;
        }
        if (roundOver) {
            fout << "PLAYER " << ((long)currentPlayerIndex) + 1 << ": round completed" << std::endl;
            sigCounter++;
            // makes sure that the players don't signal eachother forever when the round ends
            if (sigCounter == 2) {
                pthread_cond_signal(&dealerCon);
                pthread_mutex_unlock(&playerMutex);

            }
             else {
                pthread_cond_signal(&con[(((long)currentPlayerIndex) + 1) % 3]);
                pthread_mutex_unlock(&playerMutex);
            }
             // if the round isn't over, play more
        } else {
            int temp = deck.front(); // the 2nd card in the hand
            // outputs the hand
            std::cout << "PLAYER " << ((long)currentPlayerIndex) + 1 << ": HAND " << players[((long)currentPlayerIndex)].hand  <<" " << temp<< std::endl;
            // removes the card that is just drawn from the deck
            deck.pop();
            bool foundMatch = false;
            int randCard = rand() % 2; // used to tell which card to discard
            fout << "PLAYER " << ((long)currentPlayerIndex) + 1 << ": hand " << players[((long)currentPlayerIndex)].hand << std::endl;
            fout << "PLAYER " << ((long)currentPlayerIndex) + 1 << ": draws " << temp << std::endl;
            if (players[((long)currentPlayerIndex)].hand == temp) {
                // the cards in the hand match
                foundMatch = true;
                std::cout << "WIN: YES" <<std::endl;
                escapePlan++; // gets ready for the final escape
            }
            if (!foundMatch) {
                std::cout << "WIN: NO"<<std::endl;
                // discards either the first card or the second card in the hand
                if (randCard == 0) {
                    deck.push(players[((long)currentPlayerIndex)].hand);
                    fout << "PLAYER " << ((long)currentPlayerIndex) + 1 << ": discards "
                              << players[((long)currentPlayerIndex)].hand
                              << std::endl;
                    players[((long)currentPlayerIndex)].hand = temp;

                } else {
                    deck.push(temp);
                    fout << "PLAYER " << ((long)currentPlayerIndex) + 1 << ": discards " << temp << std::endl;
                }
                activePlayer = (activePlayer + 1) % 3;
                // outputs the deck:
                std::queue<int> deckChecker;
                deckChecker = deck;
                fout << "Deck:";
                while (deckChecker.size() > 0) {
                    fout << " " << deckChecker.front() << " ";
                    deckChecker.pop();
                }
                fout << std::endl;
            } else {
                fout << "PLAYER " << ((long)currentPlayerIndex) + 1 << ": wins" << std::endl;
                fout << "PLAYER " << ((long)currentPlayerIndex) + 1 << ": round completed" << std::endl;
                roundOver = true;
            }
            // signals the next player's turn, and unlocks the mutex to wait for it's next turn
            pthread_cond_signal(&con[(((long)currentPlayerIndex) + 1) % 3]);
            pthread_mutex_unlock(&playerMutex);
        }
    }
    return NULL;
}

int main() //int argc, char *argv[]
{
    // getting the thread from the command prompt
    //int seed = atoi(argv[1]);
    // threads for players
    pthread_t threads[3];
    // thread for dealer
    pthread_t dealer;
    // intitializing the condition variables
    pthread_cond_init(&con[0], NULL);
    pthread_cond_init(&con[1], NULL);
    pthread_cond_init(&con[2], NULL);
    pthread_cond_init(&dealerCon, NULL);
    //initializing the mutex for the dealer
    pthread_mutex_init(&dealerMutex, NULL);
    pthread_mutex_init(&playerMutex, NULL);
    fout.open("log.txt");
    srand(SEED);
    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, playerActions, (void *) i); // creating the player threads
    }
    pthread_create(&dealer, NULL, dealerActions, NULL); // creating the dealer thread

    // joining the player threads
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }
    // joining the dealer thread
    pthread_join(dealer, NULL);
    fout.close();
    /* Clean up and exit */
    pthread_mutex_destroy(&playerMutex);
    pthread_mutex_destroy(&dealerMutex);
    pthread_cond_destroy(&dealerCon);
    pthread_cond_destroy(&con[0]);
    pthread_cond_destroy(&con[1]);
    pthread_cond_destroy(&con[2]);
    pthread_exit(NULL);
    return 0;
}