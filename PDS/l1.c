// here no error check after calloc
// also no error check for correct input of formula, for example:
// 	if you instead "AvB" entered "vAB" or "ABv" etc:
// 		there is undefined behavior
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define BSIZE 1000
#define SSYMBOLS 128
#define ABC 3
#define ABCchar 0x41

struct abc {
	char buf[BSIZE];
	char *symbols[SSYMBOLS];
};

enum {
	A = 0,
	B,
	C
};

enum {
	AND = 0,
	OR,
	DIF,
	XOR
};

int read_row(char *buf, size_t bsize);
int sya_parse(char *dest, const char *src, size_t dssize);
void calculate_sya_sets(struct abc sets[3], char *sya_formula,
		size_t sya_formula_len, char *result, size_t rsize);
int set_contains(const char * const symbols[SSYMBOLS], char *need);
struct abc sets_and(struct abc set1, struct abc set2); // ^
struct abc sets_or(struct abc set1, struct abc set2); // v
struct abc sets_dif(struct abc set1, struct abc set2); // -
struct abc sets_xor(struct abc set1, struct abc set2); // xor
struct abc (*sets_ops[4])(struct abc set1, struct abc set2) = {
	sets_and, sets_or, sets_dif, sets_xor
};

int main()
{
	int ret, last_operation = 0;
	char *nl; // new line detect and delete
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
operators:'(' ')' '\\'(difference) 'v'(OR) '^'(AND) '~'(XOR(delta in lab task))\
\noperands: A, B, C\n");
	printf("Prioritets from highest to lowest:\n\
	() -> ^ -> v -> \\ -> ~\n");
	printf("Enter formula (min 3 symbols, e.g.: 'AvB'): ");
	ret = read_row(formula, BSIZE); // read formula
	if (ret) {
		printf("\nentered end of file symbol (formula), \
exit and again!\n");
		return 1;
	}
	(nl = strchr(formula, '\n')) && (*nl = 0); // remove newline symbol
	ret = sya_parse(sya_formula, formula, BSIZE); // parse into sya
	if (ret)
		return 2;
	if (strlen(sya_formula) < 3) {
		printf("Need min 3 symbols for formula\n");
		return 1;
	}
	printf("%s\n", sya_formula);

	printf("Enter A, B and C sets (spaces are separators of elements)\n");

	for (int i = 0; i < ABC; i++) {
		printf("%c: ", ABCchar + i); // read sets of A, B and C
		ret = read_row(sets[i].buf, BSIZE);
		if (ret) {
			printf("\nentered end of file symbol (ABC), \
exit and again!\n");
			return 3;
		}

		// remove newline symbol
		(nl = strchr(sets[i].buf, '\n')) && (*nl = 0);

		// separate symbols in sets
		for (char *c = sets[i].buf, g = 0, j = 0; *c && j < SSYMBOLS;
				c++)
		{
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

	printf("RESULT: { %s }\n", result);

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
char operator_stack[STACKSIZE];
int push_operator_stack(char n);
int pop_operator_stack();
int get_op_prio(char op)
{
	switch (op) {
		case '^' : return 1;
		case 'v' : return 2;
		case '\\': return 3;
		case '~' : return 4;
	}
	return 0;
}
int sya_parse(char *dest, const char *src, size_t dssize)
{
	char pop, prio_pop;
	char new, prio_new;
	int bracket_level = 0;
	size_t k, i;
	for (k = i = 0; i < dssize && k < dssize && src[i]; i++) {
		new = src[i];
		switch (new) {
		case '*':	// forget for now
			printf("* operator unavailable for now, exit\n");
			exit(1);
			break;
		case '^':	// prio 1
		case 'v':	// prio 2
		case '\\':	// prio 3
		case '~':	// prio 4
			pop = pop_operator_stack();
			if (pop < 0 || (pop == '(' && push_operator_stack(pop)))
				push_operator_stack(new);
			else {
				prio_new = get_op_prio(new);
				prio_pop = get_op_prio(pop);
				
				push_operator_stack(pop);

				if (prio_new <= prio_pop) {
					push_operator_stack(new);
					break;
				}

				while ((pop = pop_operator_stack()) > -1 &&
						pop != '(' &&
						(prio_pop = get_op_prio(pop)) <=
						 prio_new) {
					dest[k++] = pop;
				}
				if (pop > -1)
					push_operator_stack(pop);
				push_operator_stack(new);
			}
			break;
		case 'A':
		case 'B':
		case 'C':
			dest[k++] = new;
			break;
		case '(':
			bracket_level++;
			if (push_operator_stack(new) < 0) {
				printf("operator stack overflow, exit\n");
				exit(1);
			}
			break;
		case ')':
			if (!bracket_level) {
				printf("bad brackets, exit\n");
				exit(1);
			}
			bracket_level--;
			while ((pop = pop_operator_stack()) != '(') {
				dest[k++] = pop;
			}
			break;
		case ' ':
			break;
		default:
			printf("exit: unexpected symbol, available: \
^ v \\ ~ A B C\n");
			return 1;
		}
	}

	while ((pop = pop_operator_stack()) != -1)
		dest[k++] = pop;
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

void calculate_sya_sets(struct abc sets[3], char *sya_formula,
		size_t sya_formula_len, char *result, size_t rsize)
{
	struct abc operands[100] = {0};
	struct abc tmp = {0};
	int operands_i = 0;
	int operand1, operand2;
	int op = -1;
	for (size_t i = 0; i < sya_formula_len; i++) {
		switch (sya_formula[i]) {
		case '~': op = XOR; break;
		case '^': op = AND; break;
		case 'v': op = OR; break;
		case '\\': op = DIF; break;
		case 'A': case 'B': case 'C':
			   operands[operands_i++] = sets[sya_formula[i] - 'A'];
		}
		if (op > -1) {
			operand1 = operands_i - 2;
			operand2 = operands_i - 1;
			tmp = sets_ops[op](operands[operand1],
					operands[operand2]);
			if (!*operands[operand1].buf) {
				for (int j = 0; operands[operand1].symbols[j];
						j++)
					free(operands[operand1].symbols[j]);
			}
			if (!*operands[operand2].buf) {
				for (int j = 0; operands[operand2].symbols[j];
						j++)
					free(operands[operand2].symbols[j]);
			}
			operands[operand1] = tmp;
			operands_i = operand2;
			op = -1;
		}
	}

	size_t last_result_len;
	for (int i = 0; operands[0].symbols[i]; i++) {
		if (last_result_len) {
			*(result + last_result_len++) = ' ';
		}
		strcpy(result + last_result_len, operands[0].symbols[i]);
		free(operands[0].symbols[i]);
		last_result_len = strlen(result);
	}
}

struct abc sets_and(struct abc set1, struct abc set2)
{
	struct abc ret = {.symbols = {0}};
	int ret_si = 0;
	char *nlors;
	for (int i = 0; set1.symbols[i]; i++) { // for2 in for1
	for (int j = 0; set2.symbols[j]; j++) {
		if (!strcmp(set1.symbols[i], set2.symbols[j])) {
			ret.symbols[ret_si] =
				calloc(strlen(set1.symbols[i]) + 1,
						sizeof (char));
			strcpy(ret.symbols[ret_si++], set1.symbols[i]);
		}
	} // end for2
	} // end for1
	return ret;
}
struct abc sets_or(struct abc set1, struct abc set2)
{
	struct abc ret = {.symbols = {0}};
	int ret_si = 0;
	char already_in = 0;
	for (int i = 0; set1.symbols[i]; i++) {
		ret.symbols[ret_si] = calloc(strlen(set1.symbols[i]) + 1,
				sizeof (char));
		strcpy(ret.symbols[ret_si++], set1.symbols[i]);
	}
	for (int i = 0; set2.symbols[i]; i++) { // for2 in for1
	for (int j = 0; ret.symbols[j]; j++) {
		if (!strcmp(set2.symbols[i], ret.symbols[j])) {
			already_in = 1;
			break;
		}
	} // end for2
	if (!already_in) {
		ret.symbols[ret_si] = calloc(strlen(set2.symbols[i]) + 1,
				sizeof (char));
		strcpy(ret.symbols[ret_si++], set2.symbols[i]);
	} else
		already_in = 0;
	} // end for1
	return ret;
}
struct abc sets_dif(struct abc set1, struct abc set2)
{
	struct abc ret = {.symbols = {0}};
	int ret_si = 0;
	char need_del = 0;
	for (int i = 0; set1.symbols[i]; i++) { // for2 in for1
	for (int j = 0; set2.symbols[j]; j++) {
		if (!strcmp(set1.symbols[i], set2.symbols[j])) {
			need_del = 1;
			break;
		}
	} // end for2
	if (!need_del) {
		ret.symbols[ret_si] = calloc(strlen(set1.symbols[i]) + 1,
				sizeof (char));
		strcpy(ret.symbols[ret_si++], set1.symbols[i]);
	} else
		need_del = 0;
	} // end for1
	return ret;
}
struct abc sets_xor(struct abc set1, struct abc set2)
{
	struct abc tmp, tmp2, ret;
	tmp = sets_dif(set1, set2);
	tmp2 = sets_dif(set2, set1);
	ret = sets_or(tmp, tmp2);
	for (int i = 0; tmp.symbols[i]; i++)
		free(tmp.symbols[i]);
	for (int i = 0; tmp2.symbols[i]; i++)
		free(tmp2.symbols[i]);
	return ret;
}

int set_contains(const char * const symbols[SSYMBOLS], char *need)
{
	char *nlors_ptr;
	char nlors;
	int ret = 0;
	// in need is full string while parsing
	// so, temporary need end string elements by \0 symbol insted ' 'or'\n':
	// try find ' ' or '\n' and set there \0
	// in the end: restore this value
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
