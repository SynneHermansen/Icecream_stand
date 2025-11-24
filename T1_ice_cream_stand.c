
#include <pthread.h>
#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h> 
#include <stdbool.h> 
#include <time.h>   
#include <semaphore.h> 

// --- Globale konstanter ---
#define NUM_CUSTOMERS 5
#define NUM_WORKERS 2
#define QUEUE_SIZE 3
#define SERVES_BEFORE_BREAK 3
#define WORKER_REST_TIME 5
#define MAX_RANDOM_DURATION 9

int queue[QUEUE_SIZE];
int head = 0;   
int tail = 0;   
int count = 0;  

// Semifor
sem_t queue_lock; 
sem_t customer_waiting_sem; 
sem_t customer_ready_sem[NUM_CUSTOMERS]; 


// --- Funksjon Prototyper ---
void *worker_actions(void *employee_id);
void *customer_actions(void *personal_id);

/**
 * @brief Initialiserer alle semaforer
 */
void init_sync_primitives() {
    sem_init(&queue_lock, 0, 1); 
    sem_init(&customer_waiting_sem, 0, 0); 
    
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        sem_init(&customer_ready_sem[i], 0, 0);
    }
    
    srand(time(NULL));
}

/**
 * @brief Legger til en kunde-ID i køen.
 */
void enqueue(int customer_id) {
    queue[tail] = customer_id;
    tail = (tail + 1) % QUEUE_SIZE;
    count++;
}

/**
 * @brief Fjerner og returnerer neste kunde-ID fra køen (FIFO).
 */
int dequeue() {
    int customer_id = queue[head];
    head = (head + 1) % QUEUE_SIZE;
    count--;
    return customer_id;
}


int main(int argc, char** argv)
{
    init_sync_primitives();

    int personal_ids[NUM_CUSTOMERS];
	int employee_ids[NUM_WORKERS];
	
	pthread_t customers[NUM_CUSTOMERS];
	pthread_t workers[NUM_WORKERS];

    printf("Iskrembod-simulering Starter\n");
    printf("Arbeidere: %d, Personer: %d, Køstørrelse: %d\n", NUM_WORKERS, NUM_CUSTOMERS, QUEUE_SIZE);
    printf("Pause-policy: Hver %d servering, hvil i %d sekunder.\n\n", SERVES_BEFORE_BREAK, WORKER_REST_TIME);

    // lag arbeider-tråder
    for(int i=0; i<NUM_WORKERS; i++)
	{
        employee_ids[i] = i;
        pthread_create(&workers[i], NULL, worker_actions, (void *)&employee_ids[i]);
    }
    
    // lag kunde-tråder
    for(int i=0; i<NUM_CUSTOMERS; i++)
	{
        personal_ids[i] = i;
        pthread_create(&customers[i], NULL, customer_actions, (void *)&personal_ids[i]);
    }

    // Holder main-tråden aktiv
    sleep(60); 
    printf("\nSimulering avsluttet etter 60 sekunder.\n");
    
    // Rydder opp
    for (int i = 0; i < NUM_WORKERS; i++) pthread_cancel(workers[i]);
    for (int i = 0; i < NUM_CUSTOMERS; i++) pthread_cancel(customers[i]);

    sem_destroy(&queue_lock);
    sem_destroy(&customer_waiting_sem);
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        sem_destroy(&customer_ready_sem[i]);
    }

    return 0;
}


/**
 * @brief Arbeider-trådfunksjon.
 */
void* worker_actions(void* employee_id)
{
    int id = *(int*)employee_id;
    int serves_count = 0;
    int customer_id;

    while(true)
	{
        // --- 1. Sjekk for Obligatorisk Pause-policy ---
        if (serves_count >= SERVES_BEFORE_BREAK) {
            printf("Worker %d betjente %d kunder. **Starter UFORSTYRRELIG hvile i %d sek.**\n", 
                   id, SERVES_BEFORE_BREAK, WORKER_REST_TIME);
            sleep(WORKER_REST_TIME);
            printf("Worker %d **ferdig med hvilen.** Klar til å betjene igjen:)\n", id);
            serves_count = 0; 
        }

        // --- 2. LUR/Vent på Kunde ---
        printf("Worker %d **sover** (venter på kunde).\n", id);
        sem_wait(&customer_waiting_sem); // Blokkerer hvis ingen kunder venter
        printf("Worker %d **vekket.** Sjekker køen.\n", id);

        // --- 3. Betjen Kunde ---
        sem_wait(&queue_lock); 
        
        customer_id = dequeue();
        
        printf("Worker %d tok Person %d fra køen. **%d venter**.\n", 
               id, customer_id, count);
        
        sem_post(&queue_lock); 

        // --- 4. Forbered Iskrem (Simulert arbeid: 0 til 9 sek) ---
        int serve_time = rand() % 10; 
        printf("Worker %d forbereder is til Person %d (tar %d sek).\n", 
               id, customer_id, serve_time);
        sleep(serve_time); 
        
        // --- 5. Overlever Is og Signaliser Kunde ---
        printf("Worker %d **ferdig** med is til Person %d.\n", id, customer_id);
        
        // Vekk den spesifikke kunden
        sem_post(&customer_ready_sem[customer_id]);
        
        serves_count++; 
    }
    return NULL;
}

/**
 * @brief Kunde-trådfunksjon.
 */
void* customer_actions(void* personal_id)
{
    int id = *(int*)personal_id;

    while(true)
	{
        // --- 1. Avslapningsfase (0 til 9 sek) ---
        int relax_time = rand() % (MAX_RANDOM_DURATION + 1); 
        printf("Person %d **slapper av** i skyggen (i %d sek).\n", id, relax_time);
        sleep(relax_time);

        printf("Person %d vil ha is.\n", id);

        // --- 2. Forsøk på å bli med i køen ---
        sem_wait(&queue_lock); 
        
        if (count < QUEUE_SIZE) {
            // Køen har plass
            enqueue(id);
            printf("Person %d **stiller seg i kø.** %d venter.\n", id, count);
            
            sem_post(&customer_waiting_sem); // Vekk en arbeider

            sem_post(&queue_lock); 
            
            // --- 3. Vent på Betjening ---
            sem_wait(&customer_ready_sem[id]); // Blokker til isen er klar
            
            printf("Person %d **mottok is!** (Nam!)\n", id);

        } else {
            // Køen er full: returner til avslapning
            printf("Person %d ser køen er full (%d/%d). **Returnerer til avslapning.**\n", 
                   id, count, QUEUE_SIZE);
            sem_post(&queue_lock); 
        }
    }
    return NULL;
}