
#ifndef UTIL_H_
#define UTIL_H_

int int_cmp(const void *a, const void *b);
int pointer_address_cmp(const void *a, const void *b);

char parse_entire_int(char *str, int *result);
char parse_entire_uint(char *str, unsigned int *result);
char parse_entire_ulong(char *str, unsigned long *result);

// Load a string of numbers into an array. Separator can be any non-numerical
// character. e.g. '1,2,3' '1 2 3'
// Returns number of unsigned integers parsed. 
// Sets *more = 1 if string end not reached
uint32_t parse_uint_liststr(const char *str, uint32_t *arr, uint32_t arrlen,
                            boolean *more);

size_t count_char(const char *str, char c);

boolean bases_to_integer(const char *arg, size_t *bases);
boolean mem_to_integer(const char *arg, size_t *bytes);


unsigned int num_of_digits(unsigned long num);

// result must be long enough for result + 1 ('\0'). Max length required is:
// strlen('18,446,744,073,709,551,615')+1 = 27 bytes
// returns pointer to result
char* ulong_to_str(unsigned long num, char* result);

// result must be long enough for result + 1 ('\0'). Max length required is:
// strlen('-9,223,372,036,854,775,808')+1 = 27 bytes
char* long_to_str(long num, char* result);

// result must be long enough for result + 1 ('\0').
// Max length required is: 26+1+decimals+1 = 28+decimals bytes
// strlen('-9,223,372,036,854,775,808') = 27
// strlen('.') = 1
// +1 for \0
char* double_to_str(double num, int decimals, char* str);

// str must be 26 + 3 + 1 + num decimals + 1 = 31+decimals bytes
// breakdown:
// strlen('18,446,744,073,709,551,615') = 26
// strlen(' GB') = 3
// strlen('.') = 1
// +1 for '\0'
char* bytes_to_str(unsigned long num, int decimals, char* str);

//
// Maths
//

float log_factorial(int number);
float log_factorial_ll(long long number);
unsigned long calculate_mean_ulong(unsigned long *array, unsigned long len);

#endif /* UTIL_H_ */