#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

// КОНСТАНТЫ ДЛЯ ОБРАБОТКИ UNICODE И ОШИБОК


#define UNICODE_REPLACEMENT 0xFFFD          
#define INVALID_UTF8_START 0xDC80           
#define INVALID_UTF8_END   0xDCFF           

#define UTF8_CONTINUATION_BYTE(b) (((b) & 0xC0) == 0x80)


// ПЕРЕЧИСЛЕНИЕ ДЛЯ ПОДДЕРЖИВАЕМЫХ КОДИРОВОК


typedef enum {
    ENC_UTF8_NOBOM = 0,   
    ENC_UTF8_BOM    = 1,  
    ENC_UTF16LE     = 2,  
    ENC_UTF16BE     = 3,  
    ENC_CP1251      = 4,  
    ENC_CP866       = 5,  
    ENC_KOI8R       = 6   
} Encoding;


// СТРУКТУРА ДЛЯ ПРЕДСТАВЛЕНИЯ СИМВОЛА UNICODE


typedef struct {
    uint32_t code;      // Unicode code point (кодовая точка)
    size_t src_size;    // Количество байт, которые занимал символ в исходной кодировке
} UChar;

// ТАБЛИЦЫ ПРЕОБРАЗОВАНИЯ ДЛЯ ОДНОБАЙТОВЫХ КОДИРОВОК


// CP1251 (Windows-1251) - кириллица для Windows
// Таблица преобразования байтов 0x80-0xFF в Unicode
static const uint16_t table_cp1251[128] = {
    0x0402,0x0403,0x201A,0x0453,0x201E,0x2026,0x2020,0x2021,
    0x20AC,0x2030,0x0409,0x2039,0x040A,0x040C,0x040B,0x040F,
    0x0452,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
    0x0000,0x2122,0x0459,0x203A,0x045A,0x045C,0x045B,0x045F,
    0x00A0,0x040E,0x045E,0x0408,0x00A4,0x0490,0x00A6,0x00A7,
    0x0401,0x00A9,0x0404,0x00AB,0x00AC,0x00AD,0x00AE,0x0407,
    0x00B0,0x00B1,0x0406,0x0456,0x0491,0x00B5,0x00B6,0x00B7,
    0x0451,0x2116,0x0454,0x00BB,0x0458,0x0405,0x0455,0x0457,
    0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
    0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
    0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
    0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
    0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
    0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
    0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
    0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x044F
};

// CP866 (DOS Cyrillic) - кириллица для операционной системы DOS
static const uint16_t table_cp866[128] = {
    0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
    0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
    0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
    0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
    0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
    0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
    0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,
    0x2555,0x2563,0x2551,0x2557,0x255D,0x255C,0x255B,0x2510,
    0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x255E,0x255F,
    0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x2567,
    0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256B,
    0x256A,0x2518,0x250C,0x2588,0x2584,0x258C,0x2590,0x2580,
    0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
    0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x044F,
    0x0401,0x0451,0x0404,0x0454,0x0407,0x0457,0x0406,0x0456,
    0x0490,0x0491,0x040E,0x045E,0x0408,0x0458,0x00A0,0x00B0
};

// KOI8-R - кириллица для Unix-систем
static const uint16_t table_koi8r[128] = {
    0x2500,0x2502,0x250C,0x2510,0x2514,0x2518,0x251C,0x2524,
    0x252C,0x2534,0x253C,0x2580,0x2584,0x2588,0x258C,0x2590,
    0x2591,0x2592,0x2593,0x2320,0x25A0,0x2219,0x221A,0x2248,
    0x2264,0x2265,0x00A0,0x2321,0x00B0,0x00B2,0x00B7,0x00F7,
    0x2550,0x2551,0x2552,0x0451,0x2553,0x2554,0x2555,0x2556,
    0x2557,0x2558,0x2559,0x255A,0x255B,0x255C,0x255D,0x255E,
    0x255F,0x2560,0x2561,0x0401,0x2562,0x2563,0x2564,0x2565,
    0x2566,0x2567,0x2568,0x2569,0x256A,0x256B,0x256C,0x00A9,
    0x044E,0x0430,0x0431,0x0446,0x0434,0x0435,0x0444,0x0433,
    0x0445,0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,
    0x043F,0x044F,0x0440,0x0441,0x0442,0x0443,0x0436,0x0432,
    0x044C,0x044B,0x0437,0x0448,0x044D,0x0449,0x0447,0x044A,
    0x042E,0x0410,0x0411,0x0426,0x0414,0x0415,0x0424,0x0413,
    0x0425,0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,
    0x041F,0x042F,0x0420,0x0421,0x0422,0x0423,0x0416,0x0412,
    0x042C,0x042B,0x0417,0x0428,0x042D,0x0429,0x0427,0x042A
};


// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ РАБОТЫ С БАЙТАМИ


// Функция для перестановки байтов в 16-битном слове (меняет порядок с LE на BE или наоборот)
static uint16_t swap16(uint16_t v) { return (v >> 8) | (v << 8); }

// Определяет длину UTF-8 последовательности по первому байту
// Возвращает: 1 для ASCII (0xxxxxxx), 2, 3, 4 для многобайтных последовательностей,
// или -1 если первый байт некорректен
static int utf8_sequence_length(uint8_t first_byte) {
    if (first_byte < 0x80) return 1;               // 0xxxxxxx - ASCII символ
    if ((first_byte & 0xE0) == 0xC0) return 2;     // 110xxxxx - 2 байта
    if ((first_byte & 0xF0) == 0xE0) return 3;     // 1110xxxx - 3 байта
    if ((first_byte & 0xF8) == 0xF0) return 4;     // 11110xxx - 4 байта
    return -1;                                     // Некорректный первый байт
}

// Проверяет корректность UTF-8 последовательности и извлекает code point
// Возвращает 1 если последовательность корректна, 0 если нет
// В code_point записывается декодированное значение Unicode
static int is_valid_utf8_sequence(const uint8_t *bytes, int length, uint32_t *code_point) {
    if (length < 1 || length > 4) return 0;  // Длина UTF-8 последовательности от 1 до 4 байт
    
    // Однобайтовая последовательность (ASCII)
    if (length == 1) {
        *code_point = bytes[0];
        return (bytes[0] <= 0x7F);  // ASCII символы должны быть <= 0x7F
    }
    
    // Проверяем continuation байты (должны начинаться с 10xxxxxx)
    for (int i = 1; i < length; i++) {
        if (!UTF8_CONTINUATION_BYTE(bytes[i])) return 0;
    }
    
    uint32_t cp = 0;
    
    // Декодируем последовательности разной длины с проверкой на overlong encoding
    if (length == 2) {
        // 2-байтовая последовательность: 110xxxxx 10xxxxxx
        cp = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
        // Проверка на overlong encoding: должно кодировать символы >= 0x80
        if (cp < 0x80) return 0;
        if (cp > 0x7FF) return 0;  // Должно быть <= 0x7FF для 2 байт
    } 
    else if (length == 3) {
        // 3-байтовая последовательность: 1110xxxx 10xxxxxx 10xxxxxx
        cp = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
        if (cp < 0x800) return 0;  // Overlong encoding
        if (cp >= 0xD800 && cp <= 0xDFFF) return 0;  // Суррогатные пары недопустимы в UTF-8
    } 
    else if (length == 4) {
        // 4-байтовая последовательность: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        cp = ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) | 
             ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
        if (cp < 0x10000) return 0;  // Overlong encoding
        if (cp > 0x10FFFF) return 0;  // За пределами диапазона Unicode
    }
    
    *code_point = cp;
    return 1;  // Последовательность корректна
}


// ФУНКЦИИ ДЛЯ РАБОТЫ С ФАЙЛАМИ (ЧТЕНИЕ И ЗАПИСЬ)


// Чтение файла в буфер
// Возвращает 1 при успехе, 0 при ошибке
int read_file(const char *filename, uint8_t **buffer, size_t *size) {
    if (!filename || !buffer || !size) {
        fprintf(stderr, "Error: Invalid parameters for read_file\n");
        return 0;
    }
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open input file '%s': %s\n", 
                filename, strerror(errno));
        return 0;
    }
    
    // Определяем размер файла
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) {
        fprintf(stderr, "Error: Cannot determine file size\n");
        fclose(file);
        return 0;
    }
    
    if (file_size == 0) {
        fprintf(stderr, "Error: Input file is empty\n");
        fclose(file);
        return 0;
    }
    
    *size = (size_t)file_size;
    *buffer = malloc(*size);
    if (!*buffer) {
        fprintf(stderr, "Error: Memory allocation failed for file buffer\n");
        fclose(file);
        return 0;
    }
    
    // Читаем весь файл в буфер
    size_t bytes_read = fread(*buffer, 1, *size, file);
    if (bytes_read != *size) {
        fprintf(stderr, "Error: Failed to read entire file (read %zu of %zu bytes)\n",
                bytes_read, *size);
        free(*buffer);
        *buffer = NULL;
        fclose(file);
        return 0;
    }
    
    fclose(file);
    return 1;
}

// Запись буфера в файл
// Возвращает 1 при успехе, 0 при ошибке
int write_file(const char *filename, const uint8_t *buffer, size_t size) {
    if (!filename || !buffer) {
        fprintf(stderr, "Error: Invalid parameters for write_file\n");
        return 0;
    }
    
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Error: Cannot create output file '%s': %s\n",
                filename, strerror(errno));
        return 0;
    }
    
    size_t bytes_written = fwrite(buffer, 1, size, file);
    if (bytes_written != size) {
        fprintf(stderr, "Error: Failed to write entire file (wrote %zu of %zu bytes)\n",
                bytes_written, size);
        fclose(file);
        return 0;
    }
    
    if (fclose(file) != 0) {
        fprintf(stderr, "Error: Failed to close output file\n");
        return 0;
    }
    
    return 1;
}


// ФУНКЦИЯ АВТООПРЕДЕЛЕНИЯ КОДИРОВКИ ВХОДНОГО ФАЙЛА


// Определяет кодировку входного файла по BOM и содержимому
// Работает только с UTF-8 и UTF-16 (по условию задания)
Encoding detect_encoding(const uint8_t *data, size_t size) {
    if (!data || size == 0) {
        fprintf(stderr, "Error: Empty data for encoding detection\n");
        return ENC_UTF8_NOBOM;  // По умолчанию UTF-8 без BOM
    }
    
    // Проверка BOM (Byte Order Mark)
    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return ENC_UTF8_BOM;     // UTF-8 с BOM
    }
    if (size >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
        return ENC_UTF16LE;      // UTF-16 Little Endian
    }
    if (size >= 2 && data[0] == 0xFE && data[1] == 0xFF) {
        return ENC_UTF16BE;      // UTF-16 Big Endian
    }
    
    // Статистический анализ для определения UTF-16
    // В UTF-16 LE нулевые байты обычно на нечетных позициях, в UTF-16 BE - на четных
    if (size >= 4) {
        int zero_even = 0, zero_odd = 0;
        size_t check_limit = (size > 1024) ? 1024 : size;  // Проверяем первые 1024 байта
        
        for (size_t i = 0; i < check_limit; i++) {
            if (data[i] == 0) {
                if (i % 2 == 0) zero_even++;  // Нулевой байт на четной позиции
                else zero_odd++;              // Нулевой байт на нечетной позиции
            }
        }
        
        if (zero_even > 10 && zero_odd == 0) return ENC_UTF16BE;  // UTF-16 Big Endian
        if (zero_odd > 10 && zero_even == 0) return ENC_UTF16LE;  // UTF-16 Little Endian
    }
    
    // Пытаемся провалидировать как UTF-8
    // Если все последовательности валидны, считаем что это UTF-8 без BOM
    int valid_utf8 = 1;
    size_t pos = 0;
    while (pos < size && valid_utf8) {
        int seq_len = utf8_sequence_length(data[pos]);
        if (seq_len == -1) {
            valid_utf8 = 0;
            break;
        }
        
        // Проверяем, что последовательность не выходит за границы файла
        if (pos + seq_len > size) {
            // Неполная последовательность на конце файла - допустимо
            break;
        }
        
        uint32_t cp;
        if (!is_valid_utf8_sequence(&data[pos], seq_len, &cp)) {
            valid_utf8 = 0;
            break;
        }
        
        pos += seq_len;
    }
    
    if (valid_utf8) {
        return ENC_UTF8_NOBOM;  // Файл прошел проверку как UTF-8
    }
    
    // Если не удалось определить, по умолчанию считаем UTF-8 без BOM
    return ENC_UTF8_NOBOM;
}


// ФУНКЦИИ ДЕКОДИРОВАНИЯ (ИЗ ВХОДНОЙ КОДИРОВКИ В UNICODE)


// Декодирование UTF-8 в Unicode (UChar)
// Обрабатывает некорректные байты как символы диапазона 0xDC80..0xDCFF
int decode_utf8(const uint8_t *input, size_t input_size, 
                UChar **output, size_t *output_count) {
    if (!input || !output || !output_count) return 0;
    
    // Выделяем память под результат (максимум input_size символов)
    *output = malloc(input_size * sizeof(UChar));
    if (!*output) {
        fprintf(stderr, "Error: Memory allocation failed for Unicode buffer\n");
        return 0;
    }
    
    size_t pos = 0;  // Позиция во входном буфере
    size_t k = 0;    // Количество декодированных символов
    
    while (pos < input_size) {
        uint8_t first = input[pos];
        
        // Некорректный continuation байт (не являющийся началом последовательности)
        if (UTF8_CONTINUATION_BYTE(first)) {
            // Преобразуем в специальный диапазон Unicode 0xDC80..0xDCFF
            (*output)[k].code = INVALID_UTF8_START + (first & 0x7F);
            (*output)[k].src_size = 1;
            pos++;
            k++;
            continue;
        }
        
        int seq_len = utf8_sequence_length(first);
        
        // Некорректный первый байт UTF-8
        if (seq_len == -1 || seq_len > 4) {
            (*output)[k].code = UNICODE_REPLACEMENT;  // Символ замены �
            (*output)[k].src_size = 1;
            pos++;
            k++;
            continue;
        }
        
        // Неполная последовательность в конце файла
        if (pos + seq_len > input_size) {
            (*output)[k].code = UNICODE_REPLACEMENT;
            (*output)[k].src_size = input_size - pos;  // Все оставшиеся байты
            pos = input_size;
            k++;
            continue;
        }
        
        // Декодируем UTF-8 последовательность
        uint32_t code_point;
        if (is_valid_utf8_sequence(&input[pos], seq_len, &code_point)) {
            (*output)[k].code = code_point;
            (*output)[k].src_size = seq_len;
        } else {
            // Некорректная последовательность
            (*output)[k].code = UNICODE_REPLACEMENT;
            (*output)[k].src_size = seq_len;
        }
        
        pos += seq_len;
        k++;
    }
    
    *output_count = k;
    return 1;  // Успешное декодирование
}

// Декодирование UTF-16 в Unicode (UChar)
// Параметр big_endian: 0 = Little Endian, 1 = Big Endian
int decode_utf16(const uint8_t *input, size_t input_size, int big_endian,
                 UChar **output, size_t *output_count) {
    if (!input || !output || !output_count) return 0;
    
    // UTF-16 должен состоять из 16-битных слов, поэтому размер должен быть кратен 2
    if (input_size % 2 != 0) {
        fprintf(stderr, "Error: UTF-16 input size not multiple of 2\n");
        return 0;
    }
    
    size_t word_count = input_size / 2;  // Количество 16-битных слов
    *output = malloc(word_count * sizeof(UChar));
    if (!*output) {
        fprintf(stderr, "Error: Memory allocation failed for UTF-16 decode\n");
        return 0;
    }
    
    size_t k = 0;  // Количество декодированных символов
    
    for (size_t i = 0; i < word_count; i++) {
        uint16_t word = *(uint16_t*)(input + i * 2);
        
        // Меняем порядок байтов если необходимо
        if (big_endian) {
            word = swap16(word);
        }
        
        // Обычный символ BMP (Basic Multilingual Plane)
        if (word < 0xD800 || word > 0xDFFF) {
            (*output)[k].code = word;
            (*output)[k].src_size = 2;  // Занимает 2 байта
            k++;
        }
        // High surrogate (первая часть surrogate pair для символов > 0xFFFF)
        else if (word >= 0xD800 && word <= 0xDBFF) {
            if (i + 1 < word_count) {
                uint16_t low_word = *(uint16_t*)(input + (i + 1) * 2);
                if (big_endian) low_word = swap16(low_word);
                
                // Проверяем что low_word является low surrogate
                if (low_word >= 0xDC00 && low_word <= 0xDFFF) {
                    // Корректная surrogate pair, вычисляем Unicode code point
                    uint32_t code_point = 0x10000 + ((word - 0xD800) << 10) + (low_word - 0xDC00);
                    (*output)[k].code = code_point;
                    (*output)[k].src_size = 4;  // Занимает 4 байта (2 слова)
                    k++;
                    i++;  // Пропускаем low surrogate, так как мы его уже обработали
                } else {
                    // Некорректный low surrogate
                    (*output)[k].code = UNICODE_REPLACEMENT;
                    (*output)[k].src_size = 2;
                    k++;
                }
            } else {
                // High surrogate в конце файла (нет соответствующего low surrogate)
                (*output)[k].code = UNICODE_REPLACEMENT;
                (*output)[k].src_size = 2;
                k++;
            }
        }
        // Lone low surrogate (без соответствующего high surrogate) - некорректно
        else {
            (*output)[k].code = UNICODE_REPLACEMENT;
            (*output)[k].src_size = 2;
            k++;
        }
    }
    
    *output_count = k;
    return 1;
}

// Декодирование однобайтовой кодировки в Unicode с использованием таблицы
int decode_single_byte(const uint8_t *input, size_t input_size,
                       const uint16_t *table, UChar **output, size_t *output_count) {
    if (!input || !table || !output || !output_count) return 0;
    
    *output = malloc(input_size * sizeof(UChar));
    if (!*output) {
        fprintf(stderr, "Error: Memory allocation failed for single-byte decode\n");
        return 0;
    }
    
    // Каждый байт преобразуется в один Unicode символ
    for (size_t i = 0; i < input_size; i++) {
        uint8_t byte = input[i];
        
        if (byte < 128) {
            // ASCII символы остаются без изменений
            (*output)[i].code = byte;
        } else {
            // Не-ASCII символы преобразуем по таблице
            uint16_t unicode = table[byte - 128];
            // Если в таблице 0, значит символ не определен в этой кодировке
            (*output)[i].code = (unicode == 0) ? UNICODE_REPLACEMENT : unicode;
        }
        
        (*output)[i].src_size = 1;  // Однобайтовые символы всегда занимают 1 байт
    }
    
    *output_count = input_size;
    return 1;
}

// ФУНКЦИИ КОДИРОВАНИЯ (ИЗ UNICODE В ВЫХОДНУЮ КОДИРОВКУ)


// Кодирование Unicode в UTF-8
// Параметр add_bom: 1 = добавить BOM, 0 = не добавлять
int encode_utf8(const UChar *chars, size_t count, int add_bom,
                uint8_t **output, size_t *output_size) {
    if (!chars || !output || !output_size) return 0;
    
    // Выделяем память с запасом (максимум 4 байта на символ + 3 байта для BOM)
    *output = malloc(count * 4 + 3);
    if (!*output) {
        fprintf(stderr, "Error: Memory allocation failed for UTF-8 encode\n");
        return 0;
    }
    
    size_t pos = 0;  // Позиция в выходном буфере
    
    // Добавляем BOM если требуется
    if (add_bom) {
        (*output)[pos++] = 0xEF;
        (*output)[pos++] = 0xBB;
        (*output)[pos++] = 0xBF;
    }
    
    // Кодируем каждый Unicode символ в UTF-8
    for (size_t i = 0; i < count; i++) {
        uint32_t c = chars[i].code;
        
        // Определяем сколько байт нужно для кодирования этого символа
        if (c <= 0x7F) {
            // 1 байт: 0xxxxxxx
            (*output)[pos++] = c;
        } 
        else if (c <= 0x7FF) {
            // 2 байта: 110xxxxx 10xxxxxx
            (*output)[pos++] = 0xC0 | (c >> 6);
            (*output)[pos++] = 0x80 | (c & 0x3F);
        } 
        else if (c <= 0xFFFF) {
            // 3 байта: 1110xxxx 10xxxxxx 10xxxxxx
            // Проверяем что это не surrogate (суррогаты недопустимы в UTF-8)
            if (c >= 0xD800 && c <= 0xDFFF) {
                c = UNICODE_REPLACEMENT;  // Заменяем на символ замены
            }
            (*output)[pos++] = 0xE0 | (c >> 12);
            (*output)[pos++] = 0x80 | ((c >> 6) & 0x3F);
            (*output)[pos++] = 0x80 | (c & 0x3F);
        } 
        else if (c <= 0x10FFFF) {
            // 4 байта: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            (*output)[pos++] = 0xF0 | (c >> 18);
            (*output)[pos++] = 0x80 | ((c >> 12) & 0x3F);
            (*output)[pos++] = 0x80 | ((c >> 6) & 0x3F);
            (*output)[pos++] = 0x80 | (c & 0x3F);
        } 
        else {
            // Некорректный Unicode code point (> 0x10FFFF)
            c = UNICODE_REPLACEMENT;
            (*output)[pos++] = 0xE0 | (c >> 12);
            (*output)[pos++] = 0x80 | ((c >> 6) & 0x3F);
            (*output)[pos++] = 0x80 | (c & 0x3F);
        }
    }
    
    *output_size = pos;
    return 1;
}

// Кодирование Unicode в UTF-16
// Параметр big_endian: 0 = Little Endian, 1 = Big Endian
int encode_utf16(const UChar *chars, size_t count, int big_endian,
                 uint8_t **output, size_t *output_size) {
    if (!chars || !output || !output_size) return 0;
    
    // Выделяем память с запасом (максимум 4 байта на символ для surrogate pairs)
    *output = malloc(count * 4);
    if (!*output) {
        fprintf(stderr, "Error: Memory allocation failed for UTF-16 encode\n");
        return 0;
    }
    
    size_t pos = 0;
    
    for (size_t i = 0; i < count; i++) {
        uint32_t c = chars[i].code;
        
        if (c <= 0xFFFF) {
            // Символы BMP (Basic Multilingual Plane)
            // Проверяем что это не surrogate (суррогаты только в парах)
            if (c >= 0xD800 && c <= 0xDFFF) {
                c = UNICODE_REPLACEMENT;
            }
            
            uint16_t word = (uint16_t)c;
            if (big_endian) word = swap16(word);  // Меняем порядок байтов если нужно
            
            (*output)[pos++] = word & 0xFF;   // Младший байт
            (*output)[pos++] = word >> 8;     // Старший байт
        } 
        else if (c <= 0x10FFFF) {
            // Символы за пределами BMP требуют surrogate pair
            c -= 0x10000;
            uint16_t high = 0xD800 | (c >> 10);    // High surrogate
            uint16_t low = 0xDC00 | (c & 0x3FF);   // Low surrogate
            
            if (big_endian) {
                high = swap16(high);
                low = swap16(low);
            }
            
            // Записываем high surrogate
            (*output)[pos++] = high & 0xFF;
            (*output)[pos++] = high >> 8;
            
            // Записываем low surrogate
            (*output)[pos++] = low & 0xFF;
            (*output)[pos++] = low >> 8;
        } 
        else {
            // Некорректный Unicode code point
            uint16_t word = UNICODE_REPLACEMENT;
            if (big_endian) word = swap16(word);
            
            (*output)[pos++] = word & 0xFF;
            (*output)[pos++] = word >> 8;
        }
    }
    
    *output_size = pos;
    return 1;
}

// Кодирование Unicode в однобайтовую кодировку с использованием таблицы
int encode_single_byte(const UChar *chars, size_t count, const uint16_t *table,
                       uint8_t **output, size_t *output_size) {
    if (!chars || !table || !output || !output_size) return 0;
    
    *output = malloc(count);
    if (!*output) {
        fprintf(stderr, "Error: Memory allocation failed for single-byte encode\n");
        return 0;
    }
    
    for (size_t i = 0; i < count; i++) {
        uint32_t c = chars[i].code;
        
        if (c < 128) {
            // ASCII символы кодируем как есть
            (*output)[i] = (uint8_t)c;
        } 
        else {
            // Ищем символ в таблице преобразования
            uint8_t found = 0;
            for (int j = 0; j < 128; j++) {
                if (table[j] == c) {
                    // Нашли символ в таблице
                    (*output)[i] = j + 128;
                    found = 1;
                    break;
                }
            }
            
            if (!found) {
                // Символ не найден, пробуем использовать replacement character
                for (int j = 0; j < 128; j++) {
                    if (table[j] == UNICODE_REPLACEMENT) {
                        (*output)[i] = j + 128;
                        found = 1;
                        break;
                    }
                }
                
                // Если и replacement character не найден, используем '?'
                if (!found) {
                    (*output)[i] = '?';
                }
            }
        }
    }
    
    *output_size = count;
    return 1;
}

// ВЫВОД 


void print_usage(void) {
    fprintf(stderr, "Text Encoding Converter\n");
    fprintf(stderr, "Usage: converter <input_file> <output_file> <output_encoding> [<input_encoding>]\n");
    fprintf(stderr, "Encodings:\n");
    fprintf(stderr, "  0 - UTF-8 without BOM\n");
    fprintf(stderr, "  1 - UTF-8 with BOM\n");
    fprintf(stderr, "  2 - UTF-16 Little Endian\n");
    fprintf(stderr, "  3 - UTF-16 Big Endian\n");
    fprintf(stderr, "  4 - CP1251 (Windows Cyrillic)\n");
    fprintf(stderr, "  5 - CP866 (DOS Cyrillic)\n");
    fprintf(stderr, "  6 - KOI8-R (Unix Cyrillic)\n");
    fprintf(stderr, "\nIf input_encoding is not specified, it will be auto-detected.\n");
    fprintf(stderr, "\nExamples:\n");
    fprintf(stderr, "  converter input.txt output.txt 1           # Convert to UTF-8 with BOM (auto-detect input)\n");
    fprintf(stderr, "  converter input.txt output.txt 2 4        # Convert from CP1251 to UTF-16LE\n");
}


// main

int main(int argc, char *argv[]) {
    // Проверяем количество аргументов
    if (argc < 4) {
        print_usage();
        // Если программа запущена без аргументов, возвращаем 0 (успех)
        // Если с недостаточным количеством аргументов, возвращаем 1 (ошибка)
        return (argc == 1) ? 0 : 1;
    }
    
    // Получаем имена файлов из аргументов командной строки
    const char *input_filename = argv[1];
    const char *output_filename = argv[2];
    
    // Парсим выходную кодировку
    int output_encoding = atoi(argv[3]);
    if (output_encoding < 0 || output_encoding > 6) {
        fprintf(stderr, "Error: Invalid output encoding. Must be 0-6.\n");
        return 1;
    }
    
    // Парсим входную кодировку (если указана)
    int input_encoding = -1;
    if (argc >= 5) {
        input_encoding = atoi(argv[4]);
        if (input_encoding < 0 || input_encoding > 6) {
            fprintf(stderr, "Error: Invalid input encoding. Must be 0-6.\n");
            return 1;
        }
    }
    
    //  Чтение входного файла
    uint8_t *input_buffer = NULL;
    size_t input_size = 0;
    
    printf("Reading input file: %s\n", input_filename);
    if (!read_file(input_filename, &input_buffer, &input_size)) {
        return 1;  // Ошибка уже обработана в read_file
    }
    printf("File size: %zu bytes\n", input_size);
    
    //  Определение или получение входной кодировки
    Encoding enc_in;
    if (input_encoding >= 0) {
        // Кодировка указана явно
        enc_in = (Encoding)input_encoding;
        printf("Input encoding specified: %d\n", input_encoding);
    } else {
        // Автоопределение кодировки
        printf("Auto-detecting input encoding...\n");
        enc_in = detect_encoding(input_buffer, input_size);
        printf("Detected encoding: %d\n", enc_in);
    }
    
    // Декодирование входного файла в Unicode
    UChar *unicode_chars = NULL;
    size_t unicode_count = 0;
    int decode_success = 0;
    
    printf("Decoding input file...\n");
    switch (enc_in) {
        case ENC_UTF8_NOBOM:
            // UTF-8 без BOM
            decode_success = decode_utf8(input_buffer, input_size, 
                                         &unicode_chars, &unicode_count);
            break;
            
        case ENC_UTF8_BOM:
            // UTF-8 с BOM - пропускаем первые 3 байта (BOM)
            if (input_size >= 3 && 
                input_buffer[0] == 0xEF && 
                input_buffer[1] == 0xBB && 
                input_buffer[2] == 0xBF) {
                decode_success = decode_utf8(input_buffer + 3, input_size - 3,
                                             &unicode_chars, &unicode_count);
            } else {
                // BOM не найден, декодируем весь файл
                decode_success = decode_utf8(input_buffer, input_size,
                                             &unicode_chars, &unicode_count);
            }
            break;
            
        case ENC_UTF16LE:
            // UTF-16 Little Endian
            if (input_size >= 2 && 
                input_buffer[0] == 0xFF && 
                input_buffer[1] == 0xFE) {
                // Пропускаем BOM
                decode_success = decode_utf16(input_buffer + 2, input_size - 2,
                                              0, &unicode_chars, &unicode_count);
            } else {
                // Без BOM
                decode_success = decode_utf16(input_buffer, input_size,
                                              0, &unicode_chars, &unicode_count);
            }
            break;
            
        case ENC_UTF16BE:
            // UTF-16 Big Endian
            if (input_size >= 2 && 
                input_buffer[0] == 0xFE && 
                input_buffer[1] == 0xFF) {
                // Пропускаем BOM
                decode_success = decode_utf16(input_buffer + 2, input_size - 2,
                                              1, &unicode_chars, &unicode_count);
            } else {
                // Без BOM
                decode_success = decode_utf16(input_buffer, input_size,
                                              1, &unicode_chars, &unicode_count);
            }
            break;
            
        case ENC_CP1251:
            // Windows Cyrillic
            decode_success = decode_single_byte(input_buffer, input_size,
                                                table_cp1251, 
                                                &unicode_chars, &unicode_count);
            break;
            
        case ENC_CP866:
            // DOS Cyrillic
            decode_success = decode_single_byte(input_buffer, input_size,
                                                table_cp866,
                                                &unicode_chars, &unicode_count);
            break;
            
        case ENC_KOI8R:
            // KOI8-R
            decode_success = decode_single_byte(input_buffer, input_size,
                                                table_koi8r,
                                                &unicode_chars, &unicode_count);
            break;
            
        default:
            fprintf(stderr, "Error: Unknown input encoding\n");
            free(input_buffer);
            return 1;
    }
    
    // Освобождаем входной буфер после декодирования
    free(input_buffer);
    input_buffer = NULL;
    
    if (!decode_success) {
        fprintf(stderr, "Error: Failed to decode input file\n");
        if (unicode_chars) free(unicode_chars);
        return 1;
    }
    
    printf("Decoded %zu Unicode characters\n", unicode_count);
    
    // Кодирование Unicode в выходную кодировку
    Encoding enc_out = (Encoding)output_encoding;
    uint8_t *output_buffer = NULL;
    size_t output_size = 0;
    int encode_success = 0;
    
    printf("Encoding to output format: %d\n", output_encoding);
    switch (enc_out) {
        case ENC_UTF8_NOBOM:
            encode_success = encode_utf8(unicode_chars, unicode_count,
                                         0, &output_buffer, &output_size);
            break;
            
        case ENC_UTF8_BOM:
            encode_success = encode_utf8(unicode_chars, unicode_count,
                                         1, &output_buffer, &output_size);
            break;
            
        case ENC_UTF16LE:
            encode_success = encode_utf16(unicode_chars, unicode_count,
                                          0, &output_buffer, &output_size);
            break;
            
        case ENC_UTF16BE:
            encode_success = encode_utf16(unicode_chars, unicode_count,
                                          1, &output_buffer, &output_size);
            break;
            
        case ENC_CP1251:
            encode_success = encode_single_byte(unicode_chars, unicode_count,
                                                table_cp1251,
                                                &output_buffer, &output_size);
            break;
            
        case ENC_CP866:
            encode_success = encode_single_byte(unicode_chars, unicode_count,
                                                table_cp866,
                                                &output_buffer, &output_size);
            break;
            
        case ENC_KOI8R:
            encode_success = encode_single_byte(unicode_chars, unicode_count,
                                                table_koi8r,
                                                &output_buffer, &output_size);
            break;
            
        default:
            fprintf(stderr, "Error: Unknown output encoding\n");
            free(unicode_chars);
            return 1;
    }
    
    // Освобождаем буфер Unicode символов после кодирования
    free(unicode_chars);
    unicode_chars = NULL;
    
    if (!encode_success) {
        fprintf(stderr, "Error: Failed to encode output file\n");
        if (output_buffer) free(output_buffer);
        return 1;
    }
    
    printf("Output size: %zu bytes\n", output_size);
    
    //  Запись выходного файла
    printf("Writing output file: %s\n", output_filename);
    if (!write_file(output_filename, output_buffer, output_size)) {
        free(output_buffer);
        return 1;
    }
    
    // Освобождаем выходной буфер
    free(output_buffer);
    
    printf("Conversion completed successfully!\n");
    return 0;
}