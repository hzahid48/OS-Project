#include <iostream>
#include <unistd.h>
#include <cstdlib>

using namespace std;

int main(int argc, char* argv[]) {
    cout << "\nWelcome to P3" << endl;

    if (argc < 4) {
        cout << "File descriptor(s) and size not passed as arguments" << endl;
        exit(1);
    }

    int fd_read = atoi(argv[1]);
    int fd_write = atoi(argv[3]);
    int size = stoi(argv[2]);

    //cout << "Fd[1] output: " << fd_write << endl;

    float Input[size];
    ssize_t bytes_read = read(fd_read, Input, sizeof(float) * size);
    if (bytes_read == -1) {
        cout << "Error reading from the pipe" << endl;
        exit(1);
    }

    float sum = 0;
    for (int i = 0; i < size; i++) {
        //cout << "Data from pipe: " << Input[i] << endl;
        sum += Input[i];
    }

    cout << "Sum calculated in the output Layer: " << sum << endl;

    float sum_buffer[2];
    //sum_buffer[0] = sum;

    sum_buffer[0]=(sum*sum)+sum+1;
    sum_buffer[0]/=2;
    
    sum_buffer[1]=(sum*sum)-sum;
    sum_buffer[1]/=2;
    
    cout<<"Fx 1: "<<sum_buffer[0]<<endl;
    cout<<"Fx 2: "<<sum_buffer[1]<<endl;
    
    int back=stoi(argv[4]);
    if(back==1){
    //cout<<"Going Front\n";
    ssize_t bytes_written = write(fd_write, sum_buffer, sizeof(float) * 2);
    //cout << "Bytes written back to Layer2 from the output layer: " << bytes_written << endl;
    }

    close(fd_write);

    return 0;
}

