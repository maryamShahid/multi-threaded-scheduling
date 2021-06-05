#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#define TRACE 1
#define MAXFILENAME 128
#define TMAX 10

int Wcount;
int Bcount;
int vrunTime[TMAX];
char filename[MAXFILENAME];

#define FCFS 1
#define SJF 2
#define PRIO 3
#define VRUNTIME 4

struct rq_data {
    struct rq_data *next;
    int threadIndex;
    int burstIndex;
    int burstLength;
    int burstTime;
};

struct rq_queue {
    struct rq_data *head;
    struct rq_data *tail;
    int count;
};

struct run_queue *runQueue;

int minB;
int avgB;
int minA;
int avgA;
int algo;
int expoDistribution;
long startTime;
long endTime;

void run_queue_init(struct rq_queue *q) {
    q->count = 0;
    q->head = NULL;
    q->tail = NULL;
}

void run_queue_insert(struct rq_queue *q, struct rq_data *qe) {
    if (q->count == 0) {
        q->head = qe;
        q->tail = qe;
    } else {
        q->tail->next = qe;
        q->tail = qe;
    }
    q->count++;
}

struct rq_data * run_queue_retrieve(struct rq_queue *q) {
    struct rq_data *qe;

    if (q->count == 0)
        return NULL;

    qe = q->head;
    q->head = q->head->next;
    q->count--;

    return (qe);
}

struct rq_data *queue_retrieve_smallest(struct rq_queue *q) {
    struct rq_data *min = q->head;
    struct rq_data *temp = q->head;
    struct rq_data *prev = NULL;

    if (q->count == 0)
        return NULL;

    while (temp != NULL) {
        if (temp->next != NULL && temp->next->burstLength < min->burstLength) {
            min = temp->next;
            prev = temp;
        }
        temp = temp->next;
    }
    if (min != q->head)
        prev->next = min->next;
    else
        q->head = q->head->next;

    q->count--;
    return (min);
}

struct rq_data *queue_retrieve_prio(struct rq_queue *q) {
    struct rq_data *prio = q->head;
    struct rq_data *temp = q->head;
    struct rq_data *prev = NULL;

    if (q->count == 0)
        return NULL;

    while (temp != NULL) {
        if (temp->next != NULL && temp->next->threadIndex < prio->threadIndex) {
            prio = temp->next;
            prev = temp;
        }
        temp = temp->next;
    }
    if (prio != q->head)
        prev->next = prio->next;
    else
        q->head = q->head->next;

    q->count--;
    return (prio);
}

struct rq_data *queue_retrieve_vruntime(struct rq_queue *q) {
    struct rq_data *min = q->head;
    struct rq_data *temp = q->head;
    struct rq_data *prev = NULL;
    int vruntime = vrunTime[temp->threadIndex];

    if (q->count == 0)
        return NULL;

    while (temp != NULL) {
        if (temp->next != NULL && (vrunTime[temp->next->threadIndex] < vrunTime[min->threadIndex])) {
            vruntime = vrunTime[temp->next->threadIndex];
            min = temp->next;
            prev = temp;
        }
        temp = temp->next;
    }
    if (min != q->head)
        prev->next = min->next;
    else
        q->head = q->head->next;

    q->count--;
    printf ("vruntime = %d of thread %d\n", vruntime, min->threadIndex);
    return (min);
}

struct run_queue {
    struct rq_queue *q;
    pthread_mutex_t mutex;
    pthread_cond_t  cond_W;
    pthread_cond_t  cond_S;
};

void rq_add(struct  run_queue* rq, struct rq_data *node){

    struct timeval current_timeval;
    gettimeofday(&current_timeval, NULL);
    startTime = current_timeval.tv_sec * (long) 1000 + (long) (current_timeval.tv_usec / 1000);

    pthread_mutex_lock(&rq->mutex);
    // critical section begin
    while (rq->q->count == (Wcount*Bcount))
        pthread_cond_wait(&rq->cond_W, &rq->mutex);

    run_queue_insert(rq->q, node);

    if (TRACE) {
        printf ("producer inserted thread = %d\n", node->threadIndex);
        printf ("\tburst index = %d\n", node->burstIndex);
        printf ("\tburst length = %d\n", node->burstLength);
        printf ("\tburst time = %d\n", node->burstTime);
        fflush (stdout);
    }

    if (rq->q->count == 1)
        pthread_cond_signal(&rq->cond_S);

    // critical section end
    pthread_mutex_unlock(&rq->mutex);
}


struct rq_data *rq_rem (struct run_queue *rq) {

    struct timeval current_timeval;
    gettimeofday(&current_timeval, NULL);
    endTime = current_timeval.tv_sec * (long) 1000 + (long) (current_timeval.tv_usec / 1000);

    long int waitTime = endTime - startTime;
    struct rq_data *qe;

    pthread_mutex_lock(&rq->mutex);

    // critical section begin
    while (rq->q->count == 0) {
        pthread_cond_wait(&rq->cond_S, &rq->mutex);
    }

    if (algo == FCFS) {
        qe = run_queue_retrieve(rq->q);
    }
    else if (algo == SJF) {
        qe = queue_retrieve_smallest(rq->q);
    }
    else if (algo == PRIO) {
        qe = queue_retrieve_prio(rq->q);
    }
    else if (algo == VRUNTIME) {
        qe = queue_retrieve_vruntime(rq->q);
    }
    else {
        printf("Invalid algorithm\n");
        exit(-1);
    }

    if (qe == NULL) {
        printf("can not retrieve; should not happen\n");
        exit(1);
    }
    usleep(1000 * qe->burstLength);

    if (TRACE) {
        printf ("consumer retrieved thread = %d\n", qe->threadIndex);
        printf ("\tburst index = %d\n", qe->burstIndex);
        printf ("\tburst length = %d\n", qe->burstLength);
        printf ("\tburst time = %d\n", qe->burstTime);
        printf ("\twait time = %ld\n", waitTime);
        fflush (stdout);
    }

    if (rq->q->count == ((Wcount*Bcount) - 1))
        pthread_cond_signal(&rq->cond_W);

    // critical section end
    pthread_mutex_unlock(&rq->mutex);
    return (qe);
}

int get_exponential_random_value(int lambda, int lower_bound) {
    int value;
    do {
        double temp = rand() / (RAND_MAX + 1.0);
        value = (int) (-log(1 - temp) * lambda);
    } while (value < lower_bound);
    return value;
}

void *W_thread(void * arg) {
    struct rq_data *node;

    long int index;
    index = (long int) arg;

    if (expoDistribution) {
        srand(time(0));

        for (int i = 0; i < Bcount; i++) {
            int burstLength = get_exponential_random_value(avgB, minB);
            int burstTime = get_exponential_random_value(avgA, minA);

            node = (struct rq_data *) malloc(sizeof(struct rq_data));
            if (node == NULL) {
                perror("malloc failed\n");
                exit(1);
            }
            node->next = NULL;
            node->threadIndex = index + 1;
            node->burstIndex = i + 1;
            node->burstLength = burstLength;
            node->burstTime = burstTime;

            //usleep(node->burstTime*1000);
            rq_add(runQueue, node);
        }
        printf("producer terminating\n");
        pthread_exit(NULL);
    }
    else {
        FILE *fp;
        int i = 0;
        int burstLength, burstTime;

        char inputfilename[MAXFILENAME];
        sprintf(inputfilename, "./%s-%ld.txt", filename, index);
        fp = fopen(inputfilename, "rb");

        while (fscanf(fp, "%d %d", &burstLength, &burstTime) == 2) {
            node = (struct rq_data *) malloc (sizeof (struct rq_data));
            if (node == NULL) {
                perror ("malloc failed\n");
                exit (1);
            }
            node->next = NULL;
            node->threadIndex = index + 1;
            node->burstIndex = i + 1;
            node->burstLength = burstLength;
            node->burstTime = burstTime;

            usleep(node->burstTime*1000);
            rq_add(runQueue, node);
            i++;
        }
        fclose (fp);
        printf ("producer terminating\n");  fflush (stdout);
        pthread_exit (NULL);
    }
}

void *S_thread () {
    struct rq_data *node;
    int n = (Bcount*Wcount);

    while (1) {
        for (int i = n; i > 0; i--) {
            node = rq_rem (runQueue);
            vrunTime[node->threadIndex] += (node->burstLength) * (0.7 + 0.3 * node->threadIndex);
            free (node);
            //sleep(node->burstLength*1000);
        }
        break;
    }
    printf ("consumer terminating\n");
    fflush (stdout);
    pthread_exit (NULL);
}

void getValues(int argc, char **argv) {
    int readFile = -1;

    for (int i = 0; i < argc; i++) {
        if (strcmp("-f", argv[i]) == 0) {
            readFile = i;
        }
        if (strcmp("FCFS", argv[i]) == 0){
            algo = FCFS;
        }
        else if (strcmp("SJF", argv[i]) == 0) {
            algo = SJF;
        }
        else if (strcmp("PRIO", argv[i]) == 0) {
            algo = PRIO;
        }
        else if (strcmp("VRUNTIME", argv[i]) == 0) {
            algo = VRUNTIME;
        }
    }
    if (readFile == -1) {
        if (argc < 7) {
            printf("Invalid parameters\n");
            exit(-1);
        }
        Wcount = atoi(argv[1]);
        Bcount = atoi(argv[2]);
        minB = atoi(argv[3]);
        avgB = atoi(argv[4]);
        minA = atoi(argv[5]);
        avgA = atoi(argv[6]);

        expoDistribution = 1;
    }
    else {
        if (argc != 5) {
            printf("Invalid parameters\n");
            exit(-1);
        }
        Wcount = atoi(argv[1]);
        expoDistribution = 0;
        strcpy(filename, argv[4]);
        char infilename[MAXFILENAME];

        int i, tempCount;
        Bcount = 0;
        for (i = 0; i < Wcount; ++i) {
            int count = 0;

            sprintf(infilename, "%s-%d.txt", filename, i + 1);
            printf("%s\n", infilename);
            int number1, number2;

            FILE *fp;
            fp = fopen(infilename, "r");
            while (fscanf(fp, "%d %d", &number1, &number2) == 2) {
                count++;
            }
            fclose(fp);

            tempCount = count;
        }
        Bcount += tempCount;
    }
}

int main (int argc, char **argv) {
    pthread_t constid;
    pthread_t prodtid[TMAX];
    int i, ret;

    runQueue = (struct run_queue *) malloc(sizeof (struct run_queue));
    runQueue->q = (struct rq_queue *) malloc(sizeof (struct rq_queue));
    run_queue_init(runQueue->q);
    pthread_mutex_init(&runQueue->mutex, NULL);
    pthread_cond_init(&runQueue->cond_W, NULL);
    pthread_cond_init(&runQueue->cond_S, NULL);

    getValues(argc, argv);

    for (i = 0; i < Wcount; ++i) {
        ret = pthread_create(&prodtid[i], NULL, W_thread, (void *) (long) i);
        if (ret < 0) {
            perror("thread create failed\n");
            exit(1);
        }
    }

    ret = pthread_create (&constid, NULL, S_thread, NULL);
    if (ret != 0) {
        perror ("thread create failed\n");
        exit (1);
    }

    for (i = 0; i < Wcount; ++i)
        pthread_join(prodtid[i], NULL);

    pthread_join (constid, NULL);

    free(runQueue->q);
    free(runQueue);

    pthread_mutex_destroy(&runQueue->mutex);
    pthread_cond_destroy(&runQueue->cond_W);
    pthread_cond_destroy(&runQueue->cond_S);

    printf ("Simulation complete\n");
    return 0;
}