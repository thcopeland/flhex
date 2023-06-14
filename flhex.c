#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 0

#define EMPTY_VAL 0xff
#define LINE_BUFF_SIZE 256

#define ERROR(...) fprintf(stderr, "fhlex: " __VA_ARGS__)

struct hexdata {
    uint32_t capacity;  // allocated memory size
    uint32_t size;      // populated memory size
    uint8_t *data;      // memory
    uint8_t empty;      // unpopulated value
    uint8_t width;      // record byte count while writing
};

void *xmalloc(size_t size);
void *xcalloc(size_t nitems, size_t size);
void *xrealloc(void *ptr, size_t size);

struct hexdata *hexdata_new(uint8_t empty);
void hexdata_resize(struct hexdata *hdata, uint32_t desired);
void hexdata_write(struct hexdata *hdata, uint32_t addr, uint8_t val);
void hexdata_free(struct hexdata *hdata);
int hexdata_load(struct hexdata *hdata, FILE *file);
int hexdata_dump(struct hexdata *hdata, FILE *file);

void *xmalloc(size_t size) {
    void *data = malloc(size);
    if (data == NULL) {
        ERROR("memory allocation failed (malloc)\n");
        exit(1);
    }
    return data;
}

void *xcalloc(size_t nitems, size_t size) {
    void *data = calloc(nitems, size);
    if (data == NULL) {
        ERROR("memory allocation failed (calloc)\n");
        exit(1);
    }
    return data;
}

void *xrealloc(void *ptr, size_t size) {
    void *data = realloc(ptr, size);
    if (data == NULL) {
        ERROR("memory allocation failed (realloc)\n");
        exit(1);
    }
    return data;
}

struct hexdata *hexdata_new(uint8_t empty) {
    struct hexdata *hdata = xmalloc(sizeof(*hdata));
    hdata->capacity = 64*1024;
    hdata->size = 0;
    hdata->data = malloc(sizeof(hdata->data[0])*hdata->capacity);
    hdata->empty = empty;
    hdata->width = 16;
    memset(hdata->data, hdata->empty, hdata->capacity);
    return hdata;
}

void hexdata_resize(struct hexdata *hdata, uint32_t desired) {
    if (desired > hdata->capacity) {
        // ensure a power of 2
        if (desired & (desired - 1)) {
            desired |= desired >> 16;
            desired |= desired >> 8;
            desired |= desired >> 4;
            desired |= desired >> 2;
            desired |= desired >> 1;
            desired += 1;
        }
        hdata->data = xrealloc(hdata->data, sizeof(hdata->data[0])*desired);
        memset(hdata->data + hdata->capacity, hdata->empty, desired - hdata->capacity);
        hdata->capacity = desired;
    }
}

void hexdata_write(struct hexdata *hdata, uint32_t addr, uint8_t val) {
    if (addr >= hdata->capacity) {
        hexdata_resize(hdata, addr+1);
    }
    hdata->data[addr] = val;
    if (hdata->size <= addr) {
        hdata->size = addr+1;
    }
}

void hexdata_free(struct hexdata *hdata) {
    if (hdata) {
        free(hdata->data);
        free(hdata);
    }
}

int hexdata_load(struct hexdata *hdata, FILE *file) {
    char buff[LINE_BUFF_SIZE];
    uint32_t line = 1,
             base_addr = 0;

    while (fgets(buff, LINE_BUFF_SIZE, file)) {
        uint32_t val, addr, count, type, partition_size;
        // read the record header
        if (sscanf(buff, ":%2X%4X%2X", &count, &addr, &type) != 3) {
            ERROR("malformed header on line %d\n", line);
            return 1;
        }
        uint8_t i = 9;
        uint8_t checksum = count + addr + (addr >> 8) + type;

        if (count > hdata->width && count <= 256) {
            hdata->width = count;
        }

        // read and load the record data
        switch(type) {
            case 0x00: // data
                addr += base_addr;
                partition_size += count;
                while (count--) {
                    sscanf(buff+i, "%2X", &val);
                    hexdata_write(hdata, addr++, val);
                    checksum += val;
                    i += 2;
                }
                break;
            case 0x01: // end of file
                return 0;
            case 0x02: // extended segment address
                sscanf(buff+i, "%4X", &val);
                checksum += val + (val>>8);
                base_addr = (val << 4);
                i += 4;
                break;
            case 0x03: // start segment address, ignored
                break;
            case 0x04: // extended linear address
                sscanf(buff+i, "%4X", &val);
                checksum += val + (val>>8);
                base_addr &= 0xffff0000;
                base_addr |= (val << 16);
                i += 4;
                break;
            case 0x05: // start linear address, unsupported
            default:
                ERROR("unsupported record type %02X on line %d\n", type, line);
                return 1;
        }

        // test checksum
        sscanf(buff+i, "%2X", &val);
        if ((val + checksum) & 0xff) {
            ERROR("checksum failed on line %d (0x%02X != 0x%02X)\n", line, (-checksum) & 0xff, val);
            return 1;
        }

        line++;
    }

    return 0;
}

void write_record(FILE *file, uint8_t count, uint16_t addr, uint8_t type, uint8_t *data) {
    uint8_t checksum = count + addr + (addr >> 8) + type;
    for (uint8_t i = 0; i < count; i++) {
        checksum += data[i];
    }
    checksum = -checksum;
    fprintf(file, ":%02X%04X%02X", count, addr, type);
    for (uint8_t i = 0; i < count; i++) {
        fprintf(file, "%02X", data[i]);
    }
    fprintf(file, "%02X\n", checksum);
}

int hexdata_dump(struct hexdata *hdata, FILE *file) {
    uint32_t addr = 0;
    while (addr < hdata->size) {
        if ((addr & 0x0000ffff) == 0 && addr < 0x100000) {
            uint8_t data[2] = { (addr >> 12) & 0xf0, 0 };
            write_record(file, sizeof(data)/sizeof(data[0]), 0x0000, 0x02, data); // extended segment address
        } else if ((addr & 0x0000ffff) == 0 && addr != 0) {
            uint8_t data[2] = { addr >> 24, (addr >> 16) & 0xff };
            write_record(file, sizeof(data)/sizeof(data[0]), 0x0000, 0x04, data); // extended linear address
        }
        uint8_t *data = hdata->data+addr;
        uint8_t size = hdata->width;
        if (addr + size > hdata->size) {
            size = hdata->size - addr;
        }
        write_record(file, size, addr & 0x0000ffff, 0x00, data); // data
        addr += size;
    }
    write_record(file, 0, 0x0000, 0x01, NULL); // end of file
    return 0;
}

void print_usage_msg(void) {
    printf("Usage: flhex [OPTIONS] FILE\n");
}

void print_help_msg(void) {
    print_usage_msg();
    printf("Flatten an Intel HEX file so that there are no gaps between bytes. This\n"
           "can be used to normalize HEX files, or to work around bootloader bugs.\n"
           "\n"
           "      --count N            per-record byte count (by default, will match input)\n"
           "  -h, --help               display this help message\n"
           "      --padding N          padding value (default 255)\n"
           "  -o, --output FILE        output file (default out.hex)\n"
           "  -v, --version            print version\n");
}

int main(int argc, const char **argv) {
    if (argc < 2) {
        print_usage_msg();
        return 0;
    }

    const char *input = NULL;
    const char *output = "out.hex";
    uint8_t padding = EMPTY_VAL;
    int count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--count") == 0) {
            if (i+1 >= argc) {
                ERROR("expected argument for `%s'", argv[i]);
                return 1;
            }
            count = atoi(argv[++i]);
            if (count > 255) {
                ERROR("per-record byte count may not exceed 255 (got %d)\n", count);
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help_msg();
            return 0;
        } else if (strcmp(argv[i], "--padding") == 0) {
            if (i+1 >= argc) {
                ERROR("expected argument for `%s'", argv[i]);
                return 1;
            }
            padding = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i+1 >= argc) {
                ERROR("expected argument for `%s'", argv[i]);
                return 1;
            }
            output = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("flhex v%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
            return 0;
        } else if (argv[i][0] == '-') {
            ERROR("invalid option `%s'\nTry `flhex --help' for more information.\n", argv[i]);
            return 1;
        } else if (input == NULL) {
            input = argv[i];
        } else {
            ERROR("unexpected file `%s'\n", argv[i]);
            return 1;
        }
    }

    if (input == NULL) {
        ERROR("expected an input file\n");
        return 1;
    }

    struct hexdata *hdata = hexdata_new(padding);
    // read HEX file
    FILE *f = fopen(input, "r");
    if (f == NULL) {
        ERROR("%s (%s)\n", strerror(errno), input);
        return 1;
    }

    hexdata_load(hdata, f);
    fclose(f);

    // override with specified value
    if (count > 0) {
        hdata->width = count;
    }

    // write HEX file
    f = fopen(output, "w");
    if (f == NULL) {
        ERROR("%s (%s)\n", strerror(errno), input);
        return 1;
    }
    hexdata_dump(hdata, f);
    fclose(f);

    // clean up
    hexdata_free(hdata);

    return 0;
}
