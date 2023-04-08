//Headers
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//Structure to store the final ouput
//Modified struct final to allow for dynamic string length for Name
typedef struct final {
  char * Name;
  int flie_size;
} final;

//Structure to store data for individual lines
//Modified struct final to allow for dynamic string lengths for CNet, and file
typedef struct linebyline {
    char * CNet;
    char * file;
    int bytecount;
} linebyline;

//Function to compute the size of each variable in a file
int size_of_var(char * token) {
    int val = 0;
    if (strcmp(token,"char") == 0){
        //sizeof() function used to compute data type size
        val = sizeof(char);    
        }
    else if (strcmp(token,"int") == 0) {
        val = sizeof(int); 
        }
    else if (strcmp(token,"float") == 0) {
        val = sizeof(float); 
        }
    else if (strcmp(token,"double") == 0) {
        val = sizeof(double); 
        }
    return val;
}

//Function to find out number of users and the number of lines from the first line of the text file
int find_user_data(char * line,int *users, int *no_of_lines){
    int token_no = 0;
    char * token = strtok(line, " ");
    while(token != NULL){
        if (token_no == 0){
            *users = atoi(token);
        }
        else if (token_no == 1){
            *no_of_lines = atoi(token);
        }
        token = strtok(NULL, " ");
        token_no += 1; 
        }
    return 0;      
}

//Function to remove duplicate entries from ptr1 array and store values in ptr2
//Uses strcmp() for string comparisons, and strcpy() for string copy
void save_unique_users(linebyline *ptr1,final *ptr2, int users, int no_of_lines){
    int index = 0;
    for (int i=0; i< no_of_lines; i++){
        if (strcmp((ptr1+ i)->CNet, "NULL") == 0){
            continue;
        }
        else{
            int storesum = (ptr1+ i)->bytecount;
            for (int j = i+1; j< no_of_lines; j++){
                if (strcmp((ptr1+ i)->CNet, (ptr1+ j)->CNet) == 0){
                    storesum += (ptr1+ j)->bytecount;
                    strcpy((ptr1 + j)->CNet, "NULL");
                }
            }
            ptr2[index].Name = malloc(sizeof(char)*strlen((ptr1 + i)->CNet));
            strcpy((ptr2 + index)->Name, (ptr1 + i)->CNet);
            (ptr2+ index)->flie_size = storesum;
            index += 1;
        }      
    }
    for (int i = 0; i < users; ++i) {
    printf("%s\t%d\n", (ptr2 + i)->Name, (ptr2 + i)->flie_size);
  }
}

//Function to match users with the given CNEY ID
void user_file_check(linebyline *ptr1, int no_of_lines, char * CNet){
    int flag = 0;
    for (int i=0; i< no_of_lines; i++){
        (ptr1+ i)->CNet[strcspn((ptr1+ i)->CNet, "!")] = 0;
            if (strcmp((ptr1+ i)->CNet, CNet) == 0){
                ++flag;
                printf("%s\t %s\t %d\n", (ptr1 + i)->CNet, (ptr1 + i)->file, (ptr1 + i)->bytecount);
        }
    }
    if (flag == 0){
        printf("User not found\n");
    }
        
}

//Function to tokenize the string in all n-1 lines, and compute the variable sizes using size_of_var function
void file_size_compute(struct linebyline *ptr1, char * line, int line_no){
    int sum_of_line = 0;
    int token_count = 0;
    char * cnetid; 
    char * filename;
    char * token = strtok(line, " ");
    while (token != NULL) {
        //strcspn() function used for removing trailing whitespace
        //Citation : https://www.tutorialspoint.com/c_standard_library/c_function_strcspn.htm
        token[strcspn(token, "\n")] = 0;
        if (token_count == 0){
            cnetid = token;   
        }
        else if (token_count == 1){
            filename = token;
        }
        else{
            sum_of_line += size_of_var(token);
        }
        token = strtok(NULL, " ");
        token_count += 1;
    }
    ptr1[line_no -1].CNet = malloc(sizeof(char)*strlen(cnetid));
    strcpy((ptr1 + line_no -1)->CNet, cnetid);
    ptr1[line_no -1].file = malloc(sizeof(char)*strlen(filename));
    strcpy((ptr1 + line_no -1)->file, filename);
    (ptr1+ line_no-1)->bytecount = sum_of_line;
}

//Main function
int main(int argc, char *argv[]) {
    //Instantiated an object of struct linebyline
    struct linebyline *ptr1;
    //Instantiated an object of struct final
    struct final *ptr2;
    char *line = NULL;
    long int len = 0;
    long nRead = getline(&line, &len, stdin);
    int line_no = 0;
    int users, no_of_lines;
    int sum_of_line, token_count;
    //Tokenized the string input and reading it line by line
    while ( nRead != -1) {
        if (line_no == 0) {
            find_user_data(line, &users, &no_of_lines); 
            //Allocated memory for the structs
            ptr1 = (struct linebyline *)malloc(no_of_lines * sizeof(struct linebyline));
            ptr2 = (struct final *)malloc(users * sizeof(struct final));
        }
        else {
            //Function file_size_compute called 
            file_size_compute(ptr1, line, line_no);
        }
        line_no += 1;
        //line variable freed at every iteration
        free(line);
        line = NULL;
        nRead = getline(&line, &len, stdin);
    }
    if (argc == 2){
        user_file_check(ptr1, no_of_lines, argv[1]);
    }
    else if(argc == 1){
        save_unique_users(ptr1,ptr2,users,no_of_lines);
    }
    
    return 0;
}