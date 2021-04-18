#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "file_sys_32.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Invalid Arguments Entered\n");
        exit(EXIT_FAILURE);
    }

    openDisk(argv[1]);
    initializeStructs();
    setRootDirectory();

    if (!strcmp(argv[2], "info"))
    {
        printf("%s Information:\n", argv[1]);
        // print drive info
        deviceInfo();
    }
    else if (!strcmp(argv[2], "list"))
    {
        printf("%s Data List:\n", argv[1]);
        // list data
        print_directory_details();
    }
    else if (!strcmp(argv[2], "get"))
    {
        if (argc < 4)
        {
            printf("Filename not Entered\n");
            exit(EXIT_FAILURE);
        }
        printf("Getting %s in %s:\n", argv[3], argv[1]);
        // get file
        get_file_from_current_directory(argv[3]);
    }
    else
    {
        printf("%s is not a valid command. The valid commands are \'info\', \'list\', or \'get\'.\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    return 0;
}
