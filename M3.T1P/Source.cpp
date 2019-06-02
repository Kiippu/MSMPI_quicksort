#include "mpi.h"
#include <ostream>
#include <iostream>
#include "Timer.h"
#include <vector>
#include <ctime>
#include <algorithm>

const int ARRAY_SIZE = 1000;
const int MAX_VALUE = 1000;

MPI_Status status;

/// final array
int arrayToSort[ARRAY_SIZE];
/// helper off set and distance arrays
std::vector<int> offSetArray;
std::vector<int> offSetDistantaceArray;

/// gets the biggest aarray sector to spilt
int getBiggestArray() 
{
	// gets distances
	offSetDistantaceArray.clear();
	for (size_t i = 0; i < offSetArray.size()-1; i++)
	{
		offSetDistantaceArray.push_back(offSetArray[i+1] - offSetArray[i]);
	}

	//get left index of biggest distance
	int leftBig = 0;
	int disBig = 0;
	for (size_t i = 0; i < offSetDistantaceArray.size(); i++)
	{
		if (offSetDistantaceArray[i] > disBig)
		{
			disBig = offSetDistantaceArray[i];
			leftBig = i;
		}
	}
	// return biggets distance left index
	return leftBig;
}

/// print results of current array
void print(int* array, int arraySize)
{
	for (size_t i = 0; i < arraySize; i++)
	{
		printf("%d, ", array[i]);
	}
}

/// checks if each element after itself is larger
void checkArray(int* array, int arraySize)
{
	printf("\n\n.\n..\n...\n------------------------------------------------------------\nError Checking Array..\n");
	int errCount = 0;
	for (size_t i = 0; i < arraySize; i++)
	{
		for (size_t k = i; k < arraySize; k++)
		{
			if (array[i] > array[k])
			{
				printf("index %d is > index %d (%d > %d)\n", i, k, array[i], array[k]);
				errCount++;
			}
		}
	}
	printf("Error Checking Complete!\nErrors total = %d out of %d elements\n------------------------------------------------------------\n...\n..\n.\n\n", errCount, arraySize);
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

void masterThread(int& processorID, int& processorNum
	, int& totalProcessors, int& processorDestination, int& sourceID, int& arraySubSet, int& left, int& right) {

	srand(time(NULL));

	for (size_t i = 0; i < ARRAY_SIZE; i++)
	{
		arrayToSort[i] = ((rand() % MAX_VALUE) + 1);
	}
	int arraySize = ARRAY_SIZE;

	/// off set array first an dlast element
	offSetArray.push_back(0);
	offSetArray.push_back((arraySize - 1));

	/// fill in other sectors depends on num of processors
	// will create sectors = num totalProcessors-1 
	// will split up largest sector on each iteration
	for (size_t i = 1; i < totalProcessors; i++)
	{
		int leftStart = getBiggestArray();
		printf("-- sorting from index: %d - %d\n",leftStart,leftStart+1);
		offSetArray.push_back(section(arrayToSort, offSetArray[leftStart], offSetArray[leftStart +1]));
		std::sort(offSetArray.begin(), offSetArray.end());
	}
	// print num of elements in each sector
	for (int i = 0; i < offSetDistantaceArray.size(); i++)
	{
		printf("distance[%d] - %d, ", i, offSetDistantaceArray[i]);
		printf("\n");
	}


	printf("\nfirst sort - \n");
	print(arrayToSort, ARRAY_SIZE);
	printf("\nfirst sort - \n");

	/// set timer
	Timer::getInstance().addStartTime(eTimeLogType::TT_QUICKSORT, "Quicksort");
	/// tracks left elemenet to state with in sort
	left = 0;
	/// tracks offset index - different to for loop processor index
	int index = 0;
	for (processorDestination = 1; processorDestination <= totalProcessors; processorDestination++)
	{
		// finding left and right index
		left = offSetArray[index];
		right = offSetArray[index+1];
		if (processorDestination == totalProcessors)
			right++;
		arraySubSet = right - left;
		/// send off data to other processors
		MPI_Send(&left, 1, MPI_INT, processorDestination, 1, MPI_COMM_WORLD);
		MPI_Send(&arraySubSet, 1, MPI_INT, processorDestination, 1, MPI_COMM_WORLD);
		MPI_Send(&right, 1, MPI_INT, processorDestination, 1, MPI_COMM_WORLD);
		MPI_Send(&arrayToSort[left], arraySubSet, MPI_INT, processorDestination, 1, MPI_COMM_WORLD);
		
		index++;
	}

	/// get all data back
	for (int i = 1; i <= totalProcessors; i++)
	{
		sourceID = i;
		MPI_Recv(&left, 1, MPI_INT, sourceID, 2, MPI_COMM_WORLD, &status);
		MPI_Recv(&arraySubSet, 1, MPI_INT, sourceID, 2, MPI_COMM_WORLD, &status);
		MPI_Recv(&arrayToSort[left], arraySubSet, MPI_INT, sourceID, 2, MPI_COMM_WORLD, &status);
	}

	/// finish timer for multiplication
	Timer::getInstance().addFinishTime(eTimeLogType::TT_QUICKSORT);

	/// print time taken
	Timer::getInstance().printFinalTimeSheet();
	/// print final array sorted
	print(arrayToSort, arraySize);
	/// check final array is sorted from <
	checkArray(arrayToSort, arraySize);

};

void workerThread(int& processorID, int& processorNum
	, int& totalProcessors, int& processorDestination, int& sourceID, int& arraySubSet, int& left, int& right) {

	sourceID = 0;
	MPI_Recv(&left, 1, MPI_INT, sourceID, 1, MPI_COMM_WORLD, &status);
	MPI_Recv(&arraySubSet, 1, MPI_INT, sourceID, 1, MPI_COMM_WORLD, &status);
	MPI_Recv(&right, 1, MPI_INT, sourceID, 1, MPI_COMM_WORLD, &status);
	MPI_Recv(&arrayToSort, arraySubSet, MPI_INT, sourceID, 1, MPI_COMM_WORLD, &status);
	printf("--: BEGIN MPI PROCCESS %d :--\n", processorID);

	/// assigning array data for quicksort
	int * array;
	array = (int *)calloc(arraySubSet, sizeof(int));
	for (size_t i = 0; i < arraySubSet; i++)
	{
		array[i] = arrayToSort[i];
	}
	int arraySize = arraySubSet;

	/// run quicksort on worker data
	quicksort(array, 0, arraySize-1, arraySize);
	/// print worker array once sorted
	printf("\n- Process %d sorted data -\n", processorID);
	print(array, arraySize);
	
	/// assignonf new data back to old array
	for (size_t i = 0; i < arraySubSet; i++)
	{
		arrayToSort[i] = array[i];
	}

	/// sending matrix data back to the master thread
	MPI_Send(&left, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
	MPI_Send(&arraySubSet, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
	MPI_Send(&arrayToSort, arraySubSet, MPI_INT, 0, 2, MPI_COMM_WORLD);
	printf("\n--: FINISH MPI PROCCESS %d :--\n", processorID);
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
	int left, right;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &processorID);
	MPI_Comm_size(MPI_COMM_WORLD, &processorNum);

	totalProcessors = processorNum - 1;

	/// Master Process 
	// in charge of sending and setting arrayToSort data to processors
	if (processorID == 0) {
		masterThread(processorID, processorNum, totalProcessors, processorDestination, sourceID, matrixRows, left, right);
	}

	/// All processors but master thread
	if (processorID > 0) {
		workerThread(processorID, processorNum, totalProcessors, processorDestination, sourceID, matrixRows, left, right);
	}
	
	/// clean up MPI
	MPI_Finalize();
	return 0;
}

