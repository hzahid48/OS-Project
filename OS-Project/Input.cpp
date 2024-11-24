#include <iostream>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <iomanip>
#include <semaphore.h>
using namespace std;

int bufferindex = 0;
float **buffer = nullptr; // initialize to nullptr
pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
int threadCount = 0;
int NumCol; // for testing purposes
int NumRow; // for testing purposes

bool threadFinished = false;
sem_t semaphore;

struct ArgData
{
    int fd;
    float* arr;
};

void* Compute_layer2(void* arg)
{
    
    ArgData* data = (ArgData*) arg;
    int size = NumRow; // compute size from NumRow
    //cout<<"Num ROWS------------------------------------------: "<<size<<endl;
    float sum = 0;
    for (int j = 0; j < size; j++)
    {
        sum += data->arr[j];
    }

    pthread_mutex_lock(&thread_mutex);
    threadCount++;
    if (bufferindex < NumCol)
    {
        buffer[bufferindex] = new float[1];
        buffer[bufferindex][0] = sum;
        int e=write(data->fd, buffer[bufferindex], sizeof(float) * 1);
        //cout<<"Writng "<<sum<<" from layer 1 ro layer2 \n";
        if(e<0){
          cout<<"Error writing into the pipe for layer2\n";
        }
        bufferindex++;
    }

    // Set the shared variable to indicate thread completion
    threadFinished = true;

    sem_post(&semaphore);

    pthread_mutex_unlock(&thread_mutex);
    pthread_exit(0);
}

int main()
{
    int size_i = 0;
    int size_w = 0;
    float *I_arr = nullptr;
    float *W_arr = nullptr;
    float **IW_arr = nullptr;

    string I_filename = "I.txt";
    string W_filename = "w.txt";

    ifstream I_file;
    ifstream W_file;

    // Opening file I.txt and storing its data into a 1D array I_arr
    I_file.open(I_filename);
    if (!I_file) {
        cout << "Error opening file: " << I_filename << endl;
        return 1;
    }
    float read_I;
    vector<float> I_values;
    while (I_file >> read_I)
    {
        I_values.push_back(read_I);
    }
    I_file.close();

    size_i = I_values.size();
    I_arr = new float[size_i];
    for (int i = 0; i < size_i; i++)
    {
        I_arr[i] = I_values[i];
    }

    // Opening file w.txt and storing its data into a 1D array W_arr
    W_file.open(W_filename);
    if (!W_file) {
        cout << "Error opening file: " << W_filename << endl;
        return 1;
    }
    float read_W;
    vector<float> W_values;
    while (W_file >> read_W)
    {
        W_values.push_back(read_W);
    }
    W_file.close();

    size_w = W_values.size();
    W_arr = new float[size_w];
    for (int i = 0; i < size_w; i++)
    {
        W_arr[i] = W_values[i];
    }

    // Using the arrays I_arr and W_arr, fill in the data for the 2D IW_arr
    IW_arr = new float*[size_w];
    for (int j = 0; j < size_w; j++)
    {
        IW_arr[j] = new float[size_i];
        for (int i = 0; i < size_i; i++)
        {
            IW_arr[j][i] = W_arr[j] * I_arr[i];
        }
    }

    cout << "Printing the multiplied matrix: \n";
    for (int j = 0; j < size_w; j++)
    {
        for (int i = 0; i < size_i; i++)
        {
            cout << setw(4) << IW_arr[j][i] << " ";
        }
        cout << endl;
    }

    NumCol = size_w;
    NumRow = size_i;

    int numThreads = size_w; // for testing purposes
    vector<pthread_t> threads(numThreads);
    vector<int> threadNums(numThreads);
    buffer = new float*[NumCol];

    int fd[2];
    if (pipe(fd) == -1)
    {
        cout << "Error creating pipe" << endl;
        exit(1);
    }

    ArgData data[numThreads];
    pid_t pid = fork();
    if (pid == -1)
    {
        cout << "Error forking" << endl;
        exit(1);
    }
    else if (pid == 0)
    {
        //cout<<"Round 1 call for p2\n";
        int back=1;
        execl("./P2", "Layer2", to_string(fd[0]).c_str(), to_string(NumCol).c_str(), to_string(fd[1]).c_str(), to_string(back).c_str(), NULL);
        //execl("./P2", "Layer2", to_string(fd[0]).c_str(), to_string(NumCol).c_str(), to_string(fd[1]).c_str(), false, NULL);
        exit(0);
    }
    else
    {
        sem_init(&semaphore, 0, 0);
        for (int j = 0; j < size_w; j++)
        {
            data[j].fd = fd[1];
            data[j].arr = IW_arr[j];
            threadNums[j] = j + 1;
            pthread_create(&threads[j], NULL, Compute_layer2, (void*)&data[j]);
        }

        while (threadCount < numThreads)
        {
            usleep(100);
        }

        sem_post(&semaphore);

        while (!threadFinished)
        {
            usleep(100);
        }

        waitpid(pid, NULL, 0);
        //cout<<"all threads created successfully\n";

        for (int j = 0; j < numThreads; j++)
        {
            delete[] data[j].arr;
        }

    }

    
    cout << "\nBack to layer 1 " << endl;
    //Reading data calculated in the output layer
    float result[2];
    ssize_t bytes_read = read(fd[0], result, sizeof(float) * 2);
    if (bytes_read == -1)
    {
        cout << "Error reading from the pipe" << endl;
        exit(1);
    }

    cout << "Fx 1 in Layer 1: " << result[0] << endl;
    cout << "Fx 2 in Layer 1: " << result[1] << endl;


    //result[0]=10;
    //result[1]=20;
    // Using the arrays I_arr and W_arr, fill in the data for the 2D IW_arr
    size_i=2;
    IW_arr = new float*[size_w];
    for (int j = 0; j < size_w; j++)
    {
        IW_arr[j] = new float[size_i];
        for (int i = 0; i < size_i; i++)
        {
            IW_arr[j][i] = W_arr[j] * result[i];
            //cout<<W_arr[j]<<"*"<<result[i]<<"= "<<IW_arr[j][i]<<endl;
        }
    }

    cout << "Printing the multiplied matrix: \n";
    for (int j = 0; j < size_w; j++)
    {
        for (int i = 0; i < size_i; i++)
        {
            cout << setw(4) << IW_arr[j][i] << " ";
        }
        cout << endl;
    }


 
    NumCol = size_w;
    NumRow = size_i;
    
    threadCount = 0;
    bufferindex = 0;
    numThreads = size_w; // for testing purposes
    vector<pthread_t> threads2(numThreads);
    vector<int> threadNums2(numThreads);
    float** buffer2 = new float*[NumCol];
    
    int fd2[2];
    if (pipe(fd2) == -1)
    {
        cout << "Error creating pipe" << endl;
        exit(1);
    }

    //cout << "Creating threads for next round\n";
    ArgData data2[numThreads];
    pid_t pid2 = fork();
    if (pid2 == -1)
    {
        cout << "Error forking" << endl;
                exit(1);
    }
    else if (pid2 == 0)
    {
        //cout<<"CAlling the function P2\n";
        int back2=0;
        execl("./P2", "Layer2", to_string(fd2[0]).c_str(), to_string(NumCol).c_str(), to_string(fd2[1]).c_str(), to_string(back2).c_str(), NULL);

        //execl("./P2", "Layer2", to_string(fd2[0]).c_str(), to_string(NumCol).c_str(), to_string(fd2[1]).c_str(), true, NULL);
        exit(0);
    }
    else
    {
        //sem_init(&semaphore, 0, 0);
        for (int j = 0; j < size_w; j++)
        {
            data2[j].fd = fd2[1];
            data2[j].arr = IW_arr[j];
            threadNums2[j] = j + 1;
            pthread_create(&threads2[j], NULL, Compute_layer2, (void*)&data2[j]);
        }

        
        while (threadCount < numThreads)
        {
            usleep(100);
        }

        sem_post(&semaphore);

        while (!threadFinished)
        {
            usleep(100);
        }

        waitpid(pid2, NULL, 0);
        
        //cout<<"ALL THREADS CREATED SUCCESSSFULLY\n";

        for (int j = 0; j < numThreads; j++)
        {
            delete[] data2[j].arr;
        }

        for (int i = 0; i < bufferindex; i++)
        {
            delete[] buffer[i];
        }
        delete[] buffer;

        sem_destroy(&semaphore);
    }

    // Exit thread
    pthread_exit(0);
}

       


