#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_FILES 32
#define MAX_TOTAL_SIZE (200 * 1024 * 1024) // 200 MBytes

void error_exit(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}

int is_text_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return 0; // Unable to open the file
    }

    int c;
    while ((c = fgetc(file)) != EOF && c >= 0 && c <= 127)
        ;

    fclose(file);

    return (c == EOF); // If c is EOF, it's a text file
}

void write_organization_info(FILE *output_file, const char *file_name, int size) {
    struct stat file_stat;
    if (stat(file_name, &file_stat) != 0) {
        fclose(output_file);
        error_exit("Error: Unable to get file permissions");
    }

    char permissions[10];
    snprintf(permissions, sizeof(permissions), "%o", file_stat.st_mode & 0777);

    fprintf(output_file, "|%s,%s,%d|", file_name, permissions, size);
}

void write_archived_file(FILE *output_file, const char *filename) {
    FILE *input_file = fopen(filename, "r");
    int c;

    while ((c = fgetc(input_file)) != EOF) {
        fputc(c, output_file);
    }

    fclose(input_file);
}

void extract_archive(const char *archive_filename, const char *output_directory) {
    FILE *archive_file = fopen(archive_filename, "r");
    if (!archive_file) {
        error_exit("Error: Archive file is inappropriate or corrupt!");
    }

    int org_size;
    if (fscanf(archive_file, "%010d", &org_size) != 1) {
        fclose(archive_file);
        error_exit("Error: Unable to read the size of the organization section");
    }

    struct stat st = {0};
    if (stat(output_directory, &st) == -1) {
        if (mkdir(output_directory, 0700) != 0) {
            fclose(archive_file);
            error_exit("Error: Unable to create output directory");
        }
    }

    if (chdir(output_directory) != 0) {
        fclose(archive_file);
        error_exit("Error: Unable to change to output directory");
    }

    // Move the file pointer forward to skip the first character after the organization size
    fseek(archive_file, 1, SEEK_CUR);

    while (1) {
        char file_name[256];
        char permissions[10];
        int size;

        if (fscanf(archive_file, "%[^,],%[^,],%d|", file_name, permissions, &size) == 3) {
            FILE *output_file = fopen(file_name, "w");
            if (!output_file) {
                fclose(archive_file);
                error_exit("Error: Unable to create output file");
            }

            chmod(file_name, strtol(permissions, 0, 8));

            // Move the file pointer forward to skip the '|'
            fseek(archive_file, 1, SEEK_CUR);

            // Read and write the contents of the file
            char buffer[1024];

            size_t bytesRead;
            size_t totalBytesRead = 0;

            while (totalBytesRead < size) {
                bytesRead = fread(buffer, 1, sizeof(buffer), archive_file);

                if (bytesRead == 0) {
                    fclose(output_file);
                    fclose(archive_file);
                    error_exit("Error: Unexpected end of archive file");
                }

                fwrite(buffer, 1, bytesRead, output_file);
                totalBytesRead += bytesRead;
            }

            fclose(output_file);
        } else {
            break; // Empty line or invalid entry; stop loop
        }
    }

    fclose(archive_file);
    printf("Archive extracted successfully to %s\n", output_directory);
}

int main(int argc, char *argv[]) {
    FILE *output_file;
    char *output_filename = "a.sau";
    int total_size = 0;

    if (argc < 3 || argc > (2 + MAX_FILES * 2)) {
        error_exit("Usage: tarsau -b [-o output_file] file1 [file2 ...] | -a archive_file output_directory");
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-b") == 0) {
            continue;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_filename = argv[i + 1];
                i++;
            } else {
                error_exit("Error: No output file specified after -o");
            }
        } else if (strcmp(argv[i], "-a") == 0) {
            if (i + 2 < argc) {
                const char *archive_filename = argv[i + 1];
                const char *output_directory = argv[i + 2];
                i += 2;

                extract_archive(archive_filename, output_directory);
                return 0;
            } else {
                error_exit("Error: Missing parameters after -a");
            }
        } else {
            int is_text = is_text_file(argv[i]);

            if (!is_text) {
                error_exit("Error: Input file format is incompatible!");
            }

            FILE *input_file = fopen(argv[i], "r");

            if (!input_file) {
                error_exit("Error: Unable to open input file");
            }

            int c;
            while ((c = fgetc(input_file)) != EOF && total_size < MAX_TOTAL_SIZE) {
                total_size++;
            }

            fclose(input_file);
        }
    }

    if (total_size > MAX_TOTAL_SIZE) {
        error_exit("Error: Total size of input files exceeds 200 MBytes");
    }

    output_file = fopen(output_filename, "w");

    if (!output_file) {
        error_exit("Error: Unable to create output file");
    }

    fseek(output_file, 0, SEEK_SET);
    fprintf(output_file, "%010ld", (long int)(ftell(output_file) + 10));

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-b") != 0 && strcmp(argv[i], "-o") != 0 && strcmp(argv[i], "-a") != 0) {
            FILE *input_file = fopen(argv[i], "r");
            int file_size = 0;
            int c;

            while ((c = fgetc(input_file)) != EOF) {
                file_size++;
            }

            fclose(input_file);

            write_organization_info(output_file, argv[i], file_size);
            write_archived_file(output_file, argv[i]);
        }
    }

    fclose(output_file);

    printf("Archive created successfully: %s\n", output_filename);

    return 0;
}