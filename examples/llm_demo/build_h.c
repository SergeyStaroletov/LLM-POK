#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input.bin> <output.h> <array_name>\n", argv[0]);
        return 1;
    }
    char *input_filename = argv[1];
    char *output_filename = argv[2];
    char *array_name = argv[3];

    FILE *fin = fopen(input_filename, "rb");
    if (!fin) {
        perror("fopen input");
        return 1;
    }
    FILE *fout = fopen(output_filename, "w");
    if (!fout) {
        perror("fopen output");
        fclose(fin);
        return 1;
    }

    fprintf(fout, "/* Auto-generated from %s */\n", input_filename);
    fprintf(fout, "#ifndef %s_H\n", array_name);
    fprintf(fout, "#define %s_H\n", array_name);
    fprintf(fout, "#include <stdint.h>\n");
    fprintf(fout, "static const uint8_t %s[] = {", array_name);

    int c;
    int count = 0;
    while ((c = fgetc(fin)) != EOF) {
        if (count % 12 == 0) {
            fputs("\n    ", fout);
        }
        fprintf(fout, "0x%02X, ", c);
        count++;
    }
    fprintf(fout, "\n};\n");
    fprintf(fout, "static const size_t %s_size = sizeof(%s);\n", array_name, array_name);
    fprintf(fout, "#endif /* %s_H */\n", array_name);

    fclose(fin);
    fclose(fout);
    printf("Successfully generated %s (%d bytes)\n", output_filename, count);
    return 0;
}
