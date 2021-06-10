/*
Exercise description:

Given a variable-length array of numbers, calculate the average using all the available processors, and display the result in the console.
1. Create an array with values
Ex: in_value[n] = {1,2,3,...}
2. Execute the following function to sum in parallel threads
void fx(int a, int b) {
//a + b;
}
3. Display results to standard output
Ex: in_value[10] {0,1,2,3,4,5,6,7,8,9} | average: 4.5

Considerations
- Minimize memory allocation. [+2]
- Minimize idle clock cycles, take advantage of resources as much as possible. [+2]
- The compiler is gcc; uploading to an online compiler is desired but not required.
- Non-standard libraries could be used, but try to avoid it.
- Code comments and asserts are a great plus.
*/

//Cybergraphy:
//[1] https://www.codespeedy.com/generate-a-random-array-in-c-or-cpp/
//[2] https://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
//[3] https://www.researchgate.net/post/How_to_measure_CPU_time_for_benchmarking - Michael Grupp
//[4] https://arccoder.medium.com/c-sum-up-a-large-array-faster-using-std-async-f5b9617a545e
//[5] https://www.modernescpp.com/index.php/multithreaded-summation-of-a-vector
//[6] https://github.com/progschj/ThreadPool
//Libraries:
#include <iostream>    // std::cout, std::endl

#include <random>     //[1] mt19937 and uniform_int_distribution
#include <algorithm>  //[1] generate
#include <vector>     //[1] vector
#include <iterator>   //[1] begin, end, and ostream_iterator
#include <functional> //[1] bind

#include <chrono>     //[3][4]
#include <future>     //[3][4]
#include <numeric>    //[3][4]   // std::accumulate

#include "ThreadPool.h"

#define ARRAY_LENGTH  10000000

//----------------------------------- GLOBAL VARIABLES --------------------------------------------------
long long total_sum = 0;
std::mutex myMutex; //[5]
unsigned int nthreads = std::thread::hardware_concurrency(); //[2] Getting amount of proccesors
unsigned int ntasks = 20;
//------------------------------   FUNCTION DECLARATIONS   ----------------------------------------------

// [5] Function to create an array filled with random <int> numbers
int* create_random_data_array(int n, int min_number = 0, int max_number = 100) {
  std::vector<int> randValues;
  randValues.reserve(n);

  std::mt19937 engine;
  std::uniform_int_distribution<> uniformDist(min_number, max_number);
  for (long long i = 0; i < n; ++i) randValues.push_back(uniformDist(engine));

  //Adapting original function from [5] to return an array instead of vector type(Dynamic array https://stackoverflow.com/questions/4029870/how-to-create-a-dynamic-array-of-integers)
  int* arr = new int[n];
  //Array to vector https://stackoverflow.com/questions/2923272/how-to-convert-vector-to-array
  std::copy(randValues.begin(), randValues.end(), arr);

  return arr;
}

long long sumArr(int* arr, int start, int stop) {
  return std::accumulate(arr + start, arr + stop, 0); //discovered way[4]

  //Classical way ...
  //long long myTotal = 0;
  //for(int i = start; i < stop; i++) myTotal += arr[i];
  //return myTotal;
}

void sumArr2(int* arr, int start, int end) {
  long long aux_total = std::accumulate(arr + start, arr + end, 0); //discovered way[4]
  //Mutex for controlling the access to the global variable total_sum
  std::lock_guard<std::mutex> myLock(myMutex);
  total_sum += aux_total;
}

//-----------------------------------------   MAIN   -----------------------------------------------
int main() {

  //Creating a random array
  int* arr = create_random_data_array(ARRAY_LENGTH, 0, 1000);

  //[2]Showing number of proccesors
  std::cout << "Number of proccesors(possible concurrent threads): " << nthreads << std::endl;

  //***********************************************************************SINGLE THREAD WAY
  total_sum = 0;    //initializing total variable
  auto t1 = std::chrono::high_resolution_clock::now(); //taking first timestamp
  //----------------------------------------------------------------------------------------
  total_sum = sumArr(arr, 0, ARRAY_LENGTH);
  //----------------------------------------------------------------------------------------
  auto t2 = std::chrono::high_resolution_clock::now(); //taking second timestamp
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();    //getting difference between timestamps and converting to uSeconds
  std::cout << "Single thread total:" << total_sum << ", Execution lapse: " << duration << " microseconds " << std::endl;

  //***********************************************************************MULTI THREAD WAY(1st APPROACH[4])
  total_sum = 0;
  t1 = std::chrono::high_resolution_clock::now();
  //----------------------------------------------------------------------------------------
  //https://stackoverflow.com/questions/33769812/retrieving-values-from-array-of-future-objects-stdvector
  std::vector<std::future<long long>> partial_result;
  for (int i = 0; i < nthreads; i++) {
    partial_result.push_back(std::async(&sumArr, arr, i * ARRAY_LENGTH / nthreads, (i + 1) * ARRAY_LENGTH / nthreads));
  }
  for (auto& fut : partial_result) {
    total_sum += fut.get();
  }
  //----------------------------------------------------------------------------------------
  t2 = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();   //to uS convertion
  std::cout << "Multi thread(1) total:" << total_sum << ", Execution lapse: " << duration << " microseconds " << std::endl;

  //***********************************************************************MULTI THREAD WAY(2nd APPROACH[5])
  total_sum = 0;
  t1 = std::chrono::high_resolution_clock::now();
  //----------------------------------------------------------------------------------------
  std::thread* threads[nthreads];
  for (int j = 0; j < nthreads; j++) threads[j] = new std::thread(sumArr2, arr, j * ARRAY_LENGTH / nthreads, (j + 1) * ARRAY_LENGTH / nthreads);
  for (int j = 0; j < nthreads; j++)(*threads[j]).join();
  //----------------------------------------------------------------------------------------
  t2 = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();   //to uS convertion
  std::cout << "Multi thread(2) total:" << total_sum << ", Execution lapse: " << duration << " microseconds " << std::endl;


  //***********************************************************************THREAD POOL WAY(3nd APPROACH)

  t1 = std::chrono::high_resolution_clock::now();
  ThreadPool pool(nthreads);
  t2 = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();   //to uS convertion
  std::cout << "Threadpool creation lapse: " << duration << " microseconds " << std::endl;

  total_sum = 0;
  t1 = std::chrono::high_resolution_clock::now();
  std::vector< std::future<long long> > results;
  for (int j = 0; j < ntasks; ++j) {
    results.emplace_back(
      pool.enqueue(&sumArr, arr, j * ARRAY_LENGTH / ntasks, (j + 1) * ARRAY_LENGTH / ntasks)
    );
  }
  for (auto&& result : results)
    total_sum += result.get();

  t2 = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();   //to uS convertion
  std::cout << "Threadpool(3) total:" << total_sum << ", Execution lapse: " << duration << " microseconds " << std::endl;


  total_sum = 0;
  results.clear();
  // std::vector< std::future<long long> > results2; 
  t1 = std::chrono::high_resolution_clock::now();
  // std::vector< std::future<long long> > results;  
  for (int j = 0; j < ntasks; ++j) {
    results.emplace_back(
      pool.enqueue(&sumArr, arr, j * ARRAY_LENGTH / ntasks, (j + 1) * ARRAY_LENGTH / ntasks)
    );
  }
  for (auto&& result : results)
    total_sum += result.get();

  t2 = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();   //to uS convertion
  std::cout << "Threadpool(4) total:" << total_sum << ", Execution lapse: " << duration << " microseconds " << std::endl;

  //***************************************       RESULT  ***********************************************

  //Showing random array elements(Ex: in_value[10] {0,1,2,3,4,5,6,7,8,9} | average: 4.5)
  char user_answer = '\0';
  do {
    std::cout << "Do you want to print all vector values? y:yes(values and average) / n:no(only average)" << std::endl;
    std::cin >> user_answer;
  } while (user_answer != 'y' && user_answer != 'n');

  if (user_answer == 'y') {
    std::cout << "arr[" << ARRAY_LENGTH << "] = {";
    for (int i = 0; i < ARRAY_LENGTH; i++) {
      std::cout << arr[i] << ((i != ARRAY_LENGTH - 1) ? "," : "} | ");
    }
  } else {
    std::cout << "arr[" << ARRAY_LENGTH << "] = {...} | ";
  }
  std::cout << "average: " << total_sum / ARRAY_LENGTH << std::endl;
  return(0);
  // delete arr; //freeing up memory
  // pthread_exit(nullptr);
}


//------------------------------   CODE GARBAGE   ----------------------------------------------
// These snippets were produced during the test solving but they wasn't used in the final version of the code

// [1] Function to create an array filled with random <int> numbers
// int* create_random_data_array(int n) {
//   std::random_device r;
//   std::seed_seq      seed{r(), r(), r(), r(), r(), r(), r(), r()};
//   std::mt19937       eng(seed); // a source of random data

//   std::uniform_int_distribution<int> dist;
//   std::vector<int> v(n);

//   generate(begin(v), end(v), bind(dist, eng));

//   //Adapting original function from [1] to return an array instead of vector type(Dynamic array https://stackoverflow.com/questions/4029870/how-to-create-a-dynamic-array-of-integers)
//   int* arr = new int[n];
//   //Array to vector https://stackoverflow.com/questions/2923272/how-to-convert-vector-to-array
//   std::copy(v.begin(), v.end(), arr);

//   return arr;git




// results.emplace_back(
//           pool.enqueue([i] {
//               std::cout << "hello " << i << std::endl;
//               std::this_thread::sleep_for(std::chrono::seconds(i+1));
//               std::cout << "world " << i << std::endl;
//               return i*i;
//           })
//       );