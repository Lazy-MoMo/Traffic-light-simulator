
#include <stdio.h>
#include <stdlib.h>

#define MAX 100

typedef struct {
  int items[MAX];
  int front;
  int rear;
} Queue;

void initializeQueue(Queue *q) {
  q->front = -1;
  q->rear = -1;
}

int isFull(Queue *q) { return q->rear == MAX - 1; }

int isEmpty(Queue *q) { return q->front == -1 || q->front > q->rear; }

void enqueue(Queue *q, int value) {
  if (isFull(q)) {
    printf("Queue is full\n");
    return;
  }
  if (isEmpty(q)) {
    q->front = 0;
  }
  q->rear++;
  q->items[q->rear] = value;
  printf("Inserted %d\n", value);
}

int dequeue(Queue *q) {
  if (isEmpty(q)) {
    printf("Queue is empty\n");
    return -1;
  }
  int item = q->items[q->front];
  q->front++;
  if (q->front > q->rear) {
    q->front = q->rear = -1;
  }
  return item;
}

void displayQueue(Queue *q) {
  if (isEmpty(q)) {
    printf("Queue is empty\n");
    return;
  }
  for (int i = q->front; i <= q->rear; i++) {
    printf("%d ", q->items[i]);
  }
  printf("\n");
}
