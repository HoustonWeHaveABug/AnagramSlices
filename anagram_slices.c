#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#define LETTERS_N 26
#define CHOICES_N (LETTERS_N*2)
#define CHUNK_SIZE 65536

typedef struct {
	int symbol;
	int count;
}
letter_t;

typedef struct word_s word_t;

struct word_s {
	int *string;
	int string_len;
	int removed;
	word_t *last;
	word_t *next;
};

typedef struct node_s node_t;

struct node_s {
	word_t *word;
	node_t *children[LETTERS_N];
};

typedef struct {
	int letters_idx;
	int output_start;
	int direction;
	int words_n;
	int rnd;
}
choice_t;

node_t *new_node(void);
int set_word(word_t *, int *, int);
void link_word(word_t *, word_t *, word_t *);
int choose_letter(int, int, int, int);
int eval_choice(choice_t *, int, int, int, int);
int compare_choices(const void *, const void *);
int erand(int);
int set_choice(choice_t *, int, int, int, int);
void unchain_word(word_t *);
void rechain_word(word_t *);
int compare_symbols(const void *, const void *);
void free_node(node_t *);

int *output, *best, word_len_min, word_len_max, output_max, output_len_min, choices_max;
letter_t letters[LETTERS_N] = { { 'a', 0 }, { 'b', 0 }, { 'c', 0 }, { 'd', 0 }, { 'e', 0 }, { 'f', 0 }, { 'g', 0 }, { 'h', 0 }, { 'i', 0 }, { 'j', 0 }, { 'k', 0 }, { 'l', 0 }, { 'm', 0 }, { 'n', 0 }, { 'o', 0 }, { 'p', 0 }, { 'q', 0 }, { 'r', 0 }, { 's', 0 }, { 't', 0 }, { 'u', 0 }, { 'v', 0 }, { 'w', 0 }, { 'x', 0 }, { 'y', 0 }, { 'z', 0 } };
word_t *words_header, **removals;
node_t *root;

int main(void) {
	int *strings, strings_max, strings_idx, words_n, symbol, words_idx, string_start, output_idx, r;
	word_t *words, *word;
	strings = malloc(sizeof(int)*CHUNK_SIZE);
	if (!strings) {
		fprintf(stderr, "Could not allocate memory for strings\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	strings_max = CHUNK_SIZE;
	strings_idx = 0;
	words_n = 0;
	symbol = getchar();
	while (!feof(stdin)) {
		int letters_idx;
		if (symbol == '\n') {
			words_n++;
			letters_idx = LETTERS_N;
		}
		else {
			for (letters_idx = 0; letters_idx < LETTERS_N && letters[letters_idx].symbol != symbol; letters_idx++);
			if (letters_idx == LETTERS_N) {
				fprintf(stderr, "Invalid symbol\n");
				fflush(stderr);
				free(strings);
				return EXIT_FAILURE;
			}
			letters[letters_idx].count++;
		}
		if (strings_idx == strings_max) {
			int *strings_tmp = realloc(strings, sizeof(int)*(size_t)(strings_max+CHUNK_SIZE));
			if (!strings_tmp) {
				fprintf(stderr, "Could not reallocate memory for strings\n");
				fflush(stderr);
				free(strings);
				return EXIT_FAILURE;
			}
			strings = strings_tmp;
			strings_max += CHUNK_SIZE;
		}
		strings[strings_idx++] = letters_idx;
		symbol = getchar();
	}
	if (words_n == 0) {
		fprintf(stderr, "No words found\n");
		fflush(stderr);
		free(strings);
		return EXIT_FAILURE;
	}
	words = malloc(sizeof(word_t)*(size_t)(words_n+1));
	if (!words) {
		fprintf(stderr, "Could not allocate memory for strings\n");
		fflush(stderr);
		free(strings);
		return EXIT_FAILURE;
	}
	root = new_node();
	if (!root) {
		free(words);
		free(strings);
		return EXIT_FAILURE;
	}
	strings_max = strings_idx;
	word_len_min = INT_MAX;
	word_len_max = 0;
	words_idx = 0;
	string_start = 0;
	for (strings_idx = 0; strings_idx < strings_max; strings_idx++) {
		if (strings[strings_idx] == LETTERS_N) {
			if (strings_idx == string_start) {
				fprintf(stderr, "Found an empty word\n");
				fflush(stderr);
				free_node(root);
				free(words);
				free(strings);
				return EXIT_FAILURE;
			}
			if (!set_word(words+words_idx, strings+string_start, strings_idx-string_start)) {
				free_node(root);
				free(words);
				free(strings);
				return EXIT_FAILURE;
			}
			words_idx++;
			string_start = strings_idx+1;
		}
	}
	words_header = words+words_n;
	link_word(words, words_header, words+1);
	for (word = words+1; word < words_header; word++) {
		link_word(word, word-1, word+1);
	}
	link_word(words_header, words_header-1, words);
	for (word = words_header->next; word != words_header; word = word->next) {
		if (word->removed) {
			unchain_word(word);
		}
	}
	output_max = (strings_max-words_n)*2;
	output = malloc(sizeof(int)*(size_t)output_max);
	if (!output) {
		fprintf(stderr, "Could not allocate memory for output\n");
		fflush(stderr);
		free_node(root);
		free(words);
		free(strings);
		return EXIT_FAILURE;
	}
	for (output_idx = 0; output_idx < output_max; output_idx++) {
		output[output_idx] = LETTERS_N;
	}
	best = calloc((size_t)output_max, sizeof(int));
	if (!best) {
		fprintf(stderr, "Could not allocate memory for best\n");
		fflush(stderr);
		free(output);
		free_node(root);
		free(words);
		free(strings);
		return EXIT_FAILURE;
	}
	removals = malloc(sizeof(word_t *)*(size_t)words_n);
	if (!removals) {
		fprintf(stderr, "Could not allocate memory for removals\n");
		fflush(stderr);
		free(best);
		free(output);
		free_node(root);
		free(words);
		free(strings);
		return EXIT_FAILURE;
	}
	srand((unsigned)time(NULL));
	output_len_min = strings_max-words_n+1;
	choices_max = 1;
	printf("choices_max = %d\n", choices_max);
	fflush(stdout);
	r = choose_letter(0, strings_max-words_n-1, strings_max-words_n, 0);
	for (choices_max = 2; choices_max <= CHOICES_N && r >= 0; choices_max++) {
		printf("choices_max = %d\n", choices_max);
		fflush(stdout);
		r = choose_letter(0, strings_max-words_n-1, strings_max-words_n, 0);
	}
	free(removals);
	free(best);
	free(output);
	free_node(root);
	free(words);
	free(strings);
	return EXIT_SUCCESS;
}

node_t *new_node(void) {
	int children_idx;
	node_t *node = malloc(sizeof(node_t));
	if (!node) {
		fprintf(stderr, "Could not allocate memory for node\n");
		return NULL;
	}
	node->word = NULL;
	for (children_idx = 0; children_idx < LETTERS_N; children_idx++) {
		node->children[children_idx] = NULL;
	}
	return node;
}

int set_word(word_t *word, int *string, int string_len) {
	int string_idx;
	node_t *node = root;
	qsort(string, (size_t)string_len, sizeof(int), compare_symbols);
	for (string_idx = 0; string_idx < string_len; string_idx++) {
		if (node->children[string[string_idx]]) {
			node = node->children[string[string_idx]];
		}
		else {
			node_t *child = new_node();
			if (!child) {
				return 0;
			}
			node->children[string[string_idx]] = child;
			node = child;
		}
	}
	if (node->word) {
		node->word->removed = 1;
	}
	node->word = word;
	word->string = string;
	word->string_len = string_len;
	word->removed = 0;
	if (string_len < word_len_min) {
		word_len_min = string_len;
	}
	if (string_len > word_len_max) {
		word_len_max = string_len;
	}
	return 1;
}

void link_word(word_t *word, word_t *last, word_t *next) {
	word->last = last;
	word->next = next;
}

int choose_letter(int depth, int output_lo, int output_hi, int removals_start) {
	if (depth == output_len_min) {
		return 0;
	}
	if (words_header->next == words_header) {
		int output_idx;
		output_len_min = depth;
		printf("%d ", output_len_min);
		for (output_idx = output_lo+1; output_idx < output_hi; output_idx++) {
			putchar(letters[output[output_idx]].symbol);
		}
		puts("");
		fflush(stdout);
		return 1;
	}
	else {
		int choices_n = 0, letters_idx, output_idx1, output_idx2, r, choices_idx;
		choice_t *choices;
		for (letters_idx = 0; letters_idx < LETTERS_N; letters_idx++) {
			if (letters[letters_idx].count > 0) {
				choices_n++;
			}
		}
		for (output_idx1 = output_lo+1, output_idx2 = output_hi-1; output_idx1 < output_idx2 && output[output_idx1] == output[output_idx2]; output_idx1++, output_idx2--);
		if (output_idx1 < output_idx2) {
			choices_n *= 2;
		}
		choices = malloc(sizeof(choice_t)*(size_t)choices_n);
		if (!choices) {
			fprintf(stderr, "Could not allocate memory for choices\n");
			fflush(stderr);
			return -1;
		}
		r = 0;
		choices_idx = 0;
		for (letters_idx = 0; letters_idx < LETTERS_N && !r; letters_idx++) {
			if (letters[letters_idx].count > 0) {
				r = eval_choice(choices+choices_idx, letters_idx, output_lo, 1, removals_start);
				choices_idx++;
			}
		}
		if (r < 0) {
			return r;
		}
		if (output_idx1 < output_idx2) {
			for (letters_idx = 0; letters_idx < LETTERS_N && !r; letters_idx++) {
				if (letters[letters_idx].count > 0) {
					r = eval_choice(choices+choices_idx, letters_idx, output_hi, -1, removals_start);
					choices_idx++;
				}
			}
		}
		if (r < 0) {
			return r;
		}
		qsort(choices, (size_t)choices_n, sizeof(choice_t), compare_choices);
		if (removals_start < best[depth]) {
			choices_n = 1;
		}
		if (removals_start > best[depth]) {
			best[depth] = removals_start;
		}
		for (choices_idx = 0; choices_idx < choices_n && choices_idx < choices_max && r >= 0; choices_idx++) {
			r = set_choice(choices+choices_idx, depth, output_lo, output_hi, removals_start);
		}
		free(choices);
		return r;
	}
}

int eval_choice(choice_t *choice, int letters_idx, int output_start, int direction, int removals_start) {
	int *slice = malloc(sizeof(int)*(size_t)word_len_max), words_n, step, removals_idx;
	if (!slice) {
		fprintf(stderr, "Could not allocate memory for slice\n");
		fflush(stderr);
		return -1;
	}
	words_n = removals_start;
	output[output_start] = letters_idx;
	for (step = 1; ; step++) {
		int slice_len = 0, output_idx;
		for (output_idx = output_start; output_idx >= 0 && output_idx < output_max && output[output_idx] < LETTERS_N && slice_len < word_len_max; output_idx += step*direction) {
			slice[slice_len++] = output[output_idx];
			if (slice_len >= word_len_min) {
				int slice_idx;
				node_t *node = root;
				qsort(slice, (size_t)slice_len, sizeof(int), compare_symbols);
				for (slice_idx = 0; slice_idx < slice_len && node; slice_idx++) {
					node = node->children[slice[slice_idx]];
				}
				if (node && node->word && !node->word->removed) {
					removals[words_n++] = node->word;
					node->word->removed = 1;
				}
			}
		}
		if ((output_idx < 0 || output_idx >= output_max || output[output_idx] == LETTERS_N) && slice_len < word_len_min) {
			break;
		}
	}
	output[output_start] = LETTERS_N;
	free(slice);
	for (removals_idx = removals_start; removals_idx < words_n; removals_idx++) {
		removals[removals_idx]->removed = 0;
	}
	choice->letters_idx = letters_idx;
	choice->output_start = output_start;
	choice->direction = direction;
	choice->words_n = words_n-removals_start;
	choice->rnd = erand(letters[letters_idx].count+1);
	return 0;
}

int compare_choices(const void *a, const void *b) {
	const choice_t *choice_a = (const choice_t *)a, *choice_b = (const choice_t *)b;
	if (choice_a->words_n != choice_b->words_n) {
		return choice_b->words_n-choice_a->words_n;
	}
	if (choice_a->output_start != choice_b->output_start) {
		return abs(output_max/2-choice_a->output_start)-abs(output_max/2-choice_b->output_start);
	}
	if (choice_a->rnd != choice_b->rnd) {
		return choice_b->rnd-choice_a->rnd;
	}
	return choice_a->letters_idx-choice_b->letters_idx;
}

int erand(int max) {
	return (int)(rand()/(RAND_MAX+1.0)*max);
}

int set_choice(choice_t *choice, int depth, int output_lo, int output_hi, int removals_start) {
	int *slice = malloc(sizeof(int)*(size_t)word_len_max), words_n, step, removals_idx, r;
	if (!slice) {
		fprintf(stderr, "Could not allocate memory for slice\n");
		fflush(stderr);
		return -1;
	}
	words_n = removals_start;
	output[choice->output_start] = choice->letters_idx;
	for (step = 1; ; step++) {
		int slice_len = 0, output_idx;
		for (output_idx = choice->output_start; output_idx >= 0 && output_idx < output_max && output[output_idx] < LETTERS_N && slice_len < word_len_max; output_idx += step*choice->direction) {
			slice[slice_len++] = output[output_idx];
			if (slice_len >= word_len_min) {
				int slice_idx;
				node_t *node = root;
				qsort(slice, (size_t)slice_len, sizeof(int), compare_symbols);
				for (slice_idx = 0; slice_idx < slice_len && node; slice_idx++) {
					node = node->children[slice[slice_idx]];
				}
				if (node && node->word && !node->word->removed) {
					removals[words_n++] = node->word;
					node->word->removed = 1;
				}
			}
		}
		if ((output_idx < 0 || output_idx >= output_max || output[output_idx] == LETTERS_N) && slice_len < word_len_min) {
			break;
		}
	}
	output[choice->output_start] = LETTERS_N;
	free(slice);
	for (removals_idx = removals_start; removals_idx < words_n; removals_idx++) {
		unchain_word(removals[removals_idx]);
	}
	output[choice->output_start] = choice->letters_idx;
	if (choice->direction < 0) {
		r = choose_letter(depth+1, output_lo, output_hi+1, words_n);
	}
	else {
		r = choose_letter(depth+1, output_lo-1, output_hi, words_n);
	}
	output[choice->output_start] = LETTERS_N;
	for (removals_idx = words_n-1; removals_idx >= removals_start; removals_idx--) {
		rechain_word(removals[removals_idx]);
	}
	return r;
}

void unchain_word(word_t *word) {
	int string_idx;
	for (string_idx = 0; string_idx < word->string_len; string_idx++) {
		letters[word->string[string_idx]].count--;
	}
	word->last->next = word->next;
	word->next->last = word->last;
}

void rechain_word(word_t *word) {
	int string_idx;
	word->next->last = word;
	word->last->next = word;
	word->removed = 0;
	for (string_idx = 0; string_idx < word->string_len; string_idx++) {
		letters[word->string[string_idx]].count++;
	}
}

int compare_symbols(const void *a, const void *b) {
	const int *symbol_a = (const int *)a, *symbol_b = (const int *)b;
	return *symbol_a-*symbol_b;
}

void free_node(node_t *node) {
	int children_idx;
	for (children_idx = 0; children_idx < LETTERS_N; children_idx++) {
		if (node->children[children_idx]) {
			free_node(node->children[children_idx]);
		}
	}
	free(node);
}
