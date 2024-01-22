#include <stdio.h>

int main(int argc, char *argv[]) {
    char* path;
    if (argc != 2) {
        fprintf( stderr, "Failed: Program should start with 1 arguments.\n" );
        return -1;
    }
    path = argv[1];
    printf("Given input: path = %s\n", path);

    FILE *kernel_module_args = fopen("/sys/kernel/debug/df_stat/kernel_module_args", "w+");
    if (kernel_module_args == NULL) {
        fprintf(stderr, "File with args not found.\n");
        return -1;
    }
    fprintf(kernel_module_args, "path: %s", path);
    fprintf(stdout, "Args have been written to file.\n");
    fclose(kernel_module_args);

    FILE *kernel_module_result = fopen("/sys/kernel/debug/df_stat/kernel_module_result", "r");
    if (kernel_module_result == NULL) {
        fprintf(stderr, "File with result not found.\n");
        return -1;
    }
    char* result;
    while (fscanf(kernel_module_result, "%s", result) != EOF) {
        printf("Result:\n%s", result);
    }
    fclose(kernel_module_result);
    return 0;
}


