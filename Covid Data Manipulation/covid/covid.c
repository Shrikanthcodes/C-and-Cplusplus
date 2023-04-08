//Header files used for File I/O and string manipulation
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//Struct data that has 2 instances data_entry and output
typedef struct data {
  int zip;
  int month;
  int year;
  int cases;
  int tests;
  int deaths;
} data;

//Function to compute answers based on input file
char * compute(int zip, int month, int year, int index, int count, data *data_entry, data *output){
    for (int i=0; i<count; i++){
        if (zip == data_entry[i].zip){
            if(month == data_entry[i].month){
                if(year == data_entry[i].year){
                    output[index].cases = output[index].cases + data_entry[i].cases;
                    output[index].tests = output[index].tests + data_entry[i].tests;
                    output[index].deaths = output[index].deaths + data_entry[i].deaths;
                }
            }
        }
    }
    return 0;
}

//Function to get month from datestring
int get_month(char input[], int index) {
    char sub[2];
    sub[0] = input[index];
    sub[1] = input[index+1];
    int month = atoi(sub);
    return month;
}

//Function to get year from datestring
int get_year(char input[], int index) {
    char sub[4];
    sub[0] = input[index];
    sub[1] = input[index+1];
    sub[2] = input[index+2];
    sub[3] = input[index+3];
    int year = atoi(sub);
    return year;
}

//Function to write to outfile
void write_file(data* output, char* path, int size) {
    FILE * fout;
    fout = fopen(path, "w");
    if (fout != NULL) {
        for (int i = 0; i < size; i++) {
            fprintf(fout, "%d %d %d = %d,%d,%d\n", output[i].zip, output[i].month, output[i].year, output[i].cases, output[i].tests, output[i].deaths);
        }
        fclose(fout); 
    } 
    else {
        perror("fopen"); 
        exit(EXIT_FAILURE);
    }
}

        
//Function to search and compare records to find unique
int search(char * key, char * uniqueRecords[], int count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(uniqueRecords[i], key) == 0) {   
            return -1;
        }
    }
    return 1;
}

//Function for data entry into struct data * data_entry
int data_entry_struct(char * record[], data * data_entry, int count){
    int month = get_month(record[2],0);
    int year = get_year(record[2],6);
    int zip = atoi(record[0]);
    int cases = atoi(record[4]);
    int tests = atoi(record[8]);
    int deaths = atoi(record[14]);
    (data_entry+ count)->zip = zip;
    (data_entry+ count)->month = month;
    (data_entry+ count)->year = year;
    (data_entry+ count)->cases = cases;
    (data_entry+ count)->tests = tests;
    (data_entry+ count)->deaths = deaths;
    return 0;
}

//Function to count records and save unique records 
int countRecord(char * recordLine, char * uniqueRecords[], int *newCount, data *data_entry, data *output) {
    char * record[21];
    int count = 0;
    char * key;
    char * token = strtok(recordLine, ",");
    while (token != NULL) {
        record[count++] = token;
        token = strtok(NULL, ",");
    }
    if(count == 21){
        key = malloc(sizeof(char)*strlen(record[0]) + sizeof(char)*strlen(record[2])+ sizeof(char)*strlen(record[4])+ 
        sizeof(char)*strlen(record[8])+ sizeof(char)*strlen(record[14])+1);
        sprintf(key,"%s%s%s%s%s",record[0],record[2],record[4],record[8],record[14]);
        if(search(key, uniqueRecords, *newCount) == 1){
            uniqueRecords[*newCount] = malloc(sizeof(char)*strlen(key));
            uniqueRecords[*newCount] = key;
            data_entry_struct(record, data_entry, *newCount);
            *newCount += 1;
        }
    return 0;
    }
}

int main(int argc, char* argv[]){
    //Defining constants for file I/O
    const char * filename = "../data/covid_";
    const char * fileext = ".csv";
    const int no_of_files = 15;

    //Allocating memory for infile and outfile destination strings
    char* infile = malloc(strlen(argv[1])*sizeof(char));
    char* outfile = malloc(strlen(argv[2])*sizeof(char));
    strcpy(infile, argv[1]);
    strcpy(outfile, argv[2]);
    
    // Number of sets of input given in the input file
    int data_entry_prompts = 0;

    //Memory allocation for output struct
    data* output = malloc(sizeof(data) * 100);

    
    // Instantiate a file pointer
    FILE* fileP;
    fileP = fopen(infile, "r");
    
    // While File pointer is not null
    if (fileP != NULL) {
        int num = 0;
        int index = 0;
        while(fscanf(fileP, "%d", &num) != EOF) {
            data prompt;
            if (index == 0) {
                prompt.zip = num;
                index++;
            }
            else if (index == 1) {
                prompt.month = num;
                index++;
            }
            else if (index == 2) {
                //Saving zipcode, month and year as given in prompt
                prompt.year = num;
                //Resetting index
                index = 0;
                output[data_entry_prompts].zip = prompt.zip;
                output[data_entry_prompts].month = prompt.month;
                output[data_entry_prompts++].year = prompt.year;
            }
        }
        fclose(fileP);
    } 
    else {
        //File error condition
        perror("fopen"); 
        exit(EXIT_FAILURE); 
    }

    printf("\n");
    
    //Instantiate another object of struct data_entry
    data * data_entry;
     
    //Define buffer 
    char buffer[256]; 

    for (int i = 0; i < sizeof(buffer); i++){
        buffer[i] = 'z'; 
    }

    //Define integer to store unique entry count
    int uniqueCount;
    char * uniqueRecords[8341];
    //Allocate memory
    data_entry = (struct data *)malloc(8341 * sizeof(struct data));
    int number;
    for (int i=1; i<no_of_files+1; i++){
        FILE * fd;
        char int_str[3];
        char * name;
        //Convert i to string
        sprintf(int_str, "%d", i);
        name = malloc(sizeof(char)*strlen(filename) + sizeof(char)*strlen(int_str)+ sizeof(char)*strlen(fileext));
        //Concatenate string to show path location
        strcat(name, filename);
        strcat(name, int_str);
        strcat(name, fileext);

        //File opening
        fd = fopen(name, "r");
        if (fd != NULL){
            char *line = NULL;
            long int len = 0;
            long nRead = getline(&line, &len, fd);
            while ( nRead != -1) {
                sscanf(line,"%s %d",buffer,&number);  
                //Call function to start storing unique entries into struct
                countRecord(buffer, uniqueRecords, &uniqueCount, data_entry, output);
                //Free line pointer
                free(line);
                line = NULL;
                nRead = getline(&line, &len, fd);
                }
        }
    else{
        perror("open");
        return EXIT_FAILURE;
    }
    //Deallocate memory
    free(name);
    
}
//Iterate over input file to find prompts
for (int m=0; m<data_entry_prompts; m++){
    //Call compute function
    compute(output[m].zip, output[m].month, output[m].year, m, uniqueCount, data_entry, output);
}
//Call function to write to outfile
write_file(output, outfile, data_entry_prompts);

//Free open pointers
free(data_entry);
free(output);
}





