#include <iostream>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <iomanip>

using namespace std;

int NumRow;
int NumCol;

struct ArgData
{
    int fd;
    float* arr;
};

int main(int argc, char* argv[])
{

    cout << "\nWelcome to P2 " << endl;
    int size = stoi(argv[2]);
    //cout << "\n\nSize of buffer read: " << size << endl;

    // Check if file descriptor and size are passed as arguments
    if (argc < 3)
    {
        cout << "File descriptor and size not passed as arguments" << endl;
        exit(1);
    }

    // Convert file descriptor string to integer
    int fd[2];
    fd[0] = stoi(argv[1]);
    fd[1] = stoi(argv[3]);
    int back=stoi(argv[4]);
    float Input[size];
    read(fd[0], &Input, sizeof(float) * size);

    // Print data from pipe
    /*for (int i = 0; i < size; i++)
    {
        cout << "Data from pipe: " << Input[i] << endl;
    }*/

    //close(fd[0]); // close the reading pipe

    int LayerCount;
    cout << "Enter the number of Hidden layers you wish to add" << endl;
    cin >> LayerCount;

    int index_IW = 0;
    float** IW_arr = new float*[LayerCount];
    for (int i = 0; i < LayerCount; i++)
    {
        IW_arr[i] = new float[size];
    }

    string w = "w";
    string txt = ".txt";
    int FileCount = 1;
    string filename;

    for (int i = 0; i < LayerCount; i++)
    {
        filename = w;
        filename += to_string(FileCount);
        FileCount++;
        filename += txt;
        //cout << "Filename: " << filename << endl;

        ifstream W_file;
        W_file.open(filename);
        //cout << "Opening file\n";
        if (!W_file)
        {
            cout << "Error opening file: " << filename << endl;
            return 1;
        }
        //cout << "Reading file\n";
        float read_W;
        vector<float> W_values;
        while (W_file >> read_W)
        {
            W_values.push_back(read_W);
        }
        W_file.close();

        //cout << "File closed\n";
        float* Weights = new float[size];

        // Convert the vector of weights to an array
        for (int x = 0; x < size; x++)
        {
            Weights[x] = W_values[x];
        }

        // Create a pipe for each layer
        int pipe_fd[2];
        if (pipe(pipe_fd) == -1)
        {
            cout << "Error: Unable to create pipe" << endl;
            exit(1);
        }

        int pid = fork();
        if (pid == -1)
        {
            cerr << "Error: Unable to fork process" << endl;
            exit(1);
        }

        if (pid == 0)
        {   //Child process
            exit(0);
        }
        else
        { // Parent process
            //cout<<"Row IW_Arr\n";

            float** product = new float*[size];
            for (int x = 0; x < size; x++)
            {
                product[x] = new float[size];
            }

            for (int x = 0; x < size; x++)
            {
                for (int y = 0; y < size; y++)
                {
                    product[x][y] = Input[x] * Weights[y];
                    //cout << Input[x] << " * " << Weights[y] << " = " << product[x][y] << endl;
                }
            }

            int index_input = 0;
            for (int x = 0; x < size; x++)
            {
                Input[index_input] = 0;
                for (int y = 0; y < size; y++)
                {
                    Input[index_input] += product[x][y];
                }
                index_input++;
            }
            //cout << "Out of loop\n";

            int sum = 0;
            for (int x = 0; x < size; x++)
            {
                for (int y = 0; y < size; y++)
                {
                    sum += product[x][y];
                }
                IW_arr[index_IW][x] = sum;
                sum = 0;
            }

            for (int i = 0; i < size; i++)
            {
                cout << "Input for "<<index_IW+1<<" layer: " << Input[i] << endl;
            }

            cout<<endl;
            index_IW++;
        }
    }

    /*for (int x = 0; x < LayerCount; x++)
    {
        for (int y = 0; y < size; y++)
        {
            cout << IW_arr[x][y] << "  ";
        }
        cout << endl;
    }*/

    
    // Create a pipe
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        cout << "Error: Unable to create pipe" << endl;
        exit(1);
    }

    int pid = fork();
    if (pid == -1)
    {
        cerr << "Error: Unable to fork process" << endl;
        exit(1);
    }

    if (pid == 0)
    { // Child process
        //close(pipe_fd[1]);
        //cout << "Fd[1] Layer2: " << pipe_fd[1] << endl;
        execl("./P3", "Output", to_string(pipe_fd[0]).c_str(), to_string(size).c_str(), to_string(pipe_fd[1]).c_str(),to_string(back).c_str(), NULL);
        exit(0);
    }
    else
    { // Parent process
        //close(pipe_fd[0]);
        int e = write(pipe_fd[1], IW_arr[LayerCount - 1], size * sizeof(float));
        //cout << "Bytes: " << e << endl;
        //close(pipe_fd[1]);

        waitpid(pid, NULL, 0);
    }

    // Deallocate dynamically created arrays
    for (int i = 0; i < LayerCount; i++) {
        delete[] IW_arr[i];
    }
    delete[] IW_arr;

    if(back==1){
    //cout<<"Going Front\n";
    //Reading data calculated in the output layer
    float result[2];
    ssize_t bytes_read = read(pipe_fd[0], result, sizeof(float) * 2);
    if (bytes_read == -1) {
        cout << "Error reading from the pipe" << endl;
        exit(1);
    }
    
    cout<<"\nBack to layer2\n";
    cout<<"Fx 1 in Layer 2: "<<result[0]<<endl;
    cout<<"Fx 2 in Layer 2: "<<result[1]<<endl;
    
    //Write this sum into the pipe connected to Layer1 (The Input Layer)
    int bytes=write(fd[1],result, sizeof(float)*2);
    //cout<<"Bytes: "<<bytes<<endl;

    //cout<<"Returning to Layer 1\n";
    
    }
    
    // Close file descriptors
    close(fd[0]);
    close(fd[1]);
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    
  return 0;
}













