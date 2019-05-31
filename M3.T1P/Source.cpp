#include "mpi.h"
#include <ostream>
#include <iostream>
#include "Timer.h"
#include <vector>
#include <ctime>

const int ARRAY_SIZE = 10;
const int MAX_VALUE = 1000;

MPI_Status status;

int arrayToSort[ARRAY_SIZE];

// print results of current array
void print(int* array, int arraySize)
{
	for (size_t i = 0; i < arraySize; i++)
	{
		printf("%d, ", array[i]);
	}
	std::cout << "\n";
}

int section(int* array, const int left, const int right) {
	// get a mid point in the array
	const int mid = left + (right - left) / 2;
	const int pivotPtr = array[mid];
	// move the mid point value to the front of the array.
	std::swap(array[mid], array[left]);
	std::shared_ptr<int> i_left = std::make_shared<int>(left + 1);
	std::shared_ptr<int> j_right = std::make_shared<int>(right);
	// loop through section between leftPtr and rightPtr to find pivots correct placing
	// while swapping  < and > values in array to pivot with each other 

	while (*i_left <= *j_right) {
		// find next element from left  that is more then pivotPtr
		/// NOTE: checking for i_left and j_right are still valid
		while (*i_left <= *j_right && array[*i_left] <= pivotPtr) {
			*i_left = *i_left + 1;
		}
		// find next element for far right which is smaller then pivot
		while (*i_left <= *j_right && array[*j_right] > pivotPtr) {
			*j_right = *j_right - 1;
		}
		// double check if the left ptr is < right ptr. then swap
		if (*i_left < *j_right) {
			std::swap(array[*i_left], array[*j_right]);
		}
	}
	// swap original left with 
	std::swap(array[*i_left - 1], array[left]);
	// return new mid point
	return *i_left - 1;
}

void quicksort(int *array, const int left, const int right, const int arraySize) {
	// check if left and right ad still valid values
	if (left >= right)
		return;
	// get the new midpoint
	int midPtr = section(array, left, right);

	quicksort(array, left, midPtr - 1, arraySize);
	quicksort(array, midPtr + 1, right, arraySize);
}

void masterThread(int& i_iter, int& j_iter, int& k_iter, int& processorID, int& processorNum
	, int& totalProcessors, int& processorDestination, int& sourceID, int& arraySubSet, int& arrayOffset) {

	srand(time(NULL));

	//arrayToSort = (int *)calloc(ARRAY_SIZE, sizeof(int));
	for (size_t i = 0; i < ARRAY_SIZE; i++)
	{
		arrayToSort[i] = ((rand() % MAX_VALUE) + 1);
	}
	int arraySize = ARRAY_SIZE;

	/// split up rows for MPI processors
	//arraySubSet = ARRAY_SIZE / totalProcessors;
	arrayOffset = 0;
	print(arrayToSort, arraySize);

	int midPtr = section(arrayToSort, 0, ARRAY_SIZE - 1);
	printf("first sort - \n");
	print(arrayToSort, ARRAY_SIZE);
	printf("first sort - \n");
	/// set timer
	Timer::getInstance().addStartTime(eTimeLogType::TT_MULTIPLICATION_BEGIN, "Matric multiplication");

	printf("midPtr - %d\n", midPtr);
	/// send matrix data to workers
	arraySubSet = midPtr;
	for (processorDestination = 1; processorDestination <= totalProcessors; processorDestination++)
	{
		MPI_Send(&arrayOffset, 1, MPI_INT, processorDestination, 1, MPI_COMM_WORLD);
		MPI_Send(&arraySubSet, 1, MPI_INT, processorDestination, 1, MPI_COMM_WORLD);
		MPI_Send(&midPtr, 1, MPI_INT, processorDestination, 1, MPI_COMM_WORLD);
		MPI_Send(&arrayToSort[arrayOffset], arraySubSet, MPI_INT, processorDestination, 1, MPI_COMM_WORLD);
		/// set new rows to be sent to next iteration
		arrayOffset = arrayOffset + midPtr;
		arraySubSet = arraySize-midPtr;
	}

	/// get all data back
	for (i_iter = 1; i_iter <= totalProcessors; i_iter++)
	{
		sourceID = i_iter;
		MPI_Recv(&arrayOffset, 1, MPI_INT, sourceID, 2, MPI_COMM_WORLD, &status);
		MPI_Recv(&arraySubSet, 1, MPI_INT, sourceID, 2, MPI_COMM_WORLD, &status);
		MPI_Recv(&arrayToSort[arrayOffset], arraySubSet, MPI_INT, sourceID, 2, MPI_COMM_WORLD, &status);
	}

	/// finish timer for multiplication
	Timer::getInstance().addFinishTime(eTimeLogType::TT_MULTIPLICATION_BEGIN);

	/// print all results for the matrix
	// uncommment to see results
	/*printf("Matrix results:\n");
	for (i_iter = 0; i_iter < MAX_MATRIX_LENGTH; i_iter++) {
		for (j_iter = 0; j_iter < MAX_MATRIX_LENGTH; j_iter++)
			printf("%6.2f   ", matrix_final[i_iter][j_iter]);
		printf("\n");
	}*/

	/// print time taken
	Timer::getInstance().printFinalTimeSheet();
	print(arrayToSort, arraySize);

};

void workerThread(int& midPtr, int& j_iter, int& k_iter, int& processorID, int& processorNum
	, int& totalProcessors, int& processorDestination, int& sourceID, int& arraySubSet, int& arrayOffset) {

	sourceID = 0;
	MPI_Recv(&arrayOffset, 1, MPI_INT, sourceID, 1, MPI_COMM_WORLD, &status);
	MPI_Recv(&arraySubSet, 1, MPI_INT, sourceID, 1, MPI_COMM_WORLD, &status);
	MPI_Recv(&midPtr, 1, MPI_INT, sourceID, 1, MPI_COMM_WORLD, &status);
	MPI_Recv(&arrayToSort, arraySubSet, MPI_INT, sourceID, 1, MPI_COMM_WORLD, &status);

	/*/// per process matrix multiplication 
	for (k_iter = 0; k_iter < MAX_MATRIX_LENGTH; k_iter++)
	{
		for (i_iter = 0; i_iter < arraySubSet; i_iter++)
		{
			for (j_iter = 0; j_iter < MAX_MATRIX_LENGTH; j_iter++)
				matrix_final[i_iter][k_iter] += matrix_0[i_iter][j_iter] * matrix_1[j_iter][k_iter];
		}
	}*/

	int * array;
	array = (int *)calloc(arraySubSet, sizeof(int));
	for (size_t i = 0; i < arraySubSet; i++)
	{
		array[i] = arrayToSort[i];
	}
	int arraySize = arraySubSet;

	printf("array from %d - %d\n", arrayOffset, (arrayOffset+ arraySubSet));
	
	// run quicksort
	quicksort(array, 0, arraySize - 1, arraySize);

	print(array, arraySize);
	
	for (size_t i = 0; i < arraySubSet; i++)
	{
		arrayToSort[i] = array[i];
	}


	/// sending matrix data back to the master thread
	MPI_Send(&arrayOffset, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
	MPI_Send(&arraySubSet, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
	MPI_Send(&arrayToSort, arraySubSet, MPI_INT, 0, 2, MPI_COMM_WORLD);

};


int main(int argc, char **argv)
{
	int processorNum;
	int processorID;
	int totalProcessors;
	int processorDestination;
	int sourceID;
	int matrixRows;
	int rowOffset;
	int i_iter, j_iter, k_iter;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &processorID);
	MPI_Comm_size(MPI_COMM_WORLD, &processorNum);

	totalProcessors = processorNum - 1;

	/// Master Process 
	// in charge of sending and setting arrayToSort data to processors
	if (processorID == 0) {
		masterThread(i_iter, j_iter, k_iter, processorID, processorNum, totalProcessors, processorDestination, sourceID, matrixRows, rowOffset);
	}

	/// All processors but master thread
	if (processorID > 0) {
		workerThread(i_iter, j_iter, k_iter, processorID, processorNum, totalProcessors, processorDestination, sourceID, matrixRows, rowOffset);
	}
	
	/// clean up MPI
	MPI_Finalize();
	return 0;
}

