#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define BSIZE 1000
#define OPERATIONS 50
#define SSYMBOLS 32
#define ABC 3
#define ABCchar 0x41

struct abc {
	char buf[BSIZE];
	char *symbols[SSYMBOLS];
};

int read_row(char *buf, size_t bsize);
int sya_parse(char *dest, const char *src, size_t dssize);
void calculate_sya_sets(struct abc sets[3], char *sya_formula,
		size_t sya_formula_len, char *result, size_t rsize);
int set_contains(const char * const symbols[SSYMBOLS], char *need);
struct abc sets_and(struct abc set1, struct abc set2); // ^
struct abc sets_or(struct abc set1, struct abc set2); // v
struct abc sets_dif(struct abc set1, struct abc set2); // \

int main()
{
	int ret, last_operation = 0;
	char formula[BSIZE] = {0}; // here read formula from keyboard
	char result[BSIZE] = {0}; // here result of whole programm that will be
				  // printed
	char sya_formula[BSIZE] = {0}; // shunting yard algorithm string
				       // e.g.: A ^ B v C => AB^Cv
	struct abc sets[ABC] = {
		[0] = {.buf = {0}, .symbols = {0}},
		[1] = {.buf = {0}, .symbols = {0}},
		[2] = {.buf = {0}, .symbols = {0}},
	}; // empty buffers for symbols af A(s[0]), B(s[1]), C(s[2]

	printf("Lab work 1\n");
	printf("Next symbols are available for formula: \
operators: (, ), \\, v, ^\n\
operands: A, B, C,\n");
	printf("Enter formula: ");
	ret = read_row(formula, BSIZE); // read formula
	if (ret) {
		printf("\nentered end of file symbol (formula), \
exit and again!\n");
		return 1;
	}
	ret = sya_parse(sya_formula, formula, BSIZE); // parse into sya
	if (ret)
		return 2;
	printf("%s\n", sya_formula); // printf SYA formula looks like


	for (int i = 0; i < ABC; i++) {
		printf("%c: ", ABCchar + i); // read sets of A, B and C
		ret = read_row(sets[i].buf, BSIZE);
		if (ret) {
			printf("\nentered end of file symbol (ABC), \
exit and again!\n");
			return 3;
		}

		// separate symbols in sets
		for (char *c = sets[i].buf, g = 0, j = 0; *c && *c != '\n' &&
							j < SSYMBOLS; c++){
			if (*c == '\n')
				continue;
			if (*c == ' ')
				g = *c = 0;
			else if (!g++) {
				if (!set_contains((const char * const *)
							sets[i].symbols, c)) {
					sets[i].symbols[j++] = c;
				}
			}
		}
	}

	calculate_sya_sets(sets, sya_formula, strlen(sya_formula), result,
			sizeof result);

	printf("------------------------ debug -------------------\n");
	printf("Formula: %s\n", formula);
	for (int i = 0, j = 0; i < ABC; j++) {
		if (!sets[i].symbols[j]) {
			j = -1; i++; putchar(10);
		} else
			printf("%c[%d]: %s ",
					ABCchar + i, j, sets[i].symbols[j]);
	}

	struct abc and = sets_and(sets[0], sets[1]);
	for (int i = 0; and.symbols[i]; i++) {
		printf("%s\n", and.symbols[i]);
		free(and.symbols[i]);
	}

	return 0;
}

int read_row(char *buf, size_t bsize)
{
	char *fgets_ret = fgets(buf, bsize, stdin);
	if (!fgets_ret) {
		if (errno) {
			perror("internal error");
			exit(1);
		} else
			return 1;
	}
	return 0;
}

#define STACKSIZE 1000
int operator_ptr = 0;
int operand_ptr = 0;
char operand_stack[STACKSIZE];
char operator_stack[STACKSIZE];
int push_operator_stack(char n);
int pop_operator_stack();
int push_operand_stack(char n);
int pop_operand_stack();
void pop_operator_handle_prio_and_push_or_into_dest(char n, char *dest,
		size_t *dpos);
int sya_parse(char *dest, const char *src, size_t dssize)
{
	char pop;
	int bracket_level = 0;
	for (size_t i = 0, k = 0; i < dssize && k < dssize && src[i]; i++) {
		switch (src[i]) {
		case '*':	// forget for now
			printf("* operator unavailable for now, exit\n");
			exit(1);
			break;
		case '^':	// prio 1
		case 'v':	// prio 2
		case '\\':	// prio 3
			pop_operator_handle_prio_and_push_or_into_dest(src[i],
					dest, &k);
			break;
		case 'A':
		case 'B':
		case 'C':
			if (push_operand_stack(src[i]) < 0) {
				printf("operand stack overflow, exit\n");
				exit(1);
			}
			dest[k++] = src[i];
			break;
		case '(':
			bracket_level++;
			if (push_operator_stack(src[i]) < 0) {
				printf("operator stack overflow, exit\n");
				exit(1);
			}
			break;
		case ')':
			if (!bracket_level) {
				printf("bad brackets, exit");
				exit(1);
			}
			bracket_level--;
			while ((pop = pop_operator_stack()) != '(') {
				dest[k++] = pop;
			}
			if (bracket_level) {
				while ((pop = pop_operator_stack()) != '(')
					dest[k++] = pop;
				push_operator_stack(pop);
			}
			break;
		case '\n':
		case ' ':
			break;
		default:
			printf("exit: unexpected symbol, available: \
^ v \\ A B C\n");
			return 1;
		}
	}
	if (operator_stack)
		strcpy(dest + strlen(dest), operator_stack);
	return 0;
}

int push_operator_stack(char n)
{
	if (operator_ptr >= STACKSIZE)
		return -1;
	return (operator_stack[operator_ptr++] = n);
}
int pop_operator_stack()
{
	char ret;
	if (!operator_ptr)
		return -1;
	ret = operator_stack[--operator_ptr];
	operator_stack[operator_ptr] = 0;
	return ret;
}
int push_operand_stack(char n)
{
	if (operand_ptr >= STACKSIZE)
		return -1;
	return (operand_stack[operand_ptr++] = n);
}
int pop_operand_stack()
{
	int ret;
	if (!operand_ptr)
		return -1;
	ret = operand_stack[--operand_ptr];
	operand_stack[operand_ptr] = 0;
	return ret;
}
void pop_operator_handle_prio_and_push_or_into_dest(char n, char *dest,
		size_t *dpos)
{
	int prio_n = 0;
	int prio_pop = 0;
	int pop = pop_operator_stack();
	if (pop < 0)
		push_operator_stack(n);
	else {
		if (pop == '(') {
			push_operator_stack(pop);
			push_operator_stack(n);
			return;
		}

		if (pop == '^') prio_pop = 1;
		else if (pop == 'v') prio_pop = 2;
		else if (pop == '\\') prio_pop = 3;

		if (n == '^') prio_n = 1;
		else if (n == 'v') prio_n = 2;
		else if (n == '\\') prio_n = 3;

		if (prio_pop < prio_n)
			dest[(*dpos)++] = pop;
		else
			push_operator_stack(pop);
		push_operator_stack(n);
	}
}

void calculate_sya_sets(struct abc sets[3], char *sya_formula,
		size_t sya_formula_len, char *result, size_t rsize)
{
	for (size_t i = 0; i < sya_formula_len; i++) {
		switch (sya_formula[i]) {
		case '^':
			break;
		case 'v':
			break;
		case '\\':
			break;
		case 'A':
			break;
		case 'B':
			break;
		case 'C':
			break;
		}
	}
}

struct abc sets_and(struct abc set1, struct abc set2)
{
	struct abc ret;
	size_t ret_si = 0;
	char *nlors;
	for (int i = 0; set1.symbols[i]; i++) {
		for (int j = 0; set2.symbols[j]; j++) {
			if (!strcmp(set1.symbols[i], set2.symbols[j])) {
				ret.symbols[ret_si] = malloc(strlen(set1.symbols[i]));
				strcpy(ret.symbols[ret_si++], set1.symbols[i]);
			}
		}
	}
	return ret;
}
struct abc sets_or(struct abc set1, struct abc set2)
{
	struct abc ret;
	return ret;
}
struct abc sets_dif(struct abc set1, struct abc set2)
{
	struct abc ret;
	return ret;
}

int set_contains(const char * const symbols[SSYMBOLS], char *need)
{
	char *nlors_ptr;
	char nlors;
	int ret = 0;
	// in need is full string while parsing
	// so, temporary need end string by \0 symbol:
	// try find ' ' or '\n' and set there \0
	// in the end: restore this value
	// to continue parsing
	if (!(nlors_ptr = strchr(need, ' ')))
		nlors_ptr = strchr(need, '\n');
	if (nlors_ptr) {
		nlors = *nlors_ptr;
		*nlors_ptr = 0;
	} else
		nlors = 0;
	for (int i = 0; i < SSYMBOLS && !ret; i++) {
		if (symbols[i] && !strcmp(symbols[i], need))
			ret = 1;
	}
	if (nlors) // restore old new line or space symbol (NL or S = nlors)
		*nlors_ptr = nlors;
	return ret;
}