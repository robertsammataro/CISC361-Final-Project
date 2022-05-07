#include <iostream>
#include <fstream>
#include <queue>
#include <deque>

using namespace std;

//Helper functions that take care of getting the instructions
//in a more useable format and prints out info to help debug
vector<string> parseInstruction(string);
queue<vector<string>> getOrderedInstructions(char**);
void displayInitialSystemStatus(void);
void displaySystemStatus(int);
void handleJobArrival(vector<string>);
void checkBankersAlg();
void addProcessesAwaitingMemory(void);
bool checkBankers(int devices);
void displayReadyQueue();
void releaseDevices(int devicesToRelease);
bool checkBankersHolding(vector<int> input_process);


//Mini-class that links together processes in Holding Queue 1
class linked_process {
public:
  linked_process *previous;
  linked_process *next;
  vector<string> process;
};

queue<string> instruction_queue;
queue<vector<string>> parsed_instruction_queue;
queue<vector<int>> ready_queue;
deque<vector<int>> wait_queue;
queue<vector<string>> complete_queue;
linked_process *hold_queue_1 = NULL;
deque<vector<string>> hold_queue_2;
vector<int> running_process;
vector<vector<string>> device_instructions;
vector<vector<int>> finished_queue;

int sys_time;
int sys_memory_size;
int sys_memory_remaining;
int sys_total_devices;
int sys_devices_remaining;
int sys_quantum;
int cur_time_slice = 0;


int main(int argc, char** argv)
{

  

  //Check to see if the user provided an instruction file
  if(argc == 1){
    cout << "Please enter a file name when executing the program!" << endl;
    return(1);
  }

  parsed_instruction_queue = getOrderedInstructions(argv);

  //Logic for the scheduler. Goes to sys_time == 9999 since the last instruction is at 9999
  while(sys_time <= 9999){

    cur_time_slice = ((cur_time_slice) + 1) % sys_quantum;

    //If the next instruction arrives at this time:
    if(parsed_instruction_queue.size() != 0 && stoi(parsed_instruction_queue.front().at(1)) == sys_time){
      vector<string> instruction = parsed_instruction_queue.front();
      parsed_instruction_queue.pop();

      //If a job arrives and there's enough space:
      if(instruction.at(0) == "A" && stoi(instruction.at(3)) <= sys_memory_size){
	handleJobArrival(instruction);
      }

      //If the next instruction is a request or release of devices, send
      //to device queue (holds until it's corresponding process is running!)
      if(instruction.at(0) ==  "Q" || instruction.at(0) == "L"){
	device_instructions.push_back(instruction);
      }

      if(instruction.at(0) == "D"){
        displaySystemStatus(stoi(instruction.at(1)));
      }
      

    }

    if(running_process.size() == 0 && ready_queue.size() > 0){
      cout << "Initializing new job on the CPU, job with pid: " << ready_queue.front().at(0) << endl;
      running_process = ready_queue.front();
      ready_queue.pop();
      cur_time_slice = 0;
    }

    //This will be set to false if there is an instruction for request/release of devices
    bool run_for_unit = true;

    //This will be set to false if the process has been moved to the wait queue because of a
    //request or release of devices
    bool still_in_ready_queue = true;

    //If there's a running process...
    if(running_process.size() != 0){

      //Check to see if there's an instruction in the waiting queue
      for(int x = 0; x < device_instructions.size(); x++){
	vector<string> current_instruction = device_instructions.at(x);
	if(stoi(current_instruction.at(2)) == running_process.at(0)){

	  if(current_instruction.at(0) == "Q"){
	    cout << "FOUND A REQUEST INSTRUCTION FOR PROCESS " << running_process.at(0) << endl;
	    checkBankers(stoi(current_instruction.at(3)));
	  }

	  else{
	    cout << "FOUND A RELEASE INSTRUCTION FOR PROCESS " << running_process.at(0) << endl;
	    running_process.at(1) = running_process.at(1) -  stoi(current_instruction.at(3));
	    releaseDevices(stoi(current_instruction.at(3)));
	  }

	  device_instructions.erase(device_instructions.begin() + x);
	  cur_time_slice = sys_quantum - 1;
	  run_for_unit = false;
	  x = device_instructions.size(); //This line will have the for loop break so it only runs one instruction at a time
	}
      }

      //If there were no request/release instructions, have it take 1 off the process's remaining time
      if(run_for_unit)
	running_process.at(2) -= 1;

      if(running_process.size() != 0)
	cout << "RUNNING PROCESS STATUS\tpid: " << running_process.at(0) << " Time remaining: " << running_process.at(2) << " Devices Held: " << running_process.at(1) << " System Devices Remaining: " << sys_devices_remaining << endl;

      //Check to see if the process has completed
      if(running_process.size() != 0 && running_process.at(2) == 0 && still_in_ready_queue){

	//Make the memory and devices held by the completing process available again
	sys_memory_remaining += running_process.at(3);
	sys_devices_remaining = sys_devices_remaining + running_process.at(1);
	cout << "Just freed up " << running_process.at(1) << " devices!" << endl;
	cout << "Process " << running_process.at(0) << " has just completed at time " << sys_time << " Memory Available: " << sys_memory_remaining << " Devices Available: " << sys_devices_remaining << endl << endl;
	finished_queue.push_back(running_process);
	running_process.clear();
	releaseDevices(0); //This will move any processes in the waiting queue back to the ready queue
	addProcessesAwaitingMemory();
      }

      //If the time slice has completed, move the running process back to the ready queue 
      else if(cur_time_slice == sys_quantum - 1 && still_in_ready_queue && running_process.size() != 0){

	if(running_process.size() != 0)
	  cout << "Process " << running_process.at(0) << "'s time slice has completed at time " << sys_time << endl;
	ready_queue.push(running_process);
	running_process.clear();
      }
      
      //If the process has been moved to the waiting queue
      if(!still_in_ready_queue)
	running_process.clear();

      
    }

    sys_time++;

  }
  

  //Iteratively print the ready queue
  cout << endl << "READY QUEUE: " << endl;
  while (ready_queue.size() > 0) {
    vector<int> myVector = ready_queue.front();
    cout << "pid:\t\t" << myVector.at(0) << endl;
    cout << "Devices Held:\t" << myVector.at(1) << endl;
    cout << "Remaining Time:\t" << myVector.at(2) << endl;
    cout << "Req'd Memory:\t" << myVector.at(3) << endl;
    cout << "Max Devices:\t" << myVector.at(4) << endl << endl;
    ready_queue.pop();
  }


  //Iteratively print the priority 1 hold queue
  linked_process *current_hold = hold_queue_1;
  cout << endl << "HOLD QUEUE 1 (SJF)" << endl;
  while(current_hold){
    cout << "Arrival: \t" << current_hold->process.at(1) << endl;
    cout << "pid: \t\t" << current_hold->process.at(2) << endl;
    cout << "Mem Req'd: \t" << current_hold->process.at(3) << endl;
    cout << "Max Devices: \t" << current_hold->process.at(4) << endl;
    cout << "Runtime: \t" << current_hold->process.at(5) << endl;
    cout << "Priority: \t" << current_hold->process.at(6) << endl << endl;
    current_hold = current_hold->next;
  }

  //Iteratively print the priority 2 hold queue
  cout << endl << "HOLD QUEUE 2: (FIFO)" << endl;
  for (int x = 0; x < hold_queue_2.size(); x++) {
    vector<string> myVector = hold_queue_2[x];
    cout << "Arrival: \t" << myVector.at(1) << endl;
    cout << "pid:\t\t" << myVector.at(2) << endl;
    cout << "Mem Req'd: \t" << myVector.at(3) << endl;
    cout << "Max Devices:\t" << myVector.at(4) << endl;
    cout << "Runtime:\t" << myVector.at(5) << endl;
    cout << "Priority: \t" << myVector.at(6) << endl << endl;
  }

  cout << "WAIT QUEUE SIZE: " << wait_queue.size() << endl;
  cout << "FINISHED QUEUE SIZE: " << finished_queue.size() << endl;

}

vector<string> parseInstruction(string instruction){

  char firstChar = instruction.at(0);
  vector<string> parsedComponents;
  
  if(firstChar == 'C'){
    cout << "Configuration Instruction Found" << endl;
    int index = 0;
    string partial_string = "";

    //Create a vector, populated with each of the fields from
    //the original instruction
    while(instruction[index]){

      if(instruction[index] != ' ')
	partial_string += instruction[index];

      else{

	//Remove the M=, S=, and Q= from the components vector
	if(partial_string.at(0) == 'M' ||
	   partial_string.at(0) == 'S' ||
	   partial_string.at(0) == 'Q')
	  partial_string.erase(0, 2);
	  
	parsedComponents.push_back(partial_string);
	partial_string = "";

      }

      index++;
    }

    sys_time = stoi(parsedComponents.at(1));
    sys_memory_size = stoi(parsedComponents.at(2));
    sys_memory_remaining = sys_memory_size;
    sys_total_devices = stoi(parsedComponents.at(3));
    sys_devices_remaining = sys_total_devices;
    sys_quantum = stoi(parsedComponents.at(4));

    return parsedComponents;
  }

  else if (firstChar == 'A'){
    //cout << "Arrival Instruction Found" << endl;
    int index = 0;
    string partial_string = "";

    //Create a vector, populated with each of the fields from
    //the original instruction
    while(instruction[index]){

      if(instruction[index] != ' ')
	partial_string += instruction[index];

      else{

	//Remove the J=, M=, S=, R=, and P= from the components vector
	if(partial_string.at(0) == 'J' ||
	   partial_string.at(0) == 'M' ||
	   partial_string.at(0) == 'S' ||
	   partial_string.at(0) == 'R' ||
	   partial_string.at(0) == 'P')
	  partial_string.erase(0, 2);
	  
	parsedComponents.push_back(partial_string);
	partial_string = "";

      }

      index++;
    }
    return parsedComponents;
  }
    
  else if (firstChar == 'Q'){
    //cout << "Device Request Instruction Found" << endl;
    int index = 0;
    string partial_string = "";

    //Create a vector, populated with each of the fields from
    //the original instruction
    while(instruction[index]){

      if(instruction[index] != ' ')
	partial_string += instruction[index];

      else{

	//Remove the J= and D= from the components vector
	if(partial_string.at(0) == 'J' ||
	   partial_string.at(0) == 'D' )
	  partial_string.erase(0, 2);
	  
	parsedComponents.push_back(partial_string);
	partial_string = "";

      }

      index++;
    }
    return parsedComponents;
  }

  else if (firstChar ==  'L'){
    //cout << "Device Release Instruction Found" << endl;
    int index = 0;
    string partial_string = "";

    //Create a vector, populated with each of the fields from
    //the original instruction
    while(instruction[index]){

      if(instruction[index] != ' ')
	partial_string += instruction[index];

      else{

	//Remove the J= and D= from the components vector
	if(partial_string.at(0) == 'J' ||
	   partial_string.at(0) == 'D' )
	  partial_string.erase(0, 2);
	  
	parsedComponents.push_back(partial_string);
	partial_string = "";

      }

      index++;
    }
    return parsedComponents;
  }
    
  else if (firstChar == 'D'){
    //cout << "Display Status Instruction Found" << endl;
    int index = 0;
    string partial_string = "";

    //Create a vector, populated with each of the fields from
    //the original instruction
    while(instruction[index]){

      if(instruction[index] != ' ' && instruction[index + 1])
	partial_string += instruction[index];

      else{
	partial_string += instruction[index];
	parsedComponents.push_back(partial_string);
	partial_string = "";
      }

      index++;
    }
    return parsedComponents;
  }

  else{
    cout << "Uh oh. Invalid Instruction Found" << endl;
    return parsedComponents;
  }

}

queue<vector<string>> getOrderedInstructions(char** argv){

//READ THROUGH EVERYTHING IN THE INPUT FILE AND PUSH ONTO
  //A QUEUE TO BE PROCESSED BY THE SYSTEM!
  ifstream myFile;
  string line;

  myFile.open(argv[1]);

  //For each line in the input file...
  //All these functions are done in constant time because of the
  //containers we chose, so this is just O(r)
  while(getline(myFile, line)){

    instruction_queue.push(line);
    
  }

  //Generate vector of the instructions in order of arrival
  while(instruction_queue.size() != 0){
    string myLine = instruction_queue.front();
    instruction_queue.pop();
    vector<string> myVector = parseInstruction(myLine);
    parsed_instruction_queue.push(myVector);
  }

  return parsed_instruction_queue;

}

//This gets triggered every time a "D" command is received
void displaySystemStatus(int time){

  cout << "SYSTEM PROPERTIES AT TIME " << time << endl;
  cout << "(This is going to be implemented later!)" << endl;  

}

void handleJobArrival(vector<string> instruction){
	
  cout << "Creating process " << instruction.at(2) << " that needs " << instruction.at(3) << " memory with priority " << instruction.at(6) << " at time " << sys_time;

  //If there's enough system memory remaining, send to ready queue
  int memory_required = stoi(instruction.at(3));
  if(memory_required <= sys_memory_remaining){

    //Allocate memory and create a new process
    sys_memory_remaining = sys_memory_remaining - memory_required;
    vector<int> new_process;
    new_process.push_back(stoi(instruction.at(2))); //Process ID
    new_process.push_back(0);                       //Devices being used
    new_process.push_back(stoi(instruction.at(5))); //Remaining Time
    new_process.push_back(memory_required);         //Required Memory
    new_process.push_back(stoi(instruction.at(4))); //Max Devices
    ready_queue.push(new_process);
    cout << " || Sent to ready queue -- Memory Remaining: " << sys_memory_remaining << endl;
  }

  //Otherwise, send to the appropriate waiting queue
  else{

    //If job has priority 1, send to the P1 waiting queue
    if(stoi(instruction.at(6)) == 1){
	    
      cout << " || Sent to priority 1 waiting queue" << endl;

      linked_process *current_process = hold_queue_1;
      linked_process *inserting_process = new linked_process();
      inserting_process->previous = NULL;
      inserting_process->next = NULL;
      inserting_process->process = instruction;   

      //If there are no items in the hold_queue:
      if(hold_queue_1 == NULL){
	hold_queue_1 = inserting_process;
      }

      //Iterate through to find the spot that the inserting process should be inserted at
      else{
	      
	while(current_process){

	  //If the process being inserted runs for less time than the process at this point
	  //in the list's iteration, then it precedes it
	  if(stoi(inserting_process->process.at(5)) <= stoi(current_process->process.at(5))){

	    //Assign the process being inserting to be the head of the linked list if the previous
	    //pointer of the hold queue is NULL
	    if(current_process->previous = NULL)
	      hold_queue_1 = inserting_process;
		  
	    inserting_process->previous = current_process->previous;
	    inserting_process->next = current_process;
	    current_process->previous = inserting_process;
		  
	    cout << "Found my home! I run for " << inserting_process->process.at(5) << " My neighbor runs for " << current_process->process.at(5) << endl;
	    return;
	  }

	  //Else if the current process is still less than the process being inserted and the current_process's
	  //next pointer is null, we must be inserting it to the end of the list
	  else if (current_process->next == NULL){
	    cout << "I arrived at time " << inserting_process->process.at(1) << " and I run for: " << inserting_process->process.at(5) << ". My left neighbor arrived at "
		 << current_process->process.at(1) << " and runs for " << current_process->process.at(5) << endl;
	    current_process->next = inserting_process;
	    return;
	  }
	  current_process = current_process->next;
	}
      }   
    }

    //Otherwise, send to the P2 waiting queue
    else{
      cout << " || Sent to priority 2 waiting queue" << endl;
      hold_queue_2.push_back(instruction);
    }
  }

}

//Moves jobs from the hold queues to the ready queue if there is enough memory available
void addProcessesAwaitingMemory(void){
  
  //Move over as many items from the P1 queue as possible!
  linked_process *current_process = hold_queue_1;
  while(current_process){

    if(stoi(current_process->process.at(3)) <= sys_memory_remaining){
      vector<string> instruction = current_process->process;
      vector<int> new_process;
      new_process.push_back(stoi(instruction.at(2))); //Process ID
      new_process.push_back(0);                       //Devices being used
      new_process.push_back(stoi(instruction.at(5))); //Remaining Time
      new_process.push_back(stoi(instruction.at(3))); //Memory Required
      new_process.push_back(stoi(instruction.at(4))); //Max Devices Needed

      sys_memory_remaining -= stoi(instruction.at(3));
      ready_queue.push(new_process);

      if(current_process->next)
	current_process->next->previous = NULL;
      hold_queue_1 = current_process->next;
      current_process = hold_queue_1;

      cout << "moved process " << new_process.at(0) << " to from HQ1 to the ready queue! Mem Remaining: " << sys_memory_remaining << endl;
    }

    else
      return;

  }

  //Move over as many items from the P2 queue as possible!
  while(hold_queue_2.size() > 0){

    if(stoi(hold_queue_2.front().at(3)) <= sys_memory_remaining){
      vector<string> instruction = hold_queue_2.front();
      vector<int> new_process;
      new_process.push_back(stoi(instruction.at(2))); //Process ID
      new_process.push_back(0);                       //Devices being used
      new_process.push_back(stoi(instruction.at(5))); //Remaining Time
      new_process.push_back(stoi(instruction.at(3))); //Memory Required
      new_process.push_back(stoi(instruction.at(4))); //Max Devices Needed

      sys_memory_remaining -= stoi(instruction.at(3));
      ready_queue.push(new_process);

      hold_queue_2.pop_front();

      cout << "Just moved process " << new_process.at(0) << " to from HQ2 to the ready queue! Mem Remaining: " << sys_memory_remaining << endl;
    }
    else
      return;
  }
}

bool checkBankers(int devices){

  vector<int> held;
  vector<int> need;
  vector<bool> finish;
  int available = sys_devices_remaining;

  cout << "Process " << running_process.at(0) << " requested " << devices << " devices (System has " << sys_devices_remaining << "). Now checking:" << endl;

  //Populate held and max vectors with the values for each running process
  held.push_back(running_process.at(1));
  need.push_back(running_process.at(4) - running_process.at(1));

  queue<vector<int>> temp_ready_queue = ready_queue;
  while(temp_ready_queue.size() > 0){
    vector<int> temp_process = temp_ready_queue.front();
    held.push_back(temp_process.at(1));
    need.push_back(temp_process.at(4) - temp_process.at(1));
    finish.push_back(false);
    temp_ready_queue.pop();
  }

  int iteration = 0;
  while(iteration < finish.size()){

    if(finish[iteration] == false && need[iteration] <= available){
      finish[iteration] = true;
      available += held[iteration];
      iteration = 0;
    }
    else
      iteration++;

  }

  //Check if all finish values are total
  bool all_true = true;
  for(int count = 0; count < finish.size(); count++){
    if(finish[count] == false)
      all_true = false;
  }
  

  if(all_true && devices <= sys_devices_remaining){
    cout << "Bankers Algorithm Successful: Allocating " << devices << " devices to process " << running_process.at(0) << endl;
    running_process.at(1) += devices;
    sys_devices_remaining = sys_devices_remaining - devices;
    return true;
  }

  else{
    cout << "Bankers Algorithm Unsuccessful: Couldn't allocate " << devices << " devices to process " << running_process.at(0) << endl;
    cout << "Moving process " << running_process.at(0) << " to waiting queue" << endl;
    running_process.push_back(devices); //Add the number of devices requested to the end of the stored vector to be accessed later
    wait_queue.push_back(running_process);
    running_process.clear();
    return false;
  }


}

//This is the algorithm to use bankers on the processes in the waiting queue. This has to be a separate function since
//the processes in the waiting queue have an extra field saying how many devices they're waiting on.
//
//It probably could've been implemented as one function if I tried hard enough but i was kinda lazy to be honest
bool checkBankersHolding(vector<int> input_process){
  
  vector<int> held;
  vector<int> need;
  vector<bool> finish;
  int devices = input_process.at(5);
  int available = sys_devices_remaining;

  cout << "Checking to see if we can continue " << input_process.at(0) << " which requested " << devices << " devices (System has " << sys_devices_remaining << ")" << endl;

  //Populate held and max vectors with the values for each running process
  held.push_back(input_process.at(1));
  need.push_back(input_process.at(4) - input_process.at(1));

  queue<vector<int>> temp_ready_queue = ready_queue;
  while(temp_ready_queue.size() > 0){
    vector<int> temp_process = temp_ready_queue.front();
    held.push_back(temp_process.at(1));
    need.push_back(temp_process.at(4) - temp_process.at(1));
    finish.push_back(false);
    temp_ready_queue.pop();
  }

  int iteration = 0;
  while(iteration < finish.size()){

    if(finish[iteration] == false && need[iteration] <= available){
      finish[iteration] = true;
      available += held[iteration];
      iteration = 0;
    }
    else
      iteration++;
    

  }

  //Check if all finish values are total
  bool all_true = true;
  for(int count = 0; count < finish.size(); count++){
    if(finish[count] == false)
      all_true = false;
  }
  
  if(all_true && devices <= sys_devices_remaining){
    cout << "Bankers Algorithm Successful: Restarting Process: " << input_process.at(0) << endl;
    sys_devices_remaining = sys_devices_remaining - devices;
    return true;
  }

  else{
    cout << "Bankers Algorithm Unsuccessful: Couldn't restart process: " << input_process.at(0) << endl;
    return false;
  }

}

void releaseDevices(int devicesToRelease){

  sys_devices_remaining += devicesToRelease;
  int iteration = 0;
  while(iteration < wait_queue.size()){

    bool canResume = checkBankersHolding(wait_queue[iteration]);

    //Add to the ready queue and delete from the wait queue
    if(canResume){
      wait_queue[iteration].at(1) += wait_queue[iteration].at(5); //Allocate the number of devices it needed!
      wait_queue[iteration].erase(wait_queue[iteration].begin() + 5); //Removes the extra int value we added to keep track of how many devices it needed!
      ready_queue.push(wait_queue[iteration]);
      wait_queue.erase(wait_queue.begin() + iteration);
    }

    //Otherwise, increment the iterator
    else{
      iteration++;
    }

  }

}

void displayReadyQueue(void){
  //Iteratively print the ready queue
  cout << endl << "READY QUEUE: " << endl;
  queue<vector<int>> temp_queue = ready_queue;
  while (temp_queue.size() > 0) {
    vector<int> myVector = temp_queue.front();
    cout << "pid:\t\t" << myVector.at(0) << endl;
    cout << "Devices Held:\t" << myVector.at(1) << endl;
    cout << "Remaining Time:\t" << myVector.at(2) << endl;
    cout << "Req'd Memory:\t" << myVector.at(3) << endl;
    cout << "Max Devices:\t" << myVector.at(4) << endl << endl;
    temp_queue.pop();
  }
}
