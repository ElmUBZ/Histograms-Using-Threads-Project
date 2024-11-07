// Bobby Zita -- 10/24/2024
// Project 02: Computing a Histogram
// Creates a histogram based on the contents of a file (using threads)

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>

using namespace std;

//Eliminates the race condition by not letting 2 threads access the same data (for global histogram method)
atomic<size_t> globalIndex(0);

//Declare a mutex space so the threads aren't double counting characters
mutex histoMutex; 

//Supplied by Dr. Jeffery
//Const added by me for testing purposes --> remove later
void fileToMemoryTransfer(/* const */ char* fileName, char** data, size_t& numOfBytes) {
	streampos begin, end; 
	ifstream inFile(fileName, ios::in | ios::binary | ios::ate); 
	if (!inFile){
		cerr << "Cannot open " << fileName << endl; 
		inFile.close();
		exit(1); 
	}
	size_t size = inFile.tellg();
	char* buffer = new char[size];
	inFile.seekg(0, ios::beg);
	inFile.read(buffer, size);
	inFile.close();
	*data = buffer;
	numOfBytes = size; 
}

//Function to print the final array
void printHisto(vector<long long int> &toPrint) {
	for (int i = 0; i < toPrint.size(); i++) {
		cout << i << ": " << toPrint[i] << endl; 
	}
}

//Method that uses one single global histogram
void globalHist(char* data, size_t& numOfBytes, int numThreads) {
	cout << "Run with one global histogram" << endl;

	//Initialize a global histogram with bins 0 - 255 with the value 0 in all
	vector<long long int> GlobalHist(256, 0); 

	//Create and use threads with a lambda expression
	vector<thread> workers;
	for (int i = 0; i < numThreads; ++i) {
		//Emplace_back used as it's faster and works with more types (constructor-wise)
		workers.emplace_back([&]() {
			while (true) {//keep going until we break from running out of data
				//Get the next index to read and incriment it 
				//Taken from tutorialsPoint [atomic::fetch_add()]
				int index = globalIndex.fetch_add(1);

				//If it's processed all data, exit the loop
				if (index >= numOfBytes) {
					break;
				}

				//Takes the next piece of data and turns it into a value that maps to a valid bucket --> has to be unsigned or it maps to negative buckets in some cases
				unsigned char currData = static_cast<unsigned char>(data[index]);

				//Update the global histogram using a mutex to prevent race conditions (aka 2 threads updating it at the same time)
				histoMutex.lock(); 
				GlobalHist[currData]++;
				histoMutex.unlock(); 
			}
			});
	}

	//Join threads after they're all done
	for (auto& worker : workers) {
		worker.join();
	}

	//Print the global histogram
	printHisto(GlobalHist);
}

//Method to use local histograms and merge them together
void localHist(char* data, size_t& numOfBytes, int numThreads) {
	cout << "Run with local histograms" << endl;

	//The final merged histogram --> Bins 0 - 255 all initialized to 0
	vector<long long int> finalHisto(256, 0); 

	size_t workLoad = numOfBytes / numThreads;
	vector<thread> workers;

	//Create a 2d vector to hold the local histograms (which are also vectors)
	vector<vector<long long int>> localHistos(numThreads, vector<long long int>(256, 0));

	for (int i = 0; i < numThreads; ++i) {
		workers.emplace_back([&, i]() {
			//workLoad is an offset (so if each thread should be allocated 200 bytes, thread 2 starts at index 400)
			unsigned int start = i * workLoad;
			unsigned int end;

			//The end is the start plus the workLoad, unless its the last thread which finishes the job
			if (i == numThreads - 1)
				end = numOfBytes;
			else
				end = start + workLoad; 

			//Each thread counts the data in the assigned range
			for (auto j = start; j < end; ++j) {
				unsigned char currData = static_cast<unsigned char>(data[j]);
				//2-d vector --> accesses the currData bucket of vector i (aka thread i)
				localHistos.at(i).at(currData)++;
			}
			});
	}

	//Join threads --> waits for all threads to be done
	for (auto& worker : workers) {
		worker.join();
	}

	//Merge local histograms into the final one
	//I think if anything, this might be the most time consuming part
	for (auto& localHisto : localHistos) {
		for (int i = 0; i < 256; ++i) {
			finalHisto[i] += localHisto[i];
		}
	}

	//Print the final merged histogram
	printHisto(finalHisto);
}

/*Takes in the file name from input using 
	Arg1(count of the number of arguements entered) & 
	Arg2(array of the arguements)
*/
int main(int arg1, char* arg2[]) {
	//Step 1 -- Initialize the data pointer and number of bytes (needed for workload calc).
	char* data = nullptr;
	size_t numberOfBytes = 0.0; 

	//Step 2 -- Verify that the correct number of arguments were given at the command line
	if (arg1 > 2) {
		cerr << "Error: " << arg2[0] << endl;
		return 1; 
	}
	//Step 3 -- Initialize the fileName
	char* fileName = arg2[1];
	
	//HAS TO BE CONST FOR THE SAKE OF BEING A STRING INPUT AND NOT FROM COMMAND LINE :) -- TODELETE	
	//const char* fileName = "ElmU.jpg"; --> This is just a test case to make sure it works before i mess with the command line

	//Step 4 -- Figure out how many threads the local machine has
	int numThreads = thread::hardware_concurrency();

	//Step 5 -- Read the FileName's data into the buffer
	fileToMemoryTransfer(fileName, &data, numberOfBytes); 

	//Step 6 -- Call globalHist (let it handle the rest) -- Calculate and print the run time
	clock_t startTime = clock(); 
	globalHist(data, numberOfBytes, numThreads);
	clock_t endTime = clock();
	double totalTime = (double(endTime) - double(startTime)) / CLOCKS_PER_SEC; 
	cout << "Global method occured in " << totalTime << " seconds!" << endl; 

	//Step 7 -- Call LocalHist (let it handle the rest) -- Calculate and print the run time
	startTime = clock();
	localHist(data, numberOfBytes, numThreads);
	endTime = clock();
	totalTime = (double(endTime) - double(startTime)) / CLOCKS_PER_SEC;
	cout << "Local method occured in " << totalTime << " seconds!" << endl;

	return 0;
}
