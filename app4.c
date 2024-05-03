/*
*   James Hofer
*   1000199225
*   Compile: /usr/bin/c99 -o app4 app4.c
*
*   Usage1: ./app4 (for typing or pasting input)
*   Usage2: ./app4 < input.dat (for input redirection)
*
*   This program reads in user commands and messages (positive lengths).
*   The data is stored as a FIFO queue using two LIFO stacks (linked lists).
*   The user can find average message length, minimum message length, and
*   maximum message length efficiently.
*
*   https://ranger.uta.edu/~weems/NOTES3318/LAB/LAB4FALL23/lab4fall23.pdf
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define IN_DUMMY -1


// GLOBALS
struct Node {
    int msg;
    int min;
    int max;
    struct Node *next;
};
typedef struct Node node_t;

node_t* inStack = NULL;
node_t* outStack = NULL;
node_t* inRecycle = NULL;
node_t* outRecycle = NULL;

int inMin = 999999999;
int inMax = -1;
int sum = 0;


/**
 * @brief pop top from Stack (linked list)
 * 
 * @param top       //pointer to top of stack
 * @return node_t*  //returns popped node pointer
 */
node_t* pop(node_t** top) {
    
    if(!*top) {
        printf("Error: Unable to remove node - memory underflow\n");
        exit(EXIT_FAILURE);
    }
    node_t* popped = *top;    //popped points to old top
    *top = (*top)->next;      //point top pointer to next node
    return popped;
}


/**
 * @brief push new top to Stack (linked list)
 * 
 * @param top       //old top of stack
 * @param msg       //in or out message
 * @param min       //out min msg len
 * @param max       //out max msg len
 * @param recycle   //pointer to top of recycle stack
 * @return node_t*  //returns new top pointer
 */
node_t* push(node_t* top, int msg, int min, int max, node_t** recycle) {
    
    node_t* node;

    // recycle node if available
    if(recycle && *recycle) {
        node = *recycle;
        *recycle = (*recycle)->next;
    }
    else {
        node = (node_t*) malloc(sizeof(node_t));
        if(!node) {
            printf("Error: Unable to add new node - memory overflow\n");
            exit(EXIT_FAILURE);
        }
    }

    node->msg = msg;
    node->min = min;
    node->max = max;
    node->next = top;   //point to old top (NULL if first node)
    return node;        //return new top pointer
}


/**
 * @brief get size of Stack (linked list)
 * 
 * @param top   //top of stack
 * @return int  //returns size of stack
 */
int size(node_t* top) {
    
    int count = 0;
    while(top) {
        count++;
        top = top->next;
    }
    return count;
}


/**
 * @brief print Stack (linked list)
 *        ***DEBUG USE ONLY***
 * 
 * @param top   //top of stack
 */
void printStack(node_t* top, char* name) {
    printf("\t%s: ", name);
    if(!top) {
        printf("Stack is empty\n");
        return;
    }
    while(top) {
        printf("%d ", top->msg);
        top = top->next;
    }
    printf("\n");
}


/**
 * @brief free Stack (linked list)
 * 
 * @param top   //pointer to top of stack
 */
void freeStack(node_t** top) {
    while(*top) {
        node_t* temp = (node_t*) pop(top);
        free(temp);
    }
}


/**
 * @brief Main - user input loop (command, message)
 * 
 * @return int 
*/
int main() {
    
    int cmd, msg;
    
    // check if redirected input
    if(isatty(0))
        printf("Type input by line or Paste entire input.\n");

    while(1) {
        scanf("%d", &cmd);
        switch(cmd) {
            case 0: //exit
                printf("0\n");
                printf("In Stack available nodes %d Out Stack available nodes %d\n",
                        size(inRecycle), size(outRecycle));
                freeStack(&inStack);
                freeStack(&outStack);
                freeStack(&inRecycle);
                freeStack(&outRecycle);
                exit(EXIT_SUCCESS);

            case 1: //enqueue
                scanf("%d", &msg);

                //update inMin, inMax, sum
                if(msg > inMax)
                    inMax = msg;
                if(msg < inMin)
                    inMin = msg;
                sum += msg;

                //move to inStack
                inStack = push(inStack, msg, IN_DUMMY, IN_DUMMY, &inRecycle);
                printf("Enqueued %d\n", inStack->msg);
                break;

            case 2:; //dequeue
                node_t* popped;

                //if outStack empty, move inStack to outStack
                if(!outStack) {
                    if(!inStack) {
                        printf("Can't dequeue from an empty queue\n");
                        break;
                    }
                    
                    //first node to outStack
                    popped = pop(&inStack);
                    inRecycle = push(inRecycle, popped->msg, IN_DUMMY, IN_DUMMY, NULL);
                    popped->min = popped->max = popped->msg;
                    outStack = push(outStack, popped->msg, popped->min, popped->max, &outRecycle);
                    
                    //remaining nodes to outStack
                    while(inStack) {
                        popped = pop(&inStack);
                        inRecycle = push(inRecycle, popped->msg, IN_DUMMY, IN_DUMMY, NULL);
                        if(popped->msg > outStack->max)
                            popped->max = popped->msg;
                        else
                            popped->max = outStack->max;
                        if(popped->msg < outStack->min)
                            popped->min = popped->msg;
                        else
                            popped->min = outStack->min;
                        outStack = push(outStack, popped->msg, popped->min, popped->max, &outRecycle);
                    }

                    //reset inMin and inMax
                    inMax = -1;
                    inMin = 999999999;
                }

                //if outStack not empty, pop from outStack
                if(outStack) {
                    popped = pop(&outStack);
                    sum -= popped->msg;
                    printf("Dequeued %d\n", popped->msg);
                    outRecycle = push(outRecycle, popped->msg, popped->min, popped->max, NULL);
                }
                
                /*
                //Debug
                printStack(inStack, "inStack");
                printStack(inRecycle, "inRecycle");
                printStack(outStack, "outStack");
                printStack(outRecycle, "outRecycle");
                */
                break;

            case 3: //avg len
                if(!inStack && !outStack) {
                    printf("Can't compute average for an empty queue\n");
                    break;
                }
                printf("average length %.6f\n", (float) sum / (size(inStack) + size(outStack)));
                break;

            case 4:; //min len
                int min = inMin;
                if(!inStack && !outStack) {
                    printf("Can't compute minimum for an empty queue\n");
                    break;
                }
                if(outStack) {
                    if(outStack->min < inMin)
                        min = outStack->min;
                }
                printf("minimum length %d\n", min);
                break;

            case 5:; //max len
                int max = inMax;
                if(!inStack && !outStack) {
                    printf("Can't compute maximum for an empty queue\n");
                    break;
                }
                if(outStack) {
                    if(outStack->max > inMax)
                        max = outStack->max;
                }
                printf("maximum length %d\n", max);
                break;

            default:
                printf("Error: No matching command\n");
                exit(EXIT_FAILURE);
        }
    }
}